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
/**@name player.h - The player header file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#ifndef __PLAYER_H__
#define __PLAYER_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

//Wyrmgus start
#include "time/date.h"
//Wyrmgus end
#include "ui/icon_config.h"
#include "upgrade/upgrade_structs.h"
#include "vec2i.h"
//Wyrmgus start
#include "video/color.h"
//Wyrmgus end

#include <core/object.h>

#include <map>
#include <shared_mutex>
#include <string>
#include <tuple>

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

constexpr int STORE_OVERALL = 0;
constexpr int STORE_BUILDING = 1;
constexpr int STORE_BOTH = 2;

constexpr int SPEEDUP_FACTOR = 100;

constexpr int DEFAULT_TRADE_COST = 30;

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAge;
class CCalendar;
class CCharacter;
class CCivilization;
class Currency;
class CDeity;
class CDeityDomain;
class CDynasty;
class CFaction;
class CFile;
class CGender;
class CGraphic;
class CPlane;
class CPlayerColor;
class CPlayerQuestObjective;
class CQuest;
class CReligion;
class CSite;
class CUnit;
class CUnitType;
class PlayerAi;
//Wyrmgus start
class CFiller;
class LuaCallback;
//Wyrmgus end
struct lua_State;

/*----------------------------------------------------------------------------
--  Player type
----------------------------------------------------------------------------*/

enum _diplomacy_ {
	DiplomacyAllied,   /// Ally with opponent
	DiplomacyNeutral,  /// Don't attack be neutral
	DiplomacyEnemy,    /// Attack opponent
	//Wyrmgus start
	DiplomacyOverlord,	/// Become overlord to other player
	DiplomacyVassal,	/// Become vassal to other player
	//Wyrmgus end
	DiplomacyCrazy     /// Ally and attack opponent
}; /// Diplomacy states for CommandDiplomacy

///  Player structure
class CPlayer : public Object
{
	GDCLASS(CPlayer, Object)
	
public:
	static void SetThisPlayer(CPlayer *player);
	static CPlayer *GetThisPlayer();
	static CPlayer *GetPlayer(const int index);
	
	/// create a new player
	static void Create(const int type);
	
	/// init players
	static void InitPlayers();

	static CPlayer *GetFactionPlayer(const CFaction *faction);
	static CPlayer *GetOrAddFactionPlayer(const CFaction *faction);

	static std::vector<CPlayer *> Players;	/// All players

private:
	static CPlayer *ThisPlayer;		/// Player on local computer
	static std::shared_mutex PlayerMutex;	/// Mutex for players as a whole
	
public:
	int GetIndex() const
	{
		return this->Index;
	}

	void SetCivilization(int civilization);
	CCivilization *GetCivilization() const;
	void SetFaction(const CFaction *faction);
	void SetRandomFaction();
	
	const CFaction *GetFaction() const
	{
		return this->Faction;
	}

	const CPlayerColor *GetPrimaryColor() const
	{
		return this->PrimaryColor;
	}

	const CPlayerColor *GetSecondaryColor() const
	{
		return this->SecondaryColor;
	}

	void SetDynasty(CDynasty *dynasty);
	String GetInterface() const;
	
private:
	int Index = 0;		/// player as number
public:
	std::string Name;   /// name of non computer

	int Type = 0;		/// type of player (human,computer,...)
	int Race = 0;		/// race of player (orc,human,...)
private:
	const CFaction *Faction = nullptr;	/// the player's faction
public:
	CReligion *Religion = nullptr;	/// religion of the player
	CDynasty *Dynasty = nullptr;	/// ruling dynasty of the player
	CAge *Age = nullptr;			/// The current age the player/faction is in
	std::string AiName; /// AI for computer

	// friend enemy detection
	int Team = 0;		/// team of player

	Vec2i StartPos = Vec2i(0, 0);	/// map tile start position
	//Wyrmgus start
	int StartMapLayer = 0;			/// map tile start map layer
	
	CPlayer *Overlord = nullptr;	/// overlord of this player
	std::vector<CPlayer *> Vassals;	/// vassals of this player
	//Wyrmgus end

