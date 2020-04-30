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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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
--  Includes
----------------------------------------------------------------------------*/

//Wyrmgus start
#include "character.h" // because of "MaxCharacterTitles"
#include "color.h"
//Wyrmgus end
//Wyrmgus start
#include "item.h"
#include "time/date.h"
//Wyrmgus end
#include "ui/button_cmd.h"
#include "ui/icon.h"
//Wyrmgus start
#include "ui/ui.h" // for the UI fillers
//Wyrmgus end
#include "upgrade/upgrade_structs.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

static constexpr int STORE_OVERALL = 0;
static constexpr int STORE_BUILDING = 1;
static constexpr int STORE_BOTH = 2;

static constexpr int SPEEDUP_FACTOR = 100;

static constexpr int DefaultTradeCost = 30;

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CCurrency;
class CDeity;
class CDeityDomain;
class CDynasty;
class CFile;
class CGraphic;
class CLanguage;
class CProvince;
class CPlayerQuestObjective;
class CReligion;
class CUnit;
class CUnitType;
class PlayerAi;
//Wyrmgus start
class CFiller;
class LuaCallback;
//Wyrmgus end
enum class DiplomacyState;
struct lua_State;

namespace stratagus {
	class age;
	class calendar;
	class civilization;
	class player_color;
	class quest;
	class site;
	class unit_class;
}

/*----------------------------------------------------------------------------
--  Player type
----------------------------------------------------------------------------*/

enum class diplomacy_state {
	allied,   /// Ally with opponent
	neutral,  /// Don't attack be neutral
	enemy,    /// Attack opponent
	//Wyrmgus start
	overlord,	/// Become overlord to other player
	vassal,	/// Become vassal to other player
	//Wyrmgus end
	crazy     /// Ally and attack opponent
}; /// Diplomacy states for CommandDiplomacy

///  Player structure
class CPlayer
{
public:
	static void SetThisPlayer(CPlayer *player);
	static CPlayer *GetThisPlayer();
	static CPlayer *GetPlayer(const int index);

	static std::vector<CPlayer *> Players;	//all players

private:
	static CPlayer *ThisPlayer; //player on local computer

public:
	const stratagus::player_color *get_player_color() const
	{
		return this->player_color;
	}

	const QColor &get_minimap_color() const
	{
		return this->minimap_color;
	}

	int Index = 0;          /// player as number
	std::string Name;   /// name of non computer

	int Type = 0; //type of the player (human, computer, ...)
	int Race = 0; //race of the player (orc, human, ...)
	int Faction = -1; //faction of the player
	CReligion *Religion = nullptr; //religion of the player
	CDynasty *Dynasty = nullptr; //ruling dynasty of the player
	stratagus::age *age = nullptr; //the current age the player/faction is in
	std::string AiName; //AI for computer

	// friend enemy detection
	int Team = 0;          /// team of player

	Vec2i StartPos = Vec2i(0, 0);  /// map tile start position
	//Wyrmgus start
	int StartMapLayer = 0;  /// map tile start map layer
	
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
	std::vector<stratagus::quest *> AvailableQuests;			/// quests available to this player
	std::vector<stratagus::quest *> CurrentQuests;				/// quests being pursued by this player
	std::vector<const stratagus::quest *> CompletedQuests;		/// quests completed by this player
	std::vector<CPlayerQuestObjective *> QuestObjectives;					/// Objectives of the player's current quests
	std::vector<std::pair<CUpgrade *, int>> Modifiers;						/// Modifiers affecting the player, and until which cycle it should last
	std::vector<int> AutosellResources;
	//Wyrmgus end

	bool AiEnabled = false; //handle AI on local computer
	//Wyrmgus start
	bool Revealed = false; //whether the player has been revealed (i.e. after losing the last town hall)
	//Wyrmgus end
	PlayerAi *Ai = nullptr;          /// Ai structure pointer

	int NumBuildings = 0;   /// # buildings
	//Wyrmgus start
	int NumBuildingsUnderConstruction = 0; /// # buildings under construction
	int NumTownHalls = 0;
	//Wyrmgus end
	int Supply = 0;         /// supply available/produced
	int Demand = 0;         /// demand of player

	int UnitLimit;       /// # food units allowed
	int BuildingLimit;   /// # buildings allowed
	int TotalUnitLimit;  /// # total unit number allowed

	int Score = 0;           /// Player score points
	int TotalUnits = 0;
	int TotalBuildings = 0;
	int TotalResources[MaxCosts];
	int TotalRazings = 0;
	int TotalKills = 0;      /// How many units killed
	//Wyrmgus start
	int UnitTypeKills[UnitTypeMax];  /// total killed units of each unit type
	//Wyrmgus end

