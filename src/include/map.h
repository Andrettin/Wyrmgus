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
/**@name map.h - The map headerfile. */
//
//      (c) Copyright 1998-2006 by Vladi Shabanski, Lutz Sammer, and
//                                 Jimmy Salmon
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

#ifndef __MAP_H__
#define __MAP_H__

//@{

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CMap map.h
**
**  \#include "map.h"
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

#include <string>
//Wyrmgus start
#include <map>
//Wyrmgus end

#ifndef __MAP_TILE_H__
#include "tile.h"
#endif

#include "color.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGraphic;
class CPlayer;
class CFile;
class CTileset;
class CUnit;
class CUnitType;
//Wyrmgus start
class CFaction;
class CCharacter;
class CUniqueItem;
class CWorld;
class CPlane;
class CSettlement;
class CRegion;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Map
----------------------------------------------------------------------------*/

//Wyrmgus start
//#define MaxMapWidth  256  /// max map width supported
//#define MaxMapHeight 256  /// max map height supported
#define MaxMapWidth  512  /// max map width supported
#define MaxMapHeight 512  /// max map height supported
#define DefaultTimeOfDaySeconds (10 * 3) // every 10 seconds of gameplay = 1 hour for time of day calculations, change time of day every three hours
//Wyrmgus end

//Wyrmgus start
enum DegreeLevels {
	ExtremelyHighDegreeLevel,
	VeryHighDegreeLevel,
	HighDegreeLevel,
	MediumDegreeLevel,
	LowDegreeLevel,
	VeryLowDegreeLevel,
	
	MaxDegreeLevels
};
//Wyrmgus end

//Wyrmgus start
class CTerrainFeature
{
public:
	CTerrainFeature() :
		ID(-1), TerrainType(NULL), World(NULL)
	{
	}
	
	int ID;
	std::string Ident;
	std::string Name;
	CColor Color;
	CTerrainType *TerrainType;
	CWorld *World;
	std::map<int, std::string> CulturalNames;							/// Names for the terrain feature for each different culture/civilization
};

class CTimeline
{
public:
	CTimeline() :
		ID(-1)
	{
	}
	
	int ID;
	std::string Ident;
	std::string Name;
	CDate PointOfDivergence;											/// The point of divergence for this timeline
};

class CMapTemplate
{
public:
	CMapTemplate() :
		Width(0), Height(0), Scale(1), TimeOfDaySeconds(DefaultTimeOfDaySeconds), Layer(0), SubtemplatePosition(-1, -1),
		MainTemplate(NULL), Plane(NULL), World(NULL), BaseTerrain(NULL), BorderTerrain(NULL), SurroundingTerrain(NULL)
	{
	}

	void ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z);
	void ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z);
	void Apply(Vec2i template_start_pos, Vec2i map_start_pos, int z);
	void ApplySettlements(Vec2i template_start_pos, Vec2i map_start_pos, int z);
	void ApplyConnectors(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random = false);
	void ApplyUnits(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random = false);
	bool IsSubtemplateArea();
	
	std::string Name;
	std::string Ident;
	std::string TerrainFile;
	std::string OverlayTerrainFile;
	std::string TerrainImage;
	std::string OverlayTerrainImage;
	int Width;
	int Height;
	int Scale;													/// 1 means a map template tile will be applied as one in-game tile, 2 means a 2x2 in-game tile
	int TimeOfDaySeconds;
	int Layer;													/// Surface layer of the map template (0 for surface, 1 and above for underground layers in succession)
	Vec2i SubtemplatePosition;
	CMapTemplate *MainTemplate;									/// Main template in which this one is located
	CPlane *Plane;
	CWorld *World;
	CTerrainType *BaseTerrain;
	CTerrainType *BorderTerrain;
	CTerrainType *SurroundingTerrain;
	std::vector<CMapTemplate *> Subtemplates;
	std::vector<std::pair<CTerrainType *, int>> GeneratedTerrains;
	std::vector<std::pair<CTerrainType *, int>> ExternalGeneratedTerrains;
	std::vector<std::pair<CUnitType *, int>> GeneratedNeutralUnits; /// the first element of the pair is the resource's unit type, and the second is the quantity
	std::vector<std::pair<CUnitType *, int>> PlayerLocationGeneratedNeutralUnits;
	std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>> Resources; /// Resources (with unit type, resources held, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, CFaction *, CDate, CDate, CUniqueItem *>> Units; /// Units; first value is the tile position, and the last ones are start date and end date
	std::vector<std::tuple<Vec2i, CCharacter *, CFaction *, CDate, CDate>> Heroes; /// Heroes; first value is the tile position, and the last ones are start year and end year
	std::vector<std::tuple<Vec2i, CUnitType *, CPlane *, CUniqueItem *>> PlaneConnectors; /// Layer connectors (with unit type, plane pointer, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, CWorld *, CUniqueItem *>> WorldConnectors; /// Layer connectors (with unit type, world pointer, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, int, CUniqueItem *>> LayerConnectors; /// Layer connectors (with unit type, surface/underground layer, and unique item pointer), mapped to the tile position
	std::map<std::pair<int, int>, std::string> TileLabels; /// labels to appear for certain tiles
	std::map<std::pair<int, int>, CSettlement *> Settlements;
	std::vector<std::tuple<Vec2i, CTerrainType *, CDate>> HistoricalTerrains;	/// Terrain changes
};

