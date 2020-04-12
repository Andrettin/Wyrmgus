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

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CHistoricalLocation::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "date") {
			value = FindAndReplaceString(value, "_", "-");
			this->Date = CDate::FromString(value);
		} else if (key == "map_template") {
			this->MapTemplate = CMapTemplate::get(value);
			if (!this->MapTemplate) {
				fprintf(stderr, "Map template \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "site") {
			value = FindAndReplaceString(value, "_", "-");
			this->Site = CSite::GetSite(value);
			if (this->Site) {
				this->MapTemplate = this->Site->MapTemplate;
				this->Position = this->Site->Position;
			} else {
				fprintf(stderr, "Site \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "x") {
			this->Position.x = std::stoi(value);
		} else if (key == "y") {
			this->Position.y = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid historical location property: \"%s\".\n", key.c_str());
		}
	}
	
	if (this->Date.Year == 0) {
		fprintf(stderr, "Historical location has no date.\n");
	}
	
	if (!this->MapTemplate) {
		fprintf(stderr, "Historical location has no map template.\n");
	}
}
