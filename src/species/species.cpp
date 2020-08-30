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
#include "util/vector_util.h"

namespace wyrmgus {


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

bool species::CanEvolveToAUnitType(terrain_type *terrain, bool sapient_only) const
{
	for (const species *evolution : this->EvolvesTo) {
		if (
			(evolution->Type != nullptr && (!terrain || vector::contains(evolution->Terrains, terrain)) && (!sapient_only || evolution->Sapient))
			|| evolution->CanEvolveToAUnitType(terrain, sapient_only)
		) {
			return true;
		}
	}
	return false;
}

species *species::GetRandomEvolution(terrain_type *terrain) const
{
	std::vector<species *> potential_evolutions;
	
	for (species *evolution : this->EvolvesTo) {
		if (
			(evolution->Type != nullptr && vector::contains(evolution->Terrains, terrain))
			|| evolution->CanEvolveToAUnitType(terrain)
		) { //give preference to evolutions that are native to the current terrain
			potential_evolutions.push_back(evolution);
		}
	}
	
	if (potential_evolutions.size() == 0) {
		for (species *evolution : this->EvolvesTo) {
			if (evolution->Type != nullptr || evolution->CanEvolveToAUnitType()) {
				potential_evolutions.push_back(evolution);
			}
		}
	}
	
	if (potential_evolutions.size() > 0) {
		return potential_evolutions[SyncRand(potential_evolutions.size())];
	}
	
	return nullptr;
}

}