	//Wyrmgus start
//	inline void SetStartView(const Vec2i &pos) { StartPos = pos; }
	inline void SetStartView(const Vec2i &pos, int z) { StartPos = pos; StartMapLayer = z; }
	//Wyrmgus end

	int Resources[MaxCosts];      /// resources in overall store
	int MaxResources[MaxCosts];   /// max resources can be stored
	int StoredResources[MaxCosts];/// resources in store buildings (can't exceed MaxResources)
	int LastResources[MaxCosts];  /// last values for revenue
	int Incomes[MaxCosts];        /// income of the resources
	int Revenue[MaxCosts];        /// income rate of the resources
	//Wyrmgus start
	int Prices[MaxCosts];		  /// price of each resource
	int ResourceDemand[MaxCosts]; /// demand for the resources
	int StoredResourceDemand[MaxCosts]; /// stored demand for the resources (converted into a trade action when reaches 100)
	
	int TradeCost;					/// cost of trading
	//Wyrmgus end

	int SpeedResourcesHarvest[MaxCosts]; /// speed factor for harvesting resources
	int SpeedResourcesReturn[MaxCosts];  /// speed factor for returning resources
	int SpeedBuild;                  /// speed factor for building
	int SpeedTrain;                  /// speed factor for training
	int SpeedUpgrade;                /// speed factor for upgrading
	int SpeedResearch;               /// speed factor for researching

	std::map<const CUnitType *, int> UnitTypesCount;  						/// total units of unit-type
	std::map<const CUnitType *, int> UnitTypesUnderConstructionCount;  		/// total under construction units of unit-type
	std::map<const CUnitType *, int> UnitTypesAiActiveCount;  				/// total units of unit-type that have their AI set to active
	std::map<const CUnitType *, std::vector<CUnit *>> UnitsByType;			/// units owned by this player for each type
	std::map<const CUnitType *, std::vector<CUnit *>> AiActiveUnitsByType;	/// AI active units owned by this player for each type
	std::vector<CUnit *> Heroes;											/// hero units owned by this player
	std::vector<CDeity *> Deities;											/// deities chosen by this player
	std::vector<CQuest *> AvailableQuests;									/// quests available to this player
	std::vector<CQuest *> CurrentQuests;									/// quests being pursued by this player
	std::vector<CQuest *> CompletedQuests;									/// quests completed by this player
	std::vector<CPlayerQuestObjective *> QuestObjectives;					/// Objectives of the player's current quests
	std::vector<std::pair<CUpgrade *, int>> Modifiers;						/// Modifiers affecting the player, and until which cycle it should last
	std::vector<int> AutosellResources;
	//Wyrmgus end

	bool AiEnabled = false;	/// handle AI on local computer
	//Wyrmgus start
	bool Revealed = false;	/// Whether the player has been revealed (i.e. after losing the last town hall)
	//Wyrmgus end
	PlayerAi *Ai = nullptr;	/// Ai structure pointer

	int NumBuildings = 0;	/// # buildings
	//Wyrmgus start
	int NumBuildingsUnderConstruction = 0;	/// # buildings under construction
	int NumTownHalls = 0;
	//Wyrmgus end
	int Supply = 0;		/// supply available/produced
	int Demand = 0;		/// demand of player

	int UnitLimit;		/// # food units allowed
	int BuildingLimit;	/// # buildings allowed
	int TotalUnitLimit;	/// # total unit number allowed

	int Score = 0;		/// Player score points
	int TotalUnits = 0;
	int TotalBuildings = 0;
	int TotalResources[MaxCosts];
	int TotalRazings = 0;
	int TotalKills = 0;	/// How many units killed
	//Wyrmgus start
	int UnitTypeKills[UnitTypeMax];  /// total killed units of each unit type
	//Wyrmgus end

	//Wyrmgus start
	int LostTownHallTimer = 0;	/// The timer for when the player lost the last town hall (to make the player's units be revealed)
	int HeroCooldownTimer = 0;	/// The cooldown timer for recruiting heroes
	//Wyrmgus end
	
	IntColor Color = 0;			/// color of units on minimap

