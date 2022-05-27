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
//      (c) Copyright 2013-2022 by Joris Dauphin and Andrettin
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
#include "editor.h"
#include "engine_interface.h"
#include "game/game.h"
#include "game/results_info.h"
#include "map/map_grid_model.h"
#include "map/map_info.h"
#include "map/map_presets.h"
#include "map/tile_image_provider.h"
#include "map/tile_transition.h"
#include "network/client.h"
#include "network/network_manager.h"
#include "network/server.h"
#include "parameters.h"
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "player/faction.h"
#include "player/player.h"
#include "player/player_color.h"
#include "population/employment_type.h"
#include "population/population_type.h"
#include "quest/campaign.h"
#include "quest/quest.h"
#include "religion/pantheon.h"
#include "time/calendar.h"
#include "time/season.h"
#include "time/time_of_day.h"
#include "time/timeline.h"
#include "ui/empty_image_provider.h"
#include "ui/icon.h"
#include "ui/icon_image_provider.h"
#include "ui/interface_image_provider.h"
#include "ui/resource_icon_image_provider.h"
#include "unit/unit_list_model.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_structs.h"
#include "util/assert_util.h"
#include "util/event_loop.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/thread_pool.h"
#include "version.h"
#include "video/frame_buffer_object.h"

#pragma warning(push, 0)
#include <QDir>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#pragma warning(pop)

int main(int argc, char **argv)
{
	using namespace wyrmgus;

	try {
		qInstallMessageHandler(log::log_qt_message);

		QApplication app(argc, argv);
		app.setApplicationName(NAME);
		app.setApplicationVersion(VERSION);
		app.setOrganizationName("Wyrmsun");
		app.setOrganizationDomain("andrettin.github.io");

		qRegisterMetaType<std::vector<wyrmgus::tile_transition>>();

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
		assert_throw(pathPtr);
		database::get()->set_root_path(pathPtr);
#endif

		parameters::get()->process();

		event_loop::get()->co_spawn([argc, argv]() -> boost::asio::awaitable<void> {
			try {
				co_await stratagusMain(argc, argv);
			} catch (const std::exception &exception) {
				exception::report(exception);
				QMetaObject::invokeMethod(QApplication::instance(), [] { QApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
			}
		});

		QQmlApplicationEngine engine;

		qmlRegisterAnonymousType<calendar>("", 1);
		qmlRegisterAnonymousType<campaign>("", 1);
		qmlRegisterAnonymousType<CEditor>("", 1);
		qmlRegisterAnonymousType<civilization>("", 1);
		qmlRegisterAnonymousType<civilization_group>("", 1);
		qmlRegisterAnonymousType<client>("", 1);
		qmlRegisterAnonymousType<CPlayer>("", 1);
		qmlRegisterAnonymousType<CUpgrade>("", 1);
		qmlRegisterAnonymousType<defines>("", 1);
		qmlRegisterAnonymousType<employment_type>("", 1);
		qmlRegisterAnonymousType<faction>("", 1);
		qmlRegisterAnonymousType<game>("", 1);
		qmlRegisterAnonymousType<icon>("", 1);
		qmlRegisterAnonymousType<map_info>("", 1);
		qmlRegisterAnonymousType<map_presets>("", 1);
		qmlRegisterAnonymousType<network_manager>("", 1);
		qmlRegisterAnonymousType<pantheon>("", 1);
		qmlRegisterAnonymousType<parameters>("", 1);
		qmlRegisterAnonymousType<player_color>("", 1);
		qmlRegisterAnonymousType<population_type>("", 1);
		qmlRegisterAnonymousType<preferences>("", 1);
		qmlRegisterAnonymousType<quest>("", 1);
		qmlRegisterAnonymousType<resource>("", 1);
		qmlRegisterAnonymousType<results_info>("", 1);
		qmlRegisterAnonymousType<season>("", 1);
		qmlRegisterAnonymousType<server>("", 1);
		qmlRegisterAnonymousType<time_of_day>("", 1);
		qmlRegisterAnonymousType<timeline>("", 1);
		qmlRegisterAnonymousType<unit_type>("", 1);

		qmlRegisterType<frame_buffer_object>("frame_buffer_object", 1, 0, "FrameBufferObject");
		qmlRegisterType<map_grid_model>("map_grid_model", 1, 0, "MapGridModel");
		qmlRegisterType<unit_list_model>("unit_list_model", 1, 0, "UnitListModel");

		engine.rootContext()->setContextProperty("wyrmgus", engine_interface::get());

		engine.addImageProvider("empty", new empty_image_provider);
		engine.addImageProvider("icon", new icon_image_provider);
		engine.addImageProvider("interface", new interface_image_provider);
		engine.addImageProvider("resource_icon", new resource_icon_image_provider);
		engine.addImageProvider("tile", new tile_image_provider);

		const QString root_path = path::to_qstring(database::get()->get_root_path());

		app.setWindowIcon(QIcon(root_path + "/graphics/interface/icons/wyrmsun_icon_32.png"));

#ifdef DEBUG
		engine.addImportPath(root_path + "/libraries_debug/qml");
#else
		engine.addImportPath(root_path + "/libraries/qml");
#endif

		QUrl url = QDir(root_path + "/interface/").absoluteFilePath("Main.qml");
		url.setScheme("file");
		QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
			[url](QObject *obj, const QUrl &objUrl) {
				if (!obj && url == objUrl) {
					QCoreApplication::exit(-1);
				}
			}, Qt::QueuedConnection);
		engine.load(url);

		const int result = app.exec();

		event_loop::get()->stop();
		thread_pool::get()->stop();

		return result;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return -1;
	}
}
