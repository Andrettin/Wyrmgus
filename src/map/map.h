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
//      (c) Copyright 1998-2021 by Vladi Shabanski, Lutz Sammer,
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

#include "color.h"
#include "time/date.h"
#include "util/point_container.h"
#include "vec2i.h"

class CGraphic;
class CPlayer;
class CFile;
class CMapLayer;
class CTileset;
class CUnit;
class CUnitCache;

namespace wyrmgus {
	class faction;
	class generated_terrain;
	class map_template;
	class plane;
	class site;
	class terrain_type;
	class tile;
	class unit_type;
	class world;
}

/*----------------------------------------------------------------------------
--  Map
----------------------------------------------------------------------------*/

constexpr int MaxMapWidth = 512; /// max map width supported
constexpr int MaxMapHeight = 512; /// max map height supported

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
--  the Map itself
----------------------------------------------------------------------------*/

/// Describes the world map
class CMap
{
public:
	static CMap *get()
	{
		return &CMap::Map;
	}

	static CMap Map; //the current map

	CMap();
	~CMap();

	unsigned int getIndex(int x, int y, int z) const;
	unsigned int getIndex(const Vec2i &pos, int z) const;
	
	wyrmgus::tile *Field(const unsigned int index, const int z) const;
	/// Get the map field at location x, y
	wyrmgus::tile *Field(const int x, const int y, const int z) const;
	
	/**
	**	@brief	Get the map field at a given location
	**
	**	@param	pos	The coordinates of the map field
	**	@param	z	The map layer of the map field
	**
	**	@return	The map field
	*/
	wyrmgus::tile *Field(const Vec2i &pos, const int z) const
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

	QRect get_rect(const int z) const
	{
		return QRect(QPoint (0, 0), QPoint(this->Info.MapWidths[z] - 1, this->Info.MapHeights[z] - 1));
	}
	
	//Wyrmgus start
	void SetTileTerrain(const Vec2i &pos, const wyrmgus::terrain_type *terrain, int z);
	void RemoveTileOverlayTerrain(const Vec2i &pos, int z);
	void SetOverlayTerrainDestroyed(const Vec2i &pos, bool destroyed, int z);
	void SetOverlayTerrainDamaged(const Vec2i &pos, bool damaged, int z);
	void calculate_tile_solid_tile(const QPoint &pos, const bool overlay, const int z);
	void CalculateTileTransitions(const Vec2i &pos, bool overlay, int z);
	void CalculateTileLandmass(const Vec2i &pos, int z);
	void CalculateTileOwnershipTransition(const Vec2i &pos, int z);
	void AdjustMap();
	void AdjustTileMapIrregularities(const bool overlay, const Vec2i &min_pos, const Vec2i &max_pos, const int z);
	void AdjustTileMapTransitions(const Vec2i &min_pos, const Vec2i &max_pos, int z);
	void adjust_territory_irregularities(const QPoint &min_pos, const QPoint &max_pos, const int z);
	void GenerateTerrain(const std::unique_ptr<wyrmgus::generated_terrain> &generated_terrain, const Vec2i &min_pos, const Vec2i &max_pos, const bool preserve_coastline, const int z);
	bool CanTileBePartOfMissingTerrainGeneration(const wyrmgus::tile *tile, const wyrmgus::terrain_type *terrain_type, const wyrmgus::terrain_type *overlay_terrain_type) const;
	void generate_missing_terrain(const QRect &rect, const int z);
	void expand_terrain_features_to_same_terrain(const int z);
	void generate_settlement_territories(const int z);
	wyrmgus::point_set expand_settlement_territories(std::vector<QPoint> &&seeds, const int z, const int block_flags = 0, const int same_flags = 0);
	void process_settlement_territory_tiles(const int z);
	void calculate_settlement_resource_units();
	void generate_neutral_units(const wyrmgus::unit_type *unit_type, const int quantity, const QPoint &min_pos, const QPoint &max_pos, const bool grouped, const int z);
	//Wyrmgus end

