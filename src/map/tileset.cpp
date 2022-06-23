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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/**
**  @class tileset tileset.h
**
**  \#include "map/tileset.h"
**
**  This structure contains information about the tileset of the map.
**  It defines the look and properties of the tiles. Currently only one
**  tileset per map is supported. In the future it is planned to support
**  multiple tilesets on the same map. Also it is planned to support animated
**  tiles.
**
**  The tileset structure members:
**
**  tileset::Name
**
**      Long name of the tileset. Can be used by the level editor.
**
**  tileset::Flags
**
**      Table of the tile flags used by the editor.
**      @see wyrmgus::tile::Flags
**
**  tileset::solidTerrainTypes
**
**      Index to name of the basic tile type. FE. "light-water".
**      If the index is 0, the tile is not used.
**
**  @struct TileInfo tileset.h
**
**  \#include "map/tileset.h"
**
**  This structure includes everything about a specific tile from the tileset.
**
**  TileInfo::BaseTerrain
**
**      This is the base terrain type of a tile. Only 15 of those
**      are currently supported.
**
**  TileInfo::MixTerrain
**
**  @todo This is the terrain the tile is mixed with. This is 0 for
**    a solid tile, we should make it equal to BaseTerrain
*/

#include "stratagus.h"

#include "map/tileset.h"

#include "map/terrain_type.h"
#include "map/tile_flag.h"
#include "util/assert_util.h"
#include "util/random.h"
#include "util/util.h"
#include "util/vector_random_util.h"
#include "video/video.h"

CTile::CTile() : flag(tile_flag::none)
{
}

namespace wyrmgus {

void tileset::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "default_base_terrains") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const terrain_type *overlay_terrain = terrain_type::get(key);
			const terrain_type *base_terrain = terrain_type::get(value);

			this->default_base_terrains[overlay_terrain] = base_terrain;
		});
	} else if (tag == "tile_numbers") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const terrain_type *terrain_type = terrain_type::get(child_scope.get_tag());

			for (const std::string &value : child_scope.get_values()) {
				this->map_terrain_type_to_tile_number(terrain_type, std::stoi(value));
			}
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void tileset::map_terrain_type_to_tile_number(const terrain_type *terrain_type, const int tile_number)
{
	if (this->terrain_types_by_tile_number.contains(tile_number)) {
		throw std::runtime_error("Tile number \"" + std::to_string(tile_number) + "\" is already used by another terrain type for tileset \"" + this->get_identifier() + "\".");
	}

	this->terrain_types_by_tile_number[tile_number] = terrain_type;
}

unsigned int tileset::getDefaultTileIndex() const
{
	// TODO: remove hardcoded value.
	return 0x50;
}

unsigned int tileset::getOrAddSolidTileIndexByName(const std::string &name)
{
	for (size_t i = 0; i != solidTerrainTypes.size(); ++i) {
		if (solidTerrainTypes[i].TerrainName == name) {
			return i;
		}
	}
	// Can't find it, then we add another solid terrain type.
	SolidTerrainInfo s;
	s.TerrainName = name;
	//Wyrmgus start
	s.DefaultTileIndex = 0;
	//Wyrmgus end
	solidTerrainTypes.push_back(s);
	return solidTerrainTypes.size() - 1;
}

const std::string &tileset::getTerrainName(int solidTerrainIndex) const
{
	return solidTerrainTypes[solidTerrainIndex].TerrainName;
}

int tileset::findTileIndex(unsigned char baseTerrain, unsigned char mixTerrain) const
{
	const CTileInfo tileInfo(baseTerrain, mixTerrain);

	for (size_t i = 0; i != tiles.size();) {
		if (tiles[i].tileinfo == tileInfo) {
			return i;
		}
		// Advance solid or mixed.
		if (!tiles[i].tileinfo.MixTerrain) {
			i += 16;
		} else {
			i += 256;
		}
	}
	return -1;
}

