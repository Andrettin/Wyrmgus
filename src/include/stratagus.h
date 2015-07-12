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
/**@name stratagus.h - The main header file. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer and Jimmy Salmon
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

#ifndef __STRATAGUS_H__
#define __STRATAGUS_H__

//@{

/*============================================================================
==  Config definitions
============================================================================*/

// Dynamic loading.
//#define DYNAMIC_LOAD

/*============================================================================
==  Compiler repairs
============================================================================*/

#ifdef _MSC_VER

#define WIN32_LEAN_AND_MEAN
#define NOUSER
#define NOMINMAX // do not use min, max as macro

#pragma warning(disable:4244)               /// Conversion from double to uchar
#pragma warning(disable:4761)               /// Integral size mismatch
#pragma warning(disable:4786)               /// Truncated to 255 chars

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define snprintf _snprintf
#if !(_MSC_VER >= 1500 && _MSC_VER < 1600)
#define vsnprintf _vsnprintf
#endif
#define unlink _unlink
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#endif  // } _MSC_VER


/*============================================================================
==  Macro
============================================================================*/

// To remove warning for unused variable.
#ifdef __GNUC__
#define UNUSED(var) do {__typeof__ (&var) __attribute__ ((unused)) __tmp = &var; } while(0)
#else
#define UNUSED(var) (var)
#endif

#ifdef __GNUC__
#define PRINTF_VAARG_ATTRIBUTE(a, b) __attribute__((format (printf, a, b)))
#else
#define PRINTF_VAARG_ATTRIBUTE(a, b)
#endif

/*============================================================================
==  Debug definitions
============================================================================*/

/**
**  This simulates vararg macros.
**  Example:
**    DebugPrint("Test %d %d\n" _C_ 1 _C_ 2);
*/
#define _C_  ,    /// Debug , to simulate vararg macros

extern void PrintLocation(const char *file, int line, const char *funcName);

/// Print function in debug macros
#define PrintFunction() PrintLocation(__FILE__, __LINE__, __func__);

extern bool EnableDebugPrint;
extern bool EnableAssert;
extern bool EnableUnitDebug;

extern void AbortAt(const char *file, int line, const char *funcName, const char *conditionStr);
extern void PrintOnStdOut(const char *format, ...);