class CSettlement
{
public:
	CSettlement() :
		Position(-1, -1),
		MapTemplate(NULL),
		Major(false)
	{
	}
	
	std::string GetCulturalName(int civilization);

	std::string Ident;
	std::string Name;
	bool Major;													/// Whether the settlement is a major one; major settlements have settlement sites, and as such can have town halls
	Vec2i Position;												/// Position of the settlement in its map template
	CMapTemplate *MapTemplate;									/// Map template where this settlement is located
	std::vector<CRegion *> Regions;								/// Regions where this settlement is located
	std::map<int, std::string> CulturalNames;					/// Names for the settlement for each different culture/civilization
	std::map<CDate, CFaction *> HistoricalOwners;				/// Historical owners of the settlement
	std::map<CDate, int> HistoricalPopulation;					/// Historical population
	std::map<CUnitType *, std::map<CDate, std::pair<int, CFaction *>>> HistoricalUnits;	/// Historical quantity of a particular unit type (number of people for units representing a person)
	std::vector<std::tuple<CDate, CDate, int, CUniqueItem *, CFaction *>> HistoricalBuildings; /// Historical buildings, with start and end date
	std::vector<std::tuple<CDate, CDate, CUnitType *, CUniqueItem *, int>> HistoricalResources; /// Historical resources, with start and end date; the integer at the end is the resource quantity
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
	//Wyrmgus start
//	bool IsPointOnMap(int x, int y) const
	bool IsPointOnMap(int x, int y, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		return (x >= 0 && y >= 0 && x < MapWidth && y < MapHeight);
		return (z >= 0 && z < (int) MapWidths.size() && z < (int) MapHeights.size() && x >= 0 && y >= 0 && x < MapWidths[z] && y < MapHeights[z]);
		//Wyrmgus end
	}

	//Wyrmgus start
//	bool IsPointOnMap(const Vec2i &pos) const
	bool IsPointOnMap(const Vec2i &pos, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		return IsPointOnMap(pos.x, pos.y);
		return IsPointOnMap(pos.x, pos.y, z);
		//Wyrmgus end
	}

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
	CMap();
	~CMap();

	//Wyrmgus start
//	unsigned int getIndex(int x, int y) const
	unsigned int getIndex(int x, int y, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		return x + y * this->Info.MapWidth;
		return x + y * this->Info.MapWidths[z];
		//Wyrmgus end
	}
	//Wyrmgus start
//	unsigned int getIndex(const Vec2i &pos) const
	unsigned int getIndex(const Vec2i &pos, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		return getIndex(pos.x, pos.y);
		return getIndex(pos.x, pos.y, z);
		//Wyrmgus end
	}

	//Wyrmgus start
//	CMapField *Field(unsigned int index) const
	CMapField *Field(unsigned int index, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		return &this->Fields[index];
		return &this->Fields[z][index];
		//Wyrmgus end
	}
	/// Get the MapField at location x,y
	//Wyrmgus start
//	CMapField *Field(int x, int y) const
	CMapField *Field(int x, int y, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		return &this->Fields[x + y * this->Info.MapWidth];
		return &this->Fields[z][x + y * this->Info.MapWidths[z]];
		//Wyrmgus end
	}
	//Wyrmgus start
//	CMapField *Field(const Vec2i &pos) const
	CMapField *Field(const Vec2i &pos, int z) const
	//Wyrmgus end
	{
		//Wyrmgus start
//		return Field(pos.x, pos.y);
		return Field(pos.x, pos.y, z);
		//Wyrmgus end
	}

	/// Alocate and initialise map table.
	void Create();
	/// Build tables for map
	void Init();
	/// Clean the map
	void Clean();
	/// Cleanup memory for fog of war tables
	void CleanFogOfWar();
	
