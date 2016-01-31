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

#include <vector>

#include "character.h"
#include "vec2i.h"
#include "video.h"
#include "player.h"
#include "upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

#define BasePopulationGrowthPermyriad 12					/// Base population growth per 10,000

class CGrandStrategyFaction;
class CGrandStrategyHero;

/**
**  Indexes into pathway array.
*/
enum Pathways {
	PathwayTrail,
	PathwayRoad,
	
	MaxPathways
};

/**
**  Indexes into diplomacy state array.
*/
enum DiplomacyStates {
	DiplomacyStatePeace,
	DiplomacyStateWar,
	DiplomacyStateAlliance,
	DiplomacyStateVassal,
	DiplomacyStateSovereign,
	
	MaxDiplomacyStates
};

class WorldMapTerrainType
{
public:
	WorldMapTerrainType() :
		Name(""), Tag(""), HasTransitions(false), Water(false), BaseTile(-1), Variations(0)
	{
	}

	std::string Name;
	std::string Tag;				/// used to locate graphic files
	bool HasTransitions;
	bool Water;
	int BaseTile;
	int Variations;					/// quantity of variations
};

class WorldMapTile
{
public:
	WorldMapTile() :
		Terrain(-1), Province(-1), BaseTileVariation(-1), Variation(-1), Resource(-1),
		ResourceProspected(false), Port(false), Worked(false),
		Position(-1, -1),
		BaseTile(NULL), GraphicTile(NULL), ResourceBuildingGraphics(NULL), ResourceBuildingGraphicsPlayerColor(NULL)
	{
		memset(Borders, 0, sizeof(Borders));
		memset(River, -1, sizeof(River));
		memset(Riverhead, -1, sizeof(Riverhead));
		memset(Pathway, -1, sizeof(Pathway));
	}

	void UpdateMinimap();
	void SetResourceProspected(int resource_id, bool discovered);
	void SetPort(bool has_port);
	bool IsWater();
	bool HasResource(int resource, bool ignore_prospection = false);	/// Get whether the tile has a resource
	std::string GetCulturalName();										/// Get the tile's cultural name.
	
	int Terrain;							/// Tile terrain (i.e. plains)
	int Province;							/// Province to which the tile belongs
	int BaseTileVariation;					/// Base tile variation
	int Variation;							/// Tile variation
	int Resource;							/// The tile's resource, if any
	bool ResourceProspected;				/// Whether the tile's resource has been discovered
	bool Port;								/// Whether the tile has a port
	bool Worked;							/// Whether the tile is worked by a worker
	std::string Name;						/// Name of the tile (used for instance to name particular mountains)
	Vec2i Position;							/// Position of the tile
	CGraphic *BaseTile;
	CGraphic *GraphicTile;					/// The tile image used by this tile
	CGraphic *ResourceBuildingGraphics;
	CPlayerColorGraphic *ResourceBuildingGraphicsPlayerColor;
	bool Borders[MaxDirections];			/// Whether this tile borders a tile of another province to a particular direction
	int River[MaxDirections];				/// Whether this tile has a river to a particular direction (the value for each direction is the ID of the river)
	int Riverhead[MaxDirections];			/// Whether this tile has a riverhead to a particular direction (the value for each direction is the ID of the river)
	int Pathway[MaxDirections];				/// Whether this tile has a pathway (trail or road) to a particular direction
	std::string CulturalNames[MAX_RACES];	/// Names for the tile for each different culture/civilization
};

class CProvince
{
public:
	CProvince() :
		ID(-1), Civilization(-1), ReferenceProvince(-1), CurrentConstruction(-1),
		TotalUnits(0), TotalWorkers(0), PopulationGrowthProgress(0), FoodConsumption(0), Labor(0),
		MilitaryScore(0), OffensiveMilitaryScore(0), AttackingMilitaryScore(0),
		Water(false), Coastal(false), Movement(false), SettlementLocation(-1, -1),
		Owner(NULL), AttackedBy(NULL)
	{
		memset(SettlementBuildings, 0, sizeof(SettlementBuildings));
		memset(Units, 0, sizeof(Units));
		memset(UnderConstructionUnits, 0, sizeof(UnderConstructionUnits));
		memset(MovingUnits, 0, sizeof(MovingUnits));
		memset(AttackingUnits, 0, sizeof(AttackingUnits));
		memset(BorderProvinces, 0, sizeof(BorderProvinces));
		memset(Income, 0, sizeof(Income));
		memset(ProductionCapacity, 0, sizeof(ProductionCapacity));
		memset(ProductionCapacityFulfilled, 0, sizeof(ProductionCapacityFulfilled));
		memset(ProductionEfficiencyModifier, 0, sizeof(ProductionEfficiencyModifier));
	}
	
