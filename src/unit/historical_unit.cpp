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
/**@name historical_unit.cpp - The historical unit source file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#include "unit/historical_unit.h"

#include "config.h"
#include "faction.h"
#include "map/historical_location.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Destructor
*/
CHistoricalUnit::~CHistoricalUnit()
{
	for (CHistoricalLocation *historical_location : this->HistoricalLocations) {
		delete historical_location;
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CHistoricalUnit::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "quantity") {
			this->Quantity = std::stoi(value);
		} else if (key == "unit_type") {
			value = FindAndReplaceString(value, "_", "-");
			this->UnitType = UnitTypeByIdent(value);
		} else if (key == "faction") {
			value = FindAndReplaceString(value, "_", "-");
			this->Faction = CFaction::Get(value);
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
				
			if (historical_location->Date.Year == 0 || !historical_location->MapTemplate) {
				delete historical_location;
				continue;
			}
			
			this->HistoricalLocations.push_back(historical_location);
		} else {
			fprintf(stderr, "Invalid historical unit property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}
