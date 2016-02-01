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
//      (c) Copyright 1998-2005 by Lutz Sammer and Jimmy Salmon
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
#ifndef __ICONS_H__
#include "icons.h"
#endif
//Wyrmgus end

#include "color.h"
//Wyrmgus start
#include "item.h"
//Wyrmgus end
#include "upgrade_structs.h"

#include "vec2i.h"

class CGraphic;
//Wyrmgus start
class CCharacter;
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

#define STORE_OVERALL 0
#define STORE_BUILDING 1
#define STORE_BOTH 2

#define SPEEDUP_FACTOR 100
/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnit;
class CUnitType;
class PlayerAi;
class CFile;
struct lua_State;

/*----------------------------------------------------------------------------
--  Player type
----------------------------------------------------------------------------*/

enum _diplomacy_ {
	DiplomacyAllied,   /// Ally with opponent
	DiplomacyNeutral,  /// Don't attack be neutral
	DiplomacyEnemy,    /// Attack opponent
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
	int Faction;		/// faction of player
	//Wyrmgus end
	std::string AiName; /// AI for computer

	// friend enemy detection
	int      Team;          /// team of player

	Vec2i StartPos;  /// map tile start position

	inline void SetStartView(const Vec2i &pos) { StartPos = pos; }

	int Resources[MaxCosts];      /// resources in overall store
	int MaxResources[MaxCosts];   /// max resources can be stored
	int StoredResources[MaxCosts];/// resources in store buildings (can't exceed MaxResources)
	int LastResources[MaxCosts];  /// last values for revenue
	int Incomes[MaxCosts];        /// income of the resources
	int Revenue[MaxCosts];        /// income rate of the resources

	int SpeedResourcesHarvest[MaxCosts]; /// speed factor for harvesting resources
	int SpeedResourcesReturn[MaxCosts];  /// speed factor for returning resources
	int SpeedBuild;                  /// speed factor for building
	int SpeedTrain;                  /// speed factor for training
	int SpeedUpgrade;                /// speed factor for upgrading
	int SpeedResearch;               /// speed factor for researching

	// FIXME: shouldn't use the constant
	int UnitTypesCount[UnitTypeMax];  /// total units of unit-type
	int UnitTypesAiActiveCount[UnitTypeMax];  /// total units of unit-type that have their AI set to active
	//Wyrmgus start
	int UnitTypesNonHeroCount[UnitTypeMax];		/// total units of unit-type that isn't associated to a character
	std::vector<std::string> Heroes;			/// characters owned by this player
	//Wyrmgus end

	bool AiEnabled;        /// handle AI on local computer
	//Wyrmgus start
	bool Revealed;			/// Whether the player has been revealed (i.e. after losing the last town hall)
	//Wyrmgus end
	PlayerAi *Ai;          /// Ai structure pointer

	int    NumBuildings;   /// # buildings
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
	int    TotalKills;      /// How many unit killed
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
	void SetFaction(const std::string faction_name);
	void SetRandomFaction();
	bool IsPlayerColorUsed(int color);
	bool HasUpgradeClass(std::string upgrade_class_name);
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
	void UpdateLevelUpUnits();
	//Wyrmgus end

	/// Get a resource of the player
	int GetResource(const int resource, const int type);
	/// Adds/subtracts some resources to/from the player store
	void ChangeResource(const int resource, const int value, const bool store = false);
	/// Set a resource of the player
	void SetResource(const int resource, const int value, const int type = STORE_OVERALL);
	/// Check, if there enough resources for action.
	bool CheckResource(const int resource, const int value);

	/// Returns count of specified unittype
	int GetUnitTotalCount(const CUnitType &type) const;
	/// Check if the unit-type didn't break any unit limits and supply/demand
	int CheckLimits(const CUnitType &type) const;

