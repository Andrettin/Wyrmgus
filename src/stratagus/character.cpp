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
#include "iocompat.h"
#include "iolib.h"
#include "item.h"
#include "parameters.h"
#include "player.h"
#include "province.h"
#include "quest.h"
#include "spells.h"
#include "unit.h"
#include "upgrade.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::map<std::string, CCharacter *> Characters;
std::map<std::string, CCharacter *> CustomHeroes;
CCharacter *CurrentCustomHero = NULL;
bool LoadingPersistentHeroes = false;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CCharacter::~CCharacter()
{
	if (this->Conditions) {
		delete Conditions;
	}
}

int CCharacter::GetMartialAttribute()
{
	if ((this->Type->Class != -1 && UnitTypeClasses[this->Type->Class] == "thief") || this->Type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value > 1) {
		return DexterityAttribute;
	} else {
		return StrengthAttribute;
	}
}

int CCharacter::GetAttributeModifier(int attribute)
{
	return this->Attributes[attribute] - 10;
}

int CCharacter::GetLanguage()
{
	return PlayerRaces.GetCivilizationLanguage(this->Civilization);
}

bool CCharacter::IsParentOf(std::string child_ident)
{
	for (size_t i = 0; i < this->Children.size(); ++i) {
		if (this->Children[i]->Ident == child_ident) {
			return true;
		}
	}
	return false;
}

bool CCharacter::IsChildOf(std::string parent_ident)
{
	if ((this->Father != NULL && this->Father->Ident == parent_ident) || (this->Mother != NULL && this->Mother->Ident == parent_ident)) {
		return true;
	}
	return false;
}

bool CCharacter::IsSiblingOf(std::string sibling_ident)
{
	for (size_t i = 0; i < this->Siblings.size(); ++i) {
		if (this->Siblings[i]->Ident == sibling_ident) {
			return true;
		}
	}
	return false;
}

bool CCharacter::IsItemEquipped(const CPersistentItem *item) const
{
	int item_slot = GetItemClassSlot(item->Type->ItemClass);
	
	if (item_slot == -1) {
		return false;
	}
	
	if (std::find(EquippedItems[item_slot].begin(), EquippedItems[item_slot].end(), item) != EquippedItems[item_slot].end()) {
		return true;
	}
	
	return false;
}

bool CCharacter::CanAppear() const
{
	if (this->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0 && this->Gender != this->Type->DefaultStat.Variables[GENDER_INDEX].Value) {
		return false; // hero not usable if their unit type has a set gender which is different from the hero's (this is because this means that the unit type lacks appropriate graphics for that gender)
	}
	
	for (int i = 0; i < PlayerMax; ++i) {
		if (Players[i].HasHero(this)) {
			return false;
		}
	}

	return true;
}

std::string CCharacter::GetFullName() const
{
	std::string full_name = this->Name;
	if (!this->ExtraName.empty()) {
		full_name += " " + this->ExtraName;
	}
	if (!this->FamilyName.empty()) {
		full_name += " " + this->FamilyName;
	}
	return full_name;
}

IconConfig CCharacter::GetIcon()
{
	if (this->Level >= 3 && this->HeroicIcon.Icon) {
		return this->HeroicIcon;
	} else if (this->Icon.Icon) {
		return this->Icon;
	} else if (!this->HairVariation.empty() && this->Type->GetVariation(this->HairVariation) != NULL && !this->Type->GetVariation(this->HairVariation)->Icon.Name.empty()) {
		return this->Type->GetVariation(this->HairVariation)->Icon;
	} else {
		return this->Type->Icon;
	}
}

CPersistentItem *CCharacter::GetItem(CUnit &item)
{
	for (size_t i = 0; i < this->Items.size(); ++i) {
		if (this->Items[i]->Type == item.Type && this->Items[i]->Prefix == item.Prefix && this->Items[i]->Suffix == item.Suffix && this->Items[i]->Spell == item.Spell && this->Items[i]->Work == item.Work && this->Items[i]->Elixir == item.Elixir && this->Items[i]->Unique == item.Unique && this->Items[i]->Bound == item.Bound && this->Items[i]->Identified == item.Identified && this->IsItemEquipped(this->Items[i]) == item.Container->IsItemEquipped(&item)) {
			if (this->Items[i]->Name.empty() || this->Items[i]->Name == item.Name) {
				return this->Items[i];
			}
		}
	}
	return NULL;
}

