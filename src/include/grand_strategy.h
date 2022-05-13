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
//      (c) Copyright 2015-2022 by Andrettin
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

#pragma once

#include "character.h"
#include "economy/resource.h"
#include "map/map.h"
#include "player/faction_tier.h"
#include "player/government_type.h"
#include "province.h"
#include "vec2i.h"
#include "video/video.h"
#include "upgrade/upgrade_structs.h"

constexpr int FoodConsumptionPerWorker = 100;

class CGrandStrategyProvince;
class CGrandStrategyFaction;
class CGrandStrategyHero;
class LuaCallback;

namespace wyrmgus {
	class unit_class;
	class world;
	enum class character_title;
	enum class diplomacy_state;
}

class CGrandStrategyProvince final : public CProvince
{
public:
	CGrandStrategyProvince()
	{
		memset(SettlementBuildings, 0, sizeof(SettlementBuildings));
		memset(Units, 0, sizeof(Units));
		memset(Income, 0, sizeof(Income));
		memset(ProductionCapacity, 0, sizeof(ProductionCapacity));
		memset(ProductionCapacityFulfilled, 0, sizeof(ProductionCapacityFulfilled));
		memset(ProductionEfficiencyModifier, 0, sizeof(ProductionEfficiencyModifier));
	}
	
	void SetOwner(int civilization_id, int faction_id);					/// Set a new owner for the province
	void SetSettlementBuilding(int building_id, bool has_settlement_building);
	void SetModifier(CUpgrade *modifier, bool has_modifier);
	void SetUnitQuantity(int unit_type_id, int quantity);
	void ChangeUnitQuantity(int unit_type_id, int quantity);
	void SetHero(std::string hero_full_name, int value);
	void AddFactionClaim(int civilization_id, int faction_id);
	void RemoveFactionClaim(int civilization_id, int faction_id);
	bool HasBuildingClass(std::string building_class_name);
	bool HasModifier(CUpgrade *modifier);
	bool BordersModifier(CUpgrade *modifier);
	bool HasFactionClaim(int civilization_id, int faction_id);
	bool BordersProvince(CGrandStrategyProvince *province);
	bool HasSecondaryBorderThroughWaterWith(CGrandStrategyProvince *province);
	bool BordersFaction(int faction_civilization, int faction, bool check_through_water = false);
	int GetClassUnitType(const wyrmgus::unit_class *unit_class);
	int GetDesirabilityRating();
	std::string GenerateWorkName();
	CGrandStrategyHero *GetRandomAuthor();
	
	int civilization = -1;												/// Civilization of the province (-1 = no one).
	int TotalUnits = 0;													/// Total quantity of units in the province
	int TotalWorkers = 0;												/// Total quantity of workers in the province
	int FoodConsumption = 0;											/// How much food the people in the province consume
	int Labor = 0;														/// How much labor available this province has
	int MilitaryScore = 0;												/// Military score of the forces in the province (including fortifications and militia)
	int OffensiveMilitaryScore = 0;										/// Military score of the forces in the province which can attack other provinces
	int AttackingMilitaryScore = 0;										/// Military score of the forces attacking the province
	bool Movement = false;												/// Whether a unit or hero is currently moving to the province
	CGrandStrategyFaction *Owner = nullptr;								/// Owner of the province
	CGrandStrategyHero *Governor = nullptr;								/// Governor of this province
	bool SettlementBuildings[UnitTypeMax];								/// Buildings in the province; 0 = not constructed, 1 = under construction, 2 = constructed
	int Units[UnitTypeMax];												/// Quantity of units of a particular unit type in the province
	std::vector<CGrandStrategyHero *> Heroes;							/// Heroes in the province
	std::vector<CGrandStrategyHero *> ActiveHeroes;						/// Active (can move, attack and defend) heroes in the province
	std::vector<CGrandStrategyProvince *> BorderProvinces;				/// Which provinces this province borders
	int Income[MaxCosts];												/// Income for each resource.
	int ProductionCapacity[MaxCosts];									/// The province's capacity to produce each resource (1 for each unit of base output)
	int ProductionCapacityFulfilled[MaxCosts];							/// How much of the province's production capacity for each resource is actually fulfilled
	int ProductionEfficiencyModifier[MaxCosts];							/// Efficiency modifier for each resource.
	std::vector<CGrandStrategyFaction *> Claims;						/// Factions which have a claim to this province
	std::vector<Vec2i> ResourceTiles[MaxCosts];							/// Resources tiles in the province
	std::vector<CUpgrade *> Modifiers;									/// Modifiers affecting the province
};

class CGrandStrategyFaction
{
public:
	CGrandStrategyFaction()
	{
		memset(Technologies, 0, sizeof(Technologies));
		memset(Resources, 0, sizeof(Resources));
		memset(Income, 0, sizeof(Income));
		memset(ProductionEfficiencyModifier, 0, sizeof(ProductionEfficiencyModifier));
		memset(Trade, 0, sizeof(Trade));
		memset(MilitaryScoreBonus, 0, sizeof(MilitaryScoreBonus));
	}
	
