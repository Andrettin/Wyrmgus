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
/**@name character.h - The character headerfile. */
//
//      (c) Copyright 2015-2016 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>
#include <tuple>

#ifndef __ICONS_H__
#include "icons.h"
#endif

#ifndef __ITEM_H__
#include "item.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFaction;
class CItem;
class CProvince;
class CQuest;
class CUnitType;
class CUnit;
class CUpgrade;

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

	MaxCharacterTitles
};

class CCharacter
{
public:
	CCharacter() :
		Year(0), DeathYear(0), Civilization(-1), Faction(-1), Gender(0), Level(0), ExperiencePercent(0),
		ViolentDeath(false), Noble(false), Persistent(false), Custom(false),
		Type(NULL), Trait(NULL),
		Father(NULL), Mother(NULL), DateReferenceCharacter(NULL)
	{
		memset(Attributes, 0, sizeof(Attributes));
		memset(ForbiddenUpgrades, 0, sizeof(ForbiddenUpgrades));
	}
	
	int GetMartialAttribute();
	int GetAttributeModifier(int attribute);
	int GetLanguage();
	bool IsParentOf(std::string child_full_name);
	bool IsChildOf(std::string parent_full_name);
	bool IsSiblingOf(std::string sibling_full_name);
	bool IsItemEquipped(const CItem *item) const;
	std::string GetFullName();
	IconConfig GetIcon();
	CItem *GetItem(CUnit &item);
	void GenerateMissingData();
	void UpdateAttributes();

	int Year;					/// Year in which the character historically starts being active
	int DeathYear;				/// Year in which the character dies of natural causes
	int Civilization;			/// Culture to which the character belongs
	int Faction;				/// Faction to which the character belongs
	int Gender;					/// Character's gender
	int Level;					/// Character's level
	int ExperiencePercent;		/// Character's experience, as a percentage of the experience required to level up
	bool ViolentDeath;			/// If historical death was violent
	bool Noble;
	bool Persistent;			/// Whether this character's levels and abilities are persistent
	bool Custom;				/// Whether this character is a custom hero
	std::string Name;			/// Given name of the character
	std::string ExtraName;		/// Extra given names of the character (used if necessary to differentiate from existing heroes)
	std::string FamilyName;		/// Name of the character's family
	std::string Description;	/// Description of the character from an in-game universe perspective
	std::string Background;		/// Description of the character from a perspective outside of the game's universe
	std::string Quote;			/// A quote relating to the character
	std::string HairVariation;	/// Name of the character's hair variation
	std::string ProvinceOfOriginName;	/// Name of the province from which the character originates
	IconConfig Icon;					/// Character's icon
	IconConfig HeroicIcon;				/// Character's heroic icon (level 3 and upper)
	CUnitType *Type;
	CUpgrade *Trait;
	CCharacter *Father;					/// Character's father
	CCharacter *Mother;					/// Character's mother
	CCharacter *DateReferenceCharacter;	/// Character used as a date reference for this character; i.e. if a dwarf was the contemporary of a human hero in a saga, make the hero a date reference for the dwarf, so that the dwarf will be generated in a similar date in Nidavellir
	std::vector<CItem *> EquippedItems[MaxItemSlots];	/// Equipped items of the character, per slot
	std::vector<CCharacter *> Children;	/// Children of the character
	std::vector<CCharacter *> Siblings;	/// Siblings of the character
	std::vector<CCharacter *> DateReferredCharacters;	/// Characters who use this character as a date reference
	std::vector<CUpgrade *> Abilities;
	std::vector<CUpgrade *> ReadWorks;
	std::vector<CUpgrade *> AuthoredWorks;	/// Literary works of which this character is the author
	std::vector<CUpgrade *> LiteraryAppearances;	/// Literary works in which this character appears
	std::vector<CQuest *> QuestsInProgress;	/// Quests in progress, only for playable, custom characters
	std::vector<CQuest *> QuestsCompleted;	/// Quests completed, only for playable, custom characters
	std::vector<CItem *> Items;
	int Attributes[MaxAttributes];
	bool ForbiddenUpgrades[UnitTypeMax];	/// which unit types this character is forbidden to upgrade to
	std::vector<std::tuple<int, int, CFaction *, int>> HistoricalTitles;	/// historical titles of the character, the first element is the beginning year of the term, the second one the end year, the third the faction it pertains to (if any, if not then it is NULL), and the fourth is the character title itself (from the character title enums)
	std::vector<std::tuple<int, int, CProvince *, int>> HistoricalProvinceTitles;
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::map<std::string, CCharacter *> Characters;
extern std::map<std::string, CCharacter *> CustomHeroes;
extern CCharacter *CurrentCustomHero;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern int GetAttributeVariableIndex(int attribute);
extern void CleanCharacters();
extern CCharacter *GetCharacter(std::string character_full_name);
extern CCharacter *GetCustomHero(std::string hero_full_name);
extern void SaveHero(CCharacter *hero);
extern void SaveHeroes();
extern void HeroAddQuest(std::string hero_full_name, std::string quest_name);
extern void HeroCompleteQuest(std::string hero_full_name, std::string quest_name);
extern void SaveCustomHero(std::string hero_full_name);
extern void DeleteCustomHero(std::string hero_full_name);
extern void SetCurrentCustomHero(std::string hero_full_name);
extern std::string GetCurrentCustomHero();
extern void ChangeCustomHeroCivilization(std::string hero_full_name, std::string civilization_name, std::string new_hero_name, std::string new_hero_family_name);
extern bool IsNameValidForCustomHero(std::string hero_name, std::string hero_family_name);
extern std::string GetGenderNameById(int gender);
extern int GetGenderIdByName(std::string gender);
extern std::string GetCharacterTitleNameById(int title);
extern int GetCharacterTitleIdByName(std::string title);
extern bool IsMinisterialTitle(int title);
extern void CharacterCclRegister();

//@}

#endif // !__CHARACTER_H__