	/// Check if enough resources are available for costs
	int CheckCosts(const int *costs, bool notify = true) const;
	/// Check if enough resources are available for a new unit-type
	int CheckUnitType(const CUnitType &type) const;

	/// Add costs to the resources
	void AddCosts(const int *costs);
	/// Add costs for an unit-type to the resources
	void AddUnitType(const CUnitType &type);
	/// Add a factor of costs to the resources
	void AddCostsFactor(const int *costs, int factor);
	/// Remove costs from the resources
	void SubCosts(const int *costs);
	/// Remove costs for an unit-type from the resources
	void SubUnitType(const CUnitType &type);
	/// Remove a factor of costs from the resources
	void SubCostsFactor(const int *costs, int factor);

	/// Does the player have units of that type
	int HaveUnitTypeByType(const CUnitType &type) const;
	/// Does the player have units of that type
	int HaveUnitTypeByIdent(const std::string &ident) const;

	/// Notify player about a problem
	void Notify(int type, const Vec2i &pos, const char *fmt, ...) const PRINTF_VAARG_ATTRIBUTE(4, 5); // Don't forget to count this
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

	void SetDiplomacyNeutralWith(const CPlayer &player);
	void SetDiplomacyAlliedWith(const CPlayer &player);
	void SetDiplomacyEnemyWith(const CPlayer &player);
	void SetDiplomacyCrazyWith(const CPlayer &player);

	void ShareVisionWith(const CPlayer &player);
	void UnshareVisionWith(const CPlayer &player);

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
	GovernmentTypeMonarchy,
	GovernmentTypeRepublic,
	GovernmentTypeTheocracy,
	
	MaxGovernmentTypes
};

enum FactionTiers {
	FactionTierBarony,
	FactionTierCounty,
	FactionTierDuchy,
	FactionTierGrandDuchy,
	FactionTierKingdom,
	FactionTierEmpire,
	
	MaxFactionTiers
};

class CFaction
{
public:
	CFaction() : 
		ID(-1), Civilization(-1), DefaultTier(FactionTierBarony), ParentFaction(-1), Language(-1),
		Playable(true) //factions are playable by default
	{
	}

	std::string Name;													/// faction name
	std::string Type;													/// faction type (tribe or polity)
	std::string FactionUpgrade;											/// faction upgrade applied when the faction is set
	int ID;																/// faction ID
	int Civilization;													/// faction civilization
	int DefaultTier;													/// default faction tier
	int ParentFaction;													/// parent faction of this faction
	int Language;
	bool Playable;														/// faction playability
	std::vector<int> Colors;											/// faction colors
	std::vector<std::string> DevelopsTo;								/// to which factions this faction can develop
	std::string Titles[MaxGovernmentTypes][MaxFactionTiers];			/// this faction's title for each government type and faction tier
};

class CDeity
{
public:
	CDeity() :
		Civilization(-1), Gender(0),
		Major(false),
		Name(""), UpgradeIdent(""), Portfolio(""), ParentDeity("")
	{
	}
	
	int Civilization;	/// Culture to which the deity belongs
	int Gender;			/// Deity's gender
	bool Major;			/// Whether the deity is a major one or not
	std::string Name;	/// Name of the deity
	std::string UpgradeIdent;	/// Ident of the upgrade applied by the deity
	std::string Portfolio; ///Portfolio of the deity
	std::string ParentDeity;	/// Parent deity of this deity (deity on which it is based on); example: Wodanaz (Germanic) is the parent deity of Odin (Norse) and Woden (teuton)
	std::string Description;
	std::string Background;
	std::string Quote;
};

class LanguageWord
{
public:
	LanguageWord() : 
		Word("")
	{
	}
	
	bool HasTypeName(std::string type);
	bool HasPrefixTypeName(std::string type);
	bool HasSuffixTypeName(std::string type);
	bool HasInfixTypeName(std::string type);
	bool HasSeparatePrefixTypeName(std::string type);
	bool HasSeparateSuffixTypeName(std::string type);
	bool HasSeparateInfixTypeName(std::string type);
	bool HasMeaning(std::string meaning);