	void SetTechnology(int upgrade_id, bool has_technology, bool secondary_setting = false);
	void SetCapital(CGrandStrategyProvince *province);
	void SetMinister(const wyrmgus::character_title title, std::string hero_full_name);
	void MinisterSuccession(const wyrmgus::character_title title);
	bool IsAlive();
	bool HasTechnologyClass(std::string technology_class_name);
	bool CanHaveSuccession(const wyrmgus::character_title title, bool family_inheritance);
	bool IsConquestDesirable(CGrandStrategyProvince *province);
	int GetTroopCostModifier();
	std::string get_full_name();
	CGrandStrategyProvince *GetRandomProvinceWeightedByPopulation();
	
	int Faction = -1;											/// The faction's ID (-1 = none).
	int civilization = -1;										/// Civilization of the faction (-1 = none).
	wyrmgus::government_type government_type = government_type::monarchy;	/// Government type of the faction (-1 = none).
	CGrandStrategyProvince *Capital = nullptr;					/// Capital province of this faction
	bool Technologies[UpgradeMax];								/// Whether a faction has a particular technology or not
	std::vector<int> OwnedProvinces;							/// Provinces owned by this faction
	int Resources[MaxCosts];									/// Amount of each resource stored by the faction.
	int Income[MaxCosts];										/// Income of each resource for the faction.
	int ProductionEfficiencyModifier[MaxCosts];					/// Efficiency modifier for each resource.
	int Trade[MaxCosts]; /// How much of each resource the faction wants to trade; negative values are imports and positive ones exports
	int MilitaryScoreBonus[UnitTypeMax];
	std::map<wyrmgus::character_title, CGrandStrategyHero *> Ministers;			/// Ministers of the faction
	std::vector<CGrandStrategyProvince *> Claims;				/// Provinces which this faction claims
	std::map<wyrmgus::character_title, std::vector<CGrandStrategyHero *>> HistoricalMinisters; /// All characters who had a ministerial (or head of state or government) title in this faction
	std::map<CUpgrade *, int> HistoricalTechnologies; /// historical technologies of the faction, with the year of discovery
};

class CGrandStrategyHero : public wyrmgus::character
{
public:
	CGrandStrategyHero() : wyrmgus::character(""),
		State(0), Existed(false),
		Province(nullptr), ProvinceOfOrigin(nullptr),
		Father(nullptr), Mother(nullptr)
	{
	}
	
	void Die();
	bool IsAlive();
	bool IsVisible();
	bool IsGenerated();
	bool IsEligibleForTitle(const wyrmgus::character_title title);
	int GetTroopCostModifier();
	int GetTitleScore(const wyrmgus::character_title title);
	std::string GetMinisterEffectsString(const wyrmgus::character_title title);
	std::string GetBestDisplayTitle();
	CGrandStrategyFaction *GetFaction();
	
	int State;			/// 0 = hero isn't in the province, 1 = hero is moving to the province, 2 = hero is in the province, 3 = hero is attacking the province, 4 = hero is in the province but not defending it
	bool Existed;								/// whether the character has existed in this playthrough
	CGrandStrategyProvince *Province;
	CGrandStrategyProvince *ProvinceOfOrigin;	/// Province from which the hero originates
	CGrandStrategyHero *Father;					/// Character's father
	CGrandStrategyHero *Mother;					/// Character's mother
	std::vector<CGrandStrategyHero *> Children;	/// Children of the character
	std::vector<CGrandStrategyHero *> Siblings;	/// Siblings of the character
	std::vector<std::pair<wyrmgus::character_title, CGrandStrategyFaction *>> Titles;	/// Titles of the character (first value is the title type, and the second one is the faction
	std::vector<std::pair<wyrmgus::character_title, CGrandStrategyProvince *>> ProvinceTitles;	/// Provincial titles of the character (first value is the title type, and the second one is the province
};

class CGrandStrategyEvent final
{
public:
	CGrandStrategyEvent();
	~CGrandStrategyEvent();
	
	void Trigger(CGrandStrategyFaction *faction);
	bool CanTrigger(CGrandStrategyFaction *faction);
	
	std::string Name;
	std::string Description;
	bool Persistent = false;
	int ID = -1;
	int MinYear = 0;
	int MaxYear = 0;
	int HistoricalYear = 0;
	wyrmgus::world *World = nullptr;
	std::unique_ptr<LuaCallback> Conditions;
	std::vector<std::string> Options;
	std::vector<std::unique_ptr<LuaCallback>> OptionConditions;
	std::vector<std::unique_ptr<LuaCallback>> OptionEffects;
	std::vector<std::string> OptionTooltips;
};