void CCharacter::UpdateAttributes()
{
	if (this->Type == NULL) {
		return;
	}
	
	for (int i = 0; i < MaxAttributes; ++i) {
		int var = GetAttributeVariableIndex(i);
		this->Attributes[i] = this->Type->DefaultStat.Variables[var].Value;
		for (int z = 0; z < NumUpgradeModifiers; ++z) {
			if (
				(this->Trait != NULL && UpgradeModifiers[z]->UpgradeId == this->Trait->ID)
				|| std::find(this->Abilities.begin(), this->Abilities.end(), AllUpgrades[UpgradeModifiers[z]->UpgradeId]) != this->Abilities.end()
			) {
				if (UpgradeModifiers[z]->Modifier.Variables[var].Value != 0) {
					this->Attributes[i] += UpgradeModifiers[z]->Modifier.Variables[var].Value;
				}
			}
		}
	}
}

int GetAttributeVariableIndex(int attribute)
{
	if (attribute == StrengthAttribute) {
		return STRENGTH_INDEX;
	} else if (attribute == DexterityAttribute) {
		return DEXTERITY_INDEX;
	} else if (attribute == IntelligenceAttribute) {
		return INTELLIGENCE_INDEX;
	} else if (attribute == CharismaAttribute) {
		return CHARISMA_INDEX;
	} else {
		return -1;
	}
}

void CleanCharacters()
{
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
		for (size_t i = 0; i < iterator->second->Items.size(); ++i) {
			delete iterator->second->Items[i];
		}
		iterator->second->Items.clear();
		delete iterator->second;
	}
	Characters.clear();
	
	for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
		for (size_t i = 0; i < iterator->second->Items.size(); ++i) {
			delete iterator->second->Items[i];
		}
		iterator->second->Items.clear();
		delete iterator->second;
	}
	CustomHeroes.clear();
}

CCharacter *GetCharacter(std::string character_ident)
{
	if (Characters.find(character_ident) != Characters.end()) {
		return Characters[character_ident];
	}
	
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) { // for backwards compatibility
		if (iterator->second->GetFullName() == character_ident) {
			return iterator->second;
		}
	}

	return NULL;
}

CCharacter *GetCustomHero(std::string hero_ident)
{
	if (CustomHeroes.find(hero_ident) != CustomHeroes.end()) {
		return CustomHeroes[hero_ident];
	}
	
	for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) { // for backwards compatibility
		if (iterator->second->GetFullName() == hero_ident) {
			return iterator->second;
		}
	}
	
	return NULL;
}

/**
**  Save heroes
*/
void SaveHeroes()
{
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) { //save characters
		SaveHero(iterator->second);
	}

	for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) { //save custom heroes
		SaveHero(iterator->second);
	}
			
	//see if the old heroes.lua save file is present, and if so, delete it
	std::string path = Parameters::Instance.GetUserDirectory();

	if (!GameName.empty()) {
		path += "/";
		path += GameName;
	}
	path += "/";
	path += "heroes.lua";

	if (CanAccessFile(path.c_str())) {
		unlink(path.c_str());
	}
}

