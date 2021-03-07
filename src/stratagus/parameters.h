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

#pragma once

#include "util/singleton.h"

namespace wyrmgus {

//command line parameters
class parameters final : public QObject, public singleton<parameters>
{
	Q_OBJECT

	Q_PROPERTY(bool test_run READ is_test_run CONSTANT)

public:
	void process();
	void SetLocalPlayerNameFromEnv();

	bool is_test_run() const
	{
		return this->test_run;
	}

	void SetUserDirectory(const std::string &path) { userDirectory = path; }
	const std::string &GetUserDirectory() const { return userDirectory; }

private:
	void SetDefaultUserDirectory();

public:
	std::string luaStartFilename = "scripts/stratagus.lua";
	std::string luaEditorStartFilename = "scripts/editor.lua";
	std::string luaScriptArguments;
	std::string LocalPlayerName;        /// Name of local player
private:
	bool test_run = false;
	std::string userDirectory;          /// Directory containing user settings and data
};

}
