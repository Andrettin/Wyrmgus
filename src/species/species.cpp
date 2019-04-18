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
#include "map/terrain_type.h"
#include "species/species_category.h"
#include "util.h"
#include "world/plane.h"
#include "world/province.h" //for the era get function
#include "world/world.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CSpecies::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "era") {
		value = FindAndReplaceString(value, "_", "-");
		const int era_id = GetEraIdByName(value);
		if (era_id != -1) {
			this->Era = era_id;
		} else {
			fprintf(stderr, "Invalid era: \"%s\".\n", value.c_str());
		}
	} else if (key == "child_upgrade") {
		value = FindAndReplaceString(value, "_", "-");
		this->ChildUpgrade = value;
	} else if (key == "home_plane") {
		value = FindAndReplaceString(value, "_", "-");
		CPlane *plane = CPlane::Get(value);
		if (plane) {
			this->HomePlane = plane;
			plane->Species.push_back(this);
		}
	} else if (key == "homeworld") {
		value = FindAndReplaceString(value, "_", "-");
		CWorld *world = CWorld::Get(value);
		if (world) {
			this->Homeworld = world;
			world->Species.push_back(this);
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the species
*/
void CSpecies::Initialize()
{
	this->Initialized = true;
	
	for (const CSpecies *evolves_from : this->EvolvesFrom) {
		if (evolves_from->IsInitialized() && this->Era != -1 && evolves_from->Era != -1 && this->Era <= evolves_from->Era) {
			fprintf(stderr, "Species \"%s\" is set to evolve from \"%s\", but is from the same or an earlier era than the latter.\n", this->GetIdent().utf8().get_data(), evolves_from->GetIdent().utf8().get_data());
		}
	}
	
	for (const CSpecies *evolves_to : this->EvolvesTo) {
		if (evolves_to->IsInitialized() && this->Era != -1 && evolves_to->Era != -1 && this->Era >= evolves_to->Era) {
			fprintf(stderr, "Species \"%s\" is set to evolve to \"%s\", but is from the same or a later era than the latter.\n", this->GetIdent().utf8().get_data(), evolves_to->GetIdent().utf8().get_data());
		}
	}
}

bool CSpecies::CanEvolveToAUnitType(const CTerrainType *terrain_type, const bool sapient_only) const
{
	for (CSpecies *evolution_species : this->EvolvesTo) {
		if (
			(
				evolution_species->GetUnitType() != nullptr
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

/**
**	@brief	Get a random evolution for the species for a given terrain type
**
**	@param	terrain_type	The terrain type
**
**	@return	The evolution
*/
CSpecies *CSpecies::GetRandomEvolution(const CTerrainType *terrain_type) const
{
	std::vector<CSpecies *> potential_evolutions;
	
	for (CSpecies *evolution_species : this->EvolvesTo) {
		if (
			(evolution_species->GetUnitType() != nullptr && evolution_species->IsNativeToTerrainType(terrain_type))
			|| evolution_species->CanEvolveToAUnitType(terrain_type)
		) { //give preference to evolutions that are native to the current terrain type
			potential_evolutions.push_back(evolution_species);
		}
	}
	
	if (potential_evolutions.size() == 0) {
		for (CSpecies *evolution_species : this->EvolvesTo) {
			if (evolution_species->GetUnitType() != nullptr || evolution_species->CanEvolveToAUnitType()) {
				potential_evolutions.push_back(evolution_species);
			}
		}
	}
	
	if (potential_evolutions.size() > 0) {
		return potential_evolutions[SyncRand(potential_evolutions.size())];
	}
	
	return nullptr;
}

/**
**	@brief	Get all the categories the species ultimately belongs to (i.e. not only its own category, but also all upper categories of its category)
**
**	@return	The categories
*/
std::vector<CSpeciesCategory *> CSpecies::GetAllCategories() const
{
	if (this->Category == nullptr) {
		return std::vector<CSpeciesCategory *>();
	}
	
	std::vector<CSpeciesCategory *> categories = this->Category->GetAllUpperCategories();
	categories.push_back(this->Category);
	
	return categories;
}

void CSpecies::_bind_methods()
{
	BIND_PROPERTIES();
	
	ClassDB::bind_method(D_METHOD("get_all_categories"), [](const CSpecies *species){ return VectorToGodotArray(species->GetAllCategories()); });
}