	//Wyrmgus start
	void SetTileTerrain(const Vec2i &pos, CTerrainType *terrain, int z);
	void RemoveTileOverlayTerrain(const Vec2i &pos, int z);
	void SetOverlayTerrainDestroyed(const Vec2i &pos, bool destroyed, int z);
	void SetOverlayTerrainDamaged(const Vec2i &pos, bool damaged, int z);
	void CalculateTileTransitions(const Vec2i &pos, bool overlay, int z);
	void CalculateTileLandmass(const Vec2i &pos, int z);
	void CalculateTileOwnership(const Vec2i &pos, int z);
	void CalculateTileOwnershipTransition(const Vec2i &pos, int z);
	void AdjustMap();
	void AdjustTileMapIrregularities(bool overlay, const Vec2i &min_pos, const Vec2i &max_pos, int z);
	void AdjustTileMapTransitions(const Vec2i &min_pos, const Vec2i &max_pos, int z);
	void GenerateTerrain(CTerrainType *terrain, int seed_number, int expansion_number, const Vec2i &min_pos, const Vec2i &max_pos, bool preserve_coastline, int z);
	void GenerateNeutralUnits(CUnitType *unit_type, int quantity, const Vec2i &min_pos, const Vec2i &max_pos, bool grouped, int z);
	//Wyrmgus end

	//Wyrmgus start
	void ClearOverlayTile(const Vec2i &pos, int z);
	/*
	/// Remove wood from the map.
	void ClearWoodTile(const Vec2i &pos);
	/// Remove rock from the map.
	void ClearRockTile(const Vec2i &pos);
	*/
	//Wyrmgus end

	/// convert map pixelpos coordonates into tilepos
	Vec2i MapPixelPosToTilePos(const PixelPos &mapPos) const;
	/// convert tilepos coordonates into map pixel pos (take the top left of the tile)
	PixelPos TilePosToMapPixelPos_TopLeft(const Vec2i &tilePos) const;
	/// convert tilepos coordonates into map pixel pos (take the center of the tile)
	PixelPos TilePosToMapPixelPos_Center(const Vec2i &tilePos) const;
	
	//Wyrmgus start
	CTerrainType *GetTileTerrain(const Vec2i &pos, bool overlay, int z) const;
	CTerrainType *GetTileTopTerrain(const Vec2i &pos, bool seen, int z) const;
	int GetTileLandmass(const Vec2i &pos, int z) const;
	Vec2i GenerateUnitLocation(const CUnitType *unit_type, CFaction *faction, Vec2i min_pos, Vec2i max_pos, int z) const;
	//Wyrmgus end

	/// Mark a tile as seen by the player.
	//Wyrmgus start
//	void MarkSeenTile(CMapField &mf);
	void MarkSeenTile(CMapField &mf, int z);
	//Wyrmgus end

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
//	void HitWall(const Vec2i &pos, unsigned damage);
	void HitWall(const Vec2i &pos, unsigned damage, int z);
	//Wyrmgus end
	/// Set wall on field.
	//Wyrmgus start
	/*
	void RemoveWall(const Vec2i &pos);
	/// Set wall on field.
	void SetWall(const Vec2i &pos, bool humanwall);
	*/
	//Wyrmgus end

	/// Returns true, if wall on the map tile field
	//Wyrmgus start
//	bool WallOnMap(const Vec2i &pos) const;
	bool WallOnMap(const Vec2i &pos, int z) const;
	//Wyrmgus end
	//Wyrmgus start
	/*
	/// Returns true, if human wall on the map tile field
	bool HumanWallOnMap(const Vec2i &pos) const;
	/// Returns true, if orc wall on the map tile field
	bool OrcWallOnMap(const Vec2i &pos) const;
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	bool CurrentTerrainCanBeAt(const Vec2i &pos, bool overlay, int z);
	bool TileBordersOnlySameTerrain(const Vec2i &pos, CTerrainType *new_terrain, int z);
	bool TileBordersFlag(const Vec2i &pos, int z, int flag, bool reverse = false); // reverse means that it returns true if the tile borders one tile without the flag
	bool TileBordersBuilding(const Vec2i &pos, int z);
	bool TileBordersPathway(const Vec2i &pos, int z, bool only_railroad);
	bool TileBordersUnit(const Vec2i &pos, int z);
	bool TileHasUnitsIncompatibleWithTerrain(const Vec2i &pos, CTerrainType *terrain, int z);
	bool IsPointInASubtemplateArea(const Vec2i &pos, int z) const;
	bool IsLayerUnderground(int z) const;
	//Wyrmgus end

	//UnitCache

	/// Insert new unit into cache
	void Insert(CUnit &unit);

	/// Remove unit from cache
	void Remove(CUnit &unit);

	//Wyrmgus start
//	void Clamp(Vec2i &pos) const;
	void Clamp(Vec2i &pos, int z) const;
	//Wyrmgus end

	//Warning: we expect typical usage as xmin = x - range
	//Wyrmgus start
//	void FixSelectionArea(Vec2i &minpos, Vec2i &maxpos)
	void FixSelectionArea(Vec2i &minpos, Vec2i &maxpos, int z)
	//Wyrmgus end
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
	void InitFogOfWar();

	//Wyrmgus start
	/*
	/// Correct the surrounding seen wood fields
	void FixNeighbors(unsigned short type, int seen, const Vec2i &pos);
	/// Correct the seen wood field, depending on the surrounding
	void FixTile(unsigned short type, int seen, const Vec2i &pos);
	*/
	//Wyrmgus end

