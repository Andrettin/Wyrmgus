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
//      (c) Copyright 2015-2020 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "data_type.h"
#include "item.h"
#include "time/date.h"
#include "ui/icon.h"

class CDeity;
class CDeityDomain;
class CFile;
class CLanguage;
class CPersistentItem;
class CProvince;
class CReligion;
class CUnitType;
class CUnit;
class CUpgrade;
class LuaCallback;
class Spell_Polymorph;
struct lua_State;

void ChangeCustomHeroCivilization(const std::string &hero_full_name, const std::string &civilization_name, const std::string &new_hero_name, const std::string &new_hero_family_name);
int CclDefineCharacter(lua_State *l);
int CclDefineCustomHero(lua_State *l);

namespace stratagus {
	class calendar;
	class civilization;
	class dependency;
	class faction;
	class historical_location;
	class quest;
	class site;
}

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

namespace stratagus {

class character : public detailed_data_entry, public data_type<character>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(QString surname READ get_surname_qstring)
	Q_PROPERTY(CUnitType* unit_type READ get_unit_type WRITE set_unit_type)
	Q_PROPERTY(stratagus::civilization* civilization MEMBER civilization READ get_civilization)
	Q_PROPERTY(stratagus::faction* faction MEMBER faction READ get_faction)
	Q_PROPERTY(stratagus::site* home_settlement MEMBER home_settlement)
	Q_PROPERTY(QString variation READ get_variation_qstring)

public:
	static constexpr const char *class_identifier = "character";
	static constexpr const char *database_folder = "characters";

	static void clear();
	
	character(const std::string &identifier);
	~character();
	
	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	virtual void check() const override;

	const std::string &get_surname() const
	{
		return this->surname;
	}

	Q_INVOKABLE void set_surname(const std::string &surname)
	{
		this->surname = surname;
	}

	QString get_surname_qstring() const
	{
		return QString::fromStdString(this->get_surname());
	}

	CUnitType *get_unit_type() const
	{
		return this->unit_type;
	}

	void set_unit_type(CUnitType *unit_type)
	{
		if (unit_type == this->get_unit_type()) {
			return;
		}

		this->unit_type = unit_type;
	}

	civilization *get_civilization() const
	{
		return this->civilization;
	}

	faction *get_faction() const
	{
		return this->faction;
	}

	void GenerateMissingDates();
	int GetMartialAttribute() const;
	int GetAttributeModifier(int attribute) const;
	CReligion *GetReligion() const;
	CLanguage *GetLanguage() const;
	calendar *get_calendar() const;
	bool IsParentOf(const std::string &child_full_name) const;
	bool IsChildOf(const std::string &parent_full_name) const;
	bool IsSiblingOf(const std::string &sibling_full_name) const;
	bool IsItemEquipped(const CPersistentItem *item) const;
	bool IsUsable() const;
	bool CanAppear(bool ignore_neutral = false) const;
	bool CanWorship() const;
	bool HasMajorDeity() const;
	std::string GetFullName() const;

	const std::string &get_variation() const
	{
		return this->variation;
	}

	Q_INVOKABLE void set_variation(const std::string &variation)
	{
		this->variation = FindAndReplaceString(variation, "_", "-");
	}

	QString get_variation_qstring() const
	{
		return QString::fromStdString(this->get_variation());
	}

	IconConfig GetIcon() const;
	CPersistentItem *GetItem(CUnit &item) const;
	void UpdateAttributes();

	CDate BirthDate;			/// Date in which the character was born
	CDate StartDate;			/// Date in which the character historically starts being active
	CDate DeathDate;			/// Date in which the character historically died
private:
	civilization *civilization = nullptr;	/// Culture to which the character belongs
	faction *faction = nullptr;	/// Faction to which the character belongs
public:
	int Gender = 0;				/// Character's gender
	int Level = 0;				/// Character's level
	int ExperiencePercent = 0;	/// Character's experience, as a percentage of the experience required to level up
	bool Custom = false;		/// Whether this character is a custom hero
	std::string ExtraName;		/// Extra given names of the character (used if necessary to differentiate from existing heroes)
private:
	std::string surname; //the character's surname
	std::string variation; //the identifier of the character variation
public:
	IconConfig Icon;					/// Character's icon
	IconConfig HeroicIcon;				/// Character's heroic icon (level 3 and upper)
private:
	CUnitType *unit_type = nullptr;
public:
	CUpgrade *Trait = nullptr;
	CDeity *Deity = nullptr;			/// The deity which the character is (if it is a deity)
	character *Father = nullptr;		/// Character's father
	character *Mother = nullptr;		/// Character's mother
	LuaCallback *Conditions = nullptr;
	dependency *Predependency = nullptr;
	dependency *Dependency = nullptr;
	std::vector<CPersistentItem *> EquippedItems[MaxItemSlots];	/// Equipped items of the character, per slot
	std::vector<character *> Children;	/// Children of the character
	std::vector<character *> Siblings;	/// Siblings of the character
private:
	site *home_settlement = nullptr; //the home settlement of this character, where they can preferentially be recruited
public:
	std::vector<CDeity *> Deities;		/// Deities chosen by this character to worship
	std::vector<CUpgrade *> Abilities;
	std::vector<CUpgrade *> ReadWorks;
	std::vector<CUpgrade *> ConsumedElixirs;
	std::vector<CUpgrade *> AuthoredWorks;	/// Literary works of which this character is the author
	std::vector<CUpgrade *> LiteraryAppearances;	/// Literary works in which this character appears
	std::vector<CPersistentItem *> Items;
	int Attributes[MaxAttributes];
	std::vector<CUnitType *> ForbiddenUpgrades;	/// which unit types this character is forbidden to upgrade to
	std::vector<std::pair<CDate, stratagus::faction *>> HistoricalFactions;	/// historical locations of the character; the values are: date, faction
	std::vector<std::unique_ptr<historical_location>> HistoricalLocations;	/// historical locations of the character
	std::vector<std::tuple<CDate, CDate, stratagus::faction *, int>> HistoricalTitles;	/// historical titles of the character, the first element is the beginning date of the term, the second one the end date, the third the faction it pertains to (if any, if not then it is null), and the fourth is the character title itself (from the character title enums)
	std::vector<std::tuple<int, int, CProvince *, int>> HistoricalProvinceTitles;

	friend ::Spell_Polymorph;
	friend void ::ChangeCustomHeroCivilization(const std::string &hero_full_name, const std::string &civilization_name, const std::string &new_hero_name, const std::string &new_hero_family_name);
	friend int ::CclDefineCharacter(lua_State *l);
	friend int ::CclDefineCustomHero(lua_State *l);
};

}

extern std::map<std::string, stratagus::character *> CustomHeroes;
extern stratagus::character *CurrentCustomHero;
extern bool LoadingPersistentHeroes;

extern int GetAttributeVariableIndex(int attribute);
extern stratagus::character *GetCustomHero(const std::string &hero_ident);
extern void SaveHero(stratagus::character *hero);
extern void SaveHeroes();
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