/**
**  Assert a condition. If cond is not true abort with file,line.
*/
#define Assert(cond) \
	do { if (EnableAssert && !(cond)) { AbortAt(__FILE__, __LINE__, __func__, #cond); }} while (0)

/**
**  Print debug information with function name.
*/
#define DebugPrint(args) \
	do { if (EnableDebugPrint) { PrintFunction(); PrintOnStdOut(args); } } while (0)

/*============================================================================
==  Definitions
============================================================================*/

#include <string.h>

#ifndef __UTIL_H__
#include "util.h"
#endif

inline char *new_strdup(const char *str)
{
	int len = strlen(str) + 1;
	char *newstr = new char[len];
	strcpy_s(newstr, len, str);
	return newstr;
}

/*----------------------------------------------------------------------------
--  General
----------------------------------------------------------------------------*/

/// Text string: Name, Version, Copyright
extern const char NameLine[];

/*----------------------------------------------------------------------------
--  Some limits
----------------------------------------------------------------------------*/

#define PlayerMax    16                 /// How many players are supported
#define UnitTypeMax  2048                /// How many unit types supported
#define UpgradeMax   2048                /// How many upgrades supported
//Wyrmgus start
#define UnitTypeClassMax  128		/// How many unit type classes are supported
//#define MAX_RACES 8
#define MAX_RACES 32
#define FactionMax 128				/// Maximum number of factions a civilization can have
#define PlayerColorMax 32			/// How many player colors are supported
#define VariationMax 32				/// Maximum number of variations a unit can have
#define PersonalNameMax 1024		/// Maximum number of personal names a civilization can have
#define LanguageWordMax 1024		/// Maximum number of words a civilization's language can have

#define WorldMapWidthMax 256		/// Maximum width the grand strategy world map can have
#define WorldMapHeightMax 256		/// Maximum height the grand strategy world map can have
#define WorldMapTerrainTypeMax 32	/// Maximum height the grand strategy world map can have
#define ProvinceMax 256				/// Maximum quantity of provinces in a grand strategy game
#define ProvinceTileMax 2048		/// Maximum quantity of tiles a province can have
#define WorldMapResourceMax 2048	/// Maximum quantity of resources of a given type which can exist on the world map.
//Wyrmgus end

/// Frames per second to display (original 30-40)
#define FRAMES_PER_SECOND  30  // 1/30s
/// Game cycles per second to simulate (original 30-40)
#define CYCLES_PER_SECOND  30  // 1/30s 0.33ms

/*----------------------------------------------------------------------------
--  stratagus.cpp
----------------------------------------------------------------------------*/

extern std::string StratagusLibPath;        /// Location of stratagus data
extern std::string MenuRace;

extern unsigned long GameCycle;             /// Game simulation cycle counter
extern unsigned long FastForwardCycle;      /// Game Replay Fast Forward Counter
//Wyrmgus start
extern int GameTimeOfDay;					/// Current time of day
//Wyrmgus end

extern void Exit(int err);                  /// Exit
extern void ExitFatal(int err);             /// Exit with fatal error

extern void UpdateDisplay();            /// Game display update
extern void DrawMapArea();              /// Draw the map area
extern void GameMainLoop();             /// Game main loop
extern int stratagusMain(int argc, char **argv); /// main entry

//Wyrmgus start
//Grand Strategy elements

#include "vec2i.h"
#include "video.h"
#include "upgrade_structs.h"

class WorldMapTerrainType
{
public:
	WorldMapTerrainType() :
		Name(""), Tag(""), HasTransitions(false), BaseTile(-1), Variations(0)
	{
	}

	std::string Name;
	std::string Tag;				/// used to locate graphic files
	bool HasTransitions;
	int BaseTile;
	int Variations;					/// quantity of variations
};

class WorldMapTile
{
public:
	WorldMapTile() :
		Terrain(-1), Province(-1), Variation(-1), Resource(-1),
		ResourceProspected(false), BorderTile(false), Position(-1, -1), GraphicTile(NULL)
	{
	}

	bool HasResource(int resource, bool ignore_prospection = false);	/// Get whether the tile has a resource
	std::string GetCulturalName();										/// Get the tile's cultural name.
	
	int Terrain;							/// Tile terrain (i.e. plains)
	int Province;							/// Province to which the tile belongs
	int Variation;							/// Tile variation
	int Resource;							/// The tile's resource, if any
	bool ResourceProspected;				/// Whether the tile's resource has been discovered
	bool BorderTile;						/// Whether this tile borders a tile of another province
	std::string Name;						/// Name of the tile (used for instance to name particular mountains)
	Vec2i Position;							/// Position of the tile
	CGraphic *GraphicTile;					/// The tile image used by this tile
	std::string CulturalNames[MAX_RACES];	/// Names for the tile for each different culture/civilization
};

class CProvince
{
public:
	CProvince() :
		Name(""), SettlementName(""),
		Civilization(-1), ReferenceProvince(-1),
		Water(false), SettlementLocation(-1, -1)
	{
		memset(Owner, -1, sizeof(Owner));
		memset(AttackedBy, -1, sizeof(Owner));
		memset(SettlementBuildings, 0, sizeof(SettlementBuildings));
		memset(BorderProvinces, 0, sizeof(BorderProvinces));
		for (int i = 0; i < ProvinceTileMax; ++i) {
			Tiles[i].x = -1;
			Tiles[i].y = -1;
		}
	}
	
	bool HasBuildingClass(std::string building_class_name);
	bool BordersProvince(int province_id);
	bool BordersFaction(int faction_civilization, int faction);
	std::string GetCulturalName();										/// Get the province's cultural name.
	std::string GetCulturalSettlementName();							/// Get the province's cultural settlement name.
	std::string GenerateProvinceName(int civilization);
	std::string GenerateSettlementName(int civilization);
	std::string GenerateTileName(int civilization, int terrain);
	
	std::string Name;
	std::string SettlementName;
	int Civilization;													/// Civilization of the province (-1 = no one).
	int Owner[2];														/// Owner of the province, first number for the owner's civilization, and the second one for the faction itself (-1, -1 = no one).
	int ReferenceProvince;												/// Reference province, if a water province (used for name changing) (-1 = none).
	int AttackedBy[2];													/// Which faction the province is being attacked by (-1, -1 = none); first number for the faction's civilization, and the second number is for the faction itself.
	bool Water;															/// Whether the province is a water province or not
	bool Coastal;														/// Whether the province is a coastal province or not
	Vec2i SettlementLocation;											/// In which tile the province's settlement is located
	int SettlementBuildings[UnitTypeMax];								/// Buildings in the province; 0 = not constructed, 1 = under construction, 2 = constructed
	int BorderProvinces[ProvinceMax];									/// Which provinces this province borders
	std::string CulturalNames[MAX_RACES];								/// Names for the province for each different culture/civilization
	std::string FactionCulturalNames[MAX_RACES][FactionMax];			/// Names for the province for each different faction
	std::string CulturalSettlementNames[MAX_RACES];						/// Names for the province's settlement for each different culture/civilization
	std::string FactionCulturalSettlementNames[MAX_RACES][FactionMax];	/// Names for the province's settlement for each different faction
	Vec2i Tiles[ProvinceTileMax];
};

/**
**  Grand Strategy game instance
**  Mapped with #GrandStrategy to a symbolic name.
*/
class CGrandStrategyGame
{
public:
	CGrandStrategyGame() : WorldMapWidth(0), WorldMapHeight(0), ProvinceCount(0), BaseTile(NULL)
	{
		for (int i = 0; i < MaxCosts; ++i) {
			for (int j = 0; j < WorldMapResourceMax; ++j) {
				WorldMapResources[i][j][0] = -1;
				WorldMapResources[i][j][1] = -1;
				WorldMapResources[i][j][2] = 0;
			}
		}
	}

	void Clean();
	void DrawMap();              /// Draw the map area
	void DrawTileTooltip(int x, int y);              /// Draw the map area
	Vec2i GetTileUnderCursor();

public:
	int WorldMapWidth;
	int WorldMapHeight;
	int ProvinceCount;
	CGraphic *BaseTile;
	CGraphic *FogTile;
	CGraphic *SymbolAttack;										///symbol that a province is being attacked (drawn at the settlement location)
	CGraphic *GoldMineGraphics;
	CGraphic *BorderGraphics[8];								///one for each direction, 0 = North, 1 = Northeast, 2 = East, 3 = Southeast, 4 = South, 5 = Southwest, 6 = West, 7 = Northwest
	CPlayerColorGraphic *SettlementGraphics[MAX_RACES];
	CPlayerColorGraphic *BarracksGraphics[MAX_RACES];
	CPlayerColorGraphic *NationalBorderGraphics[8];				///one for each direction, 0 = North, 1 = Northeast, 2 = East, 3 = Southeast, 4 = South, 5 = Southwest, 6 = West, 7 = Northwest
	WorldMapTerrainType *TerrainTypes[WorldMapTerrainTypeMax];
	WorldMapTile *WorldMapTiles[WorldMapWidthMax][WorldMapHeightMax];
	CProvince *Provinces[ProvinceMax];
	int WorldMapResources[MaxCosts][WorldMapResourceMax][3];	///resources on the map; three values: the resource's x position, its y position, and whether it is discovered or not
};

extern bool GrandStrategy;								/// if the game is in grand strategy mode
extern std::string GrandStrategyWorld;
extern int WorldMapOffsetX;
extern int WorldMapOffsetY;
extern int GrandStrategyMapWidthIndent;
extern int GrandStrategyMapHeightIndent;
extern CGrandStrategyGame GrandStrategyGame;			/// Grand strategy game

extern int GetWorldMapWidth();
extern int GetWorldMapHeight();
extern std::string GetWorldMapTileTerrain(int x, int y);
extern int GetWorldMapTileTerrainVariation(int x, int y);
extern std::string GetWorldMapTileProvinceName(int x, int y);
extern int GetWorldMapTerrainTypeId(std::string terrain_type_name);
extern int GetProvinceId(std::string province_name);
extern void SetWorldMapSize(int width, int height);
extern void SetWorldMapTileTerrain(int x, int y, int terrain);
extern void SetWorldMapTileProvince(int x, int y, std::string province_name);
extern void SetWorldMapTileName(int x, int y, std::string name);
extern void SetWorldMapTileCulturalName(int x, int y, std::string civilization_name, std::string cultural_name);
extern void CalculateWorldMapTileGraphicTile(int x, int y);
extern void AddWorldMapResource(std::string resource_name, int x, int y, bool discovered);
extern void SetWorldMapResourceProspected(std::string resource_name, int x, int y, bool discovered);
extern std::string GetProvinceCulturalName(std::string province_name);
extern std::string GetProvinceCivilizationCulturalName(std::string province_name, std::string civilization_name);
extern std::string GetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name);
extern std::string GetProvinceCulturalSettlementName(std::string province_name);
extern std::string GetProvinceCivilizationCulturalSettlementName(std::string province_name, std::string civilization_name);
extern std::string GetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name);
extern std::string GetProvinceAttackedBy(std::string province_name);
extern void SetProvinceName(std::string old_province_name, std::string new_province_name);
extern void SetProvinceWater(std::string province_name, bool water);
extern void SetProvinceOwner(std::string province_name, std::string civilization_name, std::string faction_name);
extern void SetProvinceCivilization(std::string province_name, std::string civilization_name);
extern void SetProvinceSettlementName(std::string province_name, std::string settlement_name);
extern void SetProvinceSettlementLocation(std::string province_name, int x, int y);
extern void SetProvinceCulturalName(std::string province_name, std::string civilization_name, std::string province_cultural_name);
extern void SetProvinceFactionCulturalName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name);
extern void SetProvinceCulturalSettlementName(std::string province_name, std::string civilization_name, std::string province_cultural_name);
extern void SetProvinceFactionCulturalSettlementName(std::string province_name, std::string civilization_name, std::string faction_name, std::string province_cultural_name);
extern void SetProvinceReferenceProvince(std::string province_name, std::string reference_province_name);
extern void SetProvinceSettlementBuilding(std::string province_name, std::string settlement_building_ident, int value);
extern void SetProvinceAttackedBy(std::string province_name, std::string civilization_name, std::string faction_name);
extern void CleanGrandStrategyGame();
extern void InitializeGrandStrategyGame();
extern void CalculateProvinceBorders();
extern bool ProvinceBordersProvince(std::string province_name, std::string second_province_name);
extern bool ProvinceBordersFaction(std::string province_name, std::string faction_civilization_name, std::string faction_name);
//Wyrmgus end

//@}

#endif // !__STRATAGUS_H__
