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
//      (c) Copyright 2001-2021 by Lutz Sammer, Andreas Arens and Andrettin
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

#include "game/game.h"

#include "actions.h"
#include "age.h"
#include "ai.h"
#include "character.h"
#include "database/database.h"
#include "engine_interface.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_info.h"
#include "missile.h"
#include "parameters.h"
#include "player/player.h"
#include "quest/campaign.h"
#include "quest/quest.h"
#include "replay.h"
#include "results.h"
#include "script.h"
#include "script/trigger.h"
#include "settings.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/date_util.h"
#include "util/exception_util.h"
#include "util/path_util.h"
#include "util/random.h"
#include "version.h"

std::filesystem::path load_game_file;

/**
**  Save a game to file.
**
**  @param filename  File name to be stored.
**  @return  -1 if saving failed, 0 if all OK
**
**  @note  Later we want to store in a more compact binary format.
*/
int SaveGame(const std::string &file_url_str)
{
	const QUrl file_url = QString::fromStdString(file_url_str);
	const QString filepath_qstr = file_url.toLocalFile();
	const std::filesystem::path filepath = path::from_qstring(filepath_qstr);

	try {
		game::get()->save(filepath);
	} catch (const std::exception &exception) {
		exception::report(exception);
		return -1;
	}

	return 0;
}

void StartSavedGame(const std::filesystem::path &filepath)
{
	SaveGameLoading = true;
	CleanPlayers();
	LoadGame(filepath);

	StartMap(filepath, false);
}

void load_game(const std::filesystem::path &filepath)
{
	engine_interface::get()->set_loading_message("Loading Saved Game...");

	if (game::get()->is_running()) {
		set_load_game_file(filepath);
		StopGame(GameNoResult);
		return;
	}

	CclCommand("ClearPlayerDataObjectives();");

	while (true) {
		CclCommand("InitGameVariables(); LoadedGame = true;");
		StartSavedGame(filepath);

		if (GameResult != GameRestart) {
			break;
		}
	}

	game::get()->set_current_campaign(nullptr);
	CurrentQuest = nullptr;

	GameSettings.reset();
}

void set_load_game_file(const std::filesystem::path &filepath)
{
	load_game_file = filepath;
}
