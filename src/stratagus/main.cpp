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

#include "util/exception_util.h"

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

		std::thread stratagus_thread([argc, argv]() {
			try {
				stratagusMain(argc, argv);
			} catch (const std::exception &exception) {
				exception::report(exception);
				QMetaObject::invokeMethod(QApplication::instance(), [] { QApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
			}
		});

		QQmlApplicationEngine engine;
		engine.addImportPath("./libraries/qml");

		const QUrl url(QStringLiteral("./interface/Main.qml"));
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