	void ClearOverlayTile(const Vec2i &pos, int z);

	/// convert map pixelpos coordinates into tilepos
	Vec2i map_pixel_pos_to_tile_pos(const PixelPos &mapPos) const;
	Vec2i scaled_map_pixel_pos_to_tile_pos(const PixelPos &mapPos) const;
	/// convert tilepos coordinates into map pixel pos (take the top left of the tile)
	PixelPos tile_pos_to_map_pixel_pos_top_left(const Vec2i &tilePos) const;
	PixelPos tile_pos_to_scaled_map_pixel_pos_top_left(const Vec2i &tilePos) const;
	/// convert tilepos coordinates into map pixel pos (take the center of the tile)
	PixelPos tile_pos_to_map_pixel_pos_center(const Vec2i &tilePos) const;
	PixelPos tile_pos_to_scaled_map_pixel_pos_center(const Vec2i &tilePos) const;
	
	//Wyrmgus start
	const wyrmgus::terrain_type *GetTileTerrain(const Vec2i &pos, const bool overlay, const int z) const;
	const wyrmgus::terrain_type *GetTileTopTerrain(const Vec2i &pos, const bool seen, const int z, const bool ignore_destroyed = false) const;
	int GetTileLandmass(const Vec2i &pos, int z) const;
	//Wyrmgus end

	const CUnitCache &get_tile_unit_cache(const QPoint &pos, int z);

	//Wyrmgus start
	QPoint generate_unit_location(const wyrmgus::unit_type *unit_type, const wyrmgus::faction *faction, const QPoint &min_pos, const QPoint &max_pos, const int z) const;
	//Wyrmgus end

	/// Mark a tile as seen by the player.
	void MarkSeenTile(wyrmgus::tile &mf);

	void handle_destroyed_overlay_terrain();

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
	bool TileBordersTerrain(const Vec2i &pos, const wyrmgus::terrain_type *terrain_type, const int z) const;
	bool TileBordersOnlySameTerrain(const Vec2i &pos, const wyrmgus::terrain_type *new_terrain, const int z) const;
	bool TileBordersFlag(const Vec2i &pos, int z, int flag, bool reverse = false) const; // reverse means that it returns true if the tile borders one tile without the flag
	bool tile_borders_other_terrain_feature(const QPoint &pos, const int z) const;
	bool tile_borders_same_settlement_territory(const QPoint &pos, const int z, const bool diagonal_allowed) const;
	bool tile_borders_other_settlement_territory(const QPoint &pos, const int z) const;
	bool tile_borders_other_player_territory(const QPoint &pos, const int z, const int range = 0) const;
	bool TileBordersBuilding(const Vec2i &pos, int z);
	bool tile_borders_pathway(const QPoint &pos, const int z, const bool only_railroad);
	bool TileBordersUnit(const Vec2i &pos, int z);
	bool TileBordersTerrainIncompatibleWithTerrain(const Vec2i &pos, const wyrmgus::terrain_type *terrain_type, const int z) const;
	bool TileBordersTerrainIncompatibleWithTerrainPair(const Vec2i &pos, const wyrmgus::terrain_type *terrain_type, const wyrmgus::terrain_type *overlay_terrain_type, const int z) const;
	bool TileHasUnitsIncompatibleWithTerrain(const Vec2i &pos, const wyrmgus::terrain_type *terrain, const int z);
	bool is_point_in_a_subtemplate_area(const QPoint &pos, const int z) const;
	bool is_point_in_subtemplate_area(const QPoint &pos, const int z, const wyrmgus::map_template *subtemplate) const;
	bool is_subtemplate_on_map(const wyrmgus::map_template *subtemplate) const;
	const QRect &get_subtemplate_rect(const wyrmgus::map_template *subtemplate) const;
	QPoint get_subtemplate_pos(const wyrmgus::map_template *subtemplate) const;
	QPoint get_subtemplate_center_pos(const wyrmgus::map_template *subtemplate) const;
	QPoint get_subtemplate_end_pos(const wyrmgus::map_template *subtemplate) const;
	CMapLayer *get_subtemplate_map_layer(const wyrmgus::map_template *subtemplate) const;
	std::vector<CUnit *> get_map_template_layer_connectors(const wyrmgus::map_template *map_template) const;
	bool is_point_adjacent_to_non_subtemplate_area(const Vec2i &pos, const int z) const;
	bool is_rect_in_settlement(const QRect &rect, const int z, const wyrmgus::site *settlement);
	
