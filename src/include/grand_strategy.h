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
/**@name grand_strategy.h - The grand strategy headerfile. */
//
//      (c) Copyright 2015 by Andrettin
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

#ifndef __GRAND_STRATEGY_H__
#define __GRAND_STRATEGY_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "vec2i.h"
#include "video.h"
#include "upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

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
		ResourceProspected(false), Position(-1, -1), GraphicTile(NULL)
	{
		memset(Borders, 0, sizeof(Borders));
		memset(River, -1, sizeof(River));
		memset(Riverhead, -1, sizeof(Riverhead));
		memset(Road, 0, sizeof(Road));
	}

	void UpdateMinimap();
	bool HasResource(int resource, bool ignore_prospection = false);	/// Get whether the tile has a resource
	std::string GetCulturalName();										/// Get the tile's cultural name.
	
	int Terrain;							/// Tile terrain (i.e. plains)
	int Province;							/// Province to which the tile belongs
	int Variation;							/// Tile variation
	int Resource;							/// The tile's resource, if any
	bool ResourceProspected;				/// Whether the tile's resource has been discovered
	std::string Name;						/// Name of the tile (used for instance to name particular mountains)
	Vec2i Position;							/// Position of the tile
	CGraphic *GraphicTile;					/// The tile image used by this tile
	bool Borders[MaxDirections];			/// Whether this tile borders a tile of another province to a particular direction
	int River[MaxDirections];				/// Whether this tile has a river to a particular direction (the value for each direction is the ID of the river)
	int Riverhead[MaxDirections];			/// Whether this tile has a riverhead to a particular direction (the value for each direction is the ID of the river)
	bool Road[MaxDirections];				/// Whether this tile has a road to a particular direction
	std::string CulturalNames[MAX_RACES];	/// Names for the tile for each different culture/civilization
};

class CProvince
{
public:
	CProvince() :
		Name(""), SettlementName(""),
		Civilization(-1), ReferenceProvince(-1), CurrentConstruction(-1),
		Water(false), SettlementLocation(-1, -1)
	{
		memset(Owner, -1, sizeof(Owner));
		memset(AttackedBy, -1, sizeof(Owner));
		memset(SettlementBuildings, 0, sizeof(SettlementBuildings));
		memset(Units, 0, sizeof(Units));
		memset(UnderConstructionUnits, 0, sizeof(UnderConstructionUnits));
		memset(MovingUnits, 0, sizeof(MovingUnits));
		memset(AttackingUnits, 0, sizeof(AttackingUnits));
		memset(BorderProvinces, 0, sizeof(BorderProvinces));
		for (int i = 0; i < ProvinceTileMax; ++i) {
			Tiles[i].x = -1;
			Tiles[i].y = -1;
		}
	}
	
	void UpdateMinimap();
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
	int CurrentConstruction;											/// Building currently under construction (unit type index).
	int AttackedBy[2];													/// Which faction the province is being attacked by (-1, -1 = none); first number for the faction's civilization, and the second number is for the faction itself.
	bool Water;															/// Whether the province is a water province or not
	bool Coastal;														/// Whether the province is a coastal province or not
	Vec2i SettlementLocation;											/// In which tile the province's settlement is located
	bool SettlementBuildings[UnitTypeMax];								/// Buildings in the province; 0 = not constructed, 1 = under construction, 2 = constructed
	int Units[UnitTypeMax];												/// Quantity of units of a particular unit type in the province
	int UnderConstructionUnits[UnitTypeMax];							/// Quantity of units of a particular unit type being trained/constructed in the province
	int MovingUnits[UnitTypeMax];										/// Quantity of units of a particular unit type moving to the province
	int AttackingUnits[UnitTypeMax];									/// Quantity of units of a particular unit type attacking the province
	int BorderProvinces[ProvinceMax];									/// Which provinces this province borders
	std::string CulturalNames[MAX_RACES];								/// Names for the province for each different culture/civilization
	std::string FactionCulturalNames[MAX_RACES][FactionMax];			/// Names for the province for each different faction
	std::string CulturalSettlementNames[MAX_RACES];						/// Names for the province's settlement for each different culture/civilization
	std::string FactionCulturalSettlementNames[MAX_RACES][FactionMax];	/// Names for the province's settlement for each different faction
	Vec2i Tiles[ProvinceTileMax];
};

class CGrandStrategyFaction
{
public:
	CGrandStrategyFaction() :
		Faction(-1), Civilization(-1), CurrentResearch(-1)
	{
		memset(Technologies, 0, sizeof(Technologies));
		memset(Income, 0, sizeof(Income));
	}
	
	void SetTechnology(int upgrade_id, bool has_technology);
	
	int Faction;														/// The faction's ID (-1 = none).
	int Civilization;													/// Civilization of the faction (-1 = none).
	int CurrentResearch;												/// Currently researched technology (upgrade index).
	bool Technologies[UpgradeMax];										/// Whether a faction has a particualr technology or not; 0 = technology hasn't been acquired, 1 = technology is under research, 2 = technology has been researched
	int Income[MaxCosts];												/// Income of each resource for the faction.
};

class CRiver
{
public:
	CRiver() :
		Name("")
	{
	}
	
