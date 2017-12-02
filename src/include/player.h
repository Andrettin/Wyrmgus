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
/**@name player.h - The player headerfile. */
//
//      (c) Copyright 1998-2017 by Lutz Sammer, Jimmy Salmon and Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>

//Wyrmgus start
#include <map>
#include <tuple>

#ifndef __ICONS_H__
#include "icons.h"
#endif
//Wyrmgus end

//Wyrmgus start
#include "character.h" // because of "MaxCharacterTitles"
//Wyrmgus end
#include "color.h"
//Wyrmgus start
#include "item.h"
#include "ui.h" // for the UI fillers
//Wyrmgus end
#include "upgrade_structs.h"

#include "vec2i.h"

class CGraphic;

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

#define STORE_OVERALL 0
#define STORE_BUILDING 1
#define STORE_BOTH 2

#define SPEEDUP_FACTOR 100

#define DefaultTradeCost 30

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnit;
class CUnitType;
//Wyrmgus start
class CCharacter;
class CLanguage;
class CProvince;
class CPlane;
class CQuest;
class CReligion;
class CSettlement;
//WYrmgus end
class PlayerAi;
class CFile;
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
class CPlayer
{
public:
	int Index;          /// player as number
	std::string Name;   /// name of non computer

	int   Type;         /// type of player (human,computer,...)
	int   Race;         /// race of player (orc,human,...)
	//Wyrmgus start
	int Faction;		/// faction of the player
	CReligion *Religion;	/// religion of the player
	CDynasty *Dynasty;		/// ruling dynasty of the player
	//Wyrmgus end
	std::string AiName; /// AI for computer

	// friend enemy detection
	int      Team;          /// team of player

	Vec2i StartPos;  /// map tile start position
	//Wyrmgus start
	int StartMapLayer;  /// map tile start map layer
	
	CPlayer *Overlord;	/// overlord of this player
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

	std::map<const CUnitType *, int> UnitTypesCount;  /// total units of unit-type
	std::map<const CUnitType *, int> UnitTypesUnderConstructionCount;  /// total under construction units of unit-type
	std::map<const CUnitType *, int> UnitTypesAiActiveCount;  /// total units of unit-type that have their AI set to active
	//Wyrmgus start
	std::vector<CUnit *> Heroes;					/// hero units owned by this player
	std::vector<CDeity *> Deities;					/// deities chosen by this player
	std::vector<CQuest *> AvailableQuests;		/// quests available to this player
	std::vector<CQuest *> CurrentQuests;		/// quests being pursued by this player
	std::vector<CQuest *> CompletedQuests;		/// quests completed by this player
	std::vector<std::tuple<CQuest *, CUnitType *, int>> QuestBuildUnits;	/// build units objectives from quests; int is the quantity, set at start to be the same as the quest's; every time a unit is built it is reduced by one
	std::vector<std::tuple<CQuest *, int, int>> QuestBuildUnitsOfClass;	/// build units objectives from quests; the first int is the class id, and the second int is the quantity, set at start to be the same as the quest's; every time a unit is built it is reduced by one
	std::vector<std::tuple<CQuest *, CSettlement *, CUnitType *, int>> QuestBuildSettlementUnits;	/// build settlement units objectives from quests; int is the quantity, set at start to be the same as the quest's; every time a unit is built in the appropriate settlement it is reduced by one
	std::vector<std::tuple<CQuest *, CSettlement *, int, int>> QuestBuildSettlementUnitsOfClass;	/// build settlement units objectives from quests; the first int is the class id, and the second int is the quantity, set at start to be the same as the quest's; every time a unit is built in the appropriate settlement it is reduced by one
	std::vector<std::tuple<CQuest *, CUpgrade *>> QuestResearchUpgrades;
	std::vector<std::tuple<CQuest *, CUnitType *, CFaction *, int>> QuestDestroyUnits;	/// destroy units objectives from quests; int is the quantity, set at start to be the same as the quest's; every time a unit is destroyed it is reduced by one
	std::vector<std::tuple<CQuest *, CCharacter *, bool>> QuestDestroyCharacters;
	std::vector<std::tuple<CQuest *, CUniqueItem *, bool>> QuestDestroyUniques;
	std::vector<std::tuple<CQuest *, CFaction *, bool>> QuestDestroyFactions;	/// destroy factions objectives from quests; when the faction's last unit is destroyed by the player, the bool is turned to true
	std::vector<std::tuple<CQuest *, int, int>> QuestGatherResources;	/// gather resources objectives from quests; the first int is the resource ID, and the second one is the quantity, set at start to be the same as the quest's
	std::vector<std::pair<CUpgrade *, int>> Modifiers;					/// Modifiers affecting the player, and until which cycle it should last
	//Wyrmgus end