void SaveHero(CCharacter *hero)
{
	struct stat tmp;
	std::string path = Parameters::Instance.GetUserDirectory();

	if (!GameName.empty()) {
		path += "/";
		path += GameName;
	}
	path += "/";
	path += "heroes/";
	if (hero->Custom) {
		if (stat(path.c_str(), &tmp) < 0) {
			makedir(path.c_str(), 0777);
		}
		path += "custom/";
	}
	if (stat(path.c_str(), &tmp) < 0) {
		makedir(path.c_str(), 0777);
	}
	std::string old_path = path;
	path += hero->Ident;
	path += ".lua";
	old_path += hero->GetFullName();
	old_path += ".lua";
	if (CanAccessFile(old_path.c_str())) {
		unlink(old_path.c_str());
	}

	FILE *fd = fopen(path.c_str(), "w");
	if (!fd) {
		fprintf(stderr, "Cannot open file %s for writing.\n", path.c_str());
		return;
	}

	if (!hero->Custom) {
		fprintf(fd, "DefineCharacter(\"%s\", {\n", hero->Ident.c_str());
	} else {
		fprintf(fd, "DefineCustomHero(\"%s\", {\n", hero->Ident.c_str());
		fprintf(fd, "\tName = \"%s\",\n", hero->Name.c_str());
		if (!hero->ExtraName.empty()) {
			fprintf(fd, "\tExtraName = \"%s\",\n", hero->ExtraName.c_str());
		}
		if (!hero->FamilyName.empty()) {
			fprintf(fd, "\tFamilyName = \"%s\",\n", hero->FamilyName.c_str());
		}
		if (hero->Gender != NoGender) {
			fprintf(fd, "\tGender = \"%s\",\n", GetGenderNameById(hero->Gender).c_str());
		}
		if (hero->Civilization != -1) {
			fprintf(fd, "\tCivilization = \"%s\",\n", PlayerRaces.Name[hero->Civilization].c_str());
		}
	}
	if (hero->Type != NULL) {
		fprintf(fd, "\tType = \"%s\",\n", hero->Type->Ident.c_str());
	}
	if (hero->Custom) {
		if (hero->Trait != NULL) {
			fprintf(fd, "\tTrait = \"%s\",\n", hero->Trait->Ident.c_str());
		}
		if (!hero->HairVariation.empty()) {
			fprintf(fd, "\tHairVariation = \"%s\",\n", hero->HairVariation.c_str());
		}
	}
	if (hero->Level != 0) {
		fprintf(fd, "\tLevel = %d,\n", hero->Level);
	}
	if (hero->ExperiencePercent != 0) {
		fprintf(fd, "\tExperiencePercent = %d,\n", hero->ExperiencePercent);
	}
	if (hero->Abilities.size() > 0) {
		fprintf(fd, "\tAbilities = {");
		for (size_t j = 0; j < hero->Abilities.size(); ++j) {
			fprintf(fd, "\"%s\"", hero->Abilities[j]->Ident.c_str());
			if (j < (hero->Abilities.size() - 1)) {
				fprintf(fd, ", ");
			}
		}
		fprintf(fd, "},\n");
	}
	if (hero->ReadWorks.size() > 0) {
		fprintf(fd, "\tReadWorks = {");
		for (size_t j = 0; j < hero->ReadWorks.size(); ++j) {
			fprintf(fd, "\"%s\"", hero->ReadWorks[j]->Ident.c_str());
			if (j < (hero->ReadWorks.size() - 1)) {
				fprintf(fd, ", ");
			}
		}
		fprintf(fd, "},\n");
	}
	if (hero->ConsumedElixirs.size() > 0) {
		fprintf(fd, "\tConsumedElixirs = {");
		for (size_t j = 0; j < hero->ConsumedElixirs.size(); ++j) {
			fprintf(fd, "\"%s\"", hero->ConsumedElixirs[j]->Ident.c_str());
			if (j < (hero->ConsumedElixirs.size() - 1)) {
				fprintf(fd, ", ");
			}
		}
		fprintf(fd, "},\n");
	}
	if (hero->Items.size() > 0) {
		fprintf(fd, "\tItems = {");
		for (size_t j = 0; j < hero->Items.size(); ++j) {
			fprintf(fd, "\n\t\t{");
			fprintf(fd, "\n\t\t\t\"type\", \"%s\",", hero->Items[j]->Type->Ident.c_str());
			if (hero->Items[j]->Prefix != NULL) {
				fprintf(fd, "\n\t\t\t\"prefix\", \"%s\",", hero->Items[j]->Prefix->Ident.c_str());
			}
			if (hero->Items[j]->Suffix != NULL) {
				fprintf(fd, "\n\t\t\t\"suffix\", \"%s\",", hero->Items[j]->Suffix->Ident.c_str());
			}
			if (hero->Items[j]->Spell != NULL) {
				fprintf(fd, "\n\t\t\t\"spell\", \"%s\",", hero->Items[j]->Spell->Ident.c_str());
			}
			if (hero->Items[j]->Work != NULL) {
				fprintf(fd, "\n\t\t\t\"work\", \"%s\",", hero->Items[j]->Work->Ident.c_str());
			}
			if (hero->Items[j]->Elixir != NULL) {
				fprintf(fd, "\n\t\t\t\"elixir\", \"%s\",", hero->Items[j]->Elixir->Ident.c_str());
			}
			if (!hero->Items[j]->Name.empty()) {
				fprintf(fd, "\n\t\t\t\"name\", \"%s\",", hero->Items[j]->Name.c_str());
			}
			if (hero->Items[j]->Unique) { // affixes, name and etc. will be inherited from the unique item, but we set those previous characteristics for unique items anyway, so that if a unique item no longer exists in the game's code (i.e. if it is from a mod that has been deactivated) the character retains an item with the same affixes, name and etc., even though it will no longer be unique
				fprintf(fd, "\n\t\t\t\"unique\", \"%s\",", hero->Items[j]->Unique->Ident.c_str());
			}
			if (hero->Items[j]->Bound) {
				fprintf(fd, "\n\t\t\t\"bound\", true,");
			}
			if (!hero->Items[j]->Identified) {
				fprintf(fd, "\n\t\t\t\"identified\", false,");
			}
			if (hero->IsItemEquipped(hero->Items[j])) {
				fprintf(fd, "\n\t\t\t\"equipped\", true");
			}
			fprintf(fd, "\n\t\t}");
			if (j < (hero->Items.size() - 1)) {
				fprintf(fd, ",");
			}
		}
		fprintf(fd, "\n\t},\n");
	}
	
	if (hero->Custom) {
		if (hero->QuestsInProgress.size() > 0) {
			fprintf(fd, "\tQuestsInProgress = {");
			for (size_t j = 0; j < hero->QuestsInProgress.size(); ++j) {
				fprintf(fd, "\"%s\"", hero->QuestsInProgress[j]->Name.c_str());
				if (j < (hero->QuestsInProgress.size() - 1)) {
					fprintf(fd, ", ");
				}
			}
			fprintf(fd, "},\n");
		}
		if (hero->QuestsCompleted.size() > 0) {
			fprintf(fd, "\tQuestsCompleted = {");
			for (size_t j = 0; j < hero->QuestsCompleted.size(); ++j) {
				fprintf(fd, "\"%s\"", hero->QuestsCompleted[j]->Name.c_str());
				if (j < (hero->QuestsCompleted.size() - 1)) {
					fprintf(fd, ", ");
				}
			}
			fprintf(fd, "},\n");
		}
	}
	fprintf(fd, "})\n\n");
		
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

void SaveCustomHero(std::string hero_full_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
	}
	
	SaveHero(hero);
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
	
	//delete hero save file
	std::string path = Parameters::Instance.GetUserDirectory();
	if (!GameName.empty()) {
		path += "/";
		path += GameName;
	}
	path += "/";
	path += "heroes/";
	if (hero->Custom) {
		path += "custom/";
	}
	path += hero->Ident;
	path += ".lua";	
	if (CanAccessFile(path.c_str())) {
		unlink(path.c_str());
	}
	
	CustomHeroes.erase(hero_full_name);
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
		return CurrentCustomHero->Ident;
	} else {
		return "";
	}
}