	CUnitColors UnitColors;		/// Unit colors for new units

private:	
	const CPlayerColor *PrimaryColor = nullptr;
	const CPlayerColor *SecondaryColor = nullptr;

public:
	std::vector<CUnit *> FreeWorkers;	/// Container for free workers
	//Wyrmgus start
	std::vector<CUnit *> LevelUpUnits;	/// Container for units with available level up upgrades
	//Wyrmgus end

	// Upgrades/Allows:
	CAllow Allow;                 /// Allowed for player
	CUpgradeTimers UpgradeTimers; /// Timer for the upgrades

	/// Change player name
	void SetName(const std::string &name);
	
	//Wyrmgus start
	void CheckAge();
	void SetAge(CAge *age);
	Currency *GetCurrency() const;
	void ShareUpgradeProgress(CPlayer &player, CUnit &unit);
	bool IsPlayerColorAvailable(const CPlayerColor *player_color) const;
	bool HasUpgradeClass(const int upgrade_class) const;
	bool HasSettlement(const CSite *settlement) const;
	bool HasSettlementNearWaterZone(int water_zone) const;
	CSite *GetNearestSettlement(const Vec2i &pos, int z, const Vec2i &size) const;
	bool HasUnitBuilder(const CUnitType *type, const CSite *settlement = nullptr) const;
	bool HasUpgradeResearcher(const CUpgrade *upgrade) const;
	bool CanFoundFaction(const CFaction *faction, bool pre = false);
	bool CanChooseDynasty(const CDynasty *dynasty, bool pre = false);
	bool CanRecruitHero(const CCharacter *character, const bool ignore_neutral = false) const;
	bool UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade, bool ignore_lower_priority = false) const;
	std::string GetFactionTitleName() const;
	std::string GetCharacterTitleName(int title_type, const CGender *gender) const;
	void GetWorkerLandmasses(std::vector<int>& worker_landmasses, const CUnitType *building);	/// Builds a vector with worker landmasses; the building is the structure to be built by the worker in question
	std::vector<CUpgrade *> GetResearchableUpgrades();
	//Wyrmgus end

	/// Clear turn related player data
	void Clear();

	std::vector<CUnit *>::const_iterator UnitBegin() const;
	std::vector<CUnit *>::iterator UnitBegin();
	std::vector<CUnit *>::const_iterator UnitEnd() const;
	std::vector<CUnit *>::iterator UnitEnd();

	CUnit &GetUnit(int index) const;
	int GetUnitCount() const;

	void AddUnit(CUnit &unit);
	void RemoveUnit(CUnit &unit);
	void UpdateFreeWorkers();
	//Wyrmgus start
	void PerformResourceTrade();
	bool HasMarketUnit() const;
	CUnit *GetMarketUnit() const;
	std::vector<int> GetAutosellResources() const;
	void AutosellResource(const int resource);
	void UpdateLevelUpUnits();
	void UpdateQuestPool();
	void AvailableQuestsChanged();
	void UpdateCurrentQuests();
	void AcceptQuest(CQuest *quest);
	void CompleteQuest(CQuest *quest);
	void FailQuest(CQuest *quest, std::string fail_reason = "");
	void RemoveCurrentQuest(CQuest *quest);
	bool CanAcceptQuest(CQuest *quest);
	bool HasCompletedQuest(CQuest *quest);
	std::string HasFailedQuest(CQuest *quest);
	void AddModifier(CUpgrade *modifier, int cycles);
	void RemoveModifier(CUpgrade *modifier);
	bool AtPeace() const;
	//Wyrmgus end

	/// Get a resource of the player
	int GetResource(const int resource, const int type);
	/// Adds/subtracts some resources to/from the player store
	void ChangeResource(const int resource_index, const int value, const bool store = false);
	/// Set a resource of the player
	void SetResource(const int resource, const int value, const int type = STORE_OVERALL);
	/// Check, if there enough resources for action.
	bool CheckResource(const int resource, const int value);
	//Wyrmgus start
	/// Increase resource price
	void IncreaseResourcePrice(const int resource);
	/// Decrease resource price
	void DecreaseResourcePrice(const int resource);
	/// Converges prices with another player
	int ConvergePricesWith(CPlayer &player, int max_convergences);
	/// Get the resource price
	int GetResourcePrice(const int resource) const;
	/// Get the effective resource demand for the player, given the current prices
	int GetEffectiveResourceDemand(const int resource) const;
	
	int GetEffectiveResourceSellPrice(const int resource, int traded_quantity = 100) const;
	int GetEffectiveResourceBuyPrice(const int resource, int traded_quantity = 100) const;

	/// Get the total price difference between this player and another one
	int GetTotalPriceDifferenceWith(const CPlayer &player) const;
	/// Get the trade potential between this player and another one
	int GetTradePotentialWith(const CPlayer &player) const;
	//Wyrmgus end
	
	/// Returns count of specified unittype
	int GetUnitTotalCount(const CUnitType &type) const;
	/// Check if the unit-type didn't break any unit limits and supply/demand
	int CheckLimits(const CUnitType &type) const;

	/// Check if enough resources are available for costs
	int CheckCosts(const int *costs, bool notify = true) const;
	/// Check if enough resources are available for a new unit-type
	//Wyrmgus start
