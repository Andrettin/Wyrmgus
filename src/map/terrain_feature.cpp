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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "map/terrain_feature.h"

#include "map/region.h"
#include "map/terrain_type.h"
#include "player/civilization.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void terrain_feature::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();
	const gsml_operator gsml_operator = scope.get_operator();

	if (tag == "cultural_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const civilization *civilization = civilization::get(property.get_key());
			this->cultural_names[civilization] = property.get_value();
		});
	} else if (tag == "regions") {
		for (const std::string &value : values) {
			region *region = region::get(value);

			switch (gsml_operator) {
				case gsml_operator::assignment:
				case gsml_operator::addition:
					this->add_region(region);
					break;
				case gsml_operator::subtraction:
					this->remove_region(region);
					break;
				default:
					assert_throw(false);
			}
		}
		scope.for_each_property([&](const gsml_property &property) {
			const civilization *civilization = civilization::get(property.get_key());
			this->cultural_names[civilization] = property.get_value();
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void terrain_feature::set_color(const QColor &color)
{
	if (color == this->get_color()) {
		return;
	}

	if (terrain_feature::try_get_by_color(color) != nullptr) {
		throw std::runtime_error("Color is already used by another terrain feature.");
	} else if (terrain_type::try_get_by_color(color) != nullptr) {
		throw std::runtime_error("Color is already used by a terrain type.");
	}

	this->color = color;
	terrain_feature::terrain_features_by_color[color] = this;
}

void terrain_feature::add_region(region *region)
{
	if (!vector::contains(this->regions, region)) {
		this->regions.push_back(region);
	}

	region->add_terrain_feature(this);
}

void terrain_feature::remove_region(region *region)
{
	vector::remove(this->regions, region);
	region->remove_terrain_feature(this);
}

}
