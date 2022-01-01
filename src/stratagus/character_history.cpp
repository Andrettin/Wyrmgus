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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "character_history.h"

#include "character_title.h"
#include "database/sml_data.h"
#include "database/sml_property.h"
#include "map/historical_location.h"
#include "map/site.h"

namespace wyrmgus {

character_history::character_history(wyrmgus::faction *default_faction, const site *default_location_site)
	: faction(default_faction), title(character_title::none)
{
	if (default_location_site != nullptr) {
		this->location = std::make_unique<historical_location>(default_location_site);
	}
}

character_history::~character_history()
{
}

void character_history::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "location") {
		this->location = std::make_unique<historical_location>(site::get(value));
	} else {
		data_entry_history::process_sml_property(property);
	}
}

void character_history::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "location") {
		this->location = std::make_unique<historical_location>(scope);
	} else {
		data_entry_history::process_sml_scope(scope);
	}
}

}