//	int CheckUnitType(const CUnitType &type) const;
	int CheckUnitType(const CUnitType &type, bool hire = false) const;
	//Wyrmgus end

	/// Add costs to the resources
	void AddCosts(const int *costs);
	/// Add costs for an unit-type to the resources
	//Wyrmgus start
//	void AddUnitType(const CUnitType &type);
	void AddUnitType(const CUnitType &type, bool hire = false);
	//Wyrmgus end
	/// Add a factor of costs to the resources
	void AddCostsFactor(const int *costs, int factor);
	/// Remove costs from the resources
	void SubCosts(const int *costs);
	/// Remove costs for an unit-type from the resources
	//Wyrmgus start
//	void SubUnitType(const CUnitType &type);
	void SubUnitType(const CUnitType &type, bool hire = false);
	//Wyrmgus end
	/// Remove a factor of costs from the resources
	void SubCostsFactor(const int *costs, int factor);
	
	//Wyrmgus start
	void GetUnitTypeCosts(const CUnitType *type, int *type_costs, bool hire = false, bool ignore_one = false) const;
	int GetUnitTypeCostsMask(const CUnitType *type, bool hire = false) const;
	void GetUpgradeCosts(const CUpgrade *upgrade, int *upgrade_costs);
	int GetUpgradeCostsMask(const CUpgrade *upgrade) const;
	
	void SetUnitTypeCount(const CUnitType *type, int quantity);
	void ChangeUnitTypeCount(const CUnitType *type, int quantity);
	int GetUnitTypeCount(const CUnitType *type) const;
	
	void SetUnitTypeUnderConstructionCount(const CUnitType *type, int quantity);
	void ChangeUnitTypeUnderConstructionCount(const CUnitType *type, int quantity);
	int GetUnitTypeUnderConstructionCount(const CUnitType *type) const;
	
	void SetUnitTypeAiActiveCount(const CUnitType *type, int quantity);
	void ChangeUnitTypeAiActiveCount(const CUnitType *type, int quantity);
	int GetUnitTypeAiActiveCount(const CUnitType *type) const;
	
	void IncreaseCountsForUnit(CUnit *unit, bool type_change = false);
	void DecreaseCountsForUnit(CUnit *unit, bool type_change = false);
	//Wyrmgus end

	/// Does the player have units of that type
	int HaveUnitTypeByType(const CUnitType &type) const;
	/// Does the player have units of that type
	int HaveUnitTypeByIdent(const std::string &ident) const;

	/// Notify player about a problem
	//Wyrmgus start
