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
#include "detailed_data_element.h"
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
class CGender;
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
class CWord;
class ItemSlot;
class LuaCallback;
struct lua_State;

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

class CCharacter : public DetailedDataElement, public DataType<CCharacter>
{
	DATA_TYPE(CCharacter, DetailedDataElement)
	
public:
	CCharacter()
	{
		memset(Attributes, 0, sizeof(Attributes));
	}
	
	~CCharacter();
	
	static constexpr const char *ClassIdentifier = "character";
	
	static void Clear();
	static void GenerateCharacterHistory();		/// Generates history for characters
	static void ResetCharacterHistory();		/// Removes generated history data from characters
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	void GenerateHistory();
	void ResetHistory();
	void SaveHistory();
	void GenerateMissingDates();
	int GetMartialAttribute() const;
	int GetAttributeModifier(int attribute) const;
	
	const String &GetExtraName() const
	{
		return this->ExtraName;
	}
	
	const String &GetFamilyName() const
	{
		return this->FamilyName;
	}
	
	CCivilization *GetCivilization() const
	{
		return this->Civilization;
	}
	
	const CFaction *GetFaction() const
	{
		return this->Faction;
	}
	
	const CGender *GetGender() const
	{
		return this->Gender;
	}
	
	CReligion *GetReligion() const;
	CLanguage *GetLanguage() const;
	CCalendar *GetCalendar() const;
	
	const CUnitType *GetUnitType() const
	{
		return this->UnitType;
	}
	
	int GetLevel() const
	{
		return this->Level;
	}
	
	CCharacter *GetFather() const
	{
		return this->Father;
	}
	
	CCharacter *GetMother() const
	{
		return this->Mother;
	}
	
	const std::vector<CCharacter *> &GetChildren() const
	{
		return this->Children;
	}
	
	const std::vector<CCharacter *> &GetSiblings() const
	{
		return this->Siblings;
	}
	
	bool IsParentOf(const CCharacter *character) const;
	bool IsChildOf(const CCharacter *character) const;
	bool IsSiblingOf(const CCharacter *character) const;
	bool IsItemEquipped(const CPersistentItem *item) const;
	bool IsUsable() const;
	bool CanAppear(bool ignore_neutral = false) const;
	bool CanWorship() const;
	bool HasMajorDeity() const;
	String GetFullName() const;
	virtual CIcon *GetIcon() const override;
	CPersistentItem *GetItem(const CUnit *item) const;
	void UpdateAttributes();
	void SaveHistory(CFile &file);		/// Save generated history data for the character
	
	const std::vector<const CHistoricalLocation *> &GetHistoricalLocations() const
	{
		return this->HistoricalLocations;
	}

	CDate BirthDate;			/// Date in which the character was born
	CDate StartDate;			/// Date in which the character historically starts being active
	CDate DeathDate;			/// Date in which the character historically died
	CCivilization *Civilization = nullptr;	/// Culture to which the character belongs
private:
	const CFaction *Faction = nullptr;	/// Faction to which the character belongs
	const CGender *Gender = nullptr;	/// the character's gender
public:
	int Level = 0;				/// Character's level
	int ExperiencePercent = 0;	/// Character's experience, as a percentage of the experience required to level up
	bool ViolentDeath = false;	/// If historical death was violent
	bool Custom = false;		/// Whether this character is a custom hero
private:
	CWord *NameWord = nullptr;	/// the word for the character's name
	String ExtraName;			/// Extra given names of the character (used if necessary to differentiate from existing heroes)
	String FamilyName;			/// Name of the character's family
	CWord *FamilyNameWord = nullptr;	/// the word for the character's family name
public:
	std::string HairVariation;	/// Name of the character's hair variation
	IconConfig Icon;			/// Character's icon
	IconConfig HeroicIcon;		/// Character's heroic icon (level 3 and upper)
	const CUnitType *UnitType = nullptr;
	const CUpgrade *Trait = nullptr;
	CDeity *Deity = nullptr;			/// The deity which the character is (if it is a deity)
private:
	CCharacter *Father = nullptr;		/// Character's father
	CCharacter *Mother = nullptr;		/// Character's mother
public:
	LuaCallback *Conditions = nullptr;
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;
	std::map<const ItemSlot *, std::vector<CPersistentItem *>> EquippedItems;	/// Equipped items of the character, per slot
private:
	std::vector<CCharacter *> Children;	/// Children of the character
	std::vector<CCharacter *> Siblings;	/// Siblings of the character
public:
	std::vector<CFaction *> Factions;	/// Factions for which this character is available; if empty, this means all factions of the character's civilization can recruit them
	std::vector<CDeity *> Deities;		/// Deities chosen by this character to worship
	std::vector<CDeity *> GeneratedDeities;		/// Deities picked during history generation for this character to worship
	std::vector<CDeityDomain *> PreferredDeityDomains;	/// Preferred deity domains for the character, used to generate deities for it if any are lacking
	std::vector<const CUpgrade *> Abilities;
	std::vector<const CUpgrade *> ReadWorks;
	std::vector<const CUpgrade *> ConsumedElixirs;
	std::vector<CUpgrade *> AuthoredWorks;	/// Literary works of which this character is the author
	std::vector<CUpgrade *> LiteraryAppearances;	/// Literary works in which this character appears
	std::vector<CQuest *> QuestsInProgress;	/// Quests in progress, only for playable, custom characters
	std::vector<CQuest *> QuestsCompleted;	/// Quests completed, only for playable, custom characters
	std::vector<CPersistentItem *> Items;
	int Attributes[MaxAttributes];
	std::vector<const CUnitType *> ForbiddenUpgrades;	/// which unit types this character is forbidden to upgrade to
	std::vector<std::pair<CDate, CFaction *>> HistoricalFactions;	/// historical factions of the character, with the date of the character's faction change
private:
	std::vector<const CHistoricalLocation *> HistoricalLocations;	/// historical locations of the character
public:
	std::vector<std::tuple<CDate, CDate, CFaction *, int>> HistoricalTitles;	/// historical titles of the character, the first element is the beginning date of the term, the second one the end date, the third the faction it pertains to (if any, if not then it is null), and the fourth is the character title itself (from the character title enums)
	std::vector<std::tuple<int, int, CProvince *, int>> HistoricalProvinceTitles;
	
	friend int CclDefineCharacter(lua_State *l);
	friend int CclDefineCustomHero(lua_State *l);
	friend void ChangeCustomHeroCivilization(const std::string &hero_full_name, const std::string &civilization_name, const std::string &new_hero_name, const std::string &new_hero_family_name);

protected:
	static void _bind_methods();
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
extern std::string GetCharacterTitleNameById(int title);
extern int GetCharacterTitleIdByName(const std::string &title);
extern bool IsMinisterialTitle(int title);
extern void CharacterCclRegister();

#endif
