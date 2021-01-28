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
/**@name tileset.h - The tileset headerfile. */
//
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#pragma once

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#include "vec2i.h"
//Wyrmgus start
#include "color.h"
//Wyrmgus end

struct lua_State;
//Wyrmgus start
class CGraphic;
class CPlayerColorGraphic;
//Wyrmgus end

namespace wyrmgus {
	class unit_type;
}

enum MapFieldFlag : unsigned long long {
	MapFieldSpeedMask = 1 << 0,		/// Move faster on this tile
	
	MapFieldLandAllowed = 1 << 1,	/// Land units allowed
	MapFieldCoastAllowed = 1 << 2,	/// Coast (e.g. transporter) units allowed
	MapFieldWaterAllowed = 1 << 3,	/// Water units allowed
	MapFieldNoBuilding = 1 << 4,	/// No buildings allowed
	
	MapFieldUnpassable = 1 << 5,	/// Field is movement blocked
	MapFieldAirUnpassable = 1 << 6,	/// Field is movement blocked for air units and missiles
	MapFieldWall = 1 << 7,			/// Field contains wall
	MapFieldRocks = 1 << 8,			/// Field contains rocks
	MapFieldForest = 1 << 9,		/// Field contains forest
	
	MapFieldLandUnit = 1 << 10,		/// Land unit on field
	MapFieldSeaUnit = 1 << 11,		/// Water unit on field
	MapFieldAirUnit = 1 << 12,		/// Air unit on field
	MapFieldBuilding = 1 << 13,		/// Building on field
	MapFieldItem = 1 << 14,			/// Item on field
	
	MapFieldRoad = 1 << 15,			/// Road (moves faster)
	MapFieldRailroad = 1 << 16,		/// Railroad (moves faster)
	MapFieldNoRail = 1 << 17,		/// Marker that there's no railroad, used for rail movemasks
	MapFieldBridge = 1 << 18,		/// Bridge or raft
	
	MapFieldGrass = 1 << 19,		/// Used for playing grass step sounds
	MapFieldMud = 1 << 20,			/// Used for playing mud step sounds
	MapFieldStoneFloor = 1 << 21,	/// Used for playing stone step sounds
	MapFieldDirt = 1 << 22,			/// Used for playing dirt step sounds
	MapFieldDesert = 1 << 23,		/// Used for identifying desert tiles for desertstalk and dehydration
	MapFieldSnow = 1 << 24,			/// Used for playing snow step sounds
	MapFieldIce = 1 << 25,			/// Used for playing ice step sounds
	
	MapFieldGravel = 1 << 26,		/// Used for playing gravel step sounds
	MapFieldStumps = 1 << 27,		/// Used for playing stumps step sounds and identifying removed forests

	MapFieldUnderground = 1 << 28,	/// The terrain is an underground one; this is used to make it always be "night" there, for graphical purposes as well as for unit sight
	MapFieldSpace = 1 << 29	//the terrain is a space one, only being passable by space units
};

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
struct CTileInfo {
public:
	CTileInfo() : BaseTerrain(0), MixTerrain(0)
	{}
	CTileInfo(unsigned char base, unsigned char mix) : BaseTerrain(base), MixTerrain(mix)
	{}

	bool operator ==(const CTileInfo &rhs) const
	{
		return BaseTerrain == rhs.BaseTerrain && MixTerrain == rhs.MixTerrain;
	}
	bool operator !=(const CTileInfo &rhs) const { return !(*this == rhs); }

public:
	unsigned char BaseTerrain; /// Basic terrain of the tile
	unsigned char MixTerrain;  /// Terrain mixed with this
};

/// Definition for a terrain type
struct SolidTerrainInfo {
	std::string TerrainName;  /// Name of the terrain
	// TODO: When drawing with the editor add some kind fo probabilities for every tile.
	//Wyrmgus start
	int DefaultTileIndex;		/// Index for the default tile for this type
	//Wyrmgus end
};

class CTile
{
public:
	CTile() : tile(0), flag(0) {}

public:
	unsigned short tile;  /// graphical pos
	//Wyrmgus start
//	unsigned short flag;  /// Flag
	unsigned long flag;  /// Flag
	//Wyrmgus end
	CTileInfo tileinfo;   /// Tile descriptions
};

/// Tileset definition
class CTileset
{
public:
	void clear();

	unsigned int getTileCount() const { return tiles.size(); }

	unsigned int getDefaultTileIndex() const;
	//Wyrmgus start
	unsigned int getDefaultWoodTileIndex() const;
	//Wyrmgus end

	bool isAWallTile(unsigned tile) const;
	bool isAWoodTile(unsigned tile) const;
	bool isARockTile(unsigned tile) const;

	//Wyrmgus start
//	unsigned getRemovedRockTile() const { return removedRockTile; }
//	unsigned getRemovedTreeTile() const { return removedTreeTile; }
	unsigned getRemovedRockTile() const;
	unsigned getRemovedTreeTile() const;
	//Wyrmgus end
	unsigned getBottomOneTreeTile() const { return botOneTreeTile; }
	unsigned getTopOneTreeTile() const { return topOneTreeTile; }
	unsigned getMidOneTreeTile() const { return midOneTreeTile; }
	