//	void Notify(int type, const Vec2i &pos, const char *fmt, ...) const PRINTF_VAARG_ATTRIBUTE(4, 5); // Don't forget to count this
	void Notify(int type, const Vec2i &pos, int z, const char *fmt, ...) const PRINTF_VAARG_ATTRIBUTE(5, 6); // Don't forget to count this
	//Wyrmgus end
	/// Notify player about a problem
	void Notify(const char *fmt, ...) const PRINTF_VAARG_ATTRIBUTE(2, 3); // Don't forget to count this


	/**
	**  Check if the player index is an enemy
	*/
	bool IsEnemy(const int index) const
	{
		return (Index != index && (Enemy & (1 << index)) != 0);
	}

	bool IsEnemy(const CPlayer &player) const;
	bool IsEnemy(const CUnit &unit) const;
	bool IsAllied(const CPlayer &player) const;
	bool IsAllied(const CUnit &unit) const;
	bool IsVisionSharing() const;
	bool IsSharedVision(const CPlayer &player) const;
	bool IsSharedVision(const CUnit &unit) const;
	bool IsBothSharedVision(const CPlayer &player) const;
	bool IsBothSharedVision(const CUnit &unit) const;
	bool IsTeamed(const CPlayer &player) const;
	bool IsTeamed(const CUnit &unit) const;
	//Wyrmgus start
	bool IsOverlordOf(const CPlayer &player, bool include_indirect = false) const;
	bool IsVassalOf(const CPlayer &player, bool include_indirect = false) const;
	bool HasContactWith(const CPlayer &player) const;
	bool HasNeutralFactionType() const;
	bool HasBuildingAccess(const CPlayer &player, int button_action = -1) const;
	bool HasHero(const CCharacter *hero) const;
	CUnit *GetHeroUnit(const CCharacter *hero) const;
	//Wyrmgus end

	void SetDiplomacyNeutralWith(const CPlayer &player);
	void SetDiplomacyAlliedWith(const CPlayer &player);
	//Wyrmgus start
//	void SetDiplomacyEnemyWith(const CPlayer &player);
	void SetDiplomacyEnemyWith(CPlayer &player);
	//Wyrmgus end
	void SetDiplomacyCrazyWith(const CPlayer &player);

	void ShareVisionWith(const CPlayer &player);
	void UnshareVisionWith(const CPlayer &player);
	
	//Wyrmgus start
	void SetOverlord(CPlayer *player);
	//Wyrmgus end

	void Init(/* PlayerTypes */ int type);
	void Save(CFile &file) const;
	void Load(lua_State *l);
	
	void ApplyHistoricalResources();
	void ApplyHistoricalUpgrades();
	void ApplyHistoricalDiplomacyStates();

private:
	std::vector<CUnit *> Units;		/// units belonging to this player
	unsigned int Enemy = 0;			/// enemy bit field for this player
	unsigned int Allied = 0;		/// allied bit field for this player
	unsigned int SharedVision = 0;	/// shared vision bit field
	
	mutable std::shared_mutex Mutex;	/// mutex for the player

	friend void ApplyReplaySettings();
	friend int CclPlayer(lua_State *l);
	
protected:
	static void _bind_methods();
};

//Wyrmgus start
enum ForceTypes {
	LandForceType,
	NavalForceType,
	AirForceType,
	
	MaxForceTypes
};
//Wyrmgus end

/**
**  Types for the player
**
**  #PlayerNeutral
**
**    This player is controlled by the computer doing nothing.
**
**  #PlayerNobody
**
**    This player is unused. Nobody controls this player.
**
**  #PlayerComputer
**
**    This player is controlled by the computer. CPlayer::AiNum
**    selects the AI strategy.
**
**  #PlayerPerson
**
**    This player is contolled by a person. This can be the player
**    sitting on the local computer or player playing over the
**    network.
**
**  #PlayerRescuePassive
**
**    This player does nothing, the game pieces just sit in the game
**    (being passive)... when a person player moves next to a
**    PassiveRescue unit/building, then it is "rescued" and becomes
**    part of that persons team. If the city center is rescued, than
**    all units of this player are rescued.
**
**  #PlayerRescueActive
**
**    This player is controlled by the computer. CPlayer::AiNum
**    selects the AI strategy. Until it is rescued it plays like
**    an ally. The first person which reaches units of this player,
**    can rescue them. If the city center is rescued, than all units
**    of this player are rescued.
*/
enum PlayerTypes {
	PlayerNeutral = 2,        /// neutral
	PlayerNobody  = 3,        /// unused slot
	PlayerComputer = 4,       /// computer player
	PlayerPerson = 5,         /// human player
	PlayerRescuePassive = 6,  /// rescued passive
	PlayerRescueActive = 7    /// rescued  active
};

