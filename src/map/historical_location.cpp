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

#include "map/historical_location.h"

#include "config.h"
#include "config_operator.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/site.h"

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
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		if (property.Key == "date") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			this->Date = CDate::FromString(value);
		} else if (property.Key == "map_template") {
			this->MapTemplate = CMapTemplate::Get(property.Value);
		} else if (property.Key == "site") {
			this->Site = CSite::Get(property.Value);
			if (this->Site) {
				this->MapTemplate = this->Site->MapTemplate;
				this->Position = this->Site->Position;
			}
		} else if (property.Key == "x") {
			this->Position.x = std::stoi(property.Value);
		} else if (property.Key == "y") {
			this->Position.y = std::stoi(property.Value);
		} else {
			fprintf(stderr, "Invalid historical location property: \"%s\".\n", property.Key.c_str());
		}
	}
	
	if (!this->MapTemplate) {
		fprintf(stderr, "Historical location has no map template.\n");
	}
}