	//Wyrmgus start
	int LostTownHallTimer = 0;	/// The timer for when the player lost the last town hall (to make the player's units be revealed)
	int HeroCooldownTimer = 0;	/// The cooldown timer for recruiting heroes
	//Wyrmgus end
	
private:
	const stratagus::player_color *player_color = nullptr; /// player color for units and portraits
	QColor minimap_color;           /// color of units on minimap

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
	const stratagus::civilization *get_civilization() const;
	void set_civilization(int civilization);
	stratagus::faction *get_faction() const;
	void SetFaction(const stratagus::faction *faction);
	void SetRandomFaction();
	void SetDynasty(CDynasty *dynasty);
	const std::string &get_interface() const;
	void check_age();
	void set_age(stratagus::age *age);
	CCurrency *GetCurrency() const;
	void ShareUpgradeProgress(CPlayer &player, CUnit &unit);
	int get_player_color_usage_count(const stratagus::player_color *player_color) const;
	bool HasUpgradeClass(const int upgrade_class) const;
	bool HasSettlement(const stratagus::site *settlement) const;
	bool HasSettlementNearWaterZone(int water_zone) const;
	stratagus::site *GetNearestSettlement(const Vec2i &pos, int z, const Vec2i &size) const;
	bool HasUnitBuilder(const CUnitType *type, const stratagus::site *settlement = nullptr) const;
	bool HasUpgradeResearcher(const CUpgrade *upgrade) const;
	bool CanFoundFaction(stratagus::faction *faction, bool pre = false);
	bool CanChooseDynasty(CDynasty *dynasty, bool pre = false);
	bool CanRecruitHero(const CCharacter *character, bool ignore_neutral = false) const;
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
	bool HasMarketUnit() const;
	CUnit *GetMarketUnit() const;
	std::vector<int> GetAutosellResources() const;
	void AutosellResource(const int resource);
	void UpdateLevelUpUnits();
	void update_quest_pool();
	void available_quests_changed();
	void update_current_quests();
	void accept_quest(stratagus::quest *quest);
	void complete_quest(stratagus::quest *quest);
	void fail_quest(stratagus::quest *quest, const std::string &fail_reason = "");
	void remove_current_quest(stratagus::quest *quest);
	bool can_accept_quest(const stratagus::quest *quest);
	bool has_completed_quest(const stratagus::quest *quest);
	std::string has_failed_quest(const stratagus::quest *quest);
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

	/// Does the player have units of a given type
	bool has_unit_type(const CUnitType *unit_type) const;

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
	bool HasBuildingAccess(const CPlayer &player, const ButtonCmd button_action = ButtonCmd::None) const;
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
	unsigned int Enemy = 0;     /// enemy bit field for this player
	unsigned int Allied = 0;    /// allied bit field for this player
	unsigned int SharedVision = 0; /// shared vision bit field

	friend void SetPlayersPalette();
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

enum class faction_tier {
	none,
	barony,
	county,
	duchy,
	grand_duchy,
	kingdom,
	empire,
	
	count
};

enum ForceTypes {
	LandForceType,
	NavalForceType,
	AirForceType,
	
	MaxForceTypes
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

class CForceTemplate
{
public:
	const std::vector<std::pair<const stratagus::unit_class *, int>> &get_units() const
	{
		return this->units;
	}

	void add_unit(const stratagus::unit_class *unit_class, const int quantity)
	{
		this->units.push_back(std::pair<const stratagus::unit_class *, int>(unit_class, quantity));
	}

	int ForceType = -1;
	int Priority = 100;
	int Weight = 1;

private:
	std::vector<std::pair<const stratagus::unit_class *, int>> units;	/// vector containing each unit class belonging to the force template, and the respective quantity
};

class CAiBuildingTemplate
{
public:
	const stratagus::unit_class *get_unit_class() const
	{
		return this->unit_class;
	}

	void set_unit_class(const stratagus::unit_class *unit_class)
	{
		this->unit_class = unit_class;
	}

	int get_priority() const
	{
		return this->priority;
	}

	void set_priority(const int priority)
	{
		this->priority = priority;
	}

	bool is_per_settlement() const
	{
		return this->per_settlement;
	}

