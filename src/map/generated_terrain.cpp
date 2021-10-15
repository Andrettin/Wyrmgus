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

#include "map/generated_terrain.h"

#include "config.h"
#include "database/sml_data.h"
#include "database/sml_property.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/string_conversion_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void generated_terrain::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "seed_count") {
		this->seed_count = std::stoi(value);
	} else if (key == "expansion_chance") {
		this->expansion_chance = std::stoi(value);
	} else if (key == "max_percent") {
		this->max_percent = std::stoi(value);
	} else if (key == "use_existing_as_seeds") {
		this->use_existing_as_seeds = string::to_bool(value);
	} else if (key == "use_subtemplate_borders_as_seeds") {
		this->use_subtemplate_borders_as_seeds = string::to_bool(value);
	} else if (key == "target_terrain_type") {
		const wyrmgus::terrain_type *target_terrain_type = terrain_type::get(value);
		this->target_terrain_types.insert(target_terrain_type);
	} else {
		throw std::runtime_error("Invalid generated terrain property: \"" + key + "\".");
	}
}

void generated_terrain::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	throw std::runtime_error("Invalid generated terrain scope: \"" + tag + "\".");
}

void generated_terrain::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;

		if (key == "terrain_type") {
			this->terrain_type = terrain_type::get(value);
		} else if (key == "seed_count") {
			this->seed_count = std::stoi(value);
		} else if (key == "expansion_chance") {
			this->expansion_chance = std::stoi(value);
		} else if (key == "max_percent") {
			this->max_percent = std::stoi(value);
		} else if (key == "use_existing_as_seeds") {
			this->use_existing_as_seeds = string::to_bool(value);
		} else if (key == "use_subtemplate_borders_as_seeds") {
			this->use_subtemplate_borders_as_seeds = string::to_bool(value);
		} else if (key == "target_terrain_type") {
			const wyrmgus::terrain_type *target_terrain_type = terrain_type::get(value);
			this->target_terrain_types.insert(target_terrain_type);
		} else {
			fprintf(stderr, "Invalid generated terrain property: \"%s\".\n", key.c_str());
		}
	}

	if (this->get_terrain_type() == nullptr) {
		fprintf(stderr, "Generated terrain has no terrain type.\n");
	}
}

bool generated_terrain::can_use_tile_as_seed(const tile *tile) const
{
	const wyrmgus::terrain_type *top_terrain = tile->get_top_terrain();

	if (top_terrain == this->get_terrain_type()) {
		//top terrain is the same as the one for the generation, so the tile can be used as a seed
		return true;
	}

	if (this->get_terrain_type() == tile->get_terrain() && !this->targets_terrain_type(top_terrain)) {
		//the tile's base terrain is the same as the one for the generation, and its overlay terrain is not a target for the generation
		return true;
	}

	return false;
}

bool generated_terrain::can_generate_on_tile(const tile *tile) const
{
	if (this->get_terrain_type()->is_overlay()) {
		//disallow generating over terrains that aren't a target for the generation
		if (!this->targets_terrain_type(tile->get_top_terrain())) {
			return false;
		}
	} else {
		if (!this->targets_terrain_type(tile->get_top_terrain()) && !this->targets_terrain_type(tile->get_terrain())) {
			return false;
		}

		if ( //don't allow generating the terrain on the tile if it is a base terrain, and putting it there would destroy an overlay terrain that isn't a target of the generation
			tile->get_overlay_terrain() != nullptr
			&& !this->can_remove_tile_overlay_terrain(tile)
			&& !vector::contains(tile->get_overlay_terrain()->get_base_terrain_types(), this->get_terrain_type())
		) {
			return false;
		}

		if (!this->get_terrain_type()->is_border_terrain_type(tile->get_terrain())) {
			//don't allow generating on the tile if it can't be a border terrain to the terrain we want to generate
			return false;
		}
	}

	return true;
}

bool generated_terrain::can_tile_be_part_of_expansion(const tile *tile) const
{
	if (this->can_generate_on_tile(tile)) {
		return true;
	}

	if (this->get_terrain_type() == tile->get_top_terrain()) {
		return true;
	}

	if (!this->get_terrain_type()->is_overlay()) {
		if (this->get_terrain_type() == tile->get_terrain()) {
			return true;
		}
	}

	return false;
}

bool generated_terrain::can_remove_tile_overlay_terrain(const tile *tile) const
{
	if (!this->targets_terrain_type(tile->get_overlay_terrain())) {
		return false;
	}

	return true;
}

}
