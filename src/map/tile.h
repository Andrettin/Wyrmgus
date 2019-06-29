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
/**@name tile.h - The map tile header file. */
//
//      (c) Copyright 1998-2019 by Vladi Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Rafal Bursig and Andrettin
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

#ifndef __MAP_TILE_H__
#define __MAP_TILE_H__

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CMapField tile.h
**
**  \#include "map/tile.h"
**
**  CMapFieldPlayerInfo::SeenTile
**
**    This is the tile number, that the player sitting on the computer
**    currently knows. Idea: Can be uses for illusions.
**
**  CMapFieldPlayerInfo::Visible[]
**
**    Counter how many units of the player can see this field. 0 the
**    field is not explored, 1 explored, n-1 unit see it. Currently
**    no more than 253 units can see a field.
**
**  CMapFieldPlayerInfo::VisCloak[]
**
**    Visiblity for cloaking.
**
**  CMapFieldPlayerInfo::Radar[]
**
**    Visiblity for radar.
**
**  CMapFieldPlayerInfo::RadarJammer[]
**
**    Jamming capabilities.
*/

/**
**  @class CMapField tile.h
**
**  \#include "map/tile.h"
**
**  This class contains all information about a field on map.
**  It contains its look, properties and content.
**
**  The map-field class members:
**
**  CMapField::Tile
**
**    Tile is number defining the graphic image display for the
**    map-field. 65535 different tiles are supported. A tile is
**    currently 32x32 pixels. In the future is planned to support
**    animated tiles.
**
**  CMapField::Flags
**
**    Contains special information of that tile. What units are
**    on this field, what units could be placed on this field.
**
**    This is the list of all flags currently used:
**
**    ::MapFieldVisible field is visible.
**    ::MapFieldExplored field is explored.
**    ::MapFieldLandAllowed land units are allowed.
**    ::MapFieldCoastAllowed coast units (transporter) and coast buildings (shipyard) are allowed.
**    ::MapFieldWaterAllowed water units allowed.
**    ::MapFieldNoBuilding no buildings allowed.
**    ::MapFieldUnpassable field is movement blocked.
**    ::MapFieldWall field contains wall.
**    ::MapFieldLandUnit land unit on field.
**    ::MapFieldAirUnit air unit on field.
**    ::MapFieldSeaUnit water unit on field.
**    ::MapFieldBuilding building on field.
**
**    Note: We want to add support for more unit-types like under
**      ground units.
**
**  CMapField::Cost
**
**    Unit cost to move in this tile.
**
**  CMapField::Value
**
**    Extra value for each tile. This currently only used for
**    walls, contains the remaining hit points of the wall and
**    for forest, contains the frames until they grow.
**
**  CMapField::UnitCache
**
**    Contains a vector of all units currently on this field.
**    Note: currently units are only inserted at the insert point.
**    This means units of the size of 2x2 fields are inserted at the
**    top and right most map coordinate.
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "unit/unit_cache.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFile;
class CPlayer;
class CTileset;
//Wyrmgus start
class CTerrainType;
class CGraphic;
class CTerrainFeature;
//Wyrmgus end
struct lua_State;

/*----------------------------------------------------------------------------
--  Map - field
----------------------------------------------------------------------------*/

class CMapFieldPlayerInfo
{
public:
	CMapFieldPlayerInfo()
	{
		memset(Visible, 0, sizeof(Visible));
		memset(VisCloak, 0, sizeof(VisCloak));
		memset(VisEthereal, 0, sizeof(VisEthereal));
		memset(Radar, 0, sizeof(Radar));
		memset(RadarJammer, 0, sizeof(RadarJammer));
	}

	/// Check if a field for the user is explored.
	bool IsExplored(const CPlayer &player) const;
	//Wyrmgus start
	bool IsTeamExplored(const CPlayer &player) const;
	//Wyrmgus end

	/// @note Manage CMap::Map.NoFogOfWar
	bool IsVisible(const CPlayer &player) const;
	bool IsTeamVisible(const CPlayer &player) const;
	/**
	**  Find out how a field is seen (By player, or by shared vision)
	**
	**  @param player   Player to check for.
	**  @note manage fogOfWar (using CMap::Map.NoFogOfWar)
	**
	**  @return        0 unexplored, 1 explored, 2 visible.
	*/
	unsigned char TeamVisibilityState(const CPlayer &player) const;

public:
	//Wyrmgus start
//	unsigned short SeenTile = 0;              /// last seen tile (FOW)
	const CTerrainType *SeenTerrain = nullptr;	/// last seen tile (FOW)
	const CTerrainType *SeenOverlayTerrain = nullptr;
	short SeenSolidTile = 0;
	short SeenOverlaySolidTile = 0;
	std::vector<std::pair<const CTerrainType *, short>> SeenTransitionTiles;			/// Transition tiles; the pair contains the terrain type and the tile index
	std::vector<std::pair<const CTerrainType *, short>> SeenOverlayTransitionTiles;		/// Overlay transition tiles; the pair contains the terrain type and the tile index
	//Wyrmgus end
	unsigned short Visible[PlayerMax];    /// Seen counter 0 unexplored
	unsigned char VisCloak[PlayerMax];    /// Visiblity for cloaking.
	unsigned char VisEthereal[PlayerMax];    /// Visiblity for ethereal.
	unsigned char Radar[PlayerMax];       /// Visiblity for radar.
	unsigned char RadarJammer[PlayerMax]; /// Jamming capabilities.
};

