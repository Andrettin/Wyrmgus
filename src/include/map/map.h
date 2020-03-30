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
/**@name map.h - The map header file. */
//
//      (c) Copyright 1998-2020 by Vladi Shabanski, Lutz Sammer,
//                                 Jimmy Salmon and Andrettin
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
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CMap map.h
**
**  \#include "map/map.h"
**
**  This class contains all information about a Stratagus map.
**  A map is a rectangle of any size.
**
**  The map class members:
**
**  CMap::Fields
**
**    An array CMap::Info::Width * CMap::Info::Height of all fields
**    belonging to this map.
**
**  CMap::NoFogOfWar
**
**    Flag if true, the fog of war is disabled.
**
**  CMap::Tileset
**
**    Tileset data for the map. See ::CTileset. This contains all
**    information about the tile.
**
**  CMap::TileModelsFileName
**
**    Lua filename that loads all tilemodels
**
**  CMap::TileGraphic
**
**    Graphic for all the tiles
**
**  CMap::FogGraphic
**
**    Graphic for fog of war
**
**  CMap::Info
**
**    Descriptive information of the map. See ::CMapInfo.
*/

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>

#include "color.h"
#include "map/tile.h"
#include "time/date.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFaction;
class CGeneratedTerrain;
class CGraphic;
class CPlayer;
class CFile;
class CMapLayer;
class CMapTemplate;
class CPlane;
class CRegion;
class CSite;
class CTileset;
class CUniqueItem;
class CUnit;
class CUnitType;
class CWorld;

/*----------------------------------------------------------------------------
--  Map
----------------------------------------------------------------------------*/

static constexpr int MaxMapWidth = 512; /// max map width supported
static constexpr int MaxMapHeight = 512; /// max map height supported

//Wyrmgus start
class CTerrainFeature
{
public:
	CTerrainFeature() :
		ID(-1), TerrainType(nullptr), Plane(nullptr), World(nullptr)
	{
	}
	
	int ID;
	std::string Ident;
	std::string Name;
	CColor Color;
	CTerrainType *TerrainType;
	CPlane *Plane;
	CWorld *World;
	std::map<int, std::string> CulturalNames;							/// Names for the terrain feature for each different culture/civilization
};
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Map info structure
----------------------------------------------------------------------------*/

/**
**  Get info about a map.
*/
class CMapInfo
{
public:
	bool IsPointOnMap(const int x, const int y, const int z) const;

	bool IsPointOnMap(const Vec2i &pos, const int z) const;

	bool IsPointOnMap(const int x, const int y, const CMapLayer *map_layer) const;

	bool IsPointOnMap(const Vec2i &pos, const CMapLayer *map_layer) const;

	void Clear();

public:
	std::string Description;    /// Map description
	std::string Filename;       /// Map filename
	int MapWidth;               /// Map width
	int MapHeight;              /// Map height
	//Wyrmgus start
	std::vector<int> MapWidths;	/// Map width for each map layer
	std::vector<int> MapHeights; /// Map height for each map layer
	//Wyrmgus end
	int PlayerType[PlayerMax];  /// Same player->Type
	int PlayerSide[PlayerMax];  /// Same player->Side
	unsigned int MapUID;        /// Unique Map ID (hash)
};

/*----------------------------------------------------------------------------
--  Map itself
----------------------------------------------------------------------------*/

/// Describes the world map
class CMap
{
public:
	static CMap Map; //the current map

	CMap();
	~CMap();

	unsigned int getIndex(int x, int y, int z) const;
	unsigned int getIndex(const Vec2i &pos, int z) const;
	
	CMapField *Field(const unsigned int index, const int z) const;
	/// Get the map field at location x, y
	CMapField *Field(const int x, const int y, const int z) const;
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	pos	The coordinates of the map field
	**	@param	z	The map layer of the map field
	**
	**	@return	The map field
	*/
	CMapField *Field(const Vec2i &pos, const int z) const
	{
		return this->Field(pos.x, pos.y, z);
	}