	bool AiEnabled;        /// handle AI on local computer
	//Wyrmgus start
	bool Revealed;			/// Whether the player has been revealed (i.e. after losing the last town hall)
	//Wyrmgus end
	PlayerAi *Ai;          /// Ai structure pointer

	int    NumBuildings;   /// # buildings
	//Wyrmgus start
	int    NumBuildingsUnderConstruction; /// # buildings under construction
	int    NumTownHalls;
	//Wyrmgus end
	int    Supply;         /// supply available/produced
	int    Demand;         /// demand of player

	int    UnitLimit;       /// # food units allowed
	int    BuildingLimit;   /// # buildings allowed
	int    TotalUnitLimit;  /// # total unit number allowed

	int    Score;           /// Points for killing ...
	int    TotalUnits;
	int    TotalBuildings;
	int    TotalResources[MaxCosts];
	int    TotalRazings;
	int    TotalKills;      /// How many units killed
	//Wyrmgus start
	int UnitTypeKills[UnitTypeMax];  /// total killed units of unit-type
	//Wyrmgus end

	//Wyrmgus start
	int LostTownHallTimer;	/// The timer for when the player lost the last town hall (to make the player's units be revealed)
	//Wyrmgus end
	
	IntColor Color;           /// color of units on minimap

	CUnitColors UnitColors; /// Unit colors for new units

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
	void SetCivilization(int civilization);
	void SetFaction(CFaction *faction);
	void SetRandomFaction();
	void SetDynasty(CDynasty *dynasty);
	void ShareUpgradeProgress(CPlayer &player, CUnit &unit);
	bool IsPlayerColorUsed(int color);
	bool HasUpgradeClass(int upgrade_class);
	bool HasSettlement(CSettlement *settlement) const;
	bool HasSettlementNearWaterZone(int water_zone) const;
	bool HasUnitBuilder(CUnitType *type, CSettlement *settlement = NULL) const;
	bool CanFoundFaction(CFaction *faction, bool pre = false);
	bool CanChooseDynasty(CDynasty *dynasty, bool pre = false);
	bool UpgradeRemovesExistingUpgrade(const CUpgrade *upgrade, bool ignore_lower_priority = false) const;
	std::string GetFactionTitleName() const;
	std::string GetCharacterTitleName(int title_type, int gender) const;
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
	void UpdateLevelUpUnits();
	void UpdateQuestPool();
	void AvailableQuestsChanged();
	void UpdateCurrentQuests();
	void UpdateQuest(CQuest *quest);
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
	void ChangeResource(const int resource, const int value, const bool store = false);
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
	void GetUpgradeCosts(const CUpgrade *upgrade, int *upgrade_costs);
	
	void SetUnitTypeCount(const CUnitType *type, int quantity);
	void ChangeUnitTypeCount(const CUnitType *type, int quantity);
	int GetUnitTypeCount(const CUnitType *type) const;
	
	void SetUnitTypeUnderConstructionCount(const CUnitType *type, int quantity);
	void ChangeUnitTypeUnderConstructionCount(const CUnitType *type, int quantity);
	int GetUnitTypeUnderConstructionCount(const CUnitType *type) const;
	
	void SetUnitTypeAiActiveCount(const CUnitType *type, int quantity);
	void ChangeUnitTypeAiActiveCount(const CUnitType *type, int quantity);
	int GetUnitTypeAiActiveCount(const CUnitType *type) const;
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

private:
	std::vector<CUnit *> Units; /// units of this player
	unsigned int Enemy;         /// enemy bit field for this player
	unsigned int Allied;        /// allied bit field for this player
	unsigned int SharedVision;  /// shared vision bit field
};

