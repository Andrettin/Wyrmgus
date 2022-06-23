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
//      (c) Copyright 2022 by Andrettin
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

#include "map/map_template_unit.h"

#include "map/map_template.h"
#include "map/site.h"
#include "player/faction.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"

namespace wyrmgus {

void map_template_unit::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type") {
		this->type = unit_type::get(value);
	} else if (key == "unit_class") {
		this->unit_class = unit_class::get(value);
	} else if (key == "faction") {
		this->faction = faction::get(value);
	} else if (key == "site") {
		this->site = site::get(value);
	} else if (key == "player") {
		this->player_index = std::stoi(value);
	} else if (key == "resource_amount") {
		this->resource_amount = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid map template unit property: \"" + key + "\".");
	}
}

void map_template_unit::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "pos") {
		this->pos = scope.to_point();
	} else {
		throw std::runtime_error("Invalid map template unit scope: \"" + tag + "\".");
	}
}

}