/// Describes a field of the map
class CMapField
{
public:
	CMapField();

	static constexpr unsigned char CyclesPerFrame = CYCLES_PER_SECOND / 4; //same speed as color-cycling

	/**
	**	@brief	Get the current game frame for tile animations
	**
	**	@return	The current game frame for tile animations
	*/
	static unsigned int GetGameTileAnimationFrame()
	{
		return GameCycle / CMapField::CyclesPerFrame;
	}
	
	void Save(CFile &file) const;
	void parse(lua_State *l);

	//Wyrmgus start
	void SetTerrain(const CTerrainType *terrain_type);
	void RemoveOverlayTerrain();
	void SetOverlayTerrainDestroyed(bool destroyed);
	void SetOverlayTerrainDamaged(bool damaged);
	//Wyrmgus end
	void setTileIndex(const CTileset &tileset, unsigned int tileIndex, int value);
	//Wyrmgus start
	void UpdateSeenTile();
	//Wyrmgus end

	//Wyrmgus start
	/*
	unsigned int getGraphicTile() const { return tile; }

	unsigned int getTileIndex() const { return tilesetTile; }
	*/
	//Wyrmgus end

	/// Check if a field flags.
	bool CheckMask(int mask) const;
	
	/**
	**	@brief	Get the terrain of the tile
	**
	**	@param	overlay		Whether it is the overlay terrain that should be obtained
	**
	**	@return	The terrain of the tile for the given overlay parameter
	*/
	const CTerrainType *GetTerrainType(const bool overlay) const
	{
		if (overlay) {
			return this->OverlayTerrain;
		} else {
			return this->Terrain;
		}
	}

	const CTerrainType *GetTopTerrainType(const bool seen = false, const bool ignore_destroyed = false) const;

	/// Returns true, if water on the map tile field
	bool WaterOnMap() const;

	/// Returns true, if coast on the map tile field
	bool CoastOnMap() const;

	/// Returns true, if water on the map tile field
	bool ForestOnMap() const;

	/// Returns true, if coast on the map tile field
	bool RockOnMap() const;

	bool isAWall() const;
	bool isHuman() const;

	bool IsTerrainResourceOnMap(int resource) const;
	bool IsTerrainResourceOnMap() const;
	
	bool IsSeenTileCorrect() const;
	
	int GetResource() const;

	bool IsDestroyedForestTile() const;
	
	unsigned char getCost() const { return this->cost; }
	
	uint16_t GetFlags() const { return this->Flags; }
	
	/**
	**	@brief	Get the current base frame of the tile's animation
	**
	**	@return	The current base frame of the tile's animation
	*/
	unsigned char GetAnimationBaseFrame() const
	{
		return CMapField::GetGameTileAnimationFrame() + this->RandomNumber;
	}
	
	unsigned char GetAnimationFrame() const;
	unsigned char GetOverlayAnimationFrame() const;
	
	//Wyrmgus start
//	void setGraphicTile(unsigned int tile) { this->tile = tile; }
	//Wyrmgus end
public:
	uint16_t Flags = 0;	/// field flags
	//Wyrmgus start
	const CTerrainType *Terrain = nullptr;
	const CTerrainType *OverlayTerrain = nullptr;
	const CTerrainFeature *TerrainFeature = nullptr;
	short SolidTile = 0;
	short OverlaySolidTile = 0;
	bool OverlayTerrainDestroyed = false;
	bool OverlayTerrainDamaged = false;
	std::vector<std::pair<const CTerrainType *, short>> TransitionTiles;		/// Transition tiles; the pair contains the terrain type and the tile index
	std::vector<std::pair<const CTerrainType *, short>> OverlayTransitionTiles;	/// Overlay transition tiles; the pair contains the terrain type and the tile index
	//Wyrmgus end
private:
	unsigned char cost = 0;		/// unit cost to move in this tile
public:
	// FIXME: Value should be removed, walls and regeneration can be handled differently.
	//Wyrmgus start
//	unsigned char Value = 0;	/// HP for walls / wood regeneration
	short Value = 0;	/// HP for walls / wood Regeneration
	int Landmass = 0;	/// To which "landmass" (can also be water) does this map field belong (if any); a "landmass" is a collection of adjacent land tiles, or a collection of adjacent water tiles; 0 means none has been set yet
	short Owner = -1;	/// To which player (if any) this tile belongs to
	short OwnershipBorderTile = -1;	/// The transition type of the border between this tile's owner, and other players' tiles, if applicable)
	//Wyrmgus end
	CUnitCache UnitCache;	/// A unit on the map field.

	CMapFieldPlayerInfo playerInfo;	/// stuff related to player
	
private:
	unsigned char RandomNumber = 0;	/// a random number used to e.g. give the tile a seemingly-random animation frame compared to its neighbors
};

#endif