int tileset::getTileIndex(unsigned char baseTerrain, unsigned char mixTerrain, unsigned int quad) const
{
	int tileIndex = findTileIndex(baseTerrain, mixTerrain);
	if (tileIndex == -1) {
		tileIndex = findTileIndex(mixTerrain, baseTerrain);
		if (tileIndex == -1) {
			return -1;
		}
		std::swap(baseTerrain, mixTerrain);
	}
	int base = tileIndex;

	int direction = 0;
	for (int i = 0; i != 4; ++i) {
		if (((quad >> (8 * i)) & 0xFF) == baseTerrain) {
			direction |= 1 << i;
		}
	}
	//                       0  1  2  3   4  5  6  7   8  9  A   B  C   D  E  F
	static constexpr std::array<const char, 16> table = { 0, 7, 3, 11, 1, 9, 5, 13, 0, 8, 4, 12, 2, 10, 6, 0 };
	return base | (table[direction] << 4);
}

int tileset::findTileIndexByTile(unsigned int tile) const
{
	for (size_t i = 0; i != tiles.size(); ++i) {
		if (tile == tiles[i].tile) {
			return i;
		}
	}
	return -1;
}

/**
**  Get tile number.
**
**  @param basic   Basic tile number
**  @param random  Return random tile
**  @param filler  Get a decorated tile.
**
**  @return        Tile index number.
**
**  @todo  FIXME: Solid tiles are here still hardcoded.
*/
unsigned int tileset::getTileNumber(int basic, bool random, bool filler) const
{
	int tile = basic;
	if (random) {
		int n = 0;
		for (int i = 0; i < 16; ++i) {
			if (!tiles[tile + i].tile) {
				if (!filler) {
					break;
				}
			} else {
				++n;
			}
		}
		n = random::get()->generate_async(n);
		int i = -1;
		do {
			while (++i < 16 && !tiles[tile + i].tile) {
			}
		} while (i < 16 && n--);
		assert_throw(i != 16);
		return tile + i;
	}
	if (filler) {
		int i = 0;
		for (; i < 16 && tiles[tile + i].tile; ++i) {
		}
		for (; i < 16 && !tiles[tile + i].tile; ++i) {
		}
		if (i != 16) {
			return tile + i;
		}
	}
	return tile;
}

}

