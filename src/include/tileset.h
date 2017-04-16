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
//      (c) Copyright 1998-2005 by Lutz Sammer and Jimmy Salmon
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

#ifndef TILESET_H
#define TILESET_H

//@{

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#include "vec2i.h"
#include <vector>
//Wyrmgus start
#include <map>
#include <tuple>

#include "color.h"
//Wyrmgus end

struct lua_State;
//Wyrmgus start
class CGraphic;
class CUnitType;
//Wyrmgus end

//Wyrmgus start
/*
// Not used until now:
#define MapFieldSpeedMask 0x0007  /// Move faster on this tile

#define MapFieldHuman 0x0008  /// Human is owner of the field (walls)

#define MapFieldLandAllowed  0x0010  /// Land units allowed
#define MapFieldCoastAllowed 0x0020  /// Coast (transporter) units allowed
#define MapFieldWaterAllowed 0x0040  /// Water units allowed
#define MapFieldNoBuilding   0x0080  /// No buildings allowed

#define MapFieldUnpassable 0x0100  /// Field is movement blocked
#define MapFieldWall       0x0200  /// Field contains wall
#define MapFieldRocks      0x0400  /// Field contains rocks
#define MapFieldForest     0x0800  /// Field contains forest

#define MapFieldLandUnit 0x1000  /// Land unit on field
#define MapFieldAirUnit  0x2000  /// Air unit on field
#define MapFieldSeaUnit  0x4000  /// Water unit on field
#define MapFieldBuilding 0x8000  /// Building on field
*/
#define MapFieldItem 0x00000004		/// Item on field

#define MapFieldSpeedMask 0x00000007  /// Move faster on this tile

#define MapFieldHuman 0x00000008  /// Human is owner of the field (walls)

#define MapFieldLandAllowed  0x00000010  /// Land units allowed
#define MapFieldCoastAllowed 0x00000020  /// Coast (transporter) units allowed
#define MapFieldWaterAllowed 0x00000040  /// Water units allowed
#define MapFieldNoBuilding   0x00000080  /// No buildings allowed

#define MapFieldUnpassable 0x00000100  /// Field is movement blocked
#define MapFieldWall       0x00000200  /// Field contains wall
#define MapFieldRocks      0x00000400  /// Field contains rocks
#define MapFieldForest     0x00000800  /// Field contains forest

#define MapFieldLandUnit 0x00001000  /// Land unit on field
#define MapFieldAirUnit  0x00002000  /// Air unit on field
#define MapFieldSeaUnit  0x00004000  /// Water unit on field
#define MapFieldBuilding 0x00008000  /// Building on field

#define MapFieldAirUnpassable 0x00010000	/// Field is movement blocked
#define MapFieldGrass 0x00020000			/// Used for playing grass step sounds
#define MapFieldMud 0x00040000				/// Used for playing mud step sounds
#define MapFieldStoneFloor 0x00080000		/// Used for playing stone step sounds

#define MapFieldDirt 0x00100000				/// Used for playing dirt step sounds
#define MapFieldGravel 0x00200000			/// Used for playing gravel step sounds
#define MapFieldStumps 0x00400000			/// Used for playing stumps step sounds
#define MapFieldBridge 0x00800000			/// Bridge or raft

#define MapFieldRoad 0x01000000				/// Road (moves faster)
#define MapFieldRailroad 0x02000000			/// Railroad (moves faster, even faster than with the road)
#define MapFieldNoRail 0x04000000			/// Marker that there's no railroad, used for rail movemasks
//Wyrmgus end

/**
**  These are used for lookup tiles types
**  mainly used for the FOW implementation of the seen woods/rocks
**
**  @todo I think this can be removed, we can use the flags?
**  I'm not sure, if we have seen and real time to considere.
*/
enum TileType {
	TileTypeUnknown,    /// Unknown tile type
	TileTypeWood,       /// Any wood tile
	TileTypeRock,       /// Any rock tile
	TileTypeCoast,      /// Any coast tile
	TileTypeHumanWall,  /// Any human wall tile
	TileTypeOrcWall,    /// Any orc wall tile
	TileTypeWater       /// Any water tile
};

//Wyrmgus start
enum TransitionTypes {
	NorthTransitionType,
	SouthTransitionType,
	WestTransitionType,
	EastTransitionType,
	NorthwestOuterTransitionType,
	NortheastOuterTransitionType,
	SouthwestOuterTransitionType,
	SoutheastOuterTransitionType,
	NorthwestInnerTransitionType,
	NortheastInnerTransitionType,
	SouthwestInnerTransitionType,
	SoutheastInnerTransitionType,
	NorthwestSoutheastInnerTransitionType,
	NortheastSouthwestInnerTransitionType,
	