//Wyrmgus start
enum GovernmentTypes {
	GovernmentTypeNoGovernmentType,
	GovernmentTypeMonarchy,
	GovernmentTypeRepublic,
	GovernmentTypeTheocracy,
	
	MaxGovernmentTypes
};

enum FactionTypes {
	FactionTypeNoFactionType,
	FactionTypeTribe,
	FactionTypePolity,
	FactionTypeMercenaryCompany,
	FactionTypeHolyOrder,
	FactionTypeTradingCompany,
	
	MaxFactionTypes
};

enum FactionTiers {
	FactionTierNoFactionTier,
	FactionTierBarony,
	FactionTierCounty,
	FactionTierDuchy,
	FactionTierGrandDuchy,
	FactionTierKingdom,
	FactionTierEmpire,
	
	MaxFactionTiers
};

enum WordTypes {
	WordTypeNoun,
	WordTypeVerb,
	WordTypeAdjective,
	WordTypePronoun,
	WordTypeAdverb,
	WordTypeConjunction,
	WordTypeAdposition,
	WordTypeArticle,
	WordTypeNumeral,
	WordTypeAffix,
	
	MaxWordTypes
};

enum ArticleTypes {
	ArticleTypeNoArticle,
	ArticleTypeDefinite,
	ArticleTypeIndefinite,
	
	MaxArticleTypes
};

enum GrammaticalCases {
	GrammaticalCaseNoCase,
	GrammaticalCaseNominative,
	GrammaticalCaseAccusative,
	GrammaticalCaseDative,
	GrammaticalCaseGenitive,
	
	MaxGrammaticalCases
};

enum GrammaticalNumbers {
	GrammaticalNumberNoNumber,
	GrammaticalNumberSingular,
	GrammaticalNumberPlural,
	
	MaxGrammaticalNumbers
};

enum GrammaticalPersons {
	GrammaticalPersonFirstPerson,
	GrammaticalPersonSecondPerson,
	GrammaticalPersonThirdPerson,
	
	MaxGrammaticalPersons
};

enum GrammaticalGenders {
	GrammaticalGenderNoGender,
	GrammaticalGenderMasculine,
	GrammaticalGenderFeminine,
	GrammaticalGenderNeuter,
	
	MaxGrammaticalGenders
};

enum GrammaticalTenses {
	GrammaticalTenseNoTense,
	GrammaticalTensePresent,
	GrammaticalTensePast,
	GrammaticalTenseFuture,
	
	MaxGrammaticalTenses
};

enum GrammaticalMoods {
	GrammaticalMoodIndicative,
	GrammaticalMoodSubjunctive,
	
	MaxGrammaticalMoods
};

enum ComparisonDegrees {
	ComparisonDegreePositive,
	ComparisonDegreeComparative,
	ComparisonDegreeSuperlative,
	
	MaxComparisonDegrees
};

enum AffixTypes {
	AffixTypePrefix,
	AffixTypeSuffix,
	AffixTypeInfix,
	
	MaxAffixTypes
};

enum WordJunctionTypes {
	WordJunctionTypeNoWordJunction,
	WordJunctionTypeCompound,
	WordJunctionTypeSeparate,
	
	MaxWordJunctionTypes
};

class CCivilization
{
public:
	CCivilization() :
		ID(-1), ParentCivilization(-1), CalendarStartingYear(0),
		Language(NULL)
	{
	}
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	std::string GetMonthName(int month) const;
	std::map<int, std::vector<std::string>> &GetPersonalNames();
	std::vector<std::string> &GetUnitClassNames(int class_id);
	std::vector<std::string> &GetSettlementNames();
	std::vector<std::string> &GetShipNames();
	