void ChangeCustomHeroCivilization(std::string hero_full_name, std::string civilization_name, std::string new_hero_name, std::string new_hero_family_name)
{
	if (!hero_full_name.empty()) {
		CCharacter *hero = GetCustomHero(hero_full_name);
		if (!hero) {
			fprintf(stderr, "Custom hero \"%s\" doesn't exist.\n", hero_full_name.c_str());
		}
		
		int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		if (civilization != -1) {
			//delete old hero save file
			std::string path = Parameters::Instance.GetUserDirectory();
			if (!GameName.empty()) {
				path += "/";
				path += GameName;
			}
			path += "/";
			path += "heroes/";
			if (hero->Custom) {
				path += "custom/";
			}
			path += hero->Ident;
			path += ".lua";	
			if (CanAccessFile(path.c_str())) {
				unlink(path.c_str());
			}
			
			//now, update the hero
			hero->Civilization = civilization;
			int new_unit_type_id = PlayerRaces.GetCivilizationClassUnitType(hero->Civilization, hero->Type->Class);
			if (new_unit_type_id != -1) {
				hero->Type = const_cast<CUnitType *>(&(*UnitTypes[new_unit_type_id]));
				hero->Name = new_hero_name;
				hero->FamilyName = new_hero_family_name;
				SaveHero(hero);
				
				CustomHeroes.erase(hero_full_name);
				CustomHeroes[hero->Ident] = hero;
			}
		}
	}
}

