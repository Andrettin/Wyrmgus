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
/**@name character.cpp - The characters. */
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "character.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CCharacter *> Characters;
std::vector<CCharacter *> CustomHeroes;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

bool CCharacter::IsParentOf(std::string child_full_name)
{
	for (size_t i = 0; i < this->Children.size(); ++i) {
		if (this->Children[i]->GetFullName() == child_full_name) {
			return true;
		}
	}
	return false;
}

bool CCharacter::IsChildOf(std::string parent_full_name)
{
	if ((this->Father != NULL && this->Father->GetFullName() == parent_full_name) || (this->Mother != NULL && this->Mother->GetFullName() == parent_full_name)) {
		return true;
	}
	return false;
}

bool CCharacter::IsSiblingOf(std::string sibling_full_name)
{
	for (size_t i = 0; i < this->Siblings.size(); ++i) {
		if (this->Siblings[i]->GetFullName() == sibling_full_name) {
			return true;
		}
	}
	return false;
}

void CleanCharacters()
{
	for (size_t i = 0; i < Characters.size(); ++i) {
		delete Characters[i];
	}
	Characters.clear();
	
	for (size_t i = 0; i < CustomHeroes.size(); ++i) {
		delete CustomHeroes[i];
	}
	CustomHeroes.clear();
}

CCharacter *GetCharacter(std::string character_full_name)
{
	for (size_t i = 0; i < Characters.size(); ++i) {
		if (character_full_name == Characters[i]->GetFullName()) {
			return Characters[i];
		}
	}
	return NULL;
}

CCharacter *GetCustomHero(std::string hero_full_name)
{
	for (size_t i = 0; i < CustomHeroes.size(); ++i) {
		if (hero_full_name == CustomHeroes[i]->GetFullName()) {
			return CustomHeroes[i];
		}
	}
	return NULL;
}

//@}
