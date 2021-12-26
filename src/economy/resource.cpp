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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "economy/resource.h"

#include "database/defines.h"
#include "util/util.h"
#include "video/video.h"

namespace wyrmgus {

int resource::get_price(const resource_map<int> &costs)
{
	int price = 0;

	for (const auto &[resource, resource_cost] : costs) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		if (resource_cost > 0) {
			if (resource == defines::get()->get_wealth_resource()) {
				price += resource_cost;
			} else {
				price += resource_cost * resource->get_base_price() / 100;
			}
		}

	}

	return price;
}

int resource::get_mass_multiplier(const uint64_t mass, const uint64_t base_mass)
{
	int mass_multiplier = 100;

	if (mass < base_mass) {
		mass_multiplier *= 10;
		mass_multiplier /= static_cast<int>(isqrt(base_mass * 100 / mass));
	} else {
		mass_multiplier *= static_cast<int>(isqrt(mass * 100 / base_mass));
		mass_multiplier /= 10;
	}

	return mass_multiplier;
}

void resource::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "action_name") {
		this->action_name = value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void resource::initialize()
{
	if (this->final_resource != nullptr) {
		this->final_resource->ChildResources.push_back(this);
	}

	if (this->is_luxury()) {
		resource::luxury_resources.push_back(this);
	}

	data_entry::initialize();
}

bool resource::IsMineResource() const
{
	switch (this->get_index()) {
		case CopperCost:
		case SilverCost:
		case GoldCost:
		case IronCost:
		case MithrilCost:
		case CoalCost:
		case DiamondsCost:
		case EmeraldsCost:
			return true;
		default:
			return false;
	}
}

}