	unsigned getWallDirection(int tileIndex, bool human) const;

	unsigned getHumanWallTileIndex(int dirFlag) const;
	unsigned getOrcWallTileIndex(int dirFlag) const;
	unsigned getHumanWallTileIndex_broken(int dirFlag) const;
	unsigned getOrcWallTileIndex_broken(int dirFlag) const;
	unsigned getHumanWallTileIndex_destroyed(int dirFlag) const;
	unsigned getOrcWallTileIndex_destroyed(int dirFlag) const;

	unsigned int getSolidTerrainCount() const;

	const std::string &getTerrainName(int solidTerrainIndex) const;

	int findTileIndexByTile(unsigned int tile) const;
	unsigned int getTileNumber(int basic, bool random, bool filler) const;
	void fillSolidTiles(std::vector<unsigned int> *solidTiles) const;

	//Wyrmgus start
//	unsigned getQuadFromTile(unsigned int tile) const;
	unsigned getQuadFromTile(unsigned int tileIndex) const;
	//Wyrmgus end
	//Wyrmgus start
	int getFromMixedLookupTable(int base_terrain, int tile) const;
//	int getTileBySurrounding(unsigned short type
	int getTileBySurrounding(unsigned short type,
							 int tile_index,
	//Wyrmgus end
							 int up, int right,
							 int bottom, int left) const;
	int tileFromQuad(unsigned fixed, unsigned quad) const;
	//Wyrmgus start
//	bool isEquivalentTile(unsigned int tile1, unsigned int tile2) const;
	bool isEquivalentTile(unsigned int tile1, unsigned int tile2, int tile_index) const;
	//Wyrmgus end

	void parse(lua_State *l);
	void buildTable(lua_State *l);

private:
	unsigned int getOrAddSolidTileIndexByName(const std::string &name);
	int findTileIndex(unsigned char baseTerrain, unsigned char mixTerrain = 0) const;
	int getTileIndex(unsigned char baseTerrain, unsigned char mixTerrain, unsigned int quad) const;
	void buildWallReplacementTable();
	void parseSlots(lua_State *l, int t);
	void parseSpecial(lua_State *l);
	void parseSolid(lua_State *l);
	void parseMixed(lua_State *l);
	int findTilePath(int base, int goal, int length, std::vector<char> &marks, int *tileIndex) const;
public:
	std::string Name;           /// Nice name to display
	//Wyrmgus start
	std::string Ident;			/// Ident of the tileset
	//Wyrmgus end
	std::string ImageFile;      /// File containing image data

public:
	std::vector<CTile> tiles;

	// TODO: currently hardcoded
	std::vector<unsigned char> TileTypeTable;  /// For fast lookup of tile type
	//Wyrmgus start
	std::vector<SolidTerrainInfo> solidTerrainTypes; /// Information about solid terrains.
	int TreeUnderlayTerrain;
	int RockUnderlayTerrain;
	std::vector<unsigned> removedTreeTiles;  /// Tiles placed where trees are gone
	std::vector<unsigned> removedRockTiles;  /// Tiles placed where trees are gone
	//Wyrmgus end
private:
	//Wyrmgus start
//	std::vector<SolidTerrainInfo> solidTerrainTypes; /// Information about solid terrains.
	//Wyrmgus end
#if 1
	//Wyrmgus start
//	std::vector<int> mixedLookupTable;  /// Lookup for what part of tile used
	std::map<std::pair<int,int>, int> mixedLookupTable;  /// Lookup for what part of tile used; mapped to a pair, which has its first element as the tile type and the second element as the graphic tile
	//Wyrmgus end
	unsigned topOneTreeTile;   /// Tile for one tree top
	unsigned midOneTreeTile;   /// Tile for one tree middle
	unsigned botOneTreeTile;   /// Tile for one tree bottom
	//Wyrmgus start
//	unsigned removedTreeTile;  /// Tile placed where trees are gone
	//Wyrmgus end
	int woodTable[20];     /// Table for tree removable
	unsigned topOneRockTile;   /// Tile for one rock top
	unsigned midOneRockTile;   /// Tile for one rock middle
	unsigned botOneRockTile;   /// Tile for one rock bottom
	//Wyrmgus start
//	unsigned removedRockTile;  /// Tile placed where rocks are gone
	//Wyrmgus end
	int rockTable[20];     /// Removed rock placement table
	unsigned humanWallTable[16];  /// Human wall placement table
	unsigned orcWallTable[16];    /// Orc wall placement table
#endif
};

//Wyrmgus start
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void ParseTilesetTileFlags(lua_State *l, int *back, int *j);
//Wyrmgus start
extern std::string GetTransitionTypeNameById(const wyrmgus::tile_transition_type transition_type);
extern wyrmgus::tile_transition_type GetTransitionTypeIdByName(const std::string &transition_type);
//Wyrmgus end