	void UpdateMinimap();
	void SetOwner(int civilization_id, int faction_id);					/// Set a new owner for the province
	void SetCivilization(int civilization);
	void SetSettlementBuilding(int building_id, bool has_settlement_building);
	void SetUnitQuantity(int unit_type_id, int quantity);
	void ChangeUnitQuantity(int unit_type_id, int quantity);
	void SetAttackingUnitQuantity(int unit_type_id, int quantity);
	void ChangeAttackingUnitQuantity(int unit_type_id, int quantity);
	void SetHero(std::string hero_full_name, int value);
	void AllocateLabor();
	void AllocateLaborToResource(int resource);
	void DeallocateLabor();
	void ReallocateLabor();
	void CalculateIncome(int resource);
	void CalculateIncomes();
	void AddFactionClaim(int civilization_id, int faction_id);
	void RemoveFactionClaim(int civilization_id, int faction_id);
	bool HasBuildingClass(std::string building_class_name);
	bool HasFactionClaim(int civilization_id, int faction_id);
	bool HasResource(int resource, bool ignore_prospection = false);
	bool BordersProvince(int province_id);
	bool BordersFaction(int faction_civilization, int faction);
	int GetPopulation();
	int GetResourceDemand(int resource);
	int GetAdministrativeEfficiencyModifier();
	int GetRevoltRisk();
	int GetClassUnitType(int class_id);
	int GetFoodCapacity(bool subtract_non_food);
	std::string GetCulturalName();										/// Get the province's cultural name.
	std::string GetCulturalSettlementName();							/// Get the province's cultural settlement name.
	std::string GenerateProvinceName(int civilization, int faction = -1);
	std::string GenerateSettlementName(int civilization, int faction = -1);
	
	std::string Name;
	std::string SettlementName;
	int ID;																/// ID of this province
	int Civilization;													/// Civilization of the province (-1 = no one).
	int ReferenceProvince;												/// Reference province, if a water province (used for name changing) (-1 = none).
	int CurrentConstruction;											/// Building currently under construction (unit type index).
	int TotalUnits;														/// Total quantity of units in the province
	int TotalWorkers;													/// Total quantity of workers in the province
	int PopulationGrowthProgress;										/// Progress of current population growth; when reaching the population growth threshold a new worker unit will be created
	int FoodConsumption;												/// How much food the people in the province consume
	int Labor;															/// How much labor available this province has
	int MilitaryScore;													/// Military score of the forces in the province (including fortifications and militia)
	int OffensiveMilitaryScore;											/// Military score of the forces in the province which can attack other provinces
	int AttackingMilitaryScore;											/// Military score of the forces attacking the province
	bool Water;															/// Whether the province is a water province or not
	bool Coastal;														/// Whether the province is a coastal province or not
	bool Movement;														/// Whether a unit or hero is currently moving to the province
	Vec2i SettlementLocation;											/// In which tile the province's settlement is located
	CGrandStrategyFaction *Owner;										/// Owner of the province
	CGrandStrategyFaction *AttackedBy;									/// Which faction the province is being attacked by.
	bool SettlementBuildings[UnitTypeMax];								/// Buildings in the province; 0 = not constructed, 1 = under construction, 2 = constructed
	int Units[UnitTypeMax];												/// Quantity of units of a particular unit type in the province
	int UnderConstructionUnits[UnitTypeMax];							/// Quantity of units of a particular unit type being trained/constructed in the province
	int MovingUnits[UnitTypeMax];										/// Quantity of units of a particular unit type moving to the province
	int AttackingUnits[UnitTypeMax];									/// Quantity of units of a particular unit type attacking the province
	std::vector<CGrandStrategyHero *> Heroes;							/// Heroes in the province
	int BorderProvinces[ProvinceMax];									/// Which provinces this province borders
	int Income[MaxCosts];												/// Income for each resource.
	int ProductionCapacity[MaxCosts];									/// The province's capacity to produce each resource (1 for each unit of base output)
	int ProductionCapacityFulfilled[MaxCosts];							/// How much of the province's production capacity for each resource is actually fulfilled
	int ProductionEfficiencyModifier[MaxCosts];							/// Efficiency modifier for each resource.
	std::vector<CGrandStrategyFaction *> Claims;						/// Factions which claim this province
	std::string CulturalNames[MAX_RACES];								/// Names for the province for each different culture/civilization
	std::string FactionCulturalNames[MAX_RACES][FactionMax];			/// Names for the province for each different faction
	std::string CulturalSettlementNames[MAX_RACES];						/// Names for the province's settlement for each different culture/civilization
	std::string FactionCulturalSettlementNames[MAX_RACES][FactionMax];	/// Names for the province's settlement for each different faction
	std::vector<Vec2i> Tiles;
	std::vector<Vec2i> ResourceTiles[MaxCosts];							///resources tiles in the province
};

