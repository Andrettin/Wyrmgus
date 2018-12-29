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
/**@name character.cpp - The character source file. */
//
//      (c) Copyright 2015-2018 by Andrettin
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

#include "civilization.h"
#include "config.h"
#include "game.h"
#include "iocompat.h"
#include "iolib.h"
#include "item.h"
#include "map/historical_location.h"
#include "map/map_template.h"
#include "parameters.h"
#include "player.h"
#include "province.h"
#include "quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "spells.h"
#include "time/calendar.h"
#include "unit/unit.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"

#include "../ai/ai_local.h" //for using AiHelpers

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::map<std::string, CCharacter *> Characters;
std::map<std::string, CCharacter *> CustomHeroes;
CCharacter *CurrentCustomHero = nullptr;
bool LoadingPersistentHeroes = false;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CCharacter::~CCharacter()
{
	if (this->Conditions) {
		delete Conditions;
	}
	
	for (size_t i = 0; i < this->HistoricalLocations.size(); ++i) {
		delete this->HistoricalLocations[i];
	}
}

void CCharacter::GenerateCharacterHistory()
{
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
		CCharacter *character = iterator->second;
		character->GenerateHistory();
	}
}

void CCharacter::ResetCharacterHistory()
{
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) {
		CCharacter *character = iterator->second;
		character->ResetHistory();
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CCharacter::ProcessConfigData(const CConfigData *config_data)
{
	if (this->Initialized) {
		fprintf(stderr, "Character \"%s\" is being redefined.\n", this->Ident.c_str());
	}
				
	bool name_changed = false;
	bool family_name_changed = false;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
			name_changed = true;
		} else if (key == "family_name") {
			this->FamilyName = value;
			family_name_changed = true;
		} else if (key == "unit_type") {
			value = FindAndReplaceString(value, "_", "-");
			CUnitType *unit_type = UnitTypeByIdent(value);
			if (unit_type) {
				if (this->Type == nullptr || this->Type == unit_type || this->Type->CanExperienceUpgradeTo(unit_type)) {
					this->Type = unit_type;
					if (this->Level < this->Type->DefaultStat.Variables[LEVEL_INDEX].Value) {
						this->Level = this->Type->DefaultStat.Variables[LEVEL_INDEX].Value;
					}
					
					if (this->Gender == NoGender) { //if no gender was set so far, have the character be the same gender as the unit type (if the unit type has it predefined)
						if (this->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
							this->Gender = this->Type->DefaultStat.Variables[GENDER_INDEX].Value;
						}
					}
				}
			} else {
				fprintf(stderr, "Unit type \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "gender") {
			this->Gender = GetGenderIdByName(value);
		} else if (key == "civilization") {
			value = FindAndReplaceString(value, "_", "-");
			this->Civilization = CCivilization::GetCivilization(value);
		} else if (key == "faction") {
			value = FindAndReplaceString(value, "_", "-");
			CFaction *faction = PlayerRaces.GetFaction(value);
			if (faction) {
				if (!this->Faction) {
					this->Faction = faction;
				}
				this->Factions.push_back(faction);
			} else {
				fprintf(stderr, "Faction \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "hair_variation") {
			value = FindAndReplaceString(value, "_", "-");
			this->HairVariation = value;
		} else if (key == "trait") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Trait = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "level") {
			this->Level = std::stoi(value);
		} else if (key == "birth_date") {
			value = FindAndReplaceString(value, "_", "-");
			this->BirthDate = CDate::FromString(value);
		} else if (key == "start_date") {
			value = FindAndReplaceString(value, "_", "-");
			this->StartDate = CDate::FromString(value);
		} else if (key == "death_date") {
			value = FindAndReplaceString(value, "_", "-");
			this->DeathDate = CDate::FromString(value);
		} else if (key == "violent_death") {
			this->ViolentDeath = StringToBool(value);
		} else if (key == "deity") {
			value = FindAndReplaceString(value, "_", "-");
			CDeity *deity = CDeity::GetDeity(value);
			if (deity) {
				this->Deities.push_back(deity);
				if (LoadingHistory) {
					this->GeneratedDeities.push_back(deity);
				}
			}
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "background") {
			this->Background = value;
		} else if (key == "quote") {
			this->Quote = value;
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
			this->Icon.Icon->Load();
		} else if (key == "heroic_icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->HeroicIcon.Name = value;
			this->HeroicIcon.Icon = nullptr;
			this->HeroicIcon.Load();
			this->HeroicIcon.Icon->Load();
		} else if (key == "forbidden_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			CUnitType *unit_type = UnitTypeByIdent(value);
			if (unit_type) {
				this->ForbiddenUpgrades.push_back(unit_type);
			} else {
				fprintf(stderr, "Unit type \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "ability") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *ability_upgrade = CUpgrade::Get(value);
			if (ability_upgrade) {
				this->Abilities.push_back(ability_upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "read_work") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->ReadWorks.push_back(upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "consumed_elixir") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->ConsumedElixirs.push_back(upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "preferred_deity_domain") {
			value = FindAndReplaceString(value, "_", "-");
			CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(value);
			if (deity_domain) {
				this->PreferredDeityDomains.push_back(deity_domain);
			}
		} else {
			fprintf(stderr, "Invalid character property: \"%s\".\n", key.c_str());
		}
	}
	
	for (size_t i = 0; i < config_data->Children.size(); ++i) {
		const CConfigData *child_config_data = config_data->Children[i];
		
		if (child_config_data->Tag == "historical_location") {
			CHistoricalLocation *historical_location = new CHistoricalLocation;
			historical_location->ProcessConfigData(child_config_data);
				
			if (historical_location->Date.Year == 0 || !historical_location->MapTemplate || (!historical_location->Site && (historical_location->Position.x == -1 || historical_location->Position.y == -1))) {
				delete historical_location;
				continue;
			}
			
			this->HistoricalLocations.push_back(historical_location);
		} else if (child_config_data->Tag == "historical_title") {
			int title = -1;
			CDate start_date;
			CDate end_date;
			CFaction *title_faction = nullptr;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "title") {
					value = FindAndReplaceString(value, "_", "-");
					title = GetCharacterTitleIdByName(value);
					if (title == -1) {
						fprintf(stderr, "Character title \"%s\" does not exist.\n", value.c_str());
					}
				} else if (key == "start_date") {
					value = FindAndReplaceString(value, "_", "-");
					start_date = CDate::FromString(value);
				} else if (key == "end_date") {
					value = FindAndReplaceString(value, "_", "-");
					end_date = CDate::FromString(value);
				} else if (key == "faction") {
					value = FindAndReplaceString(value, "_", "-");
					title_faction = PlayerRaces.GetFaction(value);
					if (!title_faction) {
						fprintf(stderr, "Faction \"%s\" does not exist.\n", value.c_str());
					}
				} else {
					fprintf(stderr, "Invalid historical title property: \"%s\".\n", key.c_str());
				}
			}
			
			if (title == -1) {
				fprintf(stderr, "Historical title has no title.\n");
				continue;
			}
			
			if (!title_faction) {
				fprintf(stderr, "Historical title has no faction.\n");
				continue;
			}
			
			if (start_date.Year != 0 && end_date.Year != 0 && IsMinisterialTitle(title)) { // don't put in the faction's historical data if a blank year was given
				title_faction->HistoricalMinisters[std::tuple<CDate, CDate, int>(start_date, end_date, title)] = this;
			}
				
			this->HistoricalTitles.push_back(std::tuple<CDate, CDate, CFaction *, int>(start_date, end_date, title_faction, title));
		} else if (child_config_data->Tag == "item") {
			CPersistentItem *item = new CPersistentItem;
			item->Owner = this;
			this->Items.push_back(item);
			item->ProcessConfigData(child_config_data);
		} else {
			fprintf(stderr, "Invalid character property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	//use the character's name for name generation (do this only after setting all properties so that the type, civilization and gender will have been parsed if given
	if (this->Type != nullptr && this->Type->BoolFlag[FAUNA_INDEX].value) {
		if (name_changed) {
			this->Type->PersonalNames[this->Gender].push_back(this->Name);
		}
	} else if (this->Civilization) {
		if (name_changed) {
			this->Civilization->PersonalNames[this->Gender].push_back(this->Name);
		}
		if (family_name_changed) {
			this->Civilization->FamilyNames.push_back(this->FamilyName);
		}
	}
	
	if (this->Trait == nullptr) { //if no trait was set, have the character be the same trait as the unit type (if the unit type has a single one predefined)
		if (this->Type != nullptr && this->Type->Traits.size() == 1) {
			this->Trait = this->Type->Traits[0];
		}
	}
		
	//check if the abilities are correct for this character's unit type
	if (this->Type != nullptr && this->Abilities.size() > 0 && ((int) AiHelpers.LearnableAbilities.size()) > this->Type->Slot) {
		int ability_count = (int) this->Abilities.size();
		for (int i = (ability_count - 1); i >= 0; --i) {
			if (std::find(AiHelpers.LearnableAbilities[this->Type->Slot].begin(), AiHelpers.LearnableAbilities[this->Type->Slot].end(), this->Abilities[i]) == AiHelpers.LearnableAbilities[this->Type->Slot].end()) {
				this->Abilities.erase(std::remove(this->Abilities.begin(), this->Abilities.end(), this->Abilities[i]), this->Abilities.end());
			}
		}
	}

	this->GenerateMissingDates();
	this->UpdateAttributes();
	
	this->Initialized = true;
}

/**
**	@brief	Generate missing data for the character as a part of history generation
*/
void CCharacter::GenerateHistory()
{
	//generate history (but skip already-generated data)
	bool history_changed = false;
	
	if (!this->PreferredDeityDomains.empty() && this->Deities.size() < PlayerDeityMax && this->CanWorship()) { //if the character can worship deities, but worships less deities than the maximum, generate them for the character
		int deities_missing = PlayerDeityMax - this->Deities.size();
		CReligion *character_religion = this->GetReligion();
		
		for (int i = 0; i < deities_missing; ++i) {
			std::vector<CDeity *> potential_deities;
			int best_score = 0;
			const bool has_major_deity = this->HasMajorDeity();
			
			for (size_t j = 0; j < CDeity::Deities.size(); ++j) {
				CDeity *deity = CDeity::Deities[j];
				
				if (!deity->DeityUpgrade) {
					continue; //don't use deities that have no corresponding deity upgrade for the character
				}
				
				if (deity->Major == has_major_deity) {
					continue; //try to get a major deity if the character has none, or a minor deity otherwise
				}
				
				if (std::find(deity->Civilizations.begin(), deity->Civilizations.end(), this->Civilization) == deity->Civilizations.end()) {
					continue;
				}
				
				if (character_religion && std::find(deity->Religions.begin(), deity->Religions.end(), character_religion) == deity->Religions.end()) {
					continue; //the deity should be compatible with the character's religion
				}
				
				int score = 0;
				
				bool has_domains = true;
				for (size_t k = 0; k < this->PreferredDeityDomains.size(); ++k) {
					if (std::find(deity->Domains.begin(), deity->Domains.end(), this->PreferredDeityDomains[k]) != deity->Domains.end()) {
						score++;
					}
				}
				
				if (score < best_score) {
					continue;
				} else if (score > best_score) {
					potential_deities.clear();
					best_score = score;
				}
				
				potential_deities.push_back(deity);
			}
			
			if (!potential_deities.empty()) {
				CDeity *chosen_deity = potential_deities[SyncRand(potential_deities.size())];
				this->Deities.push_back(chosen_deity);
				this->GeneratedDeities.push_back(chosen_deity);
				
				if (!character_religion) {
					character_religion = this->GetReligion();
				}
				
				history_changed = true;
			} else {
				fprintf(stderr, "Could not generate deity for character: \"%s\".\n", this->Ident.c_str());
			}
		}
	}
	
	if (history_changed) {
		this->SaveHistory();
	}
}

/**
**	@brief	Reset generated history for the character
*/
void CCharacter::ResetHistory()
{
	for (size_t i = 0; i < this->GeneratedDeities.size(); ++i) {
		this->Deities.erase(std::remove(this->Deities.begin(), this->Deities.end(), this->GeneratedDeities[i]), this->Deities.end());
	}
	
	this->GeneratedDeities.clear();
}

/**
**	@brief	Save generated history for the character
*/
void CCharacter::SaveHistory()
{
	struct stat tmp;
	std::string path = Parameters::Instance.GetUserDirectory();

	if (!GameName.empty()) {
		path += "/";
		path += GameName;
	}
	path += "/";
	path += "history/";
	if (stat(path.c_str(), &tmp) < 0) {
		makedir(path.c_str(), 0777);
	}
	path += "characters/";
	if (stat(path.c_str(), &tmp) < 0) {
		makedir(path.c_str(), 0777);
	}
	path += FindAndReplaceString(this->Ident, "-", "_");
	path += ".cfg";

	FILE *fd = fopen(path.c_str(), "w");
	if (!fd) {
		fprintf(stderr, "Cannot open file %s for writing.\n", path.c_str());
		return;
	}

	fprintf(fd, "[character]\n");
	fprintf(fd, "\tident = %s\n", FindAndReplaceString(this->Ident, "-", "_").c_str());
	for (size_t i = 0; i < this->GeneratedDeities.size(); ++i) {
		fprintf(fd, "\tdeity = %s\n", FindAndReplaceString(this->GeneratedDeities[i]->Ident, "-", "_").c_str());
	}
	fprintf(fd, "[/character]\n");
		
	fclose(fd);
}

void CCharacter::GenerateMissingDates()
{
	if (this->DeathDate.Year == 0 && this->BirthDate.Year != 0) { //if the character is missing a death date so far, give it +60 years after the birth date
		this->DeathDate.Year = this->BirthDate.Year + 60;
		this->DeathDate.Month = this->BirthDate.Month;
		this->DeathDate.Day = this->BirthDate.Day;
		this->DeathDate.Timeline = this->BirthDate.Timeline;
	}
	
	if (this->BirthDate.Year == 0 && this->DeathDate.Year != 0) { //if the character is missing a birth date so far, give it 60 years before the death date
		this->BirthDate.Year = this->DeathDate.Year - 60;
		this->BirthDate.Month = this->DeathDate.Month;
		this->BirthDate.Day = this->DeathDate.Day;
		this->BirthDate.Timeline = this->DeathDate.Timeline;
	}
	
	if (this->StartDate.Year == 0 && this->BirthDate.Year != 0) { //if the character is missing a start date so far, give it +30 years after the birth date
		this->StartDate.Year = this->BirthDate.Year + 30;
		this->StartDate.Month = this->BirthDate.Month;
		this->StartDate.Day = this->BirthDate.Day;
		this->StartDate.Timeline = this->BirthDate.Timeline;
	}
	
	if (this->BirthDate.Year == 0 && this->StartDate.Year != 0) { //if the character is missing a birth date so far, give it 30 years before the start date
		this->BirthDate.Year = this->StartDate.Year - 30;
		this->BirthDate.Month = this->StartDate.Month;
		this->BirthDate.Day = this->StartDate.Day;
		this->BirthDate.Timeline = this->StartDate.Timeline;
	}
	
	if (this->DeathDate.Year == 0 && this->StartDate.Year != 0) { //if the character is missing a death date so far, give it +30 years after the start date
		this->DeathDate.Year = this->StartDate.Year + 30;
		this->DeathDate.Month = this->StartDate.Month;
		this->DeathDate.Day = this->StartDate.Day;
		this->DeathDate.Timeline = this->StartDate.Timeline;
	}
	
	if (this->StartDate.Year == 0 && this->DeathDate.Year != 0) { //if the character is missing a start date so far, give it 30 years before the death date
		this->StartDate.Year = this->DeathDate.Year - 30;
		this->StartDate.Month = this->DeathDate.Month;
		this->StartDate.Day = this->DeathDate.Day;
		this->StartDate.Timeline = this->DeathDate.Timeline;
	}
}

int CCharacter::GetMartialAttribute() const
{
	if ((this->Type->Class != -1 && UnitTypeClasses[this->Type->Class] == "thief") || this->Type->DefaultStat.Variables[ATTACKRANGE_INDEX].Value > 1) {
		return DexterityAttribute;
	} else {
		return StrengthAttribute;
	}
}

int CCharacter::GetAttributeModifier(int attribute) const
{
	return this->Attributes[attribute] - 10;
}

/**
**	@brief	Get the character's religion
**
**	@return	The religion if found, or null otherwise
*/
CReligion *CCharacter::GetReligion() const
{
	//get the first religion of the character's first deity, since at present we don't set the religion directly for the character
	
	for (size_t i = 0; i < this->Deities.size(); ++i) {
		if (!this->Deities[i]->Religions.empty()) {
			return this->Deities[i]->Religions[0];
		}
	}
	
	return nullptr;
}

CLanguage *CCharacter::GetLanguage() const
{
	return PlayerRaces.GetCivilizationLanguage(this->Civilization->ID);
}

CCalendar *CCharacter::GetCalendar() const
{
	if (this->Civilization) {
		return this->Civilization->GetCalendar();
	}
	
	return CCalendar::BaseCalendar;
}

bool CCharacter::IsParentOf(const std::string &child_ident) const
{
	for (size_t i = 0; i < this->Children.size(); ++i) {
		if (this->Children[i]->Ident == child_ident) {
			return true;
		}
	}
	return false;
}

bool CCharacter::IsChildOf(const std::string &parent_ident) const
{
	if ((this->Father != nullptr && this->Father->Ident == parent_ident) || (this->Mother != nullptr && this->Mother->Ident == parent_ident)) {
		return true;
	}
	return false;
}

bool CCharacter::IsSiblingOf(const std::string &sibling_ident) const
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

bool CCharacter::IsUsable() const
{
	if (this->Type->DefaultStat.Variables[GENDER_INDEX].Value != 0 && this->Gender != this->Type->DefaultStat.Variables[GENDER_INDEX].Value) {
		return false; // hero not usable if their unit type has a set gender which is different from the hero's (this is because this means that the unit type lacks appropriate graphics for that gender)
	}
	
	return true;
}

bool CCharacter::CanAppear(bool ignore_neutral) const
{
	if (!this->IsUsable()) {
		return false;
	}
	
	for (int i = 0; i < PlayerMax; ++i) {
		if (ignore_neutral && i == PlayerNumNeutral) {
			continue;
		}
		if (Players[i].HasHero(this)) {
			return false;
		}
	}

	return true;
}

/**
**	@brief	Get whether the character can worship a deity
**
**	@return True if the character can worship, false otherwise
*/
bool CCharacter::CanWorship() const
{
	if (this->Deity) {
		return false; //the character cannot worship a deity if it is itself a deity
	}
	
	if (this->Type->BoolFlag[FAUNA_INDEX].value) {
		return false; //the character cannot worship a deity if it is not sentient
	}
	
	return true;
}

/**
**	@brief	Get whether the character has a major deity in its worshipped deities list
**
**	@return True if the character has a major deity, false otherwise
*/
bool CCharacter::HasMajorDeity() const
{
	for (size_t i = 0; i < this->Deities.size(); ++i) {
		if (this->Deities[i]->Major) {
			return true;
		}
	}

	return false;
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

IconConfig CCharacter::GetIcon() const
{
	if (this->Level >= 3 && this->HeroicIcon.Icon) {
		return this->HeroicIcon;
	} else if (this->Icon.Icon) {
		return this->Icon;
	} else if (!this->HairVariation.empty() && this->Type->GetVariation(this->HairVariation) != nullptr && !this->Type->GetVariation(this->HairVariation)->Icon.Name.empty()) {
		return this->Type->GetVariation(this->HairVariation)->Icon;
	} else {
		return this->Type->Icon;
	}
}

CPersistentItem *CCharacter::GetItem(CUnit &item) const
{
	for (size_t i = 0; i < this->Items.size(); ++i) {
		if (this->Items[i]->Type == item.Type && this->Items[i]->Prefix == item.Prefix && this->Items[i]->Suffix == item.Suffix && this->Items[i]->Spell == item.Spell && this->Items[i]->Work == item.Work && this->Items[i]->Elixir == item.Elixir && this->Items[i]->Unique == item.Unique && this->Items[i]->Bound == item.Bound && this->Items[i]->Identified == item.Identified && this->IsItemEquipped(this->Items[i]) == item.Container->IsItemEquipped(&item)) {
			if (this->Items[i]->Name.empty() || this->Items[i]->Name == item.Name) {
				return this->Items[i];
			}
		}
	}
	return nullptr;
}

void CCharacter::UpdateAttributes()
{
	if (this->Type == nullptr) {
		return;
	}
	
	for (int i = 0; i < MaxAttributes; ++i) {
		int var = GetAttributeVariableIndex(i);
		this->Attributes[i] = this->Type->DefaultStat.Variables[var].Value;
		for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
			if (
				(this->Trait != nullptr && modifier->UpgradeId == this->Trait->ID)
				|| std::find(this->Abilities.begin(), this->Abilities.end(), AllUpgrades[modifier->UpgradeId]) != this->Abilities.end()
			) {
				if (modifier->Modifier.Variables[var].Value != 0) {
					this->Attributes[i] += modifier->Modifier.Variables[var].Value;
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

CCharacter *GetCharacter(const std::string &character_ident)
{
	if (Characters.find(character_ident) != Characters.end()) {
		return Characters[character_ident];
	}
	
	for (std::map<std::string, CCharacter *>::iterator iterator = Characters.begin(); iterator != Characters.end(); ++iterator) { // for backwards compatibility
		if (iterator->second->GetFullName() == character_ident) {
			return iterator->second;
		}
	}

	return nullptr;
}

CCharacter *GetCustomHero(const std::string &hero_ident)
{
	if (CustomHeroes.find(hero_ident) != CustomHeroes.end()) {
		return CustomHeroes[hero_ident];
	}
	
	for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) { // for backwards compatibility
		if (iterator->second->GetFullName() == hero_ident) {
			return iterator->second;
		}
	}
	
	return nullptr;
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
	if (stat(path.c_str(), &tmp) < 0) {
		makedir(path.c_str(), 0777);
	}
	if (hero->Custom) {
		path += "custom/";
		if (stat(path.c_str(), &tmp) < 0) {
			makedir(path.c_str(), 0777);
		}
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
		if (hero->Civilization) {
			fprintf(fd, "\tCivilization = \"%s\",\n", PlayerRaces.Name[hero->Civilization->ID].c_str());
		}
	}
	if (hero->Type != nullptr) {
		fprintf(fd, "\tType = \"%s\",\n", hero->Type->Ident.c_str());
	}
	if (hero->Custom) {
		if (hero->Trait != nullptr) {
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
	if (hero->Custom && hero->Deities.size() > 0) {
		fprintf(fd, "\tDeities = {");
		for (size_t j = 0; j < hero->Deities.size(); ++j) {
			fprintf(fd, "\"%s\"", hero->Deities[j]->Ident.c_str());
			if (j < (hero->Deities.size() - 1)) {
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
			if (hero->Items[j]->Prefix != nullptr) {
				fprintf(fd, "\n\t\t\t\"prefix\", \"%s\",", hero->Items[j]->Prefix->Ident.c_str());
			}
			if (hero->Items[j]->Suffix != nullptr) {
				fprintf(fd, "\n\t\t\t\"suffix\", \"%s\",", hero->Items[j]->Suffix->Ident.c_str());
			}
			if (hero->Items[j]->Spell != nullptr) {
				fprintf(fd, "\n\t\t\t\"spell\", \"%s\",", hero->Items[j]->Spell->Ident.c_str());
			}
			if (hero->Items[j]->Work != nullptr) {
				fprintf(fd, "\n\t\t\t\"work\", \"%s\",", hero->Items[j]->Work->Ident.c_str());
			}
			if (hero->Items[j]->Elixir != nullptr) {
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

void HeroAddQuest(const std::string &hero_full_name, const std::string &quest_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
	}
	
	CQuest *quest = GetQuest(quest_name);
	if (!quest) {
		fprintf(stderr, "Quest \"%s\" does not exist.\n", quest_name.c_str());
	}
	
	hero->QuestsInProgress.push_back(quest);
}

void HeroCompleteQuest(const std::string &hero_full_name, const std::string &quest_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
	}
	
	CQuest *quest = GetQuest(quest_name);
	if (!quest) {
		fprintf(stderr, "Quest \"%s\" does not exist.\n", quest_name.c_str());
	}
	
	hero->QuestsInProgress.erase(std::remove(hero->QuestsInProgress.begin(), hero->QuestsInProgress.end(), quest), hero->QuestsInProgress.end());
	hero->QuestsCompleted.push_back(quest);
}

void SaveCustomHero(const std::string &hero_full_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
	}
	
	SaveHero(hero);
}

void DeleteCustomHero(const std::string &hero_full_name)
{
	CCharacter *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
	}
	
	if (CurrentCustomHero == hero) {
		CurrentCustomHero = nullptr;
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

void SetCurrentCustomHero(const std::string &hero_full_name)
{
	if (!hero_full_name.empty()) {
		CCharacter *hero = GetCustomHero(hero_full_name);
		if (!hero) {
			fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
		}
		
		CurrentCustomHero = const_cast<CCharacter *>(&(*hero));
	} else {
		CurrentCustomHero = nullptr;
	}
}

std::string GetCurrentCustomHero()
{
	if (CurrentCustomHero != nullptr) {
		return CurrentCustomHero->Ident;
	} else {
		return "";
	}
}

void ChangeCustomHeroCivilization(const std::string &hero_full_name, const std::string &civilization_name, const std::string &new_hero_name, const std::string &new_hero_family_name)
{
	if (!hero_full_name.empty()) {
		CCharacter *hero = GetCustomHero(hero_full_name);
		if (!hero) {
			fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
		}
		
		CCivilization *civilization = CCivilization::GetCivilization(civilization_name);
		if (civilization) {
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
			int new_unit_type_id = PlayerRaces.GetCivilizationClassUnitType(hero->Civilization->ID, hero->Type->Class);
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

bool IsNameValidForCustomHero(const std::string &hero_name, const std::string &hero_family_name)
{
	std::string hero_full_name = hero_name;
	if (!hero_family_name.empty()) {
		hero_full_name += " ";
		hero_full_name += hero_family_name;
	}
	
	if (GetCustomHero(hero_full_name) != nullptr) {
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

int GetGenderIdByName(const std::string &gender)
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
	} else if (title == CharacterTitleMayor) {
		return "mayor";
	}

	return "";
}

int GetCharacterTitleIdByName(const std::string &title)
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
	} else if (title == "mayor") {
		return CharacterTitleMayor;
	}

	return -1;
}

bool IsMinisterialTitle(int title)
{
	return (title == CharacterTitleHeadOfState || title == CharacterTitleHeadOfGovernment || title == CharacterTitleEducationMinister || title == CharacterTitleFinanceMinister || title == CharacterTitleForeignMinister || title == CharacterTitleIntelligenceMinister || title == CharacterTitleInteriorMinister || title == CharacterTitleJusticeMinister || title == CharacterTitleWarMinister);
}
//@}