	std::string GetCulturalName(int civilization);						/// Get the river's cultural name for a particular civilization.
	
	std::string Name;
	std::string CulturalNames[MAX_RACES];								/// Names for the river for each different culture/civilization
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
	void DrawMap();							/// Draw the map area
	void DrawMinimap();						/// Draw the minimap
	void DrawTileTooltip(int x, int y);		/// Draw the tooltip for a tile
	void DoTurn();							/// Process the grand strategy turn
	#if defined(USE_OPENGL) || defined(USE_GLES)
	void CreateMinimapTexture();
	#endif
	void UpdateMinimap();
	Vec2i GetTileUnderCursor();

public:
	int WorldMapWidth;
	int WorldMapHeight;
	int ProvinceCount;
	CGraphic *BaseTile;
	CGraphic *FogTile;
	CGraphic *SymbolAttack;										///symbol that a province is being attacked (drawn at the settlement location)
	CGraphic *GoldMineGraphics;
	CGraphic *BorderGraphics[MaxDirections];					///one for each direction
	CGraphic *RiverGraphics[MaxDirections];
	CGraphic *RivermouthGraphics[MaxDirections][2];				///the two values are whether it is flipped or not
	CGraphic *RiverheadGraphics[MaxDirections][2];				///the two values are whether it is flipped or not
	CGraphic *RoadGraphics[MaxDirections];
	CPlayerColorGraphic *SettlementGraphics[MAX_RACES];
	CPlayerColorGraphic *BarracksGraphics[MAX_RACES];
	CPlayerColorGraphic *NationalBorderGraphics[MaxDirections];	///one for each direction
	WorldMapTerrainType *TerrainTypes[WorldMapTerrainTypeMax];
	WorldMapTile *WorldMapTiles[WorldMapWidthMax][WorldMapHeightMax];
	CProvince *Provinces[ProvinceMax];
	CGrandStrategyFaction *Factions[MAX_RACES][FactionMax];
	CRiver *Rivers[RiverMax];
	int WorldMapResources[MaxCosts][WorldMapResourceMax][3];	///resources on the map; three values: the resource's x position, its y position, and whether it is discovered or not

	int MinimapTextureWidth;
	int MinimapTextureHeight;
	int MinimapTileWidth;										/// minimap tile width (per mil)
	int MinimapTileHeight;										/// minimap tile height (per mil)
	int MinimapOffsetX;
	int MinimapOffsetY;

	#if defined(USE_OPENGL) || defined(USE_GLES)
	unsigned char *MinimapSurfaceGL;
	GLuint MinimapTexture;
	#endif

	SDL_Surface *MinimapSurface;
};

extern bool GrandStrategy;								/// if the game is in grand strategy mode
extern std::string GrandStrategyWorld;
extern int WorldMapOffsetX;
extern int WorldMapOffsetY;
extern int GrandStrategyMapWidthIndent;
extern int GrandStrategyMapHeightIndent;
extern int BattalionMultiplier;
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
extern int GetRiverId(std::string river_name);
extern void SetWorldMapTileRiver(int x, int y, std::string direction_name, std::string river_name);
extern void SetWorldMapTileRiverhead(int x, int y, std::string direction_name, std::string river_name);
extern void SetWorldMapTileRoad(int x, int y, std::string direction_name, bool has_road);
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
extern void SetProvinceSettlementBuilding(std::string province_name, std::string settlement_building_ident, bool has_settlement_building);
extern void SetProvinceCurrentConstruction(std::string province_name, std::string settlement_building_ident);
extern void SetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void ChangeProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceAttackedBy(std::string province_name, std::string civilization_name, std::string faction_name);
extern void UpdateProvinceMinimap(std::string province_name);
extern void CleanGrandStrategyGame();
extern void InitializeGrandStrategyGame();
extern void InitializeGrandStrategyMinimap();
extern void SetGrandStrategyWorld(std::string world);
extern void DoGrandStrategyTurn();
extern void CalculateProvinceBorders();
extern void CenterGrandStrategyMapOnTile(int x, int y);
extern bool ProvinceBordersProvince(std::string province_name, std::string second_province_name);
extern bool ProvinceBordersFaction(std::string province_name, std::string faction_civilization_name, std::string faction_name);
extern bool ProvinceHasBuildingClass(std::string province_name, std::string building_class);
extern bool IsGrandStrategyBuilding(const CUnitType &type);
extern bool GetProvinceSettlementBuilding(std::string province_name, std::string building_ident);
extern std::string GetProvinceCurrentConstruction(std::string province_name);
extern int GetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident);
extern void SetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident, bool has_technology);
extern bool GetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident);
extern void SetFactionCurrentResearch(std::string civilization_name, std::string faction_name, std::string upgrade_ident);
extern std::string GetFactionCurrentResearch(std::string civilization_name, std::string faction_name);
extern void AcquireFactionTechnologies(std::string civilization_from_name, std::string faction_from_name, std::string civilization_to_name, std::string faction_to_name);
extern bool IsMilitaryUnit(const CUnitType &type);
extern void CreateProvinceUnits(std::string province_name, int player, int divisor = 1, bool attacking_units = false, bool ignore_militia = false);
//Wyrmgus end

//@}

#endif // !__GRAND_STRATEGY_H__