	/// Regenerate the forest.
	//Wyrmgus start
//	void RegenerateForestTile(const Vec2i &pos);
	void RegenerateForestTile(const Vec2i &pos, int z);
	//Wyrmgus end

public:
	//Wyrmgus start
//	CMapField *Fields;              /// fields on map
	std::vector<CMapField *> Fields;              /// fields on map
	//Wyrmgus end
	bool NoFogOfWar;           /// fog of war disabled

	CTileset *Tileset;          /// tileset data
	std::string TileModelsFileName; /// lua filename that loads all tilemodels
	CGraphic *TileGraphic;     /// graphic for all the tiles
	static CGraphic *FogGraphic;      /// graphic for fog of war
	//Wyrmgus start
	CTerrainType *BorderTerrain;      	/// terrain type for borders
	int Landmasses;						/// how many landmasses are there
	std::vector<std::vector<int>> BorderLandmasses;	/// "landmasses" which border the one to which each vector belongs
	std::vector<int> TimeOfDaySeconds;		/// how many seconds it takes to change the time of day, for each map layer
	std::vector<int> TimeOfDay;				/// the time of day for each map layer
	std::vector<CPlane *> Planes;			/// the plane pointer (if any) for each map layer
	std::vector<CWorld *> Worlds;			/// the world pointer (if any) for each map layer
	std::vector<int> Layers;				/// the surface layer (if any) for each map layer
	std::vector<std::vector<CUnit *>> LayerConnectors;	/// connectors in a layer that lead to other layers
	std::map<int, std::vector<std::tuple<Vec2i, Vec2i, CMapTemplate *>>> SubtemplateAreas;
	std::vector<CUnit *> SettlementUnits;	/// the town hall / settlement site units
	//Wyrmgus end

	CMapInfo Info;             /// descriptive information
};


/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//Wyrmgus start
extern std::vector<CMapTemplate *>  MapTemplates;
extern std::map<std::string, CMapTemplate *> MapTemplateIdentToPointer;
extern std::vector<CSettlement *>  Settlements;
extern std::map<std::string, CSettlement *> SettlementIdentToPointer;
extern std::vector<CTerrainFeature *> TerrainFeatures;
extern std::map<std::string, CTerrainFeature *> TerrainFeatureIdentToPointer;
extern std::map<std::tuple<int, int, int>, int> TerrainFeatureColorToIndex;
extern std::vector<CTimeline *> Timelines;
extern std::map<std::string, CTimeline *> TimelineIdentToPointer;
//Wyrmgus end

extern CMap Map;  /// The current map
extern char CurrentMapPath[1024]; /// Path to the current map
//Wyrmgus start
extern int CurrentMapLayer;
//Wyrmgus end

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
extern CMapTemplate *GetMapTemplate(std::string map_ident);
extern CSettlement *GetSettlement(std::string settlement_ident);
extern CTerrainFeature *GetTerrainFeature(std::string terrain_feature_ident);
extern CTimeline *GetTimeline(std::string timeline_ident);
extern std::string GetDegreeLevelNameById(int degree_level);
extern int GetDegreeLevelIdByName(std::string degree_level);
//Wyrmgus end

#define MARKER_ON_INDEX
//
// in map_fog.c
//
/// Function to (un)mark the vision table.
#ifndef MARKER_ON_INDEX
//Wyrmgus start
//typedef void MapMarkerFunc(const CPlayer &player, const Vec2i &pos);
typedef void MapMarkerFunc(const CPlayer &player, const Vec2i &pos, int z);
//Wyrmgus end
#else
//Wyrmgus start
//typedef void MapMarkerFunc(const CPlayer &player, const unsigned int index);
typedef void MapMarkerFunc(const CPlayer &player, const unsigned int index, int z);
//Wyrmgus end
#endif

