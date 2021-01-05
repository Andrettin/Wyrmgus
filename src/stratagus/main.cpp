//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
//      (c) Copyright 2013-2020 by Joris Dauphin and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#include "stratagus.h"

#include "database/database.h"
#include "util/exception_util.h"

#include <QCommandLineParser>
#include <QQmlApplicationEngine>

static void write_qt_message(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	std::ostream *ostream = nullptr;

	switch (type) {
		case QtDebugMsg:
		case QtInfoMsg:
			ostream = &std::cout;
			break;
		case QtWarningMsg:
		case QtCriticalMsg:
		case QtFatalMsg:
			ostream = &std::cerr;
			break;
	}

	switch (type) {
		case QtDebugMsg:
			*ostream << "Debug: ";
			break;
		case QtInfoMsg:
			*ostream << "Info: ";
			break;
		case QtWarningMsg:
			*ostream << "Warning: ";
			break;
		case QtCriticalMsg:
			*ostream << "Critical: ";
			break;
		case QtFatalMsg:
			*ostream << "Fatal: ";
			break;
	}

	*ostream << msg.toStdString();

	if (context.file != nullptr) {
		*ostream << " (" << context.file << ": " << context.line;

		if (context.function != nullptr) {
			*ostream << ", " << context.function;
		}

		*ostream << ")";
	}

	switch (type) {
		case QtCriticalMsg:
		case QtFatalMsg:
			*ostream << std::endl;
			break;
		default:
			*ostream << '\n';
			break;
	}
}

int main(int argc, char **argv)
{
	using namespace wyrmgus;

	try {
		qInstallMessageHandler(write_qt_message);
		QApplication app(argc, argv);

		//  Setup some defaults.
#ifdef MAC_BUNDLE
		freopen("/tmp/stdout.txt", "w", stdout);
		freopen("/tmp/stderr.txt", "w", stderr);
		// Look for the specified data set inside the application bundle
		// This should be a subdir of the Resources directory
		CFURLRef pluginRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(),
			CFSTR(MAC_BUNDLE_DATADIR), nullptr, nullptr);
		CFStringRef macPath = CFURLCopyFileSystemPath(pluginRef, kCFURLPOSIXPathStyle);
		const char *pathPtr = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
		Assert(pathPtr);
		database::get()->set_root_path(pathPtr);
#endif

		QCommandLineParser cmd_parser;

		QCommandLineOption data_path_option("d", "Specify a custom data path.", "data path");
		cmd_parser.addOption(data_path_option);

		cmd_parser.process(app);

		if (cmd_parser.isSet(data_path_option)) {
			database::get()->set_root_path(cmd_parser.value(data_path_option).toStdString());
		}

		std::thread stratagus_thread([argc, argv]() {
			try {
				stratagusMain(argc, argv);
			} catch (const std::exception &exception) {
				exception::report(exception);
				QMetaObject::invokeMethod(QApplication::instance(), [] { QApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
			}
		});

		QQmlApplicationEngine engine;
		engine.addImportPath(QString::fromStdString(database::get()->get_root_path().string() + "/libraries/qml"));

		const QUrl url = "file:///" + QString::fromStdString(database::get()->get_root_path().string() + "/interface/Main.qml");
		QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject *obj, const QUrl &objUrl) {
			if (!obj && url == objUrl) {
				QCoreApplication::exit(-1);
			}
		}, Qt::QueuedConnection);
		engine.load(url);

		const int result = app.exec();

		stratagus_thread.join();

		return result;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return -1;
	}
}