bool IsNameValidForCustomHero(std::string hero_name, std::string hero_family_name)
{
	std::string hero_full_name = hero_name;
	if (!hero_family_name.empty()) {
		hero_full_name += " ";
		hero_full_name += hero_family_name;
	}
	
	if (GetCustomHero(hero_full_name) != NULL) {
		return false; //name already used
	}
	
	if (hero_name.empty()) {
		return false;
	}
	
	if (
		hero_full_name.find('\n') != -1
		|| hero_full_name.find('\\') != -1
		|| hero_full_name.find('/') != -1
		|| hero_full_name.find('.') != -1
		|| hero_full_name.find('*') != -1
		|| hero_full_name.find('[') != -1
		|| hero_full_name.find(']') != -1
		|| hero_full_name.find(':') != -1
		|| hero_full_name.find(';') != -1
		|| hero_full_name.find('=') != -1
		|| hero_full_name.find(',') != -1
		|| hero_full_name.find('<') != -1
		|| hero_full_name.find('>') != -1
		|| hero_full_name.find('?') != -1
		|| hero_full_name.find('|') != -1
	) {
		return false;
	}
	
	if (hero_name.find_first_not_of(' ') == std::string::npos) {
		return false; //name contains only spaces
	}
	
	if (!hero_family_name.empty() && hero_family_name.find_first_not_of(' ') == std::string::npos) {
		return false; //family name contains only spaces
	}
	
	return true;
}

std::string GetGenderNameById(int gender)
{
	if (gender == NoGender) {
		return "no-gender";
	} else if (gender == MaleGender) {
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
	if (gender == "no-gender") {
		return NoGender;
	} else if (gender == "male") {
		return MaleGender;
	} else if (gender == "female") {
		return FemaleGender;
	} else if (gender == "asexual") {
		return AsexualGender;
	}

	return -1;
}

std::string GetCharacterTitleNameById(int title)
{
	if (title == CharacterTitleHeadOfState) {
		return "head-of-state";
	} else if (title == CharacterTitleHeadOfGovernment) {
		return "head-of-government";
	} else if (title == CharacterTitleEducationMinister) {
		return "education-minister";
	} else if (title == CharacterTitleFinanceMinister) {
		return "finance-minister";
	} else if (title == CharacterTitleForeignMinister) {
		return "foreign-minister";
	} else if (title == CharacterTitleIntelligenceMinister) {
		return "intelligence-minister";
	} else if (title == CharacterTitleInteriorMinister) {
		return "interior-minister";
	} else if (title == CharacterTitleJusticeMinister) {
		return "justice-minister";
	} else if (title == CharacterTitleWarMinister) {
		return "war-minister";
	} else if (title == CharacterTitleGovernor) {
		return "governor";
	}

	return "";
}

int GetCharacterTitleIdByName(std::string title)
{
	if (title == "head-of-state") {
		return CharacterTitleHeadOfState;
	} else if (title == "head-of-government") {
		return CharacterTitleHeadOfGovernment;
	} else if (title == "education-minister") {
		return CharacterTitleEducationMinister;
	} else if (title == "finance-minister") {
		return CharacterTitleFinanceMinister;
	} else if (title == "foreign-minister") {
		return CharacterTitleForeignMinister;
	} else if (title == "intelligence-minister") {
		return CharacterTitleIntelligenceMinister;
	} else if (title == "interior-minister") {
		return CharacterTitleInteriorMinister;
	} else if (title == "justice-minister") {
		return CharacterTitleJusticeMinister;
	} else if (title == "war-minister") {
		return CharacterTitleWarMinister;
	} else if (title == "governor") {
		return CharacterTitleGovernor;
	}

	return -1;
}

bool IsMinisterialTitle(int title)
{
	return (title == CharacterTitleHeadOfState || title == CharacterTitleHeadOfGovernment || title == CharacterTitleEducationMinister || title == CharacterTitleFinanceMinister || title == CharacterTitleForeignMinister || title == CharacterTitleIntelligenceMinister || title == CharacterTitleInteriorMinister || title == CharacterTitleJusticeMinister || title == CharacterTitleWarMinister);
}
//@}