/**
**  Grand Strategy game instance
**  Mapped with #GrandStrategy to a symbolic name.
*/
class CGrandStrategyGame
{
public:
	CGrandStrategyGame() : 
		WorldMapWidth(0), WorldMapHeight(0),
		PlayerFaction(nullptr)
	{
		memset(CommodityPrices, 0, sizeof(CommodityPrices));
	}

	void DrawInterface(std::vector<std::function<void(renderer *)>> &render_commands);					/// Draw the interface
	void DoTurn();							/// Process the grand strategy turn
	void PerformTrade(CGrandStrategyFaction &importer_faction, CGrandStrategyFaction &exporter_faction, int resource);
	void CreateWork(CUpgrade *work, CGrandStrategyHero *author, CGrandStrategyProvince *province);
	bool TradePriority(CGrandStrategyFaction &faction_a, CGrandStrategyFaction &faction_b);
	CGrandStrategyHero *GetHero(std::string hero_full_name);

public:
	int WorldMapWidth;
	int WorldMapHeight;
	std::vector<CGrandStrategyProvince *> Provinces;
	std::map<int, std::vector<CGrandStrategyProvince *>> CultureProvinces;	/// provinces belonging to each culture
	std::vector<CGrandStrategyFaction *> Factions[MAX_RACES];
	std::vector<CGrandStrategyHero *> Heroes;
	std::vector<CUpgrade *> UnpublishedWorks;
	std::vector<CGrandStrategyEvent *> AvailableEvents;
	CGrandStrategyFaction *PlayerFaction;
	int CommodityPrices[MaxCosts];								/// price for every 100 of each commodity
};

extern bool GrandStrategy;								/// if the game is in grand strategy mode
extern int GrandStrategyYear;
extern CGrandStrategyGame GrandStrategyGame;			/// Grand strategy game
extern std::map<std::string, int> GrandStrategyHeroStringToIndex;
extern std::vector<std::unique_ptr<CGrandStrategyEvent>> GrandStrategyEvents;
extern std::map<std::string, CGrandStrategyEvent *> GrandStrategyEventStringToPointer;

extern int GetProvinceId(std::string province_name);
extern void SetProvinceOwner(std::string province_name, std::string civilization_name, std::string faction_name);
extern void SetProvinceSettlementBuilding(std::string province_name, std::string settlement_building_ident, bool has_settlement_building);
extern void SetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void ChangeProvinceUnitQuantity(std::string province_name, std::string unit_type_ident, int quantity);
extern void SetProvinceHero(std::string province_name, std::string hero_full_name, int value);
extern void AddProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name);
extern void RemoveProvinceClaim(std::string province_name, std::string civilization_name, std::string faction_name);
extern void InitializeGrandStrategyGame();
extern void FinalizeGrandStrategyInitialization();
extern void DoGrandStrategyTurn();
extern bool ProvinceBordersProvince(std::string province_name, std::string second_province_name);
extern bool ProvinceBordersFaction(std::string province_name, std::string faction_civilization_name, std::string faction_name);
extern bool ProvinceHasBuildingClass(std::string province_name, std::string building_class);
extern std::string GetProvinceCivilization(std::string province_name);
extern bool GetProvinceSettlementBuilding(std::string province_name, std::string building_ident);
extern int GetProvinceUnitQuantity(std::string province_name, std::string unit_type_ident);
extern int GetProvinceHero(std::string province_name, std::string hero_full_name);
extern int GetProvinceMilitaryScore(std::string province_name, bool attacker, bool count_defenders);
extern std::string GetProvinceOwner(std::string province_name);
extern void SetFactionGovernmentType(std::string civilization_name, std::string faction_name, std::string government_type_name);
extern void SetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name, std::string diplomacy_state_name);
extern std::string GetFactionDiplomacyStateProposal(std::string civilization_name, std::string faction_name, std::string second_civilization_name, std::string second_faction_name);
extern bool IsGrandStrategyUnit(const wyrmgus::unit_type &type);
extern bool IsMilitaryUnit(const wyrmgus::unit_type &type);
extern void SetFactionMinister(std::string civilization_name, std::string faction_name, std::string title_name, std::string hero_full_name);
extern std::string GetFactionMinister(std::string civilization_name, std::string faction_name, std::string title_name);
extern void KillGrandStrategyHero(std::string hero_full_name);
extern void GrandStrategyHeroExisted(std::string hero_full_name);
extern bool GrandStrategyHeroIsAlive(std::string hero_full_name);
extern void GrandStrategyWorkCreated(std::string work_ident);
extern void MakeGrandStrategyEventAvailable(std::string event_name);
extern bool GetGrandStrategyEventTriggered(std::string event_name);
extern void CleanGrandStrategyEvents();
extern CGrandStrategyEvent *GetGrandStrategyEvent(std::string event_name);
extern void GrandStrategyCclRegister();
