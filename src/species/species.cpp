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

#include "species/taxon.h"
#include "species/taxonomic_rank.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void species::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "pre_evolutions") {
		for (const std::string &value : values) {
			species *other_species = species::get(value);
			this->pre_evolutions.push_back(other_species);
			other_species->evolutions.push_back(this);
		}
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void species::check() const
{
	if (this->get_genus() == nullptr) {
		throw std::runtime_error("Species \"" + this->get_identifier() + "\" has no genus.");
	}

	if (this->get_genus()->get_rank() != taxonomic_rank::genus) {
		throw std::runtime_error("The genus of species \"" + this->get_identifier() + "\" has a taxonomic rank different than genus.");
	}
}

const taxon *species::get_supertaxon_of_rank(const taxonomic_rank rank) const
{
	if (this->get_genus() == nullptr) {
		return nullptr;
	}

	if (this->get_genus()->get_rank() == rank) {
		return nullptr;
	}

	return this->get_genus()->get_supertaxon_of_rank(rank);
}

bool species::has_evolution(const terrain_type *terrain, const bool sapient_only) const
{
	for (const species *evolution : this->get_evolutions()) {
		if (
			(evolution->Type != nullptr && (!terrain || vector::contains(evolution->Terrains, terrain)) && (!sapient_only || evolution->is_sapient()))
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
			(evolution->Type != nullptr && vector::contains(evolution->Terrains, terrain))
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