	/// Allocate and initialize map table.
	void Create();
	/// Build tables for map
	void Init();
	/// Clean the map
	void Clean();
	/// Cleanup memory for fog of war tables
	void CleanFogOfWar();
	/// Clear the map layers
	void ClearMapLayers();
	
	//Wyrmgus start
	void SetTileTerrain(const Vec2i &pos, CTerrainType *terrain, int z);
	void RemoveTileOverlayTerrain(const Vec2i &pos, int z);
	void SetOverlayTerrainDestroyed(const Vec2i &pos, bool destroyed, int z);
	void SetOverlayTerrainDamaged(const Vec2i &pos, bool damaged, int z);
	void CalculateTileTransitions(const Vec2i &pos, bool overlay, int z);
	void CalculateTileLandmass(const Vec2i &pos, int z);
	void CalculateTileTerrainFeature(const Vec2i &pos, int z);
	void CalculateTileOwnership(const Vec2i &pos, int z);
	void CalculateTileOwnershipTransition(const Vec2i &pos, int z);
	void AdjustMap();
	void AdjustTileMapIrregularities(bool overlay, const Vec2i &min_pos, const Vec2i &max_pos, int z);
	void AdjustTileMapTransitions(const Vec2i &min_pos, const Vec2i &max_pos, int z);
	void GenerateTerrain(const CGeneratedTerrain *generated_terrain, const Vec2i &min_pos, const Vec2i &max_pos, const bool preserve_coastline, const int z);
	void GenerateNeutralUnits(CUnitType *unit_type, int quantity, const Vec2i &min_pos, const Vec2i &max_pos, bool grouped, int z);
	//Wyrmgus end

	void ClearOverlayTile(const Vec2i &pos, int z);

	/// convert map pixelpos coordinates into tilepos
	Vec2i MapPixelPosToTilePos(const PixelPos &mapPos, const int map_layer) const;
	/// convert tilepos coordinates into map pixel pos (take the top left of the tile)
	PixelPos TilePosToMapPixelPos_TopLeft(const Vec2i &tilePos, const CMapLayer *map_layer) const;
	/// convert tilepos coordinates into map pixel pos (take the center of the tile)
	PixelPos TilePosToMapPixelPos_Center(const Vec2i &tilePos, const CMapLayer *map_layer) const;
	
	//Wyrmgus start
	CTerrainType *GetTileTerrain(const Vec2i &pos, const bool overlay, const int z) const;
	CTerrainType *GetTileTopTerrain(const Vec2i &pos, const bool seen, const int z, const bool ignore_destroyed = false) const;
	int GetTileLandmass(const Vec2i &pos, int z) const;
	Vec2i GenerateUnitLocation(const CUnitType *unit_type, const CFaction *faction, const Vec2i &min_pos, const Vec2i &max_pos, const int z) const;
	//Wyrmgus end

	/// Mark a tile as seen by the player.
	void MarkSeenTile(CMapField &mf, int z);

	/// Regenerate the forest.
	void RegenerateForest();
	/// Reveal the complete map, make everything known.
	//Wyrmgus start
//	void Reveal();
	void Reveal(bool only_person_players = false);
	//Wyrmgus end
	/// Save the map.
	void Save(CFile &file) const;

	//
	// Wall
	//
	/// Wall is hit.
	//Wyrmgus start
	void HitWall(const Vec2i &pos, unsigned damage, int z);

	/// Returns true if there is a wall on the map tile field
	bool WallOnMap(const Vec2i &pos, int z) const;
	
