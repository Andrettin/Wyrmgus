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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "vec2i.h"

struct lua_State;
//Wyrmgus start
class CGraphic;
class CPlayerColorGraphic;
//Wyrmgus end

namespace wyrmgus {
	class terrain_type;
	class unit_type;
	enum class tile_flag : uint32_t;
}

/**
**  These are used for lookup tiles types
**  mainly used for the FOW implementation of the seen woods/rocks
**
**  @todo I think this can be removed, we can use the flags?
**  I'm not sure, if we have seen and real time to considere.
*/
enum TileType {
	TileTypeUnknown,	/// Unknown tile type
	TileTypeWood,		/// Any wood tile
	TileTypeRock,		/// Any rock tile
	TileTypeCoast,		/// Any coast tile
	TileTypeWall,		/// Any wall tile
	TileTypeWater		/// Any water tile
};

namespace wyrmgus {

enum class tile_transition_type {
	none = -1,
	north,
	south,
	west,
	east,
	northwest_outer,
	northeast_outer,
	southwest_outer,
	southeast_outer,
	northwest_inner,
	northeast_inner,
	southwest_inner,
	southeast_inner,
	northwest_southeast_inner,
	northeast_southwest_inner,
	
	//single tile transition types (enabled for terrain types with the allow_single field set to true)
	single,
	north_single,
	south_single,
	west_single,
	east_single,
	north_south,
	west_east,
	northwest_northeast_southwest_southeast_inner,
	northwest_northeast_inner,
	southwest_southeast_inner,
	northwest_southwest_inner,
	northeast_southeast_inner,
	northwest_northeast_southwest_inner,
	northwest_northeast_southeast_inner,
	northwest_southwest_southeast_inner,
	northeast_southwest_southeast_inner,
	north_southwest_inner_southeast_inner,
	north_southwest_inner,
	north_southeast_inner,
	south_northwest_inner_northeast_inner,
	south_northwest_inner,
	south_northeast_inner,
	west_northeast_inner_southeast_inner,
	west_northeast_inner,
	west_southeast_inner,
	east_northwest_inner_southwest_inner,
	east_northwest_inner,
	east_southwest_inner,
	northwest_outer_southeast_inner,
	northeast_outer_southwest_inner,
	southwest_outer_northeast_inner,
	southeast_outer_northwest_inner
};

}

/// Single tile definition
struct CTileInfo final
{
public:
	CTileInfo()
	{
	}

	CTileInfo(unsigned char base, unsigned char mix) : BaseTerrain(base), MixTerrain(mix)
	{
	}

	bool operator ==(const CTileInfo &rhs) const
	{
		return BaseTerrain == rhs.BaseTerrain && MixTerrain == rhs.MixTerrain;
	}
	bool operator !=(const CTileInfo &rhs) const { return !(*this == rhs); }

public:
	unsigned char BaseTerrain = 0; /// Basic terrain of the tile
	unsigned char MixTerrain = 0;  /// Terrain mixed with this
};

/// Definition for a terrain type
struct SolidTerrainInfo final
{
	std::string TerrainName;  /// Name of the terrain
	// TODO: When drawing with the editor add some kind fo probabilities for every tile.
	//Wyrmgus start
	int DefaultTileIndex;		/// Index for the default tile for this type
	//Wyrmgus end
};

class CTile final
{
public:
	CTile();

	unsigned short tile = 0;  /// graphical pos
	tile_flag flag;  /// Flag
	CTileInfo tileinfo;   /// Tile descriptions
};

namespace wyrmgus {
	
class tileset final : public named_data_entry, public data_type<tileset>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "tileset";
	static constexpr const char *database_folder = "tilesets";

	explicit tileset(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	virtual void check() const override
	{
		if (this->get_identifier().empty()) {
			throw std::runtime_error("Tileset has no identifier.");
		}
	}

	const terrain_type *get_terrain_type_by_tile_number(const int tile_number) const
	{
		const auto find_iterator = this->terrain_types_by_tile_number.find(tile_number);
		if (find_iterator != this->terrain_types_by_tile_number.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("No terrain type found for tileset \"" + this->get_identifier() + "\" for tile number: " + std::to_string(tile_number) + ".");
	}

	void map_terrain_type_to_tile_number(const terrain_type *terrain_type, const int tile_number);

	unsigned int getTileCount() const
	{
		return this->tiles.size();
	}

	unsigned int getDefaultTileIndex() const;

	const std::string &getTerrainName(int solidTerrainIndex) const;

	int findTileIndexByTile(unsigned int tile) const;
	unsigned int getTileNumber(int basic, bool random, bool filler) const;

	void parse(lua_State *l);
	void buildTable();

private:
	unsigned int getOrAddSolidTileIndexByName(const std::string &name);
	int findTileIndex(unsigned char baseTerrain, unsigned char mixTerrain = 0) const;
	int getTileIndex(unsigned char baseTerrain, unsigned char mixTerrain, unsigned int quad) const;
	void parseSlots(lua_State *l, int t);
	void parseSpecial(lua_State *l);
	void parseSolid(lua_State *l);
	void parseMixed(lua_State *l);

public:
	//Wyrmgus start
	std::string Ident;			/// Ident of the tileset
	//Wyrmgus end

	std::vector<CTile> tiles;

	//Wyrmgus start
	std::vector<SolidTerrainInfo> solidTerrainTypes; /// Information about solid terrains.
	//Wyrmgus end

private:
	std::map<int, const terrain_type *> terrain_types_by_tile_number;
};

}

//Wyrmgus start
extern void ParseTilesetTileFlags(lua_State *l, tile_flag *back, int *j);
//Wyrmgus start
extern std::string GetTransitionTypeNameById(const wyrmgus::tile_transition_type transition_type);
extern wyrmgus::tile_transition_type GetTransitionTypeIdByName(const std::string &transition_type);
//Wyrmgus end
