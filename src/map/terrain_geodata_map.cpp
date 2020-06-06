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

#include "map/terrain_geodata_map.h"

#include "map/terrain_feature.h"
#include "map/terrain_type.h"

namespace stratagus {

bool terrain_geodata_map_compare::operator()(const terrain_geodata_key &terrain_variant, const terrain_geodata_key  &other_terrain_variant) const
{
	const terrain_type *terrain = nullptr;
	const terrain_feature *terrain_feature = nullptr;
	if (std::holds_alternative<const stratagus::terrain_feature *>(terrain_variant)) {
		terrain_feature = std::get<const stratagus::terrain_feature *>(terrain_variant);
		terrain = terrain_feature->get_terrain_type();
	} else {
		terrain = std::get<const terrain_type *>(terrain_variant);
	}

	const terrain_type *other_terrain = nullptr;
	const stratagus::terrain_feature *other_terrain_feature = nullptr;
	if (std::holds_alternative<const stratagus::terrain_feature *>(other_terrain_variant)) {
		other_terrain_feature = std::get<const stratagus::terrain_feature *>(other_terrain_variant);
		other_terrain = other_terrain_feature->get_terrain_type();
	} else {
		other_terrain = std::get<const terrain_type *>(other_terrain_variant);
	}

	if (terrain->is_water() != other_terrain->is_water()) {
		return terrain->is_water(); //give priority to water terrain, so that it is written first
	}

	if (terrain == other_terrain && terrain_feature != other_terrain_feature) {
		if (terrain_feature == nullptr || other_terrain_feature == nullptr) {
			return terrain_feature == nullptr;
		}

		return terrain_feature->get_identifier() < other_terrain_feature->get_identifier();
	}

	return terrain->get_identifier() < other_terrain->get_identifier();
}

}