	//Wyrmgus start
	bool CurrentTerrainCanBeAt(const Vec2i &pos, bool overlay, int z);
	bool TileBordersOnlySameTerrain(const Vec2i &pos, const CTerrainType *new_terrain, const int z);
	bool TileBordersFlag(const Vec2i &pos, int z, int flag, bool reverse = false); // reverse means that it returns true if the tile borders one tile without the flag
	bool TileBordersBuilding(const Vec2i &pos, int z);
	bool TileBordersPathway(const Vec2i &pos, int z, bool only_railroad);
	bool TileBordersUnit(const Vec2i &pos, int z);
	bool TileBordersTerrainIncompatibleWithTerrain(const Vec2i &pos, const CTerrainType *terrain_type, const int z);
	bool TileHasInnerBorderTerrainsIncompatibleWithOverlayTerrain(const Vec2i &pos, const CTerrainType *overlay_terrain, const int z);
	bool TileHasUnitsIncompatibleWithTerrain(const Vec2i &pos, const CTerrainType *terrain, const int z);
	bool IsPointInASubtemplateArea(const Vec2i &pos, const int z, const CMapTemplate *subtemplate = nullptr) const;
	Vec2i GetSubtemplatePos(const CMapTemplate *subtemplate) const;
	Vec2i GetSubtemplateEndPos(const CMapTemplate *subtemplate) const;
	CMapLayer *GetSubtemplateMapLayer(const CMapTemplate *subtemplate) const;
	std::vector<CUnit *> GetMapTemplateLayerConnectors(const CMapTemplate *map_template) const;
	bool IsPointAdjacentToNonSubtemplateArea(const Vec2i &pos, const int z) const;
	bool IsLayerUnderground(int z) const;
	
	void SetCurrentPlane(CPlane *plane);
	void SetCurrentWorld(CWorld *world);
	void SetCurrentSurfaceLayer(int surface_layer);
	CPlane *GetCurrentPlane() const;
	CWorld *GetCurrentWorld() const;
	int GetCurrentSurfaceLayer() const;
	PixelSize GetCurrentPixelTileSize() const;
	PixelSize GetMapLayerPixelTileSize(int map_layer) const;
	//Wyrmgus end

	//UnitCache
	#ifdef __MORPHOS__
	#undef Insert
	#undef Remove
	#endif
	/// Insert new unit into cache
	void Insert(CUnit &unit);

	/// Remove unit from cache
	void Remove(CUnit &unit);

	//Wyrmgus start
//	void Clamp(Vec2i &pos) const;
	void Clamp(Vec2i &pos, int z) const;
	//Wyrmgus end

	//Warning: we expect typical usage as xmin = x - range
	void FixSelectionArea(Vec2i &minpos, Vec2i &maxpos, int z)
	{
		minpos.x = std::max<short>(0, minpos.x);
		minpos.y = std::max<short>(0, minpos.y);

		//Wyrmgus start
//		maxpos.x = std::min<short>(maxpos.x, Info.MapWidth - 1);
//		maxpos.y = std::min<short>(maxpos.y, Info.MapHeight - 1);
		maxpos.x = std::min<short>(maxpos.x, Info.MapWidths[z] - 1);
		maxpos.y = std::min<short>(maxpos.y, Info.MapHeights[z] - 1);
		//Wyrmgus end
	}

private:
	/// Build tables for fog of war
	void InitFogOfWar(PixelSize pixel_tile_size);

	//Wyrmgus start
	/*
	/// Correct the surrounding seen wood fields
	void FixNeighbors(unsigned short type, int seen, const Vec2i &pos);
	/// Correct the seen wood field, depending on the surrounding
	void FixTile(unsigned short type, int seen, const Vec2i &pos);
	*/
	//Wyrmgus end

public:
	bool NoFogOfWar;           /// fog of war disabled

	CTileset *Tileset;          /// tileset data
	std::string TileModelsFileName; /// lua filename that loads all tilemodels
	CGraphic *TileGraphic;     /// graphic for all the tiles
	static std::map<PixelSize, CGraphic *> FogGraphics;      /// graphics for fog of war, mapped to their respective pixel sizes
	//Wyrmgus start
	CTerrainType *BorderTerrain;      	/// terrain type for borders
	int Landmasses;						/// how many landmasses are there
	std::vector<std::vector<int>> BorderLandmasses;	/// "landmasses" which border the one to which each vector belongs
	std::vector<CUnit *> SiteUnits;	/// the town hall / settlement site units
	std::vector<CMapLayer *> MapLayers;				/// the map layers composing the map
	//Wyrmgus end