class CGrandStrategyFaction
{
public:
	CGrandStrategyFaction() :
		Faction(-1), Civilization(-1), FactionTier(FactionTierBarony), CurrentResearch(-1), ProvinceCount(0), Upkeep(0),
		Ruler(NULL)
	{
		memset(Technologies, 0, sizeof(Technologies));
		memset(OwnedProvinces, -1, sizeof(OwnedProvinces));
		memset(Resources, 0, sizeof(Resources));
		memset(Income, 0, sizeof(Income));
		memset(ProductionEfficiencyModifier, 0, sizeof(ProductionEfficiencyModifier));
		memset(Trade, 0, sizeof(Trade));
		memset(MilitaryScoreBonus, 0, sizeof(MilitaryScoreBonus));
		for (int i = 0; i < MAX_RACES; ++i) {
			for (int j = 0; j < FactionMax; ++j) {
				DiplomacyState[i][j] = DiplomacyStatePeace;
				DiplomacyStateProposal[i][j] = -1;
			}
		}
	}
	
	void SetTechnology(int upgrade_id, bool has_technology, bool secondary_setting = false);
	void CalculateIncome(int resource);
	void CalculateIncomes();
	void CalculateUpkeep();
	void CheckFormableFactions(int civilization);
	void FormFaction(int civilization, int faction);
	void AcquireFactionTechnologies(int civilization, int faction);
	void SetRuler(std::string hero_full_name);
	void RulerSuccession();
	void GenerateRuler();
	bool IsAlive();
	bool HasTechnologyClass(std::string technology_class_name);
	bool CanFormFaction(int civilization, int faction);
	std::string GetFullName();
	std::string GetTitle();
	std::string GetRulerTitle();
	
	int Faction;														/// The faction's ID (-1 = none).
	int Civilization;													/// Civilization of the faction (-1 = none).
	int GovernmentType;													/// Government type of the faction (-1 = none).
	int FactionTier;													/// What is the tier of this faction (barony, etc.).
	int CurrentResearch;												/// Currently researched technology (upgrade index).
	int ProvinceCount;													/// Quantity of provinces owned by this faction.
	int Upkeep;															/// How much gold this faction has to pay per turn
	CGrandStrategyHero *Ruler;											/// Ruler of the faction
	bool Technologies[UpgradeMax];										/// Whether a faction has a particular technology or not
	int OwnedProvinces[ProvinceMax];									/// Provinces owned by this faction
	int Resources[MaxCosts];											/// Amount of each resource stored by the faction.
	int Income[MaxCosts];												/// Income of each resource for the faction.
	int ProductionEfficiencyModifier[MaxCosts];							/// Efficiency modifier for each resource.
	int Trade[MaxCosts];												/// How much of each resource the faction wants to trade; negative values are imports and positive ones exports
	int MilitaryScoreBonus[UnitTypeMax];
	int DiplomacyState[MAX_RACES][FactionMax];							/// Diplomacy state between this faction and each other faction
	int DiplomacyStateProposal[MAX_RACES][FactionMax];					/// Diplomacy state being offered by this faction to each other faction
	std::vector<CProvince *> Claims;									/// Provinces which this faction claims
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

class CGrandStrategyHero : public CCharacter
{
public:
	CGrandStrategyHero() : CCharacter(),
		State(0),
		Province(NULL), ProvinceOfOrigin(NULL),
		Father(NULL), Mother(NULL)
	{
	}
	
	void Initialize();
	void Create();
	void Die();
	void SetType(int unit_type_id);
	int GetAdministrativeEfficiencyModifier();
	std::string GetRulerEffectsString();
	
