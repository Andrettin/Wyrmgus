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
/**@name species_category.cpp - The species category source file. */
//
//      (c) Copyright 2019 by Andrettin
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

#include "species/species_category.h"

#include "config.h"
#include "species/species_category_rank.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CSpeciesCategory::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "common_name") {
			this->CommonName = value;
		} else if (key == "rank") {
			value = FindAndReplaceString(value, "_", "-");
			CSpeciesCategoryRank *rank = CSpeciesCategoryRank::Get(value);
			if (rank) {
				this->Rank = rank;
			}
		} else if (key == "lower_category") {
			value = FindAndReplaceString(value, "_", "-");
			CSpeciesCategory *category = CSpeciesCategory::Get(value);
			if (category) {
				this->LowerCategory = category;
			}
		} else if (key == "upper_category") {
			value = FindAndReplaceString(value, "_", "-");
			CSpeciesCategory *category = CSpeciesCategory::Get(value);
			if (category) {
				this->UpperCategory = category;
			}
		} else {
			fprintf(stderr, "Invalid species category property: \"%s\".\n", key.c_str());
		}
	}
}