	int ID;
	int ParentCivilization;
	int CalendarStartingYear;
	std::string Ident;			/// Ident of the civilization
	std::string Description;	/// civilization description
	std::string Quote;			/// civilization quote
	std::string Background;		/// civilization background
	std::string Adjective;		/// adjective pertaining to the civilization
	std::string YearLabel;		/// label used for years (i.e. AD)
	std::string NegativeYearLabel;	/// label used for "negative" years (i.e. BC)
	CUnitSound UnitSounds;			/// Sounds for unit events
	CLanguage *Language;		/// The language used by the civilization
	std::vector<CQuest *> Quests;	/// quests belonging to this civilization
	std::map<const CUpgrade *, int> UpgradePriorities;		/// Priority for each upgrade
	std::map<int, std::string> Months;	/// Month names for the civilization, mapped to the ID of the corresponding month
	std::map<int, std::vector<std::string>> PersonalNames;	/// Personal names for the civilization, mapped to the gender they pertain to (use NoGender for names which should be available for both genders)
	std::map<int, std::vector<std::string>> UnitClassNames;	/// Unit class names for the civilization, mapped to the unit class they pertain to, used for mechanical units, and buildings
	std::vector<std::string> FamilyNames;		/// Family names for the civilization
	std::vector<std::string> SettlementNames;	/// Settlement names for the civilization
	std::vector<std::string> ProvinceNames;		/// Province names for the civilization
	std::vector<std::string> ShipNames;			/// Ship names for the civilization
	std::string MinisterTitles[MaxCharacterTitles][MaxGenders][MaxGovernmentTypes][MaxFactionTiers]; /// this civilization's minister title for each minister type and government type
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change
};

class CFaction
{
public:
	CFaction() : 
		ID(-1), Civilization(-1), Type(FactionTypeNoFactionType), DefaultTier(FactionTierBarony), DefaultGovernmentType(GovernmentTypeMonarchy), ParentFaction(-1),
		Playable(true), DefiniteArticle(false),
		DefaultAI("land-attack"),
		HolyOrderDeity(NULL), Conditions(NULL)
	{
	}
	
	~CFaction();
	
	int GetUpgradePriority(const CUpgrade *upgrade) const;
	std::vector<std::string> &GetSettlementNames();
	std::vector<std::string> &GetShipNames();

	std::string Ident;													/// faction name
	std::string Name;
	std::string Description;											/// faction description
	std::string Quote;													/// faction quote
	std::string Background;												/// faction background
	std::string FactionUpgrade;											/// faction upgrade applied when the faction is set
	std::string DefaultAI;
	int ID;																/// faction ID
	int Civilization;													/// faction civilization
	int Type;															/// faction type (i.e. tribe or polity)
	int DefaultTier;													/// default faction tier
	int DefaultGovernmentType;											/// default government type
	int ParentFaction;													/// parent faction of this faction
	bool Playable;														/// faction playability
	bool DefiniteArticle;												/// whether the faction's name should be preceded by a definite article (i.e. "the Netherlands")
	IconConfig Icon;													/// Faction's icon
	CDeity *HolyOrderDeity;												/// deity this faction belongs to, if it is a holy order
	LuaCallback *Conditions;
	std::vector<int> Colors;											/// faction colors
	std::vector<CFaction *> DevelopsFrom;								/// from which factions can this faction develop
	std::vector<CFaction *> DevelopsTo;									/// to which factions this faction can develop
	std::vector<CDynasty *> Dynasties;									/// which dynasties are available to this faction
	std::string Titles[MaxGovernmentTypes][MaxFactionTiers];			/// this faction's title for each government type and faction tier
	std::string MinisterTitles[MaxCharacterTitles][MaxGenders][MaxGovernmentTypes][MaxFactionTiers]; /// this faction's minister title for each minister type and government type
	std::map<const CUpgrade *, int> UpgradePriorities;					/// Priority for each upgrade
	std::map<int, IconConfig> ButtonIcons;								/// icons for button actions
	std::map<int, int> ClassUnitTypes;									/// the unit type slot of a particular class for a particular faction
	std::map<int, int> ClassUpgrades;									/// the upgrade slot of a particular class for a particular faction
	std::vector<std::string> SettlementNames;							/// Settlement names for the faction
	std::vector<std::string> ProvinceNames;								/// Province names for the faction
	std::vector<std::string> ShipNames;									/// Ship names for the faction
	std::vector<CSettlement *> Cores;									/// Core settlements of this faction (required to found it)
	std::map<std::tuple<CDate, CDate, int>, CCharacter *> HistoricalMinisters;	/// historical ministers of the faction (as well as heads of state and government), mapped to the beginning and end of the rule, and the enum of the title in question
	std::map<std::string, std::map<CDate, bool>> HistoricalUpgrades;	/// historical upgrades of the faction, with the date of change
	std::map<int, int> HistoricalTiers;									/// dates in which this faction's tier changed; faction tier mapped to year
	std::map<int, int> HistoricalGovernmentTypes;						/// dates in which this faction's government type changed; government type mapped to year
	std::map<std::pair<int, CFaction *>, int> HistoricalDiplomacyStates;	/// dates in which this faction's diplomacy state to another faction changed; diplomacy state mapped to year and faction
	std::map<std::pair<CDate, int>, int> HistoricalResources;	/// dates in which this faction's storage of a particular resource changed; resource quantities mapped to date and resource
	std::vector<std::pair<CDate, std::string>> HistoricalCapitals;		/// historical capitals of the faction; the values are: date and settlement ident
	std::vector<CFiller> UIFillers;
	