constexpr int PlayerNumNeutral = (PlayerMax - 1);	/// this is the neutral player slot

/**
**  Notify types. Noties are send to the player.
*/
enum NotifyType {
	NotifyRed,     /// Red alram
	NotifyYellow,  /// Yellow alarm
	NotifyGreen    /// Green alarm
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern int NumPlayers;					/// How many player slots used
extern bool NoRescueCheck;				/// Disable rescue check
//Wyrmgus start
//extern std::vector<CColor> PlayerColorsRGB[PlayerMax]; /// Player colors
//extern std::vector<IntColor> PlayerColors[PlayerMax]; /// Player colors
//extern std::string PlayerColorNames[PlayerMax];  /// Player color names
extern std::vector<CColor> PlayerColorsRGB[PlayerColorMax]; /// Player colors
extern std::vector<IntColor> PlayerColors[PlayerColorMax]; /// Player colors
extern std::string PlayerColorNames[PlayerColorMax];  /// Player color names
extern std::vector<int> ConversiblePlayerColors; 			/// Conversible player colors
//Wyrmgus end

/**
**  Which indexes to replace with player color
*/
extern int PlayerColorIndexStart;
extern int PlayerColorIndexCount;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Clean up players
extern void CleanPlayers();
/// Save players
extern void SavePlayers(CFile &file);

/// Initialize the computer opponent AI
extern void PlayersInitAi();
/// Called each game cycle for player handlers (AI)
extern void PlayersEachCycle();
/// Called each second for a given player handler (AI)
extern void PlayersEachSecond(int player);
//Wyrmgus start
/// Called each half minute for a given player handler (AI)
extern void PlayersEachHalfMinute(int player);
/// Called each minute for a given player handler (AI)
extern void PlayersEachMinute(int player);
//Wyrmgus end

/// Change current color set to new player of the sprite
//Wyrmgus start
//extern void GraphicPlayerPixels(CPlayer &player, const CGraphic &sprite);
extern void GraphicPlayerPixels(int player, const CGraphic &sprite);
//Wyrmgus end

/// Output debug information for players
extern void DebugPlayers();

void FreePlayerColors();

/// register ccl features
extern void PlayerCclRegister();

/// Allowed to select multiple units, maybe not mine
inline bool CanSelectMultipleUnits(const CPlayer &player) { return &player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(player); }

//Wyrmgus start
extern void NetworkSetFaction(int player, const std::string &faction_name);
extern int GetPlayerColorIndexByName(const std::string &player_color_name);
extern std::string GetGovernmentTypeNameById(int government_type);
extern int GetGovernmentTypeIdByName(const std::string &government_type);
extern std::string GetForceTypeNameById(int force_type);
extern int GetForceTypeIdByName(const std::string &force_type);
extern std::string GetArticleTypeNameById(int article_type);
extern int GetArticleTypeIdByName(const std::string &article_type);
extern std::string GetGrammaticalCaseNameById(int grammatical_case);
extern int GetGrammaticalCaseIdByName(const std::string &grammatical_case);
extern std::string GetGrammaticalNumberNameById(int grammatical_number);
extern int GetGrammaticalNumberIdByName(const std::string &grammatical_number);
extern std::string GetGrammaticalPersonNameById(int grammatical_person);
extern int GetGrammaticalPersonIdByName(const std::string &grammatical_person);
extern std::string GetGrammaticalTenseNameById(int grammatical_tense);
extern int GetGrammaticalTenseIdByName(const std::string &grammatical_tense);
extern std::string GetGrammaticalMoodNameById(int grammatical_mood);
extern int GetGrammaticalMoodIdByName(const std::string &grammatical_mood);
extern std::string GetComparisonDegreeNameById(int comparison_degree);
extern int GetComparisonDegreeIdByName(const std::string &comparison_degree);
extern std::string GetAffixTypeNameById(int affix_type);
extern int GetAffixTypeIdByName(const std::string &affix_type);
extern std::string GetWordJunctionTypeNameById(int word_junction_type);
extern int GetWordJunctionTypeIdByName(const std::string &word_junction_type);
extern bool IsNameValidForWord(const std::string &word_name);
//Wyrmgus end

#endif
