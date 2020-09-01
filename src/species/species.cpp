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
//      (c) Copyright 2020 by Andrettin
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

#include "species/species.h"

#include "map/terrain_type.h"
#include "plane.h"
#include "species/geological_era.h"
#include "species/taxon.h"
#include "species/taxonomic_rank.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"
#include "world.h"

namespace wyrmgus {

species::species(const std::string &identifier) : detailed_data_entry(identifier), era(geological_era::none)
{
}

void species::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "native_terrain_types") {
		for (const std::string &value : values) {
			const terrain_type *terrain = terrain_type::get(value);
			this->native_terrain_types.push_back(terrain);
		}
	} else if (tag == "pre_evolutions") {
		for (const std::string &value : values) {
			species *other_species = species::get(value);
			this->pre_evolutions.push_back(other_species);
			other_species->evolutions.push_back(this);
		}
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void species::initialize()
{
	if (this->get_home_plane() != nullptr) {
		this->get_home_plane()->Species.push_back(this);
	}

	if (this->get_homeworld() != nullptr) {
		this->get_homeworld()->Species.push_back(this);
	}
}

void species::check() const
{
	if (this->get_supertaxon() == nullptr) {
		throw std::runtime_error("Species \"" + this->get_identifier() + "\" has no supertaxon.");
	}

	for (const species *pre_evolution : this->pre_evolutions) {
		if (this->get_era() != geological_era::none && pre_evolution->get_era() != geological_era::none && this->get_era() <= pre_evolution->get_era()) {
			throw std::runtime_error("Species \"" + this->get_identifier() + "\" is set to evolve from \"" + pre_evolution->get_identifier() + "\", but is from the same or an earlier era than the latter.");
		}
	}
}

const taxon *species::get_supertaxon_of_rank(const taxonomic_rank rank) const
{
	if (this->get_supertaxon() == nullptr) {
		return nullptr;
	}

	if (this->get_supertaxon()->get_rank() == rank) {
		return nullptr;
	}

	return this->get_supertaxon()->get_supertaxon_of_rank(rank);
}

bool species::is_subtaxon_of(const taxon *taxon) const
{
	if (taxon == this->get_supertaxon()) {
		return true;
	}

	return this->get_supertaxon()->is_subtaxon_of(taxon);
}

std::string species::get_scientific_name() const
{
	if (this->get_supertaxon() == nullptr) {
		throw std::runtime_error("Cannot get the scientific name for species \"" + this->get_identifier() + "\", as it has no supertaxon.");
	}

	if (this->get_supertaxon()->get_rank() != taxonomic_rank::genus) {
		throw std::runtime_error("Cannot get the scientific name for species \"" + this->get_identifier() + "\", as its supertaxon is not a genus.");
	}

	if (!this->get_specific_name().empty()) {
		return this->get_supertaxon()->get_name() + " " + this->get_specific_name();
	}

	return this->get_supertaxon()->get_name();
}

bool species::is_prehistoric() const
{
	return this->get_era() < geological_era::holocene;
}

bool species::has_evolution(const terrain_type *terrain, const bool sapient_only) const
{
	for (const species *evolution : this->get_evolutions()) {
		if (
			(evolution->Type != nullptr && (!terrain || vector::contains(evolution->get_native_terrain_types(), terrain)) && (!sapient_only || evolution->is_sapient()))
			|| evolution->has_evolution(terrain, sapient_only)
		) {
			return true;
		}
	}
	return false;
}

const species *species::get_random_evolution(const terrain_type *terrain) const
{
	std::vector<const species *> potential_evolutions;
	
	for (const species *evolution : this->get_evolutions()) {
		//give preference to evolutions that are native to the current terrain
		if (
			(evolution->Type != nullptr && vector::contains(evolution->get_native_terrain_types(), terrain))
			|| evolution->has_evolution(terrain)
		) {
			potential_evolutions.push_back(evolution);
		}
	}
	
	if (potential_evolutions.size() == 0) {
		for (const species *evolution : this->get_evolutions()) {
			if (evolution->Type != nullptr || evolution->has_evolution()) {
				potential_evolutions.push_back(evolution);
			}
		}
	}
	
	if (potential_evolutions.size() > 0) {
		return vector::get_random(potential_evolutions);
	}
	
	return nullptr;
}

}