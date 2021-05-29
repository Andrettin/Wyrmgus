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
//      (c) Copyright 2019-2021 by Andrettin
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
		const region_history *region_history = region->get_history();
		
		int population = region_history->get_population();

		if (population == 0) {
			continue;
		}

		int unpopulated_settlement_count = 0;

		//subtract the predefined population of settlements in the region from that of the region
		for (const site *settlement : region->settlements) {
			const site_history *settlement_history = settlement->get_history();

			if (settlement_history->get_population() != 0) {
				population -= settlement_history->get_population();
			} else {
				++unpopulated_settlement_count;
			}
		}

		if (population <= 0 || unpopulated_settlement_count == 0) {
			continue;
		}

		//apply the remaining population to settlements without a predefined population in history
		const int population_per_settlement = population / unpopulated_settlement_count;

		for (const site *settlement : region->settlements) {
			site_history *settlement_history = settlement->get_history();

			if (settlement_history->get_population() == 0) {
				settlement_history->set_population(population_per_settlement);
			}
		}
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
	this->sites.push_back(site);
}

void region::remove_site(site *site)
{
	vector::remove(this->sites, site);
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

}
