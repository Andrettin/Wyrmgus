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

#include "game.h"
#include "parameters.h"
#include "player.h"
#include "quest.h"
#include "unit.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CCharacter *> Characters;
std::vector<CCharacter *> CustomHeroes;
CCharacter *CurrentCustomHero = NULL;

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

/**
**  Save heroes
*/
void SaveHeroes()
{
	std::string path = Parameters::Instance.GetUserDirectory();

	if (!GameName.empty()) {
		path += "/";
		path += GameName;
	}
	path += "/";
	path += "heroes";
	path += ".lua";

	FILE *fd = fopen(path.c_str(), "w");
	if (!fd) {
		DebugPrint("Cannot open file %s for writing\n" _C_ path.c_str());
		return;
	}

	for (size_t i = 0; i < Characters.size(); ++i) { //save persistent characters
		if (Characters[i]->Persistent) {
			fprintf(fd, "DefineCharacter(\"%s\", {\n", Characters[i]->GetFullName().c_str());
			if (Characters[i]->Type != NULL) {
				fprintf(fd, "\tType = \"%s\",\n", Characters[i]->Type->Ident.c_str());
			}
			if (Characters[i]->Level != 0) {
				fprintf(fd, "\tLevel = %d,\n", Characters[i]->Level);
			}
			if (Characters[i]->Abilities.size() > 0) {
				fprintf(fd, "\tAbilities = {");
				for (size_t j = 0; j < Characters[i]->Abilities.size(); ++j) {
					fprintf(fd, "\"%s\"", Characters[i]->Abilities[j]->Ident.c_str());
					if (j < (Characters[i]->Abilities.size() - 1)) {
						fprintf(fd, ", ");
					}
				}
				fprintf(fd, "}\n");
			}
			fprintf(fd, "})\n\n");
		}
	}
		
	for (size_t i = 0; i < CustomHeroes.size(); ++i) { //save custom heroes
		if (CustomHeroes[i]->Persistent) {
			fprintf(fd, "DefineCustomHero(\"%s\", {\n", CustomHeroes[i]->GetFullName().c_str());
			fprintf(fd, "\tName = \"%s\",\n", CustomHeroes[i]->Name.c_str());
			if (!CustomHeroes[i]->ExtraName.empty()) {
				fprintf(fd, "\tExtraName = \"%s\",\n", CustomHeroes[i]->ExtraName.c_str());
			}
			if (!CustomHeroes[i]->Dynasty.empty()) {
				fprintf(fd, "\tDynasty = \"%s\",\n", CustomHeroes[i]->Dynasty.c_str());
			}
			if (CustomHeroes[i]->Gender != NoGender) {
				fprintf(fd, "\tGender = \"%s\",\n", GetGenderNameById(CustomHeroes[i]->Gender).c_str());
			}
			if (CustomHeroes[i]->Civilization != -1) {
				fprintf(fd, "\tCivilization = \"%s\",\n", PlayerRaces.Name[CustomHeroes[i]->Civilization].c_str());
			}
			if (CustomHeroes[i]->Type != NULL) {
				fprintf(fd, "\tType = \"%s\",\n", CustomHeroes[i]->Type->Ident.c_str());
			}
			if (CustomHeroes[i]->Trait != NULL) {
				fprintf(fd, "\tTrait = \"%s\",\n", CustomHeroes[i]->Trait->Ident.c_str());
			}
			if (!CustomHeroes[i]->Variation.empty()) {
				fprintf(fd, "\tVariation = \"%s\",\n", CustomHeroes[i]->Variation.c_str());
			}
			if (CustomHeroes[i]->Level != 0) {
				fprintf(fd, "\tLevel = %d,\n", CustomHeroes[i]->Level);
			}
			if (CustomHeroes[i]->Abilities.size() > 0) {
				fprintf(fd, "\tAbilities = {");
				for (size_t j = 0; j < CustomHeroes[i]->Abilities.size(); ++j) {
					fprintf(fd, "\"%s\"", CustomHeroes[i]->Abilities[j]->Ident.c_str());
					if (j < (CustomHeroes[i]->Abilities.size() - 1)) {
						fprintf(fd, ", ");
					}
				}
				fprintf(fd, "},\n");
			}
			if (CustomHeroes[i]->QuestsInProgress.size() > 0) {
				fprintf(fd, "\tQuestsInProgress = {");
				for (size_t j = 0; j < CustomHeroes[i]->QuestsInProgress.size(); ++j) {
					fprintf(fd, "\"%s\"", CustomHeroes[i]->QuestsInProgress[j]->Name.c_str());
					if (j < (CustomHeroes[i]->QuestsInProgress.size() - 1)) {
						fprintf(fd, ", ");
					}
				}
				fprintf(fd, "},\n");
			}
			if (CustomHeroes[i]->QuestsCompleted.size() > 0) {
				fprintf(fd, "\tQuestsCompleted = {");
				for (size_t j = 0; j < CustomHeroes[i]->QuestsCompleted.size(); ++j) {
					fprintf(fd, "\"%s\"", CustomHeroes[i]->QuestsCompleted[j]->Name.c_str());
					if (j < (CustomHeroes[i]->QuestsCompleted.size() - 1)) {
						fprintf(fd, ", ");
					}
				}
				fprintf(fd, "},\n");
			}
			
			fprintf(fd, "\tForbiddenUpgrades = {");
			for (int j = 0; j < UnitTypeMax; ++j) {
				if (CustomHeroes[i]->ForbiddenUpgrades[j]) {
					fprintf(fd, "\"%s\", ", UnitTypes[j]->Ident.c_str());
				}
			}
			fprintf(fd, "},\n");
			
			fprintf(fd, "})\n\n");
		}
	}
		
	fclose(fd);
}