	std::string Word;				/// Word name / ID.
	std::vector<std::string> Meanings;			/// Meanings of the word in English.
	std::vector<std::string> TypeName;
	std::vector<std::string> PrefixTypeName;
	std::vector<std::string> SuffixTypeName;
	std::vector<std::string> InfixTypeName;
	std::vector<std::string> SeparatePrefixTypeName;
	std::vector<std::string> SeparateSuffixTypeName;
	std::vector<std::string> SeparateInfixTypeName;
};

class LanguageNoun : public LanguageWord
{
public:
	LanguageNoun() : LanguageWord(),
		Uncountable(false),
		NameSingular(false), NamePlural(false),
		PrefixSingular(false), PrefixPlural(false),
		SuffixSingular(false), SuffixPlural(false),
		InfixSingular(false), InfixPlural(false)
	{
	}

	std::string Verb;				/// Equivalent verb, if any.
	std::string Adjective;			/// Equivalent adjective, if any.
	std::string SingularNominative;
	std::string SingularAccusative;
	std::string SingularDative;
	std::string SingularGenitive;
	std::string PluralNominative;
	std::string PluralAccusative;
	std::string PluralDative;
	std::string PluralGenitive;
	std::string Gender;				/// What is the gender of the noun (Male, Female or Neutral)
	bool Uncountable;				/// Whether the noun is uncountable or not.
	bool NameSingular;				/// Whether the noun's singular form can be used as a name
	bool NamePlural;				/// Whether the noun's plural form can be used as a name
	bool PrefixSingular;			/// Whether the noun's singular form can be used as a prefix
	bool PrefixPlural;				/// Whether the noun's plural form can be used as a prefix
	bool SuffixSingular;			/// Whether the noun's singular form can be used as a suffix
	bool SuffixPlural;				/// Whether the noun's plural form can be used as a suffix
	bool InfixSingular;				/// Whether the noun's singular form can be used as an infix
	bool InfixPlural;				/// Whether the noun's plural form can be used as an infix
};

class LanguageVerb : public LanguageWord
{
public:
	LanguageVerb() : LanguageWord()
	{
	}

	std::string Noun;								/// Equivalent noun, if any.
	std::string Adjective;							/// Equivalent adjective, if any.
	std::string Infinitive;
	std::string SingularFirstPersonPresent;
	std::string SingularSecondPersonPresent;
	std::string SingularThirdPersonPresent;
	std::string PluralFirstPersonPresent;
	std::string PluralSecondPersonPresent;
	std::string PluralThirdPersonPresent;
	std::string SingularFirstPersonPresentSubjunctive;
	std::string SingularSecondPersonPresentSubjunctive;
	std::string SingularThirdPersonPresentSubjunctive;
	std::string PluralFirstPersonPresentSubjunctive;
	std::string PluralSecondPersonPresentSubjunctive;
	std::string PluralThirdPersonPresentSubjunctive;
	std::string SingularFirstPersonPast;
	std::string SingularSecondPersonPast;
	std::string SingularThirdPersonPast;
	std::string PluralFirstPersonPast;
	std::string PluralSecondPersonPast;
	std::string PluralThirdPersonPast;
	std::string SingularFirstPersonFuture;
	std::string SingularSecondPersonFuture;
	std::string SingularThirdPersonFuture;
	std::string PluralFirstPersonFuture;
	std::string PluralSecondPersonFuture;
	std::string PluralThirdPersonFuture;
	std::string ParticiplePresent;
	std::string ParticiplePast;
};

class LanguageAdjective : public LanguageWord
{
public:
	LanguageAdjective() : LanguageWord()
	{
	}

