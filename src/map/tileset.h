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
/**@name tileset.h - The tileset header file. */
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

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#include "vec2i.h"
//Wyrmgus start
#include "video/color.h"
//Wyrmgus end

//Wyrmgus start
#include <map>
#include <tuple>
//Wyrmgus end
#include <vector>

struct lua_State;
//Wyrmgus start
class CGraphic;
class CPlayerColorGraphic;
class CUnitType;
//Wyrmgus end

enum MapFieldFlag : unsigned long {
	MapFieldLandAllowed = 1 << 0,	/// Land units allowed
	MapFieldCoastAllowed = 1 << 1,	/// Coast (e.g. transporter) units allowed
	MapFieldWaterAllowed = 1 << 2,	/// Water units allowed
	MapFieldNoBuilding = 1 << 3,	/// No buildings allowed
	
	MapFieldUnpassable = 1 << 4,	/// Field is movement blocked
	MapFieldAirUnpassable = 1 << 5,	/// Field is movement blocked for air units and missiles
	MapFieldWall = 1 << 6,			/// Field contains wall
	
	MapFieldLandUnit = 1 << 7,		/// Land unit on field
	MapFieldSeaUnit = 1 << 8,		/// Water unit on field
	MapFieldAirUnit = 1 << 9,		/// Air unit on field
	MapFieldBuilding = 1 << 10,		/// Building on field
	MapFieldItem = 1 << 11,			/// Item on field
	
	MapFieldRoad = 1 << 12,			/// Road (moves faster)
	MapFieldRailroad = 1 << 13,		/// Railroad (moves faster)
	MapFieldNoRail = 1 << 14,		/// Marker that there's no railroad, used for rail movemasks
	MapFieldBridge = 1 << 15,		/// Bridge or raft
	
	MapFieldMud = 1 << 16,			/// Used for identifying mud tiles for swampstalk
	MapFieldDesert = 1 << 17		/// Used for identifying desert tiles for desertstalk and dehydration
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
	NorthwestNortheastSouthwestInnerTransitionType,
	NorthwestNortheastSoutheastInnerTransitionType,
	NorthwestSouthwestSoutheastInnerTransitionType,
	NortheastSouthwestSoutheastInnerTransitionType,
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
//Wyrmgus end

/// Single tile definition
struct CTileInfo {
public:
	CTileInfo()
	{}
	
	CTileInfo(unsigned char base, unsigned char mix) : BaseTerrain(base), MixTerrain(mix)
	{}

	bool operator ==(const CTileInfo &rhs) const
	{
		return BaseTerrain == rhs.BaseTerrain && MixTerrain == rhs.MixTerrain;
	}
	bool operator !=(const CTileInfo &rhs) const { return !(*this == rhs); }

public:
	unsigned char BaseTerrain = 0;	/// Basic terrain of the tile
	unsigned char MixTerrain = 0;	/// Terrain mixed with this
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
	unsigned short tile = 0;	/// graphical pos
	//Wyrmgus start
//	unsigned short flag = 0;	/// Flag
	unsigned long flag = 0;		/// Flag
	//Wyrmgus end
	CTileInfo tileinfo;			/// Tile descriptions
};

/// Tileset definition
class CTileset
{
public:
	void clear();

	unsigned int getTileCount() const { return tiles.size(); }

	unsigned int getDefaultTileIndex() const;

	const PixelSize &getPixelTileSize() const { return pixelTileSize; }

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
	int tileFromQuad(unsigned fixed, unsigned quad) const;

	void parse(lua_State *l);
	void buildTable(lua_State *l);

private:
	unsigned int getOrAddSolidTileIndexByName(const std::string &name);
	int findTileIndex(unsigned char baseTerrain, unsigned char mixTerrain = 0) const;
	int getTileIndex(unsigned char baseTerrain, unsigned char mixTerrain, unsigned int quad) const;
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

	//Wyrmgus start
	std::vector<SolidTerrainInfo> solidTerrainTypes; /// Information about solid terrains.
	//Wyrmgus end
private:
	PixelSize pixelTileSize;    /// Size of a tile in pixel
	//Wyrmgus start
//	std::vector<SolidTerrainInfo> solidTerrainTypes; /// Information about solid terrains.
	//Wyrmgus end
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void ParseTilesetTileFlags(lua_State *l, int *back, int *j);
//Wyrmgus start
extern std::string GetTransitionTypeNameById(int transition_type);
extern int GetTransitionTypeIdByName(const std::string &transition_type);
//Wyrmgus end

#endif