void HeroAddQuest(std::string hero_full_name, std::string quest_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	
	CQuest *quest = GetQuest(quest_name);
	if (!quest) {
		fprintf(stderr, "Quest \"%s\" doesn't exist.\n", quest_name.c_str());
	}
	
	hero->QuestsInProgress.push_back(quest);
}

void HeroCompleteQuest(std::string hero_full_name, std::string quest_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	
	CQuest *quest = GetQuest(quest_name);
	if (!quest) {
		fprintf(stderr, "Quest \"%s\" doesn't exist.\n", quest_name.c_str());
	}
	
	hero->QuestsInProgress.erase(std::remove(hero->QuestsInProgress.begin(), hero->QuestsInProgress.end(), quest), hero->QuestsInProgress.end());
	hero->QuestsCompleted.push_back(quest);
}

void DeleteCustomHero(std::string hero_full_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	
	if (CurrentCustomHero == hero) {
		CurrentCustomHero = NULL;
	}
	CustomHeroes.erase(std::remove(CustomHeroes.begin(), CustomHeroes.end(), hero), CustomHeroes.end());
	delete hero;
}

void SetCurrentCustomHero(std::string hero_full_name)
{
	if (!hero_full_name.empty()) {
		CCharacter *hero = GetCustomHero(hero_full_name);
		if (!hero) {
			fprintf(stderr, "Custom hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
		}
		
		CurrentCustomHero = const_cast<CCharacter *>(&(*hero));
	} else {
		CurrentCustomHero = NULL;
	}
}

std::string GetCurrentCustomHero()
{
	if (CurrentCustomHero != NULL) {
		return CurrentCustomHero->GetFullName();
	} else {
		return "";
	}
}

std::string GetGenderNameById(int gender)
{
	if (gender == MaleGender) {
		return "male";
	} else if (gender == FemaleGender) {
		return "female";
	} else if (gender == AsexualGender) {
		return "asexual";
	}

	return "";
}

int GetGenderIdByName(std::string gender)
{
	if (gender == "male") {
		return MaleGender;
	} else if (gender == "female") {
		return FemaleGender;
	} else if (gender == "asexual") {
		return AsexualGender;
	}

	return -1;
}

//@}