	std::string Mod;													/// To which mod (or map), if any, this faction belongs
};

class CDynasty
{
public:
	CDynasty() : 
		ID(-1), Civilization(-1),
		DynastyUpgrade(NULL), Conditions(NULL)
	{
	}
	
	~CDynasty();
	
	std::string Ident;													/// dynasty name
	std::string Name;
	std::string Description;											/// dynasty description
	std::string Quote;													/// dynasty quote
	std::string Background;												/// dynasty background
	CUpgrade *DynastyUpgrade;											/// dynasty upgrade applied when the dynasty is set
	int ID;																/// dynasty ID
	int Civilization;													/// dynasty civilization
	IconConfig Icon;													/// Dynasty's icon
	LuaCallback *Conditions;
	std::vector<CFaction *> Factions;									/// to which factions is this dynasty available
};

class CDeityDomain
{
public:
	CDeityDomain() :
		Ident("")
	{
	}
	
	std::string Ident;			/// Ident of the domain
	std::string Name;			/// Name of the domain
	std::vector<CUpgrade *> Abilities;	/// Abilities linked to this domain
};

class CReligion
{
public:
	CReligion() :
		CulturalDeities(false)
	{
	}
	
	std::string Ident;			/// Ident of the religion
	std::string Name;			/// Name of the religion
	std::string Description;
	std::string Background;
	std::string Quote;
	bool CulturalDeities;		/// Whether the religion's deities (or equivalent) must belong to the civilization that has the religion; for instance: the deities under paganism must belong to the civilization of the player, but under hinduism they musn't (meaning that a Teuton player which has hinduism as a religion can select Hindu deities, but an Indian pagan cannot select Teuton pagan deities)
	std::vector<CDeityDomain *> Domains;
};

class CDeity
{
public:
	CDeity() :
		Gender(0), Major(false), HomePlane(NULL), DeityUpgrade(NULL), CharacterUpgrade(NULL)
	{
	}
	
	int Gender;					/// Deity's gender
	bool Major;					/// Whether the deity is a major one or not
	std::string Ident;			/// Ident of the deity
	std::string Name;			/// Name of the deity
	std::string UpgradeIdent;	/// Ident of the upgrade applied by the deity
	std::string Pantheon;		/// Pantheon to which the deity belongs
	std::string Description;
	std::string Background;
	std::string Quote;
	CPlane *HomePlane;			/// The home plane of the deity
	CUpgrade *DeityUpgrade;		/// The deity's upgrade applied to a player that worships it
	CUpgrade *CharacterUpgrade;	/// The deity's upgrade applied to its character as an individual upgrade
	IconConfig Icon;			/// Deity's icon
	std::vector<int> Civilizations;	/// Civilizations which may worship the deity
	std::vector<CReligion *> Religions;	/// Religions for which this deity is available
	std::vector<std::string> Feasts;
	std::vector<CDeityDomain *> Domains;
	std::vector<CFaction *> HolyOrders;	/// Holy orders of this deity
	std::vector<CUpgrade *> Abilities;	/// Abilities linked to this deity
	std::map<int, std::string> CulturalNames;	/// Names of the deity in different cultures (for example, Odin is known as Hroptatyr by the dwarves)
};

class LanguageWord
{
public:
	LanguageWord() : 
		Type(-1), Gender(-1), GrammaticalNumber(-1),
		Language(NULL), DerivesFrom(NULL),
		Archaic(false),
		Uncountable(false),
		ArticleType(-1),
		Number(-1)
	{
	}
	