std::string GetTransitionTypeNameById(const wyrmgus::tile_transition_type transition_type)
{
	if (transition_type == wyrmgus::tile_transition_type::north) {
		return "north";
	} else if (transition_type == wyrmgus::tile_transition_type::south) {
		return "south";
	} else if (transition_type == wyrmgus::tile_transition_type::west) {
		return "west";
	} else if (transition_type == wyrmgus::tile_transition_type::east) {
		return "east";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_outer) {
		return "northwest-outer";
	} else if (transition_type == wyrmgus::tile_transition_type::northeast_outer) {
		return "northeast-outer";
	} else if (transition_type == wyrmgus::tile_transition_type::southwest_outer) {
		return "southwest-outer";
	} else if (transition_type == wyrmgus::tile_transition_type::southeast_outer) {
		return "southeast-outer";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_inner) {
		return "northwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northeast_inner) {
		return "northeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::southwest_inner) {
		return "southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::southeast_inner) {
		return "southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_southeast_inner) {
		return "northwest-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northeast_southwest_inner) {
		return "northeast-southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::single) {
		return "single";
	} else if (transition_type == wyrmgus::tile_transition_type::north_single) {
		return "north-single";
	} else if (transition_type == wyrmgus::tile_transition_type::south_single) {
		return "south-single";
	} else if (transition_type == wyrmgus::tile_transition_type::west_single) {
		return "west-single";
	} else if (transition_type == wyrmgus::tile_transition_type::east_single) {
		return "east-single";
	} else if (transition_type == wyrmgus::tile_transition_type::north_south) {
		return "north-south";
	} else if (transition_type == wyrmgus::tile_transition_type::west_east) {
		return "west-east";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_northeast_southwest_southeast_inner) {
		return "northwest-northeast-southwest-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_northeast_inner) {
		return "northwest-northeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::southwest_southeast_inner) {
		return "southwest-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_southwest_inner) {
		return "northwest-southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northeast_southeast_inner) {
		return "northeast-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_northeast_southwest_inner) {
		return "northwest-northeast-southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_northeast_southeast_inner) {
		return "northwest-northeast-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_southwest_southeast_inner) {
		return "northwest-southwest-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northeast_southwest_southeast_inner) {
		return "northeast-southwest-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::north_southwest_inner_southeast_inner) {
		return "north-southwest-inner-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::north_southwest_inner) {
		return "north-southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::north_southeast_inner) {
		return "north-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::south_northwest_inner_northeast_inner) {
		return "south-northwest-inner-northeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::south_northwest_inner) {
		return "south-northwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::south_northeast_inner) {
		return "south-northeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::west_northeast_inner_southeast_inner) {
		return "west-northeast-inner-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::west_northeast_inner) {
		return "west-northeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::west_southeast_inner) {
		return "west-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::east_northwest_inner_southwest_inner) {
		return "east-northwest-inner-southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::east_northwest_inner) {
		return "east-northwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::east_southwest_inner) {
		return "east-southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northwest_outer_southeast_inner) {
		return "northwest-outer-southeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::northeast_outer_southwest_inner) {
		return "northeast-outer-southwest-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::southwest_outer_northeast_inner) {
		return "southwest-outer-northeast-inner";
	} else if (transition_type == wyrmgus::tile_transition_type::southeast_outer_northwest_inner) {
		return "southeast-outer-northwest-inner";
	}

	throw std::runtime_error("Invalid tile transition type: \"" + std::to_string(static_cast<int>(transition_type)) + "\".");
}