/// Filter map flags through fog
//Wyrmgus start
//extern int MapFogFilterFlags(CPlayer &player, const Vec2i &pos, int mask);
//extern int MapFogFilterFlags(CPlayer &player, const unsigned int index, int mask);
extern int MapFogFilterFlags(CPlayer &player, const Vec2i &pos, int mask, int z);
extern int MapFogFilterFlags(CPlayer &player, const unsigned int index, int mask, int z);
//Wyrmgus end
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
					 //Wyrmgus start
//					 int h, int range, MapMarkerFunc *marker);
					 int h, int range, MapMarkerFunc *marker, int z);
					 //Wyrmgus end
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
//Wyrmgus start
//extern void SetTile(unsigned int tile, const Vec2i &pos, int value = 0);
//inline void SetTile(unsigned int tile, int x, int y, int value = 0)
extern void SetTile(unsigned int tile, const Vec2i &pos, int value = 0, int z = 0);
inline void SetTile(unsigned int tile, int x, int y, int value = 0, int z = 0)
//Wyrmgus end
{
	const Vec2i pos(x, y);
	//Wyrmgus start
//	SetTile(tile, pos, value);
	SetTile(tile, pos, value, z);
	//Wyrmgus end
}

//Wyrmgus start
extern void SetTileTerrain(std::string terrain_ident, const Vec2i &pos, int value = 0, int z = 0);
inline void SetTileTerrain(std::string terrain_ident, int x, int y, int value = 0, int z = 0)
{
	const Vec2i pos(x, y);
	SetTileTerrain(terrain_ident, pos, value, z);
}
extern void ApplyMapTemplate(std::string map_template_ident, int start_x = 0, int start_y = 0, int map_start_x = 0, int map_start_y = 0, int z = 0);
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
//Wyrmgus start
//extern bool CheckedCanMoveToMask(const Vec2i &pos, int mask);
extern bool CheckedCanMoveToMask(const Vec2i &pos, int mask, int z);
//Wyrmgus end
/// Returns true, if the unit-type can enter the field
//Wyrmgus start
//extern bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos);
extern bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos, int z);
//Wyrmgus end
/// Returns true, if the unit can enter the field
//Wyrmgus start
//extern bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos);
extern bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos, int z);
//Wyrmgus end

/// Preprocess map, for internal use.
extern void PreprocessMap();

//Wyrmgus start
extern int GetMapLayer(std::string plane_name = "", std::string world_name = "", int surface_layer = 0);
extern int GetSubtemplateStartX(std::string subtemplate_ident);
extern int GetSubtemplateStartY(std::string subtemplate_ident);
extern void ChangeCurrentMapLayer(int z);
extern void SetTimeOfDay(int time_of_day, int z = 0);
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
//Wyrmgus start
//inline bool CanMoveToMask(const Vec2i &pos, int mask)
inline bool CanMoveToMask(const Vec2i &pos, int mask, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	return !Map.Field(pos)->CheckMask(mask);
	return !Map.Field(pos, z)->CheckMask(mask);
	//Wyrmgus end
}

/// Handle Marking and Unmarking of radar vision
//Wyrmgus start
//inline void MapMarkRadar(const CPlayer &player, const Vec2i &pos, int w, int h, int range)
inline void MapMarkRadar(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapSight(player, pos, w, h, range, MapMarkTileRadar);
	MapSight(player, pos, w, h, range, MapMarkTileRadar, z);
	//Wyrmgus end
}
//Wyrmgus start
//inline void MapUnmarkRadar(const CPlayer &player, const Vec2i &pos, int w, int h, int range)
inline void MapUnmarkRadar(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapSight(player, pos, w, h, range, MapUnmarkTileRadar);
	MapSight(player, pos, w, h, range, MapUnmarkTileRadar, z);
	//Wyrmgus end
}
/// Handle Marking and Unmarking of radar vision
//Wyrmgus start
//inline void MapMarkRadarJammer(const CPlayer &player, const Vec2i &pos, int w, int h, int range)
inline void MapMarkRadarJammer(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapSight(player, pos, w, h, range, MapMarkTileRadarJammer);
	MapSight(player, pos, w, h, range, MapMarkTileRadarJammer, z);
	//Wyrmgus end
}
//Wyrmgus start
//inline void MapUnmarkRadarJammer(const CPlayer &player, const Vec2i &pos, int w, int h, int range)
inline void MapUnmarkRadarJammer(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapSight(player, pos, w, h, range, MapUnmarkTileRadarJammer);
	MapSight(player, pos, w, h, range, MapUnmarkTileRadarJammer, z);
	//Wyrmgus end
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

//@}

#endif // !__MAP_H__
