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
/**@name species.cpp - The species source file. */
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

#include "species/species.h"

#include "config.h"
#include "include/plane.h"
#include "map/terrain_type.h"
#include "province.h" //for the era get function
#include "species/species_category.h"
#include "world.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CSpecies::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "quote") {
			this->Quote = value;
		} else if (key == "background") {
			this->Background = value;
		} else if (key == "era") {
			value = FindAndReplaceString(value, "_", "-");
			const int era_id = GetEraIdByName(value);
			if (era_id != -1) {
				this->Era = era_id;
			} else {
				fprintf(stderr, "Invalid era: \"%s\".\n", value.c_str());
			}
		} else if (key == "sapient") {
			this->Sapient = StringToBool(value);
		} else if (key == "prehistoric") {
			this->Prehistoric = StringToBool(value);
		} else if (key == "category") {
			value = FindAndReplaceString(value, "_", "-");
			CSpeciesCategory *category = CSpeciesCategory::Get(value);
			if (category) {
				this->Category = category;
			}
		} else if (key == "species") {
			this->Species = value;
		} else if (key == "child_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			this->ChildUpgrade = value;
		} else if (key == "home_plane") {
			value = FindAndReplaceString(value, "_", "-");
			CPlane *plane = CPlane::GetPlane(value);
			if (plane) {
				this->HomePlane = plane;
				plane->Species.push_back(this);
			}
		} else if (key == "homeworld") {
			value = FindAndReplaceString(value, "_", "-");
			CWorld *world = CWorld::GetWorld(value);
			if (world) {
				this->Homeworld = world;
				world->Species.push_back(this);
			}
		} else if (key == "terrain_type") {
			value = FindAndReplaceString(value, "_", "-");
			CTerrainType *terrain_type = CTerrainType::GetTerrainType(value);
			if (terrain_type) {
				this->Terrains.push_back(terrain_type);
			}
		} else if (key == "evolves_from") {
			value = FindAndReplaceString(value, "_", "-");
			CSpecies *evolves_from = CSpecies::Get(value);
			if (evolves_from) {
				this->EvolvesFrom.push_back(evolves_from);
				evolves_from->EvolvesTo.push_back(this);
			}
		} else {
			fprintf(stderr, "Invalid species property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CSpecies *evolves_from : this->EvolvesFrom) {
		if (this->Era != -1 && evolves_from->Era != -1 && this->Era <= evolves_from->Era) {
			fprintf(stderr, "Species \"%s\" is set to evolve from \"%s\", but is from the same or an earlier era than the latter.\n", this->GetIdent().utf8().get_data(), evolves_from->GetIdent().utf8().get_data());
		}
	}
}

bool CSpecies::CanEvolveToAUnitType(const CTerrainType *terrain_type, const bool sapient_only) const
{
	for (CSpecies *evolution_species : this->EvolvesTo) {
		if (
			(
				evolution_species->Type != nullptr
				&& (!terrain_type || evolution_species->IsNativeToTerrainType(terrain_type))
				&& (!sapient_only || evolution_species->Sapient)
			)
			|| evolution_species->CanEvolveToAUnitType(terrain_type, sapient_only)
		) {
			return true;
		}
	}
	
	return false;
}

CSpecies *CSpecies::GetRandomEvolution(const CTerrainType *terrain_type) const
{
	std::vector<CSpecies *> potential_evolutions;
	
	for (CSpecies *evolution_species : this->EvolvesTo) {
		if (
			(evolution_species->Type != nullptr && evolution_species->IsNativeToTerrainType(terrain_type))
			|| evolution_species->CanEvolveToAUnitType(terrain_type)
		) { //give preference to evolutions that are native to the current terrain type
			potential_evolutions.push_back(evolution_species);
		}
	}
	
	if (potential_evolutions.size() == 0) {
		for (CSpecies *evolution_species : this->EvolvesTo) {
			if (evolution_species->Type != nullptr || evolution_species->CanEvolveToAUnitType()) {
				potential_evolutions.push_back(evolution_species);
			}
		}
	}
	
	if (potential_evolutions.size() > 0) {
		return potential_evolutions[SyncRand(potential_evolutions.size())];
	}
	
	return nullptr;
}