	//single tile transition types (enabled for terrain types with the AllowSingle field)
	SingleTransitionType,
	NorthSingleTransitionType,
	SouthSingleTransitionType,
	WestSingleTransitionType,
	EastSingleTransitionType,
	NorthSouthTransitionType,
	WestEastTransitionType,
	NorthwestNortheastSouthwestSoutheastInnerTransitionType,
	NorthwestNortheastInnerTransitionType,
	SouthwestSoutheastInnerTransitionType,
	NorthwestSouthwestInnerTransitionType,
	NortheastSoutheastInnerTransitionType,
	NorthSouthwestInnerSoutheastInnerTransitionType,
	NorthSouthwestInnerTransitionType,
	NorthSoutheastInnerTransitionType,
	SouthNorthwestInnerNortheastInnerTransitionType,
	SouthNorthwestInnerTransitionType,
	SouthNortheastInnerTransitionType,
	WestNortheastInnerSoutheastInnerTransitionType,
	WestNortheastInnerTransitionType,
	WestSoutheastInnerTransitionType,
	EastNorthwestInnerSouthwestInnerTransitionType,
	EastNorthwestInnerTransitionType,
	EastSouthwestInnerTransitionType,
	NorthwestOuterSoutheastInnerTransitionType,
	NortheastOuterSouthwestInnerTransitionType,
	SouthwestOuterNortheastInnerTransitionType,
	SoutheastOuterNorthwestInnerTransitionType,
	
	MaxTransitionTypes
};

class CTerrainType
{
public:
	CTerrainType() :
		ID(-1), Flags(0), SolidAnimationFrames(0),
		Overlay(false), Buildable(false), AllowSingle(false),
		UnitType(NULL), Graphics(NULL)
	{
		Color.R = 0;
		Color.G = 0;
		Color.B = 0;
		Color.A = 0;
	}

	std::string Name;
	std::string Ident;
	std::string Character;
	CColor Color;
	int ID;
	int SolidAnimationFrames;
	unsigned int Flags;
	bool Overlay;												/// Whether this terrain type belongs to the overlay layer
	bool Buildable;
	bool AllowSingle;											/// Whether this terrain type has transitions for single tiles
	CUnitType *UnitType;
	CGraphic *Graphics;
	std::vector<CTerrainType *> BaseTerrains;					/// Possible base terrains for this terrain type (if is an overlay terrain)
	std::vector<CTerrainType *> BorderTerrains;					/// Terrain types which this one can border
	std::vector<CTerrainType *> InnerBorderTerrains;			/// Terrain types which this one can border, and which "enter" this tile type in transitions
	std::vector<CTerrainType *> OuterBorderTerrains;			/// Terrain types which this one can border, and which are "entered" by this tile type in transitions
	std::vector<int> SolidTiles;
	std::vector<int> DamagedTiles;
	std::vector<int> DestroyedTiles;
	std::map<std::tuple<int, int>, std::vector<int>> TransitionTiles;	/// Transition graphics, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)
	std::map<std::tuple<int, int>, std::vector<int>> AdjacentTransitionTiles;	/// Transition graphics for the tiles adjacent to this terrain type, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)
};
//Wyrmgus end

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
	bool isARaceWallTile(unsigned tile, bool human) const;
	bool isAWoodTile(unsigned tile) const;
	bool isARockTile(unsigned tile) const;

	const PixelSize &getPixelTileSize() const { return pixelTileSize; }

	//Wyrmgus start
//	unsigned getRemovedRockTile() const { return removedRockTile; }
//	unsigned getRemovedTreeTile() const { return removedTreeTile; }
	unsigned getRemovedRockTile() const { return removedRockTiles.size() > 0 ? removedRockTiles[SyncRand(removedRockTiles.size())] : -1; }
	unsigned getRemovedTreeTile() const { return removedTreeTiles.size() > 0 ? removedTreeTiles[SyncRand(removedTreeTiles.size())] : -1; }
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
	PixelSize pixelTileSize;    /// Size of a tile in pixel
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
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CTerrainType *>  TerrainTypes;
extern std::map<std::string, int> TerrainTypeStringToIndex;
extern std::map<std::string, int> TerrainTypeCharacterToIndex;
extern std::map<std::tuple<int, int, int>, int> TerrainTypeColorToIndex;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void ParseTilesetTileFlags(lua_State *l, int *back, int *j);
//Wyrmgus start
extern std::string GetTransitionTypeNameById(int transition_type);
extern int GetTransitionTypeIdByName(std::string transition_type);
extern CTerrainType *GetTerrainType(std::string terrain_ident);
extern void LoadTerrainTypes();
//Wyrmgus end

//@}

#endif // !TILESET_H
