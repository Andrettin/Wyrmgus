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
#include "language/word.h"
#include "map/terrain_type.h"
#include "species/gender.h"
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
		CPlane *plane = CPlane::Get(value);
		if (plane) {
			this->HomePlane = plane;
			plane->AddSpecies(this);
		}
	} else if (key == "homeworld") {
		CWorld *world = CWorld::Get(value);
		if (world) {
			this->Homeworld = world;
			world->AddSpecies(this);
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
	
	for (const CSpecies *evolves_from : this->GetEvolvesFrom()) {
		if (evolves_from->IsInitialized() && this->Era != -1 && evolves_from->Era != -1 && this->Era <= evolves_from->Era) {
			fprintf(stderr, "Species \"%s\" is set to evolve from \"%s\", but is from the same or an earlier era than the latter.\n", this->GetIdent().utf8().get_data(), evolves_from->GetIdent().utf8().get_data());
		}
	}
	
	for (const CSpecies *evolves_to : this->GetEvolvesTo()) {
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
std::vector<const CSpeciesCategory *> CSpecies::GetAllCategories() const
{
	if (this->Category == nullptr) {
		return std::vector<const CSpeciesCategory *>();
	}
	
	std::vector<const CSpeciesCategory *> categories = this->Category->GetAllUpperCategories();
	categories.push_back(this->Category);
	
	return categories;
}

const std::vector<const CGender *> &CSpecies::GetGenders() const
{
	if (this->Genders.empty() && this->Category != nullptr) {
		return this->Category->GetGenders();
	}
	
	return this->Genders;
}
	
void CSpecies::AddSpecimenNameWord(CWord *word, const CGender *gender)
{
	this->SpecimenNameWords[gender].push_back(word);
	
	if (this->Category != nullptr) {
		this->Category->AddSpecimenNameWord(word, gender);
	}
}

const std::vector<CWord *> &CSpecies::GetSpecimenNameWords(const CGender *gender)
{
	auto find_iterator = this->SpecimenNameWords.find(gender);
	if (find_iterator != this->SpecimenNameWords.end() && find_iterator->second.size() >= CWord::MinimumWordsForNameGeneration) {
		return find_iterator->second;
	}
	
	if (this->Category != nullptr) {
		return this->Category->GetSpecimenNameWords(gender);
	}
	
	return this->SpecimenNameWords[gender];
}

void CSpecies::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_scientific_name", "scientific_name"), +[](CSpecies *species, const String &scientific_name){ species->ScientificName = scientific_name; });
	ClassDB::bind_method(D_METHOD("get_scientific_name"), +[](const CSpecies *species){ return species->ScientificName; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "scientific_name"), "set_scientific_name", "get_scientific_name");
	
	ClassDB::bind_method(D_METHOD("set_name_plural", "name_plural"), +[](CSpecies *species, const String &name_plural){ species->ScientificName = name_plural; });
	ClassDB::bind_method(D_METHOD("get_name_plural"), +[](const CSpecies *species){ return species->NamePlural; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name_plural"), "set_name_plural", "get_name_plural");
	
	ClassDB::bind_method(D_METHOD("set_category", "category_ident"), +[](CSpecies *species, const String &category_ident){ species->Category = CSpeciesCategory::Get(category_ident); });
	ClassDB::bind_method(D_METHOD("get_category"), +[](const CSpecies *species){ return const_cast<CSpeciesCategory *>(species->GetCategory()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "category"), "set_category", "get_category");
	ClassDB::bind_method(D_METHOD("get_all_categories"), +[](const CSpecies *species){ return VectorToGodotArray(species->GetAllCategories()); });
	
	ClassDB::bind_method(D_METHOD("set_sapient", "sapient"), +[](CSpecies *species, const bool sapient){ species->Sapient = sapient; });
	ClassDB::bind_method(D_METHOD("is_sapient"), &CSpecies::IsSapient);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sapient"), "set_sapient", "is_sapient");
	
	ClassDB::bind_method(D_METHOD("set_prehistoric", "prehistoric"), +[](CSpecies *species, const bool prehistoric){ species->Prehistoric = prehistoric; });
	ClassDB::bind_method(D_METHOD("is_prehistoric"), &CSpecies::IsPrehistoric);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "prehistoric"), "set_prehistoric", "is_prehistoric");
	
	ClassDB::bind_method(D_METHOD("add_to_genders", "gender"), +[](CSpecies *species, const String &gender){ species->Genders.push_back(CGender::Get(gender)); });
	ClassDB::bind_method(D_METHOD("remove_from_genders", "gender"), +[](CSpecies *species, const String &gender){ species->Genders.erase(std::remove(species->Genders.begin(), species->Genders.end(), CGender::Get(gender)), species->Genders.end()); });
	ClassDB::bind_method(D_METHOD("get_genders"), +[](const CSpecies *species){ return VectorToGodotArray(species->Genders); });
	
	ClassDB::bind_method(D_METHOD("add_to_native_terrain_types", "ident"), +[](CSpecies *species, const String &ident){ species->NativeTerrainTypes.insert(CTerrainType::Get(ident)); });
	ClassDB::bind_method(D_METHOD("remove_from_native_terrain_types", "ident"), +[](CSpecies *species, const String &ident){ species->NativeTerrainTypes.erase(CTerrainType::Get(ident)); });
	ClassDB::bind_method(D_METHOD("get_native_terrain_types"), +[](const CSpecies *species){ return SetToGodotArray(species->NativeTerrainTypes); });
	
	ClassDB::bind_method(D_METHOD("add_to_evolves_from", "ident"), +[](CSpecies *species, const String &ident){
		CSpecies *evolves_from = CSpecies::Get(ident);
		species->EvolvesFrom.push_back(evolves_from);
		evolves_from->EvolvesTo.push_back(species);
	});
	ClassDB::bind_method(D_METHOD("remove_from_evolves_from", "ident"), +[](CSpecies *species, const String &ident){
		CSpecies *evolves_from = CSpecies::Get(ident);
		species->EvolvesFrom.erase(std::remove(species->EvolvesFrom.begin(), species->EvolvesFrom.end(), evolves_from), species->EvolvesFrom.end());
		evolves_from->EvolvesTo.erase(std::remove(species->EvolvesTo.begin(), species->EvolvesTo.end(), species), species->EvolvesTo.end());
	});
	ClassDB::bind_method(D_METHOD("get_evolves_from"), +[](const CSpecies *species){ return VectorToGodotArray(species->EvolvesFrom); });
	
	ClassDB::bind_method(D_METHOD("add_to_evolves_to", "ident"), +[](CSpecies *species, const String &ident){
		CSpecies *evolves_to = CSpecies::Get(ident);
		species->EvolvesTo.push_back(evolves_to);
		evolves_to->EvolvesFrom.push_back(species);
	});
	ClassDB::bind_method(D_METHOD("remove_from_evolves_to", "ident"), +[](CSpecies *species, const String &ident){
		CSpecies *evolves_to = CSpecies::Get(ident);
		species->EvolvesTo.erase(std::remove(species->EvolvesTo.begin(), species->EvolvesTo.end(), evolves_to), species->EvolvesTo.end());
		evolves_to->EvolvesFrom.erase(std::remove(species->EvolvesFrom.begin(), species->EvolvesFrom.end(), species), species->EvolvesFrom.end());
	});
	ClassDB::bind_method(D_METHOD("get_evolves_to"), +[](const CSpecies *species){ return VectorToGodotArray(species->EvolvesTo); });
}
