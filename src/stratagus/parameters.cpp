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
//      (c) Copyright 1998-2021 by Joris Dauphin and Andrettin
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

#include "parameters.h"

#include "database/database.h"

namespace wyrmgus {

void parameters::SetDefaultValues()
{
	applicationName = "stratagus";
	luaStartFilename = "scripts/stratagus.lua";
	luaEditorStartFilename = "scripts/editor.lua";
	isTestRun = false;
	SetDefaultUserDirectory();
}

void parameters::SetDefaultUserDirectory()
{
#ifdef USE_GAME_DIR
	userDirectory = wyrmgus::database::get()->get_root_path().string();
#elif USE_WIN32
	userDirectory = getenv("APPDATA");
#elif __MORPHOS__
	userDirectory = "home";
#else
	userDirectory = getenv("HOME");
#endif

	if (!userDirectory.empty()) {
		userDirectory += "/";
	}

#ifdef USE_GAME_DIR
#elif USE_WIN32
	userDirectory += "Stratagus";
#elif defined(USE_MAC)
	userDirectory += "Library/Stratagus";
#else
	userDirectory += ".stratagus";
#endif
}

static std::string GetLocalPlayerNameFromEnv()
{
	const char *userName = nullptr;

#ifdef USE_WIN32
	userName = getenv("USERNAME");
#else
	userName = getenv("USER");
#endif

	if (userName && userName[0]) {
		return userName;
	} else {
		return "Anonymous";
	}
}

void parameters::SetLocalPlayerNameFromEnv()
{
	LocalPlayerName = GetLocalPlayerNameFromEnv();
}

}