	std::string Noun;				/// Equivalent noun, if any.
	std::string Verb;				/// Equivalent verb, if any.
	std::string Positive;			/// Positive form of the adjective.
	std::string Comparative;		/// Comparative form of the adjective.
	std::string Superlative;		/// Superlative form of the adjective.
	std::string PositivePlural;		/// Positive plural form of the adjective.
	std::string ComparativePlural;	/// Comparative plural form of the adjective.
	std::string SuperlativePlural;	/// Superlative plural form of the adjective.
};

class LanguagePronoun : public LanguageWord
{
public:
	LanguagePronoun() : LanguageWord()
	{
	}

	std::string Nominative;			/// Nominative case for the pronoun (if any)
	std::string Accusative;			/// Accusative case for the pronoun (if any)
	std::string Dative;				/// Dative case for the pronoun (if any)
	std::string Genitive;			/// Genitive case for the pronoun (if any)
};

class LanguageAdverb : public LanguageWord
{
public:
	LanguageAdverb() : LanguageWord()
	{
	}

	std::string Adjective;			/// Equivalent adjective, if any (i.e. "beautifully"'s equivalent adjective would be "beautiful".
};

class LanguageConjunction : public LanguageWord
{
public:
	LanguageConjunction() : LanguageWord()
	{
	}

};

class LanguageAdposition : public LanguageWord
{
public:
	LanguageAdposition() : LanguageWord()
	{
	}

};

class LanguageArticle : public LanguageWord
{
public:
	LanguageArticle() : LanguageWord(),
		Definite(false)
	{
	}

	std::string Nominative;			/// Nominative case for the article
	std::string Accusative;			/// Accusative case for the article
	std::string Dative;				/// Dative case for the article
	std::string Genitive;			/// Genitive case for the article
	std::string Gender;				/// Gender of the article
	bool Definite;					/// Whether the article is definite
};

class LanguageNumeral : public LanguageWord
{
public:
	LanguageNumeral() : LanguageWord(),
		Number(-1)
	{
	}

	int Number;
};

class CLanguage
{
public:
	CLanguage() :
		Ident(""), Name("")
	{
	}
	
	std::string GetArticle(std::string gender, std::string grammatical_case, bool definite);
	
	std::string Ident;	/// Ident of the language
	std::string Name;	/// Name of the language
	std::vector<LanguageNoun *> LanguageNouns;								/// nouns of the language
	std::vector<LanguageVerb *> LanguageVerbs;								/// verbs of the language
	std::vector<LanguageAdjective *> LanguageAdjectives;					/// adjectives of the language
	std::vector<LanguagePronoun *> LanguagePronouns;						/// pronouns of the language
	std::vector<LanguageAdverb *> LanguageAdverbs;							/// adverbs of the language
	std::vector<LanguageConjunction *> LanguageConjunctions;				/// conjunctions of the language
	std::vector<LanguageAdposition *> LanguageAdpositions;					/// adpositions of the language
	std::vector<LanguageArticle *> LanguageArticles;						/// articles of the language
	std::vector<LanguageNumeral *> LanguageNumerals;						/// numerals of the language
	std::string NameTranslations[PersonalNameMax][2];		/// name translations (2 values: one for the name to be translated, and another for the translation)
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
		for (int i = 0; i < MAX_RACES; ++i) {
			ParentCivilization[i] = -1;
			for (int j = 0; j < UnitTypeClassMax; ++j) {
				CivilizationClassUnitTypes[i][j] = -1;
				CivilizationClassUpgrades[i][j] = -1;
			}
			for (int j = 0; j < FactionMax; ++j) {
				for (int k = 0; k < UnitTypeClassMax; ++k) {
					FactionClassUnitTypes[i][j][k] = -1;
					FactionClassUpgrades[i][j][k] = -1;
				}
			}
		}
		memset(Playable, 0, sizeof(Playable));
		memset(CivilizationLanguage, -1, sizeof(CivilizationLanguage));
		//Wyrmgus end
	}

	void Clean();
	int GetRaceIndexByName(const char *raceName) const;
	//Wyrmgus start
	int GetFactionIndexByName(const int civilization, const std::string faction_name) const;
	int GetDeityIndexByName(const int civilization, std::string deity_name) const;
	int GetLanguageIndexByIdent(std::string language_ident) const;
	int GetCivilizationClassUnitType(int civilization, int class_id);
	int GetCivilizationClassUpgrade(int civilization, int class_id);
	int GetFactionClassUnitType(int civilization, int faction, int class_id);
	int GetFactionClassUpgrade(int civilization, int faction, int class_id);
	int GetCivilizationLanguage(int civilization);
	int GetFactionLanguage(int civilization, int faction);
	std::string TranslateName(std::string name, int language);
	//Wyrmgus end