	int State;			/// 0 = hero isn't in the province, 1 = hero is moving to the province, 2 = hero is in the province, 3 = hero is attacking the province
	CProvince *Province;
	CProvince *ProvinceOfOrigin;	/// Province from which the hero originates
	CGrandStrategyHero *Father;			/// Character's father
	CGrandStrategyHero *Mother;					/// Character's mother
	std::vector<CGrandStrategyHero *> Children;	/// Children of the character
	std::vector<CGrandStrategyHero *> Siblings;	/// Siblings of the character
};

/**
**  Grand Strategy game instance
**  Mapped with #GrandStrategy to a symbolic name.
*/
class CGrandStrategyGame
{
public:
	CGrandStrategyGame() : 
		WorldMapWidth(0), WorldMapHeight(0), ProvinceCount(0), SelectedProvince(-1),
		FogTile(NULL), SymbolMove(NULL), SymbolAttack(NULL), SymbolHero(NULL), SymbolResourceNotWorked(NULL),
		PlayerFaction(NULL)
	{
		for (int i = 0; i < MaxCosts; ++i) {
			for (int j = 0; j < WorldMapResourceMax; ++j) {
				WorldMapResources[i][j].x = -1;
				WorldMapResources[i][j].y = -1;
			}
		}
		for (int i = 0; i < MaxDirections; ++i) {
			for (int j = 0; j < 2; ++j) {
				RivermouthGraphics[i][j] = NULL;
				RiverheadGraphics[i][j] = NULL;
			}
		}
		for (int i = 0; i < MaxPathways; ++i) {
			for (int j = 0; j < MaxDirections; ++j) {
				PathwayGraphics[i][j] = NULL;
			}
		}
		for (int x = 0; x < WorldMapWidthMax; ++x) {
			for (int y = 0; y < WorldMapHeightMax; ++y) {
				WorldMapTiles[x][y] = NULL;
			}
		}
		for (int i = 0; i < MAX_RACES; ++i) {
			for (int j = 0; j < FactionMax; ++j) {
				Factions[i][j] = NULL;
			}
		}
		memset(BorderGraphics, 0, sizeof(BorderGraphics));
		memset(SettlementGraphics, 0, sizeof(SettlementGraphics));
		memset(BarracksGraphics, 0, sizeof(BarracksGraphics));
		memset(SettlementMasonryGraphics, 0, sizeof(SettlementMasonryGraphics));
		memset(NationalBorderGraphics, 0, sizeof(NationalBorderGraphics));
		memset(TerrainTypes, 0, sizeof(TerrainTypes));
		memset(Provinces, 0, sizeof(Provinces));
		memset(Rivers, 0, sizeof(Rivers));
		memset(CommodityPrices, 0, sizeof(CommodityPrices));
	}

