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

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnitType;
class CUpgrade;

class CCharacter
{
public:
	CCharacter() :
		Year(0), DeathYear(0), Civilization(-1), Gender(0), Level(0),
		Persistent(false), Custom(false),
		Name(""), ExtraName(""), Dynasty(""), ProvinceOfOriginName(""),
		Type(NULL), Trait(NULL),
		Father(NULL), Mother(NULL)
	{
	}
	
	bool IsParentOf(std::string child_full_name);
	bool IsChildOf(std::string parent_full_name);
	bool IsSiblingOf(std::string sibling_full_name);
	std::string GetFullName();
	
	int Year;			/// Year in which the character historically starts being active
	int DeathYear;		/// Year in which the character dies of natural causes
	int Civilization;	/// Culture to which the character belongs
	int Gender;			/// Character's gender
	int Level;			/// Character's level
	bool Persistent;	/// Whether this character's levels and abilities are persistent
	bool Custom;		/// Whether this character is a custom hero
	std::string Name;	/// Given name of the character
	std::string ExtraName;	/// Extra given names of the character (used if necessary to differentiate from existing heroes)
	std::string Dynasty;	/// Name of the character's dynasty
	std::string ProvinceOfOriginName;	/// Name of the province from which the character originates
	IconConfig Icon;					/// Character's icon
	CUnitType *Type;
	CUpgrade *Trait;
	CCharacter *Father;					/// Character's father
	CCharacter *Mother;					/// Character's mother
	std::vector<CCharacter *> Children;	/// Children of the character
	std::vector<CCharacter *> Siblings;	/// Siblings of the character
	std::vector<CUpgrade *> Abilities;
//	std::vector<CItem *> Items;
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CCharacter *> Characters;
extern std::vector<CCharacter *> CustomHeroes;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanCharacters();
extern CCharacter *GetCharacter(std::string character_full_name);
extern CCharacter *GetCustomHero(std::string hero_full_name);
extern void CharacterCclRegister();

//@}

#endif // !__CHARACTER_H__
