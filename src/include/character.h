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
/**@name character.h - The character header file. */
//
//      (c) Copyright 2015-2019 by Andrettin
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

#ifndef __CHARACTER_H__
#define __CHARACTER_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "item.h"
#include "time/date.h"
#include "ui/icon_config.h"

#include <tuple>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCalendar;
class CCivilization;
class CDeity;
class CDeityDomain;
class CDependency;
class CFaction;
class CFile;
class CHistoricalLocation;
class CMapTemplate;
class CLanguage;
class CPersistentItem;
class CProvince;
class CQuest;
class CReligion;
class CSite;
class CUnitType;
class CUnit;
class CUpgrade;
class LuaCallback;

/**
**  Indexes into gender array.
*/
enum Genders {
	NoGender,
	MaleGender,
	FemaleGender,
	AsexualGender, //i.e. slimes reproduce asexually

	MaxGenders
};

enum Attributes {
	StrengthAttribute,
	DexterityAttribute,
	IntelligenceAttribute,
	CharismaAttribute,

	MaxAttributes
};

/**
**  Indexes into character title array.
*/
enum CharacterTitles {
	CharacterTitleHeadOfState, // also used for titulars to aristocratic titles which were formal only; for example: the French duke of Orléans did not rule over Orléans, but here we consider the "head of state" title to also encompass such cases
	CharacterTitleHeadOfGovernment,
	CharacterTitleEducationMinister,
	CharacterTitleFinanceMinister,
	CharacterTitleForeignMinister,
	CharacterTitleIntelligenceMinister,
	CharacterTitleInteriorMinister,
	CharacterTitleJusticeMinister,
	CharacterTitleWarMinister,
	
	CharacterTitleGovernor,
	CharacterTitleMayor,

	MaxCharacterTitles
};

class CCharacter : public CDataType
{
public:
	CCharacter()
	{
		memset(Attributes, 0, sizeof(Attributes));
	}
	
	static CCharacter *GetCharacter(const std::string &ident, const bool should_find = true);
	static CCharacter *GetOrAddCharacter(const std::string &ident);
	static void ClearCharacters();
	static void GenerateCharacterHistory();		/// Generates history for characters
	static void ResetCharacterHistory();		/// Removes generated history data from characters
	
	static std::vector<CCharacter *> Characters;
	static std::map<std::string, CCharacter *> CharactersByIdent;
	
	~CCharacter();
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	void GenerateHistory();
	void ResetHistory();
	void SaveHistory();
	void GenerateMissingDates();
	int GetMartialAttribute() const;
	int GetAttributeModifier(int attribute) const;
	CReligion *GetReligion() const;
	CLanguage *GetLanguage() const;
	CCalendar *GetCalendar() const;
	bool IsParentOf(const std::string &child_full_name) const;
	bool IsChildOf(const std::string &parent_full_name) const;
	bool IsSiblingOf(const std::string &sibling_full_name) const;
	bool IsItemEquipped(const CPersistentItem *item) const;
	bool IsUsable() const;
	bool CanAppear(bool ignore_neutral = false) const;
	bool CanWorship() const;
	bool HasMajorDeity() const;
	std::string GetFullName() const;
	IconConfig GetIcon() const;
	CPersistentItem *GetItem(CUnit &item) const;
	void UpdateAttributes();
	void SaveHistory(CFile &file);		/// Save generated history data for the character

