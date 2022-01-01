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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"

namespace wyrmgus {

class button_level final : public data_entry, public data_type<button_level>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "button_level";
	static constexpr const char *database_folder = "button_levels";

	static button_level *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		button_level *button_level = data_type::add(identifier, data_module);
		button_level->index = button_level::get_all().size() - 1;
		return button_level;
	}

	button_level(const std::string &identifier) : data_entry(identifier)
	{
	}

	int get_index() const
	{
		return this->index + 1; //the index starts at 1, so that buttons with a null button level have a value of 0
	}
	
private:
	int index = -1;
};

}
