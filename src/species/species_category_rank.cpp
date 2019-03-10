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
/**@name species_category_rank.cpp - The species category rank source file. */
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

#include "species/species_category_rank.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CSpeciesCategoryRank::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "lower_rank") {
			value = FindAndReplaceString(value, "_", "-");
			CSpeciesCategoryRank *rank = CSpeciesCategoryRank::Get(value);
			if (rank) {
				this->LowerRank = rank;
				rank->UpperRank = this;
			}
		} else if (key == "upper_rank") {
			value = FindAndReplaceString(value, "_", "-");
			CSpeciesCategoryRank *rank = CSpeciesCategoryRank::Get(value);
			if (rank) {
				this->UpperRank = rank;
				rank->LowerRank = this;
			}
		} else {
			fprintf(stderr, "Invalid species category rank property: \"%s\".\n", key.c_str());
		}
	}
	
	this->Initialized = true;
	
	//check if the rank is linked properly to other ones, if all other ones have already been initialized
	if (CSpeciesCategoryRank::AreAllInitialized()) {
		//count of ranks linked to this one, plus itself
		size_t linked_rank_count = 1;
		
		CSpeciesCategoryRank *rank = this;
		while ((rank = rank->GetUpperRank())) {
			if (rank == this) {
				fprintf(stderr, "Species category rank \"%s\" is in a circular rank list.\n", this->Ident.c_str());
				break;
			}
			
			linked_rank_count++;
		}
		
		rank = this;
		while ((rank = rank->GetLowerRank())) {
			linked_rank_count++;
		}
		
		if (linked_rank_count != CSpeciesCategoryRank::GetAll().size()) {
			fprintf(stderr, "Species category rank \"%s\" is only linked to %d ranks, while %d exist.\n", this->Ident.c_str(), linked_rank_count, CSpeciesCategoryRank::GetAll().size());
		}
	}
}
