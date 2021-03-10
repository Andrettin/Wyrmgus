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
//      (c) Copyright 2013-2021 by Joris Dauphin and Andrettin
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

#include "stratagus.h"

#include "database/database.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "engine_interface.h"
#include "game.h"
#include "map/map_grid_model.h"
#include "map/tile_image_provider.h"
#include "parameters.h"
#include "ui/interface_image_provider.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "version.h"
#include "video/frame_buffer_object.h"

#include <QDir>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char **argv)
{
	using namespace wyrmgus;

	try {
		qInstallMessageHandler(log::log_qt_message);

		QApplication app(argc, argv);
		app.setApplicationName(NAME);
		app.setApplicationVersion(VERSION);

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

		parameters::get()->process();

		std::thread stratagus_thread([argc, argv]() {
			try {
				stratagusMain(argc, argv);
			} catch (const std::exception &exception) {
				exception::report(exception);
				QMetaObject::invokeMethod(QApplication::instance(), [] { QApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
			}
		});

		QQmlApplicationEngine engine;

		qmlRegisterType<defines>();
		qmlRegisterType<game>();
		qmlRegisterType<parameters>();
		qmlRegisterType<preferences>();

		qmlRegisterType<frame_buffer_object>("frame_buffer_object", 1, 0, "FrameBufferObject");
		qmlRegisterType<map_grid_model>("map_grid_model", 1, 0, "MapGridModel");

		engine.rootContext()->setContextProperty("wyrmgus", engine_interface::get());

		engine.addImageProvider("interface", new interface_image_provider);
		engine.addImageProvider("tile", new tile_image_provider);

		const QString root_path = QString::fromStdString(database::get()->get_root_path().string());

		app.setWindowIcon(QIcon(root_path + "/graphics/interface/icons/wyrmsun_icon_32.png"));

		engine.addImportPath(root_path + "/libraries/qml");

		QUrl url = QDir(root_path + "/interface/").absoluteFilePath("Main.qml");
		url.setScheme("file");
		QObject::connect(
				&engine, &QQmlApplicationEngine::objectCreated, &app,
				[url](QObject *obj, const QUrl &objUrl) {
					if (!obj && url == objUrl) QCoreApplication::exit(-1);
				},
				Qt::QueuedConnection);
		engine.load(url);

		const int result = app.exec();

		stratagus_thread.join();

		return result;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return -1;
	}
}
