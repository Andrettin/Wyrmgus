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

#pragma once

class CConfigData;

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace wyrmgus {

class terrain_type;
class tile;

class generated_terrain final
{
public:
	generated_terrain()
	{
	}

	explicit generated_terrain(wyrmgus::terrain_type *terrain_type) : terrain_type(terrain_type)
	{
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void ProcessConfigData(const CConfigData *config_data);

	const wyrmgus::terrain_type *get_terrain_type() const
	{
		return this->terrain_type;
	}

	int get_seed_count() const
	{
		return this->seed_count;
	}

	int get_expansion_chance() const
	{
		return this->expansion_chance;
	}

	int get_max_percent() const
	{
		return this->max_percent;
	}

	bool uses_existing_as_seeds() const
	{
		return this->use_existing_as_seeds;
	}

	bool uses_subtemplate_borders_as_seeds() const
	{
		return this->use_subtemplate_borders_as_seeds;
	}

	bool targets_terrain_type(const wyrmgus::terrain_type *terrain_type) const
	{
		return this->target_terrain_types.contains(terrain_type);
	}

	bool can_use_tile_as_seed(const tile *tile) const;
	bool can_generate_on_tile(const tile *tile) const;
	bool can_tile_be_part_of_expansion(const tile *tile) const;
	bool can_remove_tile_overlay_terrain(const tile *tile) const;

private:
	const wyrmgus::terrain_type *terrain_type = nullptr;
	int seed_count = 0;
	int expansion_chance = 50; //50% chance to expand to a tile by default
	int max_percent = 0; //the maximum percentage of tiles in the map subtemplate that should have the generated terrain type as their top tile
	bool use_existing_as_seeds = false; //whether to use existing tiles of the given terrain in the map layer as seeds for this terrain generation
	bool use_subtemplate_borders_as_seeds = false; //whether to use the border tiles of subtemplates that have the given terrain as seeds for this terrain generation
	std::set<const wyrmgus::terrain_type *> target_terrain_types; //the terrain types over which the terrain is to be generated
};

}
