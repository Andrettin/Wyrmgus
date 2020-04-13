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

#include "stratagus.h"

#include "unit/historical_unit.h"

#include "config.h"
#include "map/historical_location.h"
#include "player.h"
#include "unit/unit_type.h"

namespace stratagus {

historical_unit::~historical_unit()
{
	for (size_t i = 0; i < this->HistoricalLocations.size(); ++i) {
		delete this->HistoricalLocations[i];
	}
}

void historical_unit::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "quantity") {
			this->quantity = std::stoi(value);
		} else if (key == "unit_type") {
			this->UnitType = CUnitType::get(value);
		} else if (key == "faction") {
			value = FindAndReplaceString(value, "_", "-");
			this->Faction = PlayerRaces.GetFaction(value);
		} else if (key == "start_date") {
			value = FindAndReplaceString(value, "_", "-");
			this->StartDate = CDate::FromString(value);
		} else if (key == "end_date") {
			value = FindAndReplaceString(value, "_", "-");
			this->EndDate = CDate::FromString(value);
		} else {
			fprintf(stderr, "Invalid historical unit property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "historical_location") {
			CHistoricalLocation *historical_location = new CHistoricalLocation;
			historical_location->ProcessConfigData(child_config_data);
				
			if (historical_location->Date.Year == 0 || !historical_location->map_template) {
				delete historical_location;
				continue;
			}
			
			this->HistoricalLocations.push_back(historical_location);
		} else {
			fprintf(stderr, "Invalid historical unit property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void historical_unit::check() const
{
	if (this->UnitType == nullptr) {
		throw std::runtime_error("Historical unit \"" + this->get_identifier() + "\" does not have a unit type.");
	}

	if (this->HistoricalLocations.empty()) {
		throw std::runtime_error("Historical unit \"" + this->get_identifier() + "\" does not have any historical locations.");
	}
}

}