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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "map/region.h"

#include "map/region_history.h"
#include "map/site.h"
#include "map/site_history.h"
#include "map/terrain_feature.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void region::load_history_database()
{
	data_type::load_history_database();

	std::vector<region *> regions = region::get_all();

	std::sort(regions.begin(), regions.end(), [](const region *lhs, const region *rhs) {
		//give priority to subregions
		if (lhs->is_part_of(rhs)) {
			return true;
		} else if (rhs->is_part_of(lhs)) {
			return false;
		}

		//give priority to smaller regions
		if (lhs->settlements.size() != rhs->settlements.size()) {
			return lhs->settlements.size() < rhs->settlements.size();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const region *region : regions) {
		region->distribute_population();
		region->distribute_population_groups();
	}
}

region::region(const std::string &identifier) : data_entry(identifier)
{
}

region::~region()
{
}

void region::initialize()
{
	for (region *subregion : this->subregions) {
		//initialize subregions, so that the sites of their own subregions are added to them
		if (!subregion->is_initialized()) {
			subregion->initialize();
		}

		//add sites from subregions
		for (site *site : subregion->get_sites()) {
			if (vector::contains(this->sites, site)) {
				continue;
			}

			this->sites.push_back(site);
			site->add_region(this);
		}

		//add terrain features from subregions
		for (terrain_feature *terrain_feature : subregion->get_terrain_features()) {
			if (vector::contains(this->terrain_features, terrain_feature)) {
				continue;
			}

			this->terrain_features.push_back(terrain_feature);
			terrain_feature->add_region(this);
		}
	}

	for (site *site : this->get_sites()) {
		if (site->is_settlement()) {
			this->settlements.push_back(site);
		}
	}

	data_entry::initialize();
}

data_entry_history *region::get_history_base()
{
	return this->history.get();
}

void region::reset_history()
{
	this->history = std::make_unique<region_history>();
}

void region::add_site(site *site)
{
	if (vector::contains(this->sites, site)) {
		return;
	}

	this->sites.push_back(site);
}

void region::remove_site(site *site)
{
	vector::remove(this->sites, site);
}

void region::add_terrain_feature(terrain_feature *terrain_feature)
{
	if (vector::contains(this->terrain_features, terrain_feature)) {
		return;
	}

	this->terrain_features.push_back(terrain_feature);
}

void region::remove_terrain_feature(terrain_feature *terrain_feature)
{
	vector::remove(this->terrain_features, terrain_feature);
}

QVariantList region::get_superregions_qvariant_list() const
{
	return container::to_qvariant_list(this->superregions);
}

void region::remove_superregion(region *superregion)
{
	vector::remove(this->superregions, superregion);
	vector::remove(superregion->subregions, this);
}

bool region::is_part_of(const region *other_region) const
{
	if (vector::contains(this->superregions, other_region)) {
		return true;
	}

	for (const region *superregion : this->superregions) {
		if (superregion->is_part_of(other_region)) {
			return true;
		}
	}

	return false;
}

void region::distribute_population() const
{
	const region_history *region_history = this->get_history();

	int64_t population = region_history->get_population();

	if (population == 0) {
		return;
	}

	int unpopulated_site_count = 0;

	//subtract the predefined population of settlements in the region from that of the region
	for (const site *site : this->get_sites()) {
		if (!site->can_have_population()) {
			continue;
		}

		const site_history *site_history = site->get_history();

		if (site_history->get_population() != 0) {
			population -= site_history->get_population();
		} else {
			++unpopulated_site_count;
		}
	}

	if (population <= 0 || unpopulated_site_count == 0) {
		return;
	}

	//apply the remaining population to sites without a predefined population in history
	const int64_t population_per_site = population / unpopulated_site_count;

	for (const site *site : this->get_sites()) {
		if (!site->can_have_population()) {
			continue;
		}

		site_history *site_history = site->get_history();

		if (site_history->get_population() == 0) {
			site_history->set_population(population_per_site);
		}
	}
}

void region::distribute_population_groups() const
{
	const region_history *region_history = this->get_history();

	for (const auto &[population_class, population] : region_history->get_population_groups()) {
		this->distribute_population_group(population_class, population);
	}
}

void region::distribute_population_group(const population_class *population_class, int64_t population) const
{
	if (population == 0) {
		return;
	}

	int unpopulated_site_count = 0; //sites without population of the specific class

	//subtract the predefined population of settlements in the region from that of the region, for the appropriate population class
	for (const site *site : this->get_sites()) {
		if (!site->can_have_population()) {
			continue;
		}

		const site_history *site_history = site->get_history();

		const int64_t site_population = site_history->get_group_population(population_class);

		if (site_population != 0) {
			population -= site_population;
		} else {
			++unpopulated_site_count;
		}
	}

	if (population <= 0 || unpopulated_site_count == 0) {
		return;
	}

	//apply the remaining population to sites without a predefined population for the population class in history
	const int64_t population_per_site = population / unpopulated_site_count;

	for (const site *site : this->get_sites()) {
		if (!site->can_have_population()) {
			continue;
		}

		site_history *site_history = site->get_history();

		const int64_t site_population = site_history->get_group_population(population_class);

		if (site_population == 0) {
			site_history->set_group_population(population_class, population_per_site);
		}
	}
}

}