	CMapInfo Info;             /// descriptive information
};


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//Wyrmgus start
extern std::vector<CTerrainFeature *> TerrainFeatures;
extern std::map<std::string, CTerrainFeature *> TerrainFeatureIdentToPointer;
extern std::map<std::tuple<int, int, int>, int> TerrainFeatureColorToIndex;
//Wyrmgus end

extern char CurrentMapPath[1024]; /// Path to the current map

/// Contrast of fog of war
extern int FogOfWarOpacity;
/// fog of war color
extern CColor FogOfWarColor;
/// Forest regeneration
extern int ForestRegeneration;
/// Flag must reveal the map
extern int FlagRevealMap;
/// Flag must reveal map when in replay
extern int ReplayRevealMap;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//Wyrmgus start
extern CTerrainFeature *GetTerrainFeature(const std::string &terrain_feature_ident);
//Wyrmgus end

#define MARKER_ON_INDEX
//
// in map_fog.c
//
/// Function to (un)mark the vision table.
#ifndef MARKER_ON_INDEX
typedef void MapMarkerFunc(const CPlayer &player, const Vec2i &pos, int z);
#else
typedef void MapMarkerFunc(const CPlayer &player, const unsigned int index, int z);
#endif

/// Filter map flags through fog
extern int MapFogFilterFlags(CPlayer &player, const Vec2i &pos, int mask, int z);
extern int MapFogFilterFlags(CPlayer &player, const unsigned int index, int mask, int z);
/// Mark a tile for normal sight
extern MapMarkerFunc MapMarkTileSight;
/// Unmark a tile for normal sight
extern MapMarkerFunc MapUnmarkTileSight;
/// Mark a tile for cloak detection
extern MapMarkerFunc MapMarkTileDetectCloak;
/// Unmark a tile for cloak detection
extern MapMarkerFunc MapUnmarkTileDetectCloak;
//Wyrmgus start
/// Mark a tile for ethereal detection
extern MapMarkerFunc MapMarkTileDetectEthereal;
/// Unmark a tile for ethereal detection
extern MapMarkerFunc MapUnmarkTileDetectEthereal;
//Wyrmgus end

/// Mark sight changes
extern void MapSight(const CPlayer &player, const Vec2i &pos, int w,
					 int h, int range, MapMarkerFunc *marker, int z);
/// Update fog of war
extern void UpdateFogOfWarChange();

//
// in map_radar.c
//

/// Mark a tile as radar visible, or incrase radar vision
extern MapMarkerFunc MapMarkTileRadar;

/// Unmark a tile as radar visible, decrease is visible by other radar
extern MapMarkerFunc MapUnmarkTileRadar;

/// Mark a tile as radar jammed, or incrase radar jamming'ness
extern MapMarkerFunc MapMarkTileRadarJammer;

/// Unmark a tile as jammed, decrease is jamming'ness
extern MapMarkerFunc MapUnmarkTileRadarJammer;

//Wyrmgus start
/// Mark a tile for ownership
extern MapMarkerFunc MapMarkTileOwnership;

/// Unmark a tile for ownership
extern MapMarkerFunc MapUnmarkTileOwnership;
//Wyrmgus end

//
// in map_wall.c
//
//Wyrmgus start
/*
/// Correct the seen wall field, depending on the surrounding
extern void MapFixSeenWallTile(const Vec2i &pos);
/// Correct the surrounding seen wall fields
extern void MapFixSeenWallNeighbors(const Vec2i &pos);
/// Correct the real wall field, depending on the surrounding
extern void MapFixWallTile(const Vec2i &pos);
*/
//Wyrmgus end

