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
/**@name historical_location.cpp - The historical location source file. */
//
//      (c) Copyright 2018-2020 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "map/historical_location.h"

#include "config.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/site.h"

namespace stratagus {
	
void historical_location::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "date") {
		this->Date = CDate::FromString(value);
	} else if (key == "map_template") {
		this->map_template = map_template::get(value);
	} else if (key == "site") {
		this->site = site::get(value);
		this->map_template = this->site->get_map_template();
		this->Position = this->site->get_pos();
	} else if (key == "x") {
		this->Position.x = std::stoi(value);
	} else if (key == "y") {
		this->Position.y = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid historical location property: \"" + key + "\".");
	}
}

void historical_location::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid historical location scope: \"" + scope.get_tag() + "\".");
}

void historical_location::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "date") {
			value = FindAndReplaceString(value, "_", "-");
			this->Date = CDate::FromString(value);
		} else if (key == "map_template") {
			this->map_template = map_template::get(value);
		} else if (key == "site") {
			this->site = site::get(value);
			this->map_template = this->site->get_map_template();
			this->Position = this->site->get_pos();
		} else if (key == "x") {
			this->Position.x = std::stoi(value);
		} else if (key == "y") {
			this->Position.y = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid historical location property: \"%s\".\n", key.c_str());
		}
	}

	this->check();
}

void historical_location::check()
{
	if (this->Date.Year == 0) {
		throw std::runtime_error("Historical location has no date.");
	}

	if (this->map_template == nullptr) {
		throw std::runtime_error("Historical location has no map template.");
	}
}

}