wyrmgus::tile_transition_type GetTransitionTypeIdByName(const std::string &transition_type)
{
	if (transition_type == "north") {
		return wyrmgus::tile_transition_type::north;
	} else if (transition_type == "south") {
		return wyrmgus::tile_transition_type::south;
	} else if (transition_type == "west") {
		return wyrmgus::tile_transition_type::west;
	} else if (transition_type == "east") {
		return wyrmgus::tile_transition_type::east;
	} else if (transition_type == "northwest-outer") {
		return wyrmgus::tile_transition_type::northwest_outer;
	} else if (transition_type == "northeast-outer") {
		return wyrmgus::tile_transition_type::northeast_outer;
	} else if (transition_type == "southwest-outer") {
		return wyrmgus::tile_transition_type::southwest_outer;
	} else if (transition_type == "southeast-outer") {
		return wyrmgus::tile_transition_type::southeast_outer;
	} else if (transition_type == "northwest-inner") {
		return wyrmgus::tile_transition_type::northwest_inner;
	} else if (transition_type == "northeast-inner") {
		return wyrmgus::tile_transition_type::northeast_inner;
	} else if (transition_type == "southwest-inner") {
		return wyrmgus::tile_transition_type::southwest_inner;
	} else if (transition_type == "southeast-inner") {
		return wyrmgus::tile_transition_type::southeast_inner;
	} else if (transition_type == "northwest-southeast-inner") {
		return wyrmgus::tile_transition_type::northwest_southeast_inner;
	} else if (transition_type == "northeast-southwest-inner") {
		return wyrmgus::tile_transition_type::northeast_southwest_inner;
	} else if (transition_type == "single") {
		return wyrmgus::tile_transition_type::single;
	} else if (transition_type == "north-single") {
		return wyrmgus::tile_transition_type::north_single;
	} else if (transition_type == "south-single") {
		return wyrmgus::tile_transition_type::south_single;
	} else if (transition_type == "west-single") {
		return wyrmgus::tile_transition_type::west_single;
	} else if (transition_type == "east-single") {
		return wyrmgus::tile_transition_type::east_single;
	} else if (transition_type == "north-south") {
		return wyrmgus::tile_transition_type::north_south;
	} else if (transition_type == "west-east") {
		return wyrmgus::tile_transition_type::west_east;
	} else if (transition_type == "northwest-northeast-southwest-southeast-inner") {
		return wyrmgus::tile_transition_type::northwest_northeast_southwest_southeast_inner;
	} else if (transition_type == "northwest-northeast-inner") {
		return wyrmgus::tile_transition_type::northwest_northeast_inner;
	} else if (transition_type == "southwest-southeast-inner") {
		return wyrmgus::tile_transition_type::southwest_southeast_inner;
	} else if (transition_type == "northwest-southwest-inner") {
		return wyrmgus::tile_transition_type::northwest_southwest_inner;
	} else if (transition_type == "northeast-southeast-inner") {
		return wyrmgus::tile_transition_type::northeast_southeast_inner;
	} else if (transition_type == "northwest-northeast-southwest-inner") {
		return wyrmgus::tile_transition_type::northwest_northeast_southwest_inner;
	} else if (transition_type == "northwest-northeast-southeast-inner") {
		return wyrmgus::tile_transition_type::northwest_northeast_southeast_inner;
	} else if (transition_type == "northwest-southwest-southeast-inner") {
		return wyrmgus::tile_transition_type::northwest_southwest_southeast_inner;
	} else if (transition_type == "northeast-southwest-southeast-inner") {
		return wyrmgus::tile_transition_type::northeast_southwest_southeast_inner;
	} else if (transition_type == "north-southwest-inner-southeast-inner") {
		return wyrmgus::tile_transition_type::north_southwest_inner_southeast_inner;
	} else if (transition_type == "north-southwest-inner") {
		return wyrmgus::tile_transition_type::north_southwest_inner;
	} else if (transition_type == "north-southeast-inner") {
		return wyrmgus::tile_transition_type::north_southeast_inner;
	} else if (transition_type == "south-northwest-inner-northeast-inner") {
		return wyrmgus::tile_transition_type::south_northwest_inner_northeast_inner;
	} else if (transition_type == "south-northwest-inner") {
		return wyrmgus::tile_transition_type::south_northwest_inner;
	} else if (transition_type == "south-northeast-inner") {
		return wyrmgus::tile_transition_type::south_northeast_inner;
	} else if (transition_type == "west-northeast-inner-southeast-inner") {
		return wyrmgus::tile_transition_type::west_northeast_inner_southeast_inner;
	} else if (transition_type == "west-northeast-inner") {
		return wyrmgus::tile_transition_type::west_northeast_inner;
	} else if (transition_type == "west-southeast-inner") {
		return wyrmgus::tile_transition_type::west_southeast_inner;
	} else if (transition_type == "east-northwest-inner-southwest-inner") {
		return wyrmgus::tile_transition_type::east_northwest_inner_southwest_inner;
	} else if (transition_type == "east-northwest-inner") {
		return wyrmgus::tile_transition_type::east_northwest_inner;
	} else if (transition_type == "east-southwest-inner") {
		return wyrmgus::tile_transition_type::east_southwest_inner;
	} else if (transition_type == "northwest-outer-southeast-inner") {
		return wyrmgus::tile_transition_type::northwest_outer_southeast_inner;
	} else if (transition_type == "northeast-outer-southwest-inner") {
		return wyrmgus::tile_transition_type::northeast_outer_southwest_inner;
	} else if (transition_type == "southwest-outer-northeast-inner") {
		return wyrmgus::tile_transition_type::southwest_outer_northeast_inner;
	} else if (transition_type == "southeast-outer-northwest-inner") {
		return wyrmgus::tile_transition_type::southeast_outer_northwest_inner;
	}

	throw std::runtime_error("Invalid tile transition type: \"" + transition_type + "\".");
}