	bool HasMeaning(std::string meaning);
	std::string GetNounInflection(int grammatical_number, int grammatical_case, int word_junction_type = -1);
	std::string GetVerbInflection(int grammatical_number, int grammatical_person, int grammatical_tense, int grammatical_mood);
	std::string GetAdjectiveInflection(int comparison_degree, int article_type = -1, int grammatical_case = -1, int grammatical_number = -1, int grammatical_gender = -1);
	std::string GetParticiple(int grammatical_tense);
	void RemoveFromVector(std::vector<LanguageWord *>& word_vector);

	std::string Word;									/// Word name / ID.
	CLanguage *Language;								/// The language the word belongs to
	int Type;											/// Word type
	int Gender;											/// What is the gender of the noun or article (Masculine, Feminine or Neuter)
	int GrammaticalNumber;								/// Grammatical number (i.e. whether the word is necessarily plural or not)
	bool Archaic;										/// Whether the word is archaic (whether it is used in current speech)
	std::map<std::tuple<int, int>, std::string> NumberCaseInflections;	/// For nouns, mapped to grammatical number and grammatical case
	std::map<std::tuple<int, int, int, int>, std::string> NumberPersonTenseMoodInflections;	/// For verbs, mapped to grammatical number, grammatical person, grammatical tense and grammatical mood
	std::string ComparisonDegreeCaseInflections[MaxComparisonDegrees][MaxGrammaticalCases];	/// For adjectives
	std::string Participles[MaxGrammaticalTenses];		/// For verbs
	std::vector<std::string> Meanings;					/// Meanings of the word in English.
	LanguageWord *DerivesFrom;    						/// From which word does this word derive
	std::vector<LanguageWord *> DerivesTo;				/// Which words derive from this word
	LanguageWord *CompoundElements[MaxAffixTypes];    	/// From which compound elements is this word formed
	std::vector<LanguageWord *> CompoundElementOf[MaxAffixTypes];	/// Which words are formed from this word as a compound element
	
	// noun-specific variables
	bool Uncountable;				/// Whether the noun is uncountable or not.
	
	//pronoun and article-specific variables
	std::string Nominative;			/// Nominative case for the pronoun (if any)
	std::string Accusative;			/// Accusative case for the pronoun (if any)
	std::string Dative;				/// Dative case for the pronoun (if any)
	std::string Genitive;			/// Genitive case for the pronoun (if any)
	
	//article-specific variables
	int ArticleType;				/// Which article type this article belongs to
	
	//numeral-specific variables
	int Number;
	
	std::string Mod;				/// To which mod (or map), if any, this word belongs
};

class CLanguage
{
public:
	CLanguage() :
		UsedByCivilizationOrFaction(false),
		DialectOf(NULL)
	{
	}

	LanguageWord *GetWord(const std::string word, int word_type, std::vector<std::string>& word_meanings) const;
	std::string GetArticle(int gender, int grammatical_case, int article_type, int grammatical_number);
	std::string GetNounEnding(int grammatical_number, int grammatical_case, int word_junction_type = -1);
	std::string GetAdjectiveEnding(int article_type, int grammatical_case, int grammatical_number, int grammatical_gender);
	void RemoveWord(LanguageWord *word);
	
	std::string Ident;											/// Ident of the language
	std::string Name;											/// Name of the language
	std::string Family;											/// Family of the language
	std::string NounEndings[MaxGrammaticalNumbers][MaxGrammaticalCases][MaxWordJunctionTypes];
	std::string AdjectiveEndings[MaxArticleTypes][MaxGrammaticalCases][MaxGrammaticalNumbers][MaxGrammaticalGenders];
	bool UsedByCivilizationOrFaction;
	CLanguage *DialectOf;										/// Of which language this is a dialect of (if at all); dialects inherit the words from the parent language unless specified otherwise
	std::vector<CLanguage *> Dialects;							/// Dialects of this language
	std::vector<LanguageWord *> LanguageWords;					/// Words of the language
	std::map<std::string, std::vector<std::string>> NameTranslations;	/// Name translations; possible translations mapped to the name to be translated
};
//Wyrmgus end