	void set_per_settlement(const bool per_settlement)
	{
		this->per_settlement = per_settlement;
	}

private:
	const stratagus::unit_class *unit_class = nullptr; /// Building's unit class
	int priority = 100;
	bool per_settlement = false;	/// Whether the building should be constructed for each settlement
};

class CDynasty
{
public:
	CDynasty() : 
		ID(-1), civilization(-1),
		DynastyUpgrade(nullptr), Conditions(nullptr)
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
	int civilization;													/// dynasty civilization
	IconConfig Icon;													/// Dynasty's icon
	LuaCallback *Conditions;
	std::vector<stratagus::faction *> Factions;									/// to which factions is this dynasty available
};

class LanguageWord
{
public:
	LanguageWord() : 
		Type(-1), Gender(-1), GrammaticalNumber(-1),
		Language(nullptr), DerivesFrom(nullptr),
		Archaic(false),
		Uncountable(false),
		ArticleType(-1),
		Number(-1)
	{
	}
	
	bool HasMeaning(const std::string &meaning);
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
		used_by_civilization_or_faction(false),
		DialectOf(nullptr)
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
	bool used_by_civilization_or_faction;
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
	void Clean();
	//Wyrmgus start
	CDynasty *GetDynasty(const std::string &dynasty_ident) const;
	CLanguage *GetLanguage(const std::string &language_ident) const;
	int get_civilization_class_upgrade(int civilization, int class_id);
	int GetFactionClassUpgrade(int faction, int class_id);
	CLanguage *get_civilization_language(int civilization);
	std::vector<CFiller> get_civilization_ui_fillers(int civilization);
	std::vector<CFiller> GetFactionUIFillers(int faction);
	std::string TranslateName(const std::string &name, CLanguage *language);
	//Wyrmgus end

public:
	//Wyrmgus start
	std::string Species[MAX_RACES];										/// civilization's species (i.e. human)
	std::string civilization_upgrades[MAX_RACES];
	std::map<int, int> civilization_class_upgrades[MAX_RACES];			/// the upgrade slot of a particular class for a particular civilization
	std::map<ButtonCmd, IconConfig> ButtonIcons[MAX_RACES];					/// icons for button actions
	std::vector<int> DevelopsFrom[MAX_RACES];							/// from which civilizations this civilization develops
	std::vector<int> DevelopsTo[MAX_RACES];								/// to which civilizations this civilization develops
	std::vector<CFiller> civilization_ui_fillers[MAX_RACES];
	std::vector<CLanguage *> Languages;									/// languages
	std::vector<CDynasty *> Dynasties;    								/// dynasties
	//Wyrmgus end
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

static constexpr int PlayerNumNeutral = PlayerMax - 1;  /// this is the neutral player slot

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

extern int NumPlayers; //how many player slots used
extern bool NoRescueCheck; //disable rescue check
//Wyrmgus start
extern std::map<std::string, int> DynastyStringToIndex;

extern bool LanguageCacheOutdated;
//Wyrmgus end

extern PlayerRace PlayerRaces;  /// Player races

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
extern CPlayer *GetFactionPlayer(const stratagus::faction *faction);
extern CPlayer *GetOrAddFactionPlayer(const stratagus::faction *faction);
//Wyrmgus end

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

/// register ccl features
extern void PlayerCclRegister();

/// Allowed to select multiple units, maybe not mine
inline bool CanSelectMultipleUnits(const CPlayer &player)
{
	return &player == CPlayer::GetThisPlayer() || CPlayer::GetThisPlayer()->IsTeamed(player);
}

//Wyrmgus start
extern void NetworkSetFaction(int player, const std::string &faction_name);
extern std::string GetFactionTypeNameById(int faction_type);
extern int GetFactionTypeIdByName(const std::string &faction_type);
extern std::string GetGovernmentTypeNameById(int government_type);
extern int GetGovernmentTypeIdByName(const std::string &government_type);
extern std::string GetForceTypeNameById(int force_type);
extern int GetForceTypeIdByName(const std::string &force_type);
extern std::string GetWordTypeNameById(int word_type);
extern int GetWordTypeIdByName(const std::string &word_type);
extern std::string GetArticleTypeNameById(int article_type);
extern int GetArticleTypeIdByName(const std::string &article_type);
extern std::string GetGrammaticalCaseNameById(int grammatical_case);
extern int GetGrammaticalCaseIdByName(const std::string &grammatical_case);
extern std::string GetGrammaticalNumberNameById(int grammatical_number);
extern int GetGrammaticalNumberIdByName(const std::string &grammatical_number);
extern std::string GetGrammaticalPersonNameById(int grammatical_person);
extern int GetGrammaticalPersonIdByName(const std::string &grammatical_person);
extern std::string GetGrammaticalGenderNameById(int grammatical_gender);
extern int GetGrammaticalGenderIdByName(const std::string &grammatical_gender);
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

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern std::map<std::string, CLanguage *> LanguageIdentToPointer;
//Wyrmgus end