	void Clean();
	void DrawMap();							/// Draw the map area
	void DrawMinimap();						/// Draw the minimap
	void DrawInterface();					/// Draw the interface
	void DrawTileTooltip(int x, int y);		/// Draw the tooltip for a tile
	void DoTurn();							/// Process the grand strategy turn
	void DoTrade();							/// Process trade deals
	void DoProspection();					/// Process prospection for the turn
	void PerformTrade(CGrandStrategyFaction &importer_faction, CGrandStrategyFaction &exporter_faction, int resource);
	#if defined(USE_OPENGL) || defined(USE_GLES)
	void CreateMinimapTexture();
	#endif
	void UpdateMinimap();
	bool IsPointOnMap(int x, int y);
	bool TradePriority(CGrandStrategyFaction &faction_a, CGrandStrategyFaction &faction_b);
	Vec2i GetTileUnderCursor();
	CGrandStrategyHero *GetHero(std::string hero_full_name);

public:
	int WorldMapWidth;
	int WorldMapHeight;
	int ProvinceCount;
	int SelectedProvince;
	CGraphic *FogTile;
	CGraphic *SymbolMove;										///symbol that units are moving to the province (drawn at the settlement location)
	CGraphic *SymbolAttack;										///symbol that a province is being attacked (drawn at the settlement location)
	CGraphic *SymbolHero;										///symbol that a hero is present in the province (drawn at the settlement location)
	CGraphic *SymbolResourceNotWorked;							///symbol that a resource is not being worked
	CGraphic *BorderGraphics[MaxDirections];					///one for each direction
	CGraphic *RiverGraphics[MaxDirections];
	CGraphic *RivermouthGraphics[MaxDirections][2];				///the two values are whether it is flipped or not
	CGraphic *RiverheadGraphics[MaxDirections][2];				///the two values are whether it is flipped or not
	CGraphic *PathwayGraphics[MaxPathways][MaxDirections];
	CPlayerColorGraphic *SettlementGraphics[MAX_RACES];
	CPlayerColorGraphic *BarracksGraphics[MAX_RACES];
	CPlayerColorGraphic *SettlementMasonryGraphics[MAX_RACES];
	CPlayerColorGraphic *NationalBorderGraphics[MaxDirections];	///one for each direction
	WorldMapTerrainType *TerrainTypes[WorldMapTerrainTypeMax];
	WorldMapTile *WorldMapTiles[WorldMapWidthMax][WorldMapHeightMax];
	CProvince *Provinces[ProvinceMax];
	CGrandStrategyFaction *Factions[MAX_RACES][FactionMax];
	CRiver *Rivers[RiverMax];
	std::vector<CGrandStrategyHero *> Heroes;
	CGrandStrategyFaction *PlayerFaction;
	Vec2i WorldMapResources[MaxCosts][WorldMapResourceMax];	///resources on the map; three values: the resource's x position, its y position, and whether it is discovered or not
	int CommodityPrices[MaxCosts];								///price for every 100 of each commodity

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

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern bool GrandStrategy;								/// if the game is in grand strategy mode
extern bool GrandStrategyGamePaused;					/// if the grand strategy game is paused
extern bool GrandStrategyGameInitialized;				/// if the grand strategy game has been initialized
extern int GrandStrategyYear;
extern std::string GrandStrategyWorld;
extern int WorldMapOffsetX;
extern int WorldMapOffsetY;
extern int GrandStrategyMapWidthIndent;
extern int GrandStrategyMapHeightIndent;
extern int BattalionMultiplier;
extern int PopulationGrowthThreshold;					/// How much population growth progress must be accumulated before a new worker unit is created in the province
extern std::string GrandStrategyInterfaceState;
extern CGrandStrategyGame GrandStrategyGame;			/// Grand strategy game
extern std::map<std::string, int> GrandStrategyHeroStringToIndex;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern std::string GetDiplomacyStateNameById(int diplomacy_state);
extern int GetDiplomacyStateIdByName(std::string diplomacy_state);
extern std::string GetFactionTierNameById(int faction_tier);
extern int GetFactionTierIdByName(std::string faction_tier);
extern int GetWorldMapWidth();
extern int GetWorldMapHeight();
extern std::string GetWorldMapTileTerrain(int x, int y);
extern int GetWorldMapTileTerrainVariation(int x, int y);
extern std::string GetWorldMapTileProvinceName(int x, int y);
extern bool WorldMapTileHasResource(int x, int y, std::string resource_name, bool ignore_prospection);
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
extern void SetRiverCulturalName(std::string river_name, std::string civilization_name, std::string cultural_name);
extern void SetWorldMapTilePathway(int x, int y, std::string direction_name, std::string pathway_name);
extern void SetWorldMapTilePort(int x, int y, bool has_port);
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
extern void SetProvincePopulation(std::string province_name, int quantity);
extern void SetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void ChangeProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceHero(std::string province_name, std::string hero_full_name, int value);
extern void SetProvinceFood(std::string province_name, int quantity);
extern void ChangeProvinceFood(std::string province_name, int quantity);
extern void SetProvinceAttackedBy(std::string province_name, std::string civilization_name, std::string faction_name);
extern void SetSelectedProvince(std::string province_name);
extern void AddProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name);
extern void RemoveProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name);
extern void UpdateProvinceMinimap(std::string province_name);
extern void CleanGrandStrategyGame();
extern void InitializeGrandStrategyGame(bool show_loading = true);
extern void InitializeGrandStrategyMinimap();
extern void InitializeGrandStrategyFactions();
extern void SetGrandStrategyWorld(std::string world);
extern void DoGrandStrategyTurn();
extern void DoProspection();
extern void CalculateProvinceBorders();
extern void CenterGrandStrategyMapOnTile(int x, int y);
extern bool ProvinceBordersProvince(std::string province_name, std::string second_province_name);
extern bool ProvinceBordersFaction(std::string province_name, std::string faction_civilization_name, std::string faction_name);
extern bool ProvinceHasBuildingClass(std::string province_name, std::string building_class);
extern bool ProvinceHasClaim(std::string province_name, std::string faction_civilization_name, std::string faction_name);
extern bool ProvinceHasResource(std::string province_name, std::string resource_name, bool ignore_prospection);
extern bool IsGrandStrategyBuilding(const CUnitType &type);
extern std::string GetProvinceCivilization(std::string province_name);
extern bool GetProvinceSettlementBuilding(std::string province_name, std::string building_ident);
extern std::string GetProvinceCurrentConstruction(std::string province_name);
extern int GetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceUnderConstructionUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceMovingUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceAttackingUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceHero(std::string province_name, std::string hero_full_name);
extern int GetProvinceLabor(std::string province_name);
extern int GetProvinceAvailableWorkersForTraining(std::string province_name);
extern int GetProvinceTotalWorkers(std::string province_name);
extern int GetProvinceMilitaryScore(std::string province_name, bool attacker, bool count_defenders);
extern std::string GetProvinceOwner(std::string province_name);
extern bool GetProvinceWater(std::string province_name);
extern int GetProvinceFoodCapacity(std::string province_name, bool subtract_non_food = false);
extern void SetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident, bool has_technology);
extern bool GetFactionTechnology(std::string civilization_name, std::string faction_name, std::string upgrade_ident);
extern void SetFactionGovernmentType(std::string civilization_name, std::string faction_name, std::string government_type_name);
extern void SetFactionDiplomacyState(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name);
extern std::string GetFactionDiplomacyState(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name);
extern void SetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name);
extern std::string GetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name);
extern void SetFactionTier(std::string civilization_name, std::string faction_name, std::string faction_tier_name);
extern std::string GetFactionTier(std::string civilization_name, std::string faction_name);
extern void SetFactionCurrentResearch(std::string civilization_name, std::string faction_name, std::string upgrade_ident);
extern std::string GetFactionCurrentResearch(std::string civilization_name, std::string faction_name);
extern std::string GetFactionFullName(std::string civilization_name, std::string faction_name);
extern void AcquireFactionTechnologies(std::string civilization_from_name, std::string faction_from_name, std::string civilization_to_name, std::string faction_to_name);
extern void SetPlayerFaction(std::string civilization_name, std::string faction_name);
extern std::string GetPlayerFactionName();
extern void SetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity);
extern void ChangeFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name, int resource_quantity);
extern int GetFactionResource(std::string civilization_name, std::string faction_name, std::string resource_name);
extern void CalculateFactionIncomes(std::string civilization_name, std::string faction_name);
extern void CalculateFactionUpkeeps();
extern int GetFactionIncome(std::string civilization_name, std::string faction_name, std::string resource_name);
extern int GetFactionUpkeep(std::string civilization_name, std::string faction_name);
extern bool IsGrandStrategyUnit(const CUnitType &type);
extern bool IsMilitaryUnit(const CUnitType &type);
extern void CreateProvinceUnits(std::string province_name, int player, int divisor = 1, bool attacking_units = false, bool ignore_militia = false);
extern void FormFaction(std::string old_civilization_name, std::string old_faction_name, std::string new_civilization_name, std::string new_faction_name);
extern void SetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity);
extern void ChangeFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name, int quantity);
extern int GetFactionCommodityTrade(std::string civilization_name, std::string faction_name, std::string resource_name);
extern bool FactionHasHero(std::string civilization_name, std::string faction_name, std::string hero_full_name);
extern void SetFactionRuler(std::string civilization_name, std::string faction_name, std::string hero_full_name);
extern std::string GetFactionRuler(std::string civilization_name, std::string faction_name);
extern void CreateGrandStrategyHero(std::string hero_full_name);
extern void CreateGrandStrategyCustomHero(std::string hero_full_name);
extern void KillGrandStrategyHero(std::string hero_full_name);
extern void SetGrandStrategyHeroUnitType(std::string hero_full_name, std::string unit_type_ident);
extern std::string GetGrandStrategyHeroUnitType(std::string hero_full_name);
extern bool GrandStrategyHeroIsAlive(std::string hero_full_name);
extern bool GrandStrategyHeroIsCustom(std::string hero_full_name);
extern void SetCommodityPrice(std::string resource_name, int price);
extern int GetCommodityPrice(std::string resource_name);
extern void SetResourceBasePrice(std::string resource_name, int price);
extern void SetResourceBaseLaborInput(std::string resource_name, int input);
extern void SetResourceBaseOutput(std::string resource_name, int output);
extern void SetResourceGrandStrategyBuildingVariations(std::string resource_name, int variation_quantity);
extern void SetResourceGrandStrategyBuildingTerrainSpecificGraphic(std::string resource_name, std::string terrain_type_name, bool has_terrain_specific_graphic);

//@}

#endif // !__GRAND_STRATEGY_H__