/**
**  Races for the player
**  Mapped with #PlayerRaces to a symbolic name.
*/
class PlayerRace
{
public:
	PlayerRace() : Count(0)
	{
		memset(Visible, 0, sizeof(Visible));
		//Wyrmgus start
		memset(Playable, 0, sizeof(Playable));
		//Wyrmgus end
	}

	void Clean();
	int GetRaceIndexByName(const char *raceName) const;
	//Wyrmgus start
	int GetFactionIndexByName(const std::string faction_name) const;
	CFaction *GetFaction(const std::string faction_name) const;
	CDynasty *GetDynasty(const std::string dynasty_name) const;
	int GetReligionIndexByIdent(std::string religion_ident) const;
	int GetDeityDomainIndexByIdent(std::string deity_domain_ident) const;
	CDeityDomain *GetDeityDomain(std::string deity_domain_ident) const;
	int GetDeityIndexByIdent(std::string deity_ident) const;
	CDeity *GetDeity(std::string deity_ident) const;
	CLanguage *GetLanguage(std::string language_ident) const;
	int GetCivilizationClassUnitType(int civilization, int class_id);
	int GetCivilizationClassUpgrade(int civilization, int class_id);
	int GetFactionClassUnitType(int faction, int class_id);
	int GetFactionClassUpgrade(int faction, int class_id);
	CLanguage *GetCivilizationLanguage(int civilization);
	std::vector<CFiller> GetCivilizationUIFillers(int civilization);
	std::vector<CFiller> GetFactionUIFillers(int faction);
	std::string TranslateName(std::string name, CLanguage *language);
	//Wyrmgus end

public:
	bool Visible[MAX_RACES];        /// race should be visible in pulldown
	std::string Name[MAX_RACES];    /// race names
	std::string Display[MAX_RACES]; /// text to display in pulldown
	//Wyrmgus start
	bool Playable[MAX_RACES];											/// civilization is playable?
	std::string Species[MAX_RACES];										/// civilization's species (i.e. human)
	std::string DefaultColor[MAX_RACES];								/// name of the civilization's default color (used for the encyclopedia, tech tree, etc.)
	std::string CivilizationUpgrades[MAX_RACES];
	std::map<int, int> CivilizationClassUnitTypes[MAX_RACES];			/// the unit type slot of a particular class for a particular civilization
	std::map<int, int> CivilizationClassUpgrades[MAX_RACES];			/// the upgrade slot of a particular class for a particular civilization
	std::map<int, IconConfig> ButtonIcons[MAX_RACES];					/// icons for button actions
	std::vector<CCivilization *> Civilizations;    						/// civilizations
	std::vector<CFaction *> Factions;    								/// factions
	std::vector<int> DevelopsFrom[MAX_RACES];							/// from which civilizations this civilization develops
	std::vector<int> DevelopsTo[MAX_RACES];								/// to which civilizations this civilization develops
	std::vector<CFiller> CivilizationUIFillers[MAX_RACES];
	std::vector<CLanguage *> Languages;									/// languages
	std::vector<CDynasty *> Dynasties;    								/// dynasties
	std::vector<CReligion *> Religions;									/// religions
	std::vector<CDeityDomain *> DeityDomains;							/// deity domains
	std::vector<CDeity *> Deities;										/// deities
	//Wyrmgus end
	unsigned int Count;             /// number of races
};


enum PlayerRacesOld {
	PlayerRaceHuman = 0,  /// belongs to human
	PlayerRaceOrc  = 1    /// belongs to orc
};

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

#define PlayerNumNeutral (PlayerMax - 1)  /// this is the neutral player slot

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

extern int NumPlayers;             /// How many player slots used
extern CPlayer Players[PlayerMax];  /// All players
extern CPlayer *ThisPlayer;         /// Player on local computer
extern bool NoRescueCheck;          /// Disable rescue check
//Wyrmgus start
//extern std::vector<CColor> PlayerColorsRGB[PlayerMax]; /// Player colors
//extern std::vector<IntColor> PlayerColors[PlayerMax]; /// Player colors
//extern std::string PlayerColorNames[PlayerMax];  /// Player color names
extern std::vector<CColor> PlayerColorsRGB[PlayerColorMax]; /// Player colors
extern std::vector<IntColor> PlayerColors[PlayerColorMax]; /// Player colors
extern std::string PlayerColorNames[PlayerColorMax];  /// Player color names
extern std::vector<int> ConversiblePlayerColors; 			/// Conversible player colors

