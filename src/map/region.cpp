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

#include "map/region.h"

#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

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

	data_entry::initialize();
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

}