//
// in script_map.cpp
//
/// Set a tile
extern void SetTile(unsigned int tile, const Vec2i &pos, int value = 0, int z = 0);
inline void SetTile(unsigned int tile, int x, int y, int value = 0, int z = 0)
{
	const Vec2i pos(x, y);
	SetTile(tile, pos, value, z);
}

//Wyrmgus start
extern void SetTileTerrain(const std::string &terrain_ident, const Vec2i &pos, int value = 0, int z = 0);
inline void SetTileTerrain(const std::string &terrain_ident, int x, int y, int value = 0, int z = 0)
{
	const Vec2i pos(x, y);
	SetTileTerrain(terrain_ident, pos, value, z);
}
extern void ApplyMapTemplate(const std::string &map_template_ident, int start_x = 0, int start_y = 0, int map_start_x = 0, int map_start_y = 0, int z = 0);
extern void ApplyCampaignMap(const std::string &campaign_ident);
//Wyrmgus end

/// register ccl features
extern void MapCclRegister();

//
// mixed sources
//
/// Save a stratagus map (smp format)
//Wyrmgus start
//extern int SaveStratagusMap(const std::string &filename, CMap &map, int writeTerrain);
extern int SaveStratagusMap(const std::string &filename, CMap &map, int writeTerrain, bool is_mod = false);
//Wyrmgus end


/// Load map presentation
extern void LoadStratagusMapInfo(const std::string &mapname);

/// Returns true, if the unit-type(mask can enter field with bounds check
extern bool CheckedCanMoveToMask(const Vec2i &pos, int mask, int z);
/// Returns true, if the unit-type can enter the field
extern bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos, int z);
/// Returns true, if the unit can enter the field
extern bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos, int z);

/// Preprocess map, for internal use.
extern void PreprocessMap();

//Wyrmgus start
extern int GetMapLayer(const std::string &plane_ident = "", const std::string &world_ident = "", const int surface_layer = 0);
extern int GetSubtemplateStartX(const std::string &subtemplate_ident);
extern int GetSubtemplateStartY(const std::string &subtemplate_ident);
extern void ChangeToPreviousMapLayer();
extern void ChangeCurrentMapLayer(const int z);
extern void SetTimeOfDay(const std::string &time_of_day_ident, int z = 0);
extern void SetTimeOfDaySchedule(const std::string &time_of_day_schedule_ident, int z = 0);
extern void SetSeason(const std::string &season_ident, int z = 0);
extern void SetSeasonSchedule(const std::string &season_schedule_ident, int z = 0);
//Wyrmgus end

// in unit.c

/// Mark on vision table the Sight of the unit.
void MapMarkUnitSight(CUnit &unit);
/// Unmark on vision table the Sight of the unit.
void MapUnmarkUnitSight(CUnit &unit);

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

/// Can a unit with 'mask' enter the field
inline bool CanMoveToMask(const Vec2i &pos, int mask, int z)
{
	return !CMap::Map.Field(pos, z)->CheckMask(mask);
}

/// Handle Marking and Unmarking of radar vision
inline void MapMarkRadar(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
{
	MapSight(player, pos, w, h, range, MapMarkTileRadar, z);
}
inline void MapUnmarkRadar(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
{
	MapSight(player, pos, w, h, range, MapUnmarkTileRadar, z);
}
/// Handle Marking and Unmarking of radar vision
inline void MapMarkRadarJammer(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
{
	MapSight(player, pos, w, h, range, MapMarkTileRadarJammer, z);
}
inline void MapUnmarkRadarJammer(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
{
	MapSight(player, pos, w, h, range, MapUnmarkTileRadarJammer, z);
}

//Wyrmgus start
inline void MapMarkOwnership(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
{
	MapSight(player, pos, w, h, range, MapMarkTileOwnership, z);
}
inline void MapUnmarkOwnership(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
{
	MapSight(player, pos, w, h, range, MapUnmarkTileOwnership, z);
}
//Wyrmgus end