	void SetCurrentPlane(wyrmgus::plane *plane);
	void SetCurrentWorld(wyrmgus::world *world);
	const wyrmgus::plane *GetCurrentPlane() const;
	const wyrmgus::world *GetCurrentWorld() const;
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

	const std::vector<CUnit *> &get_settlement_units() const
	{
		return this->settlement_units;
	}

	void add_settlement_unit(CUnit *settlement_unit)
	{
		this->settlement_units.push_back(settlement_unit);
	}

	void remove_settlement_unit(CUnit *settlement_unit);

private:
	/// Build tables for fog of war
	void InitFogOfWar();

	//Wyrmgus start
	/*
	/// Correct the surrounding seen wood fields
	void FixNeighbors(unsigned short type, int seen, const Vec2i &pos);
	/// Correct the seen wood field, depending on the surrounding
	void FixTile(unsigned short type, int seen, const Vec2i &pos);
	*/
	//Wyrmgus end

public:
	bool NoFogOfWar = false;           /// fog of war disabled

	std::unique_ptr<CTileset> Tileset;          /// tileset data
	std::string TileModelsFileName; /// lua filename that loads all tilemodels
	std::shared_ptr<CGraphic> TileGraphic;     /// graphic for all the tiles
	static std::shared_ptr<CGraphic> FogGraphics; //graphics for fog of war
	//Wyrmgus start
	int Landmasses = 0;						/// how many landmasses are there
	std::vector<std::vector<int>> BorderLandmasses;	/// "landmasses" which border the one to which each vector belongs
private:
	std::vector<CUnit *> settlement_units;	/// the town hall / settlement site units
public:
	std::vector<std::unique_ptr<CMapLayer>> MapLayers;	/// the map layers composing the map
	//Wyrmgus end

	CMapInfo Info;             /// descriptive information
};

extern char CurrentMapPath[1024]; /// Path to the current map

/// Contrast of fog of war
extern int FogOfWarOpacity;
/// fog of war color
extern CColor FogOfWarColor;
/// Flag must reveal the map
extern int FlagRevealMap;
/// Flag must reveal map when in replay
extern int ReplayRevealMap;

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
extern bool UnitTypeCanBeAt(const wyrmgus::unit_type &type, const Vec2i &pos, int z);
/// Returns true, if the unit can enter the field
extern bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos, int z);

/// Preprocess map, for internal use.
extern void PreprocessMap();

//Wyrmgus start
extern int GetMapLayer(const std::string &plane_ident = "", const std::string &world_ident = "");
extern void ChangeToPreviousMapLayer();
extern void ChangeCurrentMapLayer(const int z);
extern void SetTimeOfDay(const std::string &time_of_day_ident, int z = 0);
extern void SetTimeOfDaySchedule(const std::string &time_of_day_schedule_ident, const int z = 0);
extern void SetSeason(const std::string &season_ident, int z = 0);
extern void SetSeasonSchedule(const std::string &season_schedule_ident, int z = 0);
//Wyrmgus end

// in unit.c

/// Mark on vision table the Sight of the unit.
void MapMarkUnitSight(CUnit &unit);
/// Unmark on vision table the Sight of the unit.
void MapUnmarkUnitSight(CUnit &unit);

/// Can a unit with 'mask' enter the field
extern bool CanMoveToMask(const Vec2i &pos, int mask, int z);

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
