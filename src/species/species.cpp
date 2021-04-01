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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "stratagus.h"

#include "species/species.h"

#include "map/terrain_type.h"
#include "map/world.h"
#include "species/geological_era.h"
#include "species/taxon.h"
#include "species/taxonomic_rank.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

std::map<const taxon *, int> species::get_supertaxon_counts(const std::vector<const species *> &species_list, const std::vector<const taxon *> &taxons)
{
	std::map<const taxon *, int> supertaxon_counts;

	for (const species *species : species_list) {
		const taxon *supertaxon = species->get_supertaxon();
		while (supertaxon != nullptr) {
			supertaxon_counts[supertaxon]++;
			supertaxon = supertaxon->get_supertaxon();
		}
	}

	for (const taxon *taxon : taxons) {
		const wyrmgus::taxon *supertaxon = taxon->get_supertaxon();
		while (supertaxon != nullptr) {
			supertaxon_counts[supertaxon]++;
			supertaxon = supertaxon->get_supertaxon();
		}
	}

	return supertaxon_counts;
}

std::vector<std::string> species::get_name_list(const std::vector<const species *> &source_species_list)
{
	static constexpr size_t max_species_names_size = 20;

	std::vector<const species *> species_list = source_species_list;
	std::vector<const taxon *> taxons;
	
	std::vector<std::string> species_names;
	for (const species *species : species_list) {
		species_names.push_back(species->get_name());
	}
	
	bool changed_name_list = true;
	std::map<const taxon *, int> supertaxon_counts;
	while (species_names.size() > max_species_names_size && changed_name_list == true) {
		changed_name_list = false;
		
		supertaxon_counts = get_supertaxon_counts(species_list, taxons);

		//get the taxon with lowest rank and highest count
		const taxon *best_taxon = nullptr;
		taxonomic_rank best_taxonomic_rank = taxonomic_rank::none;
		int best_value = 0;
		for (const auto &kv_pair : supertaxon_counts) {
			const taxon *taxon = kv_pair.first;
			const int value = kv_pair.second;
			if (value > 1) {
				const taxonomic_rank taxonomic_rank = taxon->get_rank();
				if (best_taxon == nullptr || taxonomic_rank < best_taxonomic_rank || (taxonomic_rank == best_taxonomic_rank && value > best_value)) {
					best_taxon = taxon;
					best_taxonomic_rank = taxonomic_rank;
					best_value = value;
				}
			}
		}
		
		if (best_taxon != nullptr) {
			changed_name_list = true;
			
			std::vector<const species *> new_species_list;
			std::vector<const taxon *> new_taxons;
			
			for (const species *species : species_list) {
				if (!species->is_subtaxon_of(best_taxon)) {
					new_species_list.push_back(species);
				}
			}
			for (const taxon *taxon : taxons) {
				if (!taxon->is_subtaxon_of(best_taxon)) {
					new_taxons.push_back(taxon);
				}
			}
			new_taxons.push_back(best_taxon);
			
			species_list = new_species_list;
			taxons = new_taxons;
			
			species_names.clear();
			
			for (const species *species : species_list) {
				species_names.push_back(species->get_name());
			}
			for (const taxon *taxon : taxons) {
				species_names.push_back(taxon->get_common_name());
			}
		}
	}

	std::sort(species_names.begin(), species_names.end());

	return species_names;
}

species::species(const std::string &identifier) : taxon_base(identifier), era(geological_era::none)
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
		taxon_base::process_sml_scope(scope);
	}
}

void species::initialize()
{
	if (this->get_homeworld() != nullptr) {
		this->get_homeworld()->add_native_species(this);
	}

	taxon_base::initialize();
}

void species::check() const
{
	if (this->get_supertaxon() == nullptr) {
		throw std::runtime_error("Species \"" + this->get_identifier() + "\" has no supertaxon.");
	}

	for (const species *pre_evolution : this->get_pre_evolutions()) {
		if (this->get_era() != geological_era::none && pre_evolution->get_era() != geological_era::none && this->get_era() <= pre_evolution->get_era()) {
			throw std::runtime_error("Species \"" + this->get_identifier() + "\" is set to evolve from \"" + pre_evolution->get_identifier() + "\", but is from the same or an earlier era than the latter.");
		}
	}
}

taxonomic_rank species::get_rank() const
{
	return taxonomic_rank::species;
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
			(evolution->get_unit_type() != nullptr && (!terrain || vector::contains(evolution->get_native_terrain_types(), terrain)) && (!sapient_only || evolution->is_sapient()))
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
			(evolution->get_unit_type() != nullptr && vector::contains(evolution->get_native_terrain_types(), terrain))
			|| evolution->has_evolution(terrain)
		) {
			potential_evolutions.push_back(evolution);
		}
	}
	
	if (potential_evolutions.size() == 0) {
		for (const species *evolution : this->get_evolutions()) {
			if (evolution->get_unit_type() != nullptr || evolution->has_evolution()) {
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