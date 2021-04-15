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
//      (c) Copyright 2021 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"

namespace wyrmgus {

class game_concept final : public detailed_data_entry, public data_type<game_concept>
{
	Q_OBJECT

	Q_PROPERTY(QString hotkey READ get_hotkey_qstring NOTIFY changed)

public:
	static constexpr const char *class_identifier = "game_concept";
	static constexpr const char *database_folder = "game_concepts";

	explicit game_concept(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	QString get_hotkey_qstring() const
	{
		return QString::fromStdString(this->hotkey);
	}

	Q_INVOKABLE void set_hotkey(const std::string &hotkey)
	{
		this->hotkey = hotkey;
	}

signals:
	void changed();

private:
	std::string hotkey;
};

}