extern std::string HairColorNames[HairColorMax];  /// Hair color names
extern std::vector<CColor> HairColorsRGB[HairColorMax]; /// Hair colors
extern std::vector<int> ConversibleHairColors; 			/// Conversible hair colors

extern std::map<std::string, int> CivilizationStringToIndex;
extern std::map<std::string, int> FactionStringToIndex;
extern std::map<std::string, int> DynastyStringToIndex;

extern bool LanguageCacheOutdated;
//Wyrmgus end

extern PlayerRace PlayerRaces;  /// Player races

/**
**  Which indexes to replace with player color
*/
extern int PlayerColorIndexStart;
extern int PlayerColorIndexCount;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Init players
extern void InitPlayers();
/// Clean up players
extern void CleanPlayers();
/// Save players
extern void SavePlayers(CFile &file);

/// Create a new player
extern void CreatePlayer(int type);

//Wyrmgus start
extern CPlayer *GetFactionPlayer(CFaction *faction);
extern CPlayer *GetOrAddFactionPlayer(CFaction *faction);
//Wyrmgus end

/// Initialize the computer opponent AI
extern void PlayersInitAi();
/// Called each game cycle for player handlers (AI)
extern void PlayersEachCycle();
/// Called each second for a given player handler (AI)
extern void PlayersEachSecond(int player);
//Wyrmgus start
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
inline bool CanSelectMultipleUnits(const CPlayer &player) { return &player == ThisPlayer || ThisPlayer->IsTeamed(player); }

//Wyrmgus start
extern void SetCivilizationStringToIndex(std::string civilization_name, int civilization_id);
extern void SetFactionStringToIndex(std::string faction_name, int faction_id);
extern void NetworkSetFaction(int player, std::string faction_name);
extern int GetPlayerColorIndexByName(std::string player_color_name);
extern int GetHairColorIndexByName(std::string hair_color_name);
extern std::string GetFactionTypeNameById(int faction_type);
extern int GetFactionTypeIdByName(std::string faction_type);
extern std::string GetGovernmentTypeNameById(int government_type);
extern int GetGovernmentTypeIdByName(std::string government_type);
extern std::string GetWordTypeNameById(int word_type);
extern int GetWordTypeIdByName(std::string word_type);
extern std::string GetArticleTypeNameById(int article_type);
extern int GetArticleTypeIdByName(std::string article_type);
extern std::string GetGrammaticalCaseNameById(int grammatical_case);
extern int GetGrammaticalCaseIdByName(std::string grammatical_case);
extern std::string GetGrammaticalNumberNameById(int grammatical_number);
extern int GetGrammaticalNumberIdByName(std::string grammatical_number);
extern std::string GetGrammaticalPersonNameById(int grammatical_person);
extern int GetGrammaticalPersonIdByName(std::string grammatical_person);
extern std::string GetGrammaticalGenderNameById(int grammatical_gender);
extern int GetGrammaticalGenderIdByName(std::string grammatical_gender);
extern std::string GetGrammaticalTenseNameById(int grammatical_tense);
extern int GetGrammaticalTenseIdByName(std::string grammatical_tense);
extern std::string GetGrammaticalMoodNameById(int grammatical_mood);
extern int GetGrammaticalMoodIdByName(std::string grammatical_mood);
extern std::string GetComparisonDegreeNameById(int comparison_degree);
extern int GetComparisonDegreeIdByName(std::string comparison_degree);
extern std::string GetAffixTypeNameById(int affix_type);
extern int GetAffixTypeIdByName(std::string affix_type);
extern std::string GetWordJunctionTypeNameById(int word_junction_type);
extern int GetWordJunctionTypeIdByName(std::string word_junction_type);
extern bool IsNameValidForWord(std::string word_name);

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern std::map<std::string, CLanguage *> LanguageIdentToPointer;
//Wyrmgus end

//@}

#endif // !__PLAYER_H__
