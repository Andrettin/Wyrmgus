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

#ifndef __CHARACTER_H__
#define __CHARACTER_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>

#ifndef __ICONS_H__
#include "icons.h"
#endif

#ifndef __ITEM_H__
#include "item.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CItem;
class CQuest;
class CUnitType;
class CUnit;
class CUpgrade;

/**
**  Indexes into item type array.
*/
enum Genders {
	NoGender,
	MaleGender,
	FemaleGender,
	AsexualGender, //i.e. slimes reproduce asexually

	MaxGenders
};

class CCharacter
{
public:
	CCharacter() :
		Year(0), DeathYear(0), Civilization(-1), Faction(-1), Gender(0), Level(0), ExperiencePercent(0),
		Persistent(false), Custom(false),
		Type(NULL), Trait(NULL),
		Father(NULL), Mother(NULL)
	{
		memset(ForbiddenUpgrades, 0, sizeof(ForbiddenUpgrades));
	}
	
	bool IsParentOf(std::string child_full_name);
	bool IsChildOf(std::string parent_full_name);
	bool IsSiblingOf(std::string sibling_full_name);
	bool IsItemEquipped(const CItem *item) const;
	std::string GetFullName();
	CItem *GetItem(CUnit &item);

	int Year;					/// Year in which the character historically starts being active
	int DeathYear;				/// Year in which the character dies of natural causes
	int Civilization;			/// Culture to which the character belongs
	int Faction;				/// Faction to which the character belongs
	int Gender;					/// Character's gender
	int Level;					/// Character's level
	int ExperiencePercent;		/// Character's experience, as a percentage of the experience required to level up
	bool Persistent;			/// Whether this character's levels and abilities are persistent
	bool Custom;				/// Whether this character is a custom hero
	std::string Name;			/// Given name of the character
	std::string ExtraName;		/// Extra given names of the character (used if necessary to differentiate from existing heroes)
	std::string Dynasty;		/// Name of the character's dynasty
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
	std::vector<CItem *> EquippedItems[MaxItemSlots];	/// Equipped items of the character, per slot
	std::vector<CCharacter *> Children;	/// Children of the character
	std::vector<CCharacter *> Siblings;	/// Siblings of the character
	std::vector<CUpgrade *> Abilities;
	std::vector<CUpgrade *> ReadWorks;
	std::vector<CQuest *> QuestsInProgress;	/// Quests in progress, only for playable, custom characters
	std::vector<CQuest *> QuestsCompleted;	/// Quests completed, only for playable, custom characters
	std::vector<CItem *> Items;
	bool ForbiddenUpgrades[UnitTypeMax];	/// which unit types this character is forbidden to upgrade to
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CCharacter *> Characters;
extern std::vector<CCharacter *> CustomHeroes;
extern CCharacter *CurrentCustomHero;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

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
extern void ChangeCustomHeroCivilization(std::string hero_full_name, std::string civilization_name, std::string new_hero_name, std::string new_hero_dynasty_name);
extern bool IsNameValidForCustomHero(std::string hero_name, std::string hero_dynasty_name);
extern std::string GetGenderNameById(int gender);
extern int GetGenderIdByName(std::string gender);
extern void CharacterCclRegister();

//@}

#endif // !__CHARACTER_H__