public:
	bool Visible[MAX_RACES];        /// race should be visible in pulldown
	std::string Name[MAX_RACES];    /// race names
	std::string Display[MAX_RACES]; /// text to display in pulldown
	//Wyrmgus start
	int CivilizationClassUnitTypes[MAX_RACES][UnitTypeClassMax];		/// the unit type slot of a particular class for a particular civilization
	int CivilizationClassUpgrades[MAX_RACES][UnitTypeClassMax];			/// the upgrade slot of a particular class for a particular civilization
	int FactionClassUnitTypes[MAX_RACES][FactionMax][UnitTypeClassMax];	/// the unit type slot of a particular class for a particular faction
	int FactionClassUpgrades[MAX_RACES][FactionMax][UnitTypeClassMax];	/// the upgrade slot of a particular class for a particular faction
	bool Playable[MAX_RACES];											/// civilization is playable?
	std::string Species[MAX_RACES];										/// civilization's species (i.e. human)
	std::string DefaultColor[MAX_RACES];								/// name of the civilization's default color (used for the encyclopedia, tech tree, etc.)
	int ParentCivilization[MAX_RACES];									/// civilization's parent civilization, if any
	CFaction *Factions[MAX_RACES][FactionMax];    						/// factions
	std::vector<CDeity *> Deities[MAX_RACES];							/// deities
	std::string PersonalNames[MAX_RACES][PersonalNameMax];				/// personal names
	std::string PersonalNamePrefixes[MAX_RACES][PersonalNameMax];		/// personal name prefixes
	std::string PersonalNameSuffixes[MAX_RACES][PersonalNameMax];		/// personal name suffixes
	std::string ProvinceNames[MAX_RACES][PersonalNameMax];				/// province names
	std::string ProvinceNamePrefixes[MAX_RACES][PersonalNameMax];		/// province name prefixes
	std::string ProvinceNameSuffixes[MAX_RACES][PersonalNameMax];		/// province name suffixes
	std::string SettlementNames[MAX_RACES][PersonalNameMax];			/// settlement names
	std::string SettlementNamePrefixes[MAX_RACES][PersonalNameMax];		/// settlement name prefixes
	std::string SettlementNameSuffixes[MAX_RACES][PersonalNameMax];		/// settlement name suffixes
	int CivilizationLanguage[MAX_RACES];
	std::vector<CLanguage *> Languages;									/// languages
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


/// Initialize the computer opponent AI
extern void PlayersInitAi();
/// Called each game cycle for player handlers (AI)
extern void PlayersEachCycle();
/// Called each second for a given player handler (AI)
extern void PlayersEachSecond(int player);

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
extern void SetFactionStringToIndex(int civilization, std::string faction_name, int faction_id);
extern void NetworkSetFaction(int player, std::string faction_name);
extern std::string GetFactionEffectsString(std::string civilization_name, std::string faction_name);
extern std::string GetGovernmentTypeNameById(int government_type);
extern int GetGovernmentTypeIdByName(std::string government_type);
//Wyrmgus end

//@}

#endif // !__PLAYER_H__