	CDate BirthDate;			/// Date in which the character was born
	CDate StartDate;			/// Date in which the character historically starts being active
	CDate DeathDate;			/// Date in which the character historically died
	CCivilization *Civilization = nullptr;	/// Culture to which the character belongs
	const CFaction *Faction = nullptr;	/// Faction to which the character belongs
	int Gender = 0;				/// Character's gender
	int Level = 0;				/// Character's level
	int ExperiencePercent = 0;	/// Character's experience, as a percentage of the experience required to level up
	bool ViolentDeath = false;	/// If historical death was violent
	bool Custom = false;		/// Whether this character is a custom hero
	bool Initialized = false;	/// Whether the character has already been initialized
	std::string Name;			/// Given name of the character
	std::string ExtraName;		/// Extra given names of the character (used if necessary to differentiate from existing heroes)
	std::string FamilyName;		/// Name of the character's family
	std::string Description;	/// Description of the character from an in-game universe perspective
	std::string Background;		/// Description of the character from a perspective outside of the game's universe
	std::string Quote;			/// A quote relating to the character
	std::string HairVariation;	/// Name of the character's hair variation
	IconConfig Icon;					/// Character's icon
	IconConfig HeroicIcon;				/// Character's heroic icon (level 3 and upper)
	CUnitType *Type = nullptr;
	const CUpgrade *Trait = nullptr;
	CDeity *Deity = nullptr;			/// The deity which the character is (if it is a deity)
	CCharacter *Father = nullptr;		/// Character's father
	CCharacter *Mother = nullptr;		/// Character's mother
	LuaCallback *Conditions = nullptr;
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;
	std::vector<CPersistentItem *> EquippedItems[MaxItemSlots];	/// Equipped items of the character, per slot
	std::vector<CCharacter *> Children;	/// Children of the character
	std::vector<CCharacter *> Siblings;	/// Siblings of the character
	std::vector<CFaction *> Factions;	/// Factions for which this character is available; if empty, this means all factions of the character's civilization can recruit them
	std::vector<CDeity *> Deities;		/// Deities chosen by this character to worship
	std::vector<CDeity *> GeneratedDeities;		/// Deities picked during history generation for this character to worship
	std::vector<CDeityDomain *> PreferredDeityDomains;	/// Preferred deity domains for the character, used to generate deities for it if any are lacking
	std::vector<CUpgrade *> Abilities;
	std::vector<CUpgrade *> ReadWorks;
	std::vector<CUpgrade *> ConsumedElixirs;
	std::vector<CUpgrade *> AuthoredWorks;	/// Literary works of which this character is the author
	std::vector<CUpgrade *> LiteraryAppearances;	/// Literary works in which this character appears
	std::vector<CQuest *> QuestsInProgress;	/// Quests in progress, only for playable, custom characters
	std::vector<CQuest *> QuestsCompleted;	/// Quests completed, only for playable, custom characters
	std::vector<CPersistentItem *> Items;
	int Attributes[MaxAttributes];
	std::vector<CUnitType *> ForbiddenUpgrades;	/// which unit types this character is forbidden to upgrade to
	std::vector<std::pair<CDate, CFaction *>> HistoricalFactions;	/// historical locations of the character; the values are: date, faction
	std::vector<CHistoricalLocation *> HistoricalLocations;	/// historical locations of the character
	std::vector<std::tuple<CDate, CDate, CFaction *, int>> HistoricalTitles;	/// historical titles of the character, the first element is the beginning date of the term, the second one the end date, the third the faction it pertains to (if any, if not then it is null), and the fourth is the character title itself (from the character title enums)
	std::vector<std::tuple<int, int, CProvince *, int>> HistoricalProvinceTitles;
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::map<std::string, CCharacter *> CustomHeroes;
extern CCharacter *CurrentCustomHero;
extern bool LoadingPersistentHeroes;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern int GetAttributeVariableIndex(int attribute);
extern CCharacter *GetCustomHero(const std::string &hero_ident);
extern void SaveHero(CCharacter *hero);
extern void SaveHeroes();
extern void HeroAddQuest(const std::string &hero_ident, const std::string &quest_ident);
extern void HeroCompleteQuest(const std::string &hero_ident, const std::string &quest_ident);
extern void SaveCustomHero(const std::string &hero_ident);
extern void DeleteCustomHero(const std::string &hero_ident);
extern void SetCurrentCustomHero(const std::string &hero_ident);
extern std::string GetCurrentCustomHero();
extern void ChangeCustomHeroCivilization(const std::string &hero_name, const std::string &civilization_ident, const std::string &new_hero_name, const std::string &new_hero_family_name);
extern bool IsNameValidForCustomHero(const std::string &hero_name, const std::string &hero_family_name);
extern std::string GetGenderNameById(int gender);
extern int GetGenderIdByName(const std::string &gender);
extern std::string GetCharacterTitleNameById(int title);
extern int GetCharacterTitleIdByName(const std::string &title);
extern bool IsMinisterialTitle(int title);
extern void CharacterCclRegister();

#endif
