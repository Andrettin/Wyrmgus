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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "character.h"

#include "ai/ai_local.h" //for using AiHelpers
#include "civilization.h"
#include "config.h"
#include "config_operator.h"
#include "faction.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "item/item.h"
#include "item/item_class.h"
#include "item/unique_item.h"
#include "language/word.h"
#include "map/historical_location.h"
#include "map/map_template.h"
#include "map/site.h"
#include "parameters.h"
#include "player.h"
#include "quest/quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "species/gender.h"
#include "spell/spells.h"
#include "time/calendar.h"
#include "ui/icon.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "world/province.h"

#include <ctype.h>
#include <map>
#include <string>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

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
	
	for (const CHistoricalLocation *historical_location : this->HistoricalLocations) {
		delete historical_location;
	}
	
	for (CPersistentItem *item : this->Items) {
		delete item;
	}
}

void CCharacter::Clear()
{
	DataType<CCharacter>::Clear();
	
	for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
		delete iterator->second;
	}
	CustomHeroes.clear();
}

void CCharacter::GenerateCharacterHistory()
{
	for (CCharacter *character : CCharacter::GetAll()) {
		character->GenerateHistory();
	}
}

void CCharacter::ResetCharacterHistory()
{
	for (CCharacter *character : CCharacter::GetAll()) {
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
	
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
			continue;
		}
		
		String key = property.Key;
		String value = property.Value;
		
		if (key == "name") {
			this->Name = value.utf8().get_data();
			name_changed = true;
		} else if (key == "name_word") {
			CWord *name_word = CWord::Get(value);
			if (name_word != nullptr) {
				this->NameWord = name_word;
				if (this->Name.empty()) {
					this->Name = this->NameWord->GetAnglicizedName();
				}
			}
		} else if (key == "family_name") {
			this->FamilyName = value.c_str();
			family_name_changed = true;
		} else if (key == "family_name_word") {
			CWord *family_name_word = CWord::Get(value);
			if (family_name_word != nullptr) {
				this->FamilyNameWord = family_name_word;
				if (this->FamilyName.empty()) {
					this->FamilyName = this->FamilyNameWord->GetAnglicizedName();
				}
			}
		} else if (key == "description") {
			this->Description = value.c_str();
		} else if (key == "background") {
			this->Background = value.c_str();
		} else if (key == "quote") {
			this->Quote = value.c_str();
		} else if (key == "unit_type") {
			CUnitType *unit_type = CUnitType::Get(value);
			if (unit_type) {
				if (this->UnitType == nullptr || this->UnitType == unit_type || this->UnitType->CanExperienceUpgradeTo(unit_type)) {
					this->UnitType = unit_type;
					if (this->Level < this->UnitType->DefaultStat.Variables[LEVEL_INDEX].Value) {
						this->Level = this->UnitType->DefaultStat.Variables[LEVEL_INDEX].Value;
					}
					
					if (this->Gender == nullptr) { //if no gender was set so far, have the character be the same gender as the unit type (if the unit type has it predefined)
						if (this->UnitType->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
							this->Gender = CGender::Get(this->UnitType->DefaultStat.Variables[GENDER_INDEX].Value - 1);
						}
					}
				}
			} else {
				fprintf(stderr, "Unit type \"%s\" does not exist.\n", value.utf8().get_data());
			}
		} else if (key == "gender") {
			this->Gender = CGender::Get(value);
		} else if (key == "civilization") {
			this->Civilization = CCivilization::Get(value);
		} else if (key == "faction") {
			CFaction *faction = CFaction::Get(value);
			if (faction != nullptr) {
				if (this->Faction == nullptr) {
					this->Faction = faction;
				}
				this->Factions.push_back(faction);
			}
		} else if (key == "home_site") {
			this->HomeSite = CSite::Get(value);
		} else if (key == "hair_variation") {
			value = value.replace("_", "-");
			this->HairVariation = value.utf8().get_data();
		} else if (key == "trait") {
			value = value.replace("_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value.utf8().get_data());
			if (upgrade) {
				this->Trait = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.utf8().get_data());
			}
		} else if (key == "level") {
			this->Level = value.to_int();
		} else if (key == "birth_date") {
			value = value.replace("_", "-");
			this->BirthDate = CDate::FromString(value.utf8().get_data());
		} else if (key == "start_date") {
			value = value.replace("_", "-");
			this->StartDate = CDate::FromString(value.utf8().get_data());
		} else if (key == "death_date") {
			value = value.replace("_", "-");
			this->DeathDate = CDate::FromString(value.utf8().get_data());
		} else if (key == "violent_death") {
			this->ViolentDeath = StringToBool(value);
		} else if (key == "father") {
			CCharacter *father = CCharacter::Get(value);
			if (father) {
				if (!father->IsInitialized() || father->Gender->IsFather()) {
					this->Father = father;
					if (!father->IsParentOf(this)) { //check whether the character has already been set as a child of the father
						father->Children.push_back(this);
					}
					// see if the father's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
					for (CCharacter *sibling : father->Children) {
						if (sibling != this) {
							if (!this->IsSiblingOf(sibling)) {
								this->Siblings.push_back(sibling);
							}
							if (!sibling->IsSiblingOf(this)) {
								sibling->Siblings.push_back(this);
							}
						}
					}
				} else {
					fprintf(stderr, "Character \"%s\" set to be the biological father of \"%s\", but isn't male.\n", value.utf8().get_data(), this->Ident.c_str());
				}
			}
		} else if (key == "mother") {
			CCharacter *mother = CCharacter::Get(value);
			if (mother) {
				if (!mother->IsInitialized() || !mother->Gender->IsFather()) {
					this->Mother = mother;
					if (!mother->IsParentOf(this)) { //check whether the character has already been set as a child of the mother
						mother->Children.push_back(this);
					}
					// see if the mother's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
					for (CCharacter *sibling : mother->Children) {
						if (sibling != this) {
							if (!this->IsSiblingOf(sibling)) {
								this->Siblings.push_back(sibling);
							}
							if (!sibling->IsSiblingOf(this)) {
								sibling->Siblings.push_back(this);
							}
						}
					}
				} else {
					fprintf(stderr, "Character \"%s\" set to be the biological mother of \"%s\", but isn't female.\n", value.utf8().get_data(), this->Ident.c_str());
				}
			}
		} else if (key == "deity") {
			CDeity *deity = CDeity::Get(value);
			if (deity) {
				this->Deities.push_back(deity);
				if (LoadingHistory) {
					this->GeneratedDeities.push_back(deity);
				}
			}
		} else if (key == "icon") {
			value = value.replace("_", "-");
			this->Icon.Name = value.utf8().get_data();
			this->Icon.Icon = nullptr;
			this->Icon.Load();
			this->Icon.Icon->Load();
		} else if (key == "heroic_icon") {
			value = value.replace("_", "-");
			this->HeroicIcon.Name = value.utf8().get_data();
			this->HeroicIcon.Icon = nullptr;
			this->HeroicIcon.Load();
			this->HeroicIcon.Icon->Load();
		} else if (key == "forbidden_upgrade") {
			CUnitType *unit_type = CUnitType::Get(value);
			if (unit_type) {
				this->ForbiddenUpgrades.push_back(unit_type);
			} else {
				fprintf(stderr, "Unit type \"%s\" does not exist.\n", value.utf8().get_data());
			}
		} else if (key == "ability") {
			value = value.replace("_", "-");
			const CUpgrade *ability_upgrade = CUpgrade::Get(value.utf8().get_data());
			if (ability_upgrade != nullptr) {
				this->Abilities.push_back(ability_upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.utf8().get_data());
			}
		} else if (key == "read_work") {
			value = value.replace("_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value.utf8().get_data());
			if (upgrade) {
				this->ReadWorks.push_back(upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.utf8().get_data());
			}
		} else if (key == "consumed_elixir") {
			value = value.replace("_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value.utf8().get_data());
			if (upgrade) {
				this->ConsumedElixirs.push_back(upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.utf8().get_data());
			}
		} else if (key == "preferred_deity_domain") {
			CDeityDomain *deity_domain = CDeityDomain::Get(value);
			if (deity_domain) {
				this->PreferredDeityDomains.push_back(deity_domain);
			}
		} else {
			fprintf(stderr, "Invalid character property: \"%s\".\n", key.utf8().get_data());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "historical_location") {
			CHistoricalLocation *historical_location = new CHistoricalLocation;
			historical_location->ProcessConfigData(section);
			this->HistoricalLocations.push_back(historical_location);
		} else if (section->Tag == "historical_title") {
			int title = -1;
			CDate start_date;
			CDate end_date;
			CFaction *title_faction = nullptr;
				
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
					continue;
				}
				
				String key = property.Key;
				String value = property.Value;
				
				if (key == "title") {
					value = value.replace("_", "-");
					title = GetCharacterTitleIdByName(value.utf8().get_data());
					if (title == -1) {
						fprintf(stderr, "Character title \"%s\" does not exist.\n", value.utf8().get_data());
					}
				} else if (key == "start_date") {
					value = value.replace("_", "-");
					start_date = CDate::FromString(value.utf8().get_data());
				} else if (key == "end_date") {
					value = value.replace("_", "-");
					end_date = CDate::FromString(value.utf8().get_data());
				} else if (key == "faction") {
					title_faction = CFaction::Get(value);
				} else {
					fprintf(stderr, "Invalid historical title property: \"%s\".\n", key.utf8().get_data());
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
		} else if (section->Tag == "item") {
			CPersistentItem *item = new CPersistentItem;
			item->Owner = this;
			this->Items.push_back(item);
			item->ProcessConfigData(section);
		} else {
			fprintf(stderr, "Invalid character property: \"%s\".\n", section->Tag.utf8().get_data());
		}
	}
	
	//use the character's name for name generation (do this only after setting all properties so that the type, civilization and gender will have been parsed if given
	if (!this->UnitType->BoolFlag[FAUNA_INDEX].value && this->Civilization) {
		if (name_changed) {
			this->Civilization->PersonalNames[this->Gender].push_back(this->Name.utf8().get_data());
		}
		if (family_name_changed) {
			this->Civilization->FamilyNames.push_back(this->FamilyName.utf8().get_data());
		}
	}
	
	if (this->Trait == nullptr) { //if no trait was set, have the character be the same trait as the unit type (if the unit type has a single one predefined)
		if (this->UnitType != nullptr && this->UnitType->Traits.size() == 1) {
			this->Trait = this->UnitType->Traits[0];
		}
	}
		
	//check if the abilities are correct for this character's unit type
	if (this->UnitType != nullptr && this->Abilities.size() > 0 && ((int) AiHelpers.LearnableAbilities.size()) > this->UnitType->GetIndex()) {
		int ability_count = (int) this->Abilities.size();
		for (int i = (ability_count - 1); i >= 0; --i) {
			const CUpgrade *ability_upgrade = this->Abilities[i];
			if (std::find(AiHelpers.LearnableAbilities[this->UnitType->GetIndex()].begin(), AiHelpers.LearnableAbilities[this->UnitType->GetIndex()].end(), ability_upgrade) == AiHelpers.LearnableAbilities[this->UnitType->GetIndex()].end()) {
				this->Abilities.erase(std::remove(this->Abilities.begin(), this->Abilities.end(), ability_upgrade), this->Abilities.end());
			}
		}
	}
	
	if (this->NameWord != nullptr) {
		if (this->UnitType != nullptr && this->UnitType->GetSpecies() != nullptr && this->UnitType->BoolFlag[FAUNA_INDEX].value) {
			if (this->Gender != nullptr) {
				this->NameWord->ChangeSpecimenNameWeight(this->UnitType->GetSpecies(), this->Gender, 1);
			} else {
				//add name weight for all genders
				for (const CGender *gender : CGender::GetAll()) {
					this->NameWord->ChangeSpecimenNameWeight(this->UnitType->GetSpecies(), gender, 1);
				}
			}
		} else {
			if (this->Gender != nullptr) {
				this->NameWord->ChangePersonalNameWeight(this->Gender, 1);
			} else {
				//add name weight for all genders
				for (const CGender *gender : CGender::GetAll()) {
					this->NameWord->ChangePersonalNameWeight(gender, 1);
				}
			}
		}
	}

	if (this->FamilyNameWord != nullptr) {
		this->FamilyNameWord->ChangeFamilyNameWeight(1);
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
			
			for (CDeity *deity : CDeity::GetAll()) {
				if (!deity->DeityUpgrade) {
					continue; //don't use deities that have no corresponding deity upgrade for the character
				}
				
				if (deity->IsMajor() == has_major_deity) {
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
					if (std::find(deity->GetDomains().begin(), deity->GetDomains().end(), this->PreferredDeityDomains[k]) != deity->GetDomains().end()) {
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
	if ((this->UnitType->GetClass() != nullptr && this->UnitType->GetClass()->Ident == "thief") || this->UnitType->DefaultStat.Variables[ATTACKRANGE_INDEX].Value > 1) {
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
	return this->Civilization->GetLanguage();
}

CCalendar *CCharacter::GetCalendar() const
{
	if (this->Civilization) {
		return this->Civilization->GetCalendar();
	}
	
	return CCalendar::BaseCalendar;
}

bool CCharacter::IsParentOf(const CCharacter *character) const
{
	return std::find(this->Children.begin(), this->Children.end(), character) != this->Children.end();
}

bool CCharacter::IsChildOf(const CCharacter *character) const
{
	if (character != nullptr && (this->Father == character || this->Mother == character)) {
		return true;
	}
	
	return false;
}

bool CCharacter::IsSiblingOf(const CCharacter *character) const
{
	return std::find(this->Siblings.begin(), this->Siblings.end(), character) != this->Siblings.end();
}

bool CCharacter::IsItemEquipped(const CPersistentItem *item) const
{
	const ItemSlot *item_slot = item->Type->ItemClass->GetSlot();
	
	if (item_slot == nullptr) {
		return false;
	}
	
	std::map<const ItemSlot *, std::vector<CPersistentItem *>>::const_iterator find_iterator = this->EquippedItems.find(item_slot);
	if (find_iterator != this->EquippedItems.end() && std::find(find_iterator->second.begin(), find_iterator->second.end(), item) != find_iterator->second.end()) {
		return true;
	}
	
	return false;
}

bool CCharacter::IsUsable() const
{
	if (this->UnitType->DefaultStat.Variables[GENDER_INDEX].Value != 0 && (this->Gender->GetIndex() + 1) != this->UnitType->DefaultStat.Variables[GENDER_INDEX].Value) {
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
		if (CPlayer::Players[i]->HasHero(this)) {
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
	
	if (this->UnitType->BoolFlag[FAUNA_INDEX].value) {
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
	for (const CDeity *deity : this->Deities) {
		if (deity->IsMajor()) {
			return true;
		}
	}

	return false;
}

String CCharacter::GetFullName() const
{
	String full_name = this->GetName();
	if (!this->GetExtraName().empty()) {
		full_name += " " + this->GetExtraName();
	}
	if (!this->GetFamilyName().empty()) {
		full_name += " " + this->GetFamilyName();
	}
	return full_name;
}

CIcon *CCharacter::GetIcon() const
{
	if (this->Level >= 3 && this->HeroicIcon.Icon) {
		return this->HeroicIcon.Icon;
	} else if (this->Icon.Icon) {
		return this->Icon.Icon;
	} else if (!this->HairVariation.empty() && this->UnitType->GetVariation(this->HairVariation) != nullptr && !this->UnitType->GetVariation(this->HairVariation)->Icon.Name.empty()) {
		return this->UnitType->GetVariation(this->HairVariation)->Icon.Icon;
	} else {
		return this->UnitType->GetIcon();
	}
}

CPersistentItem *CCharacter::GetItem(const CUnit *item) const
{
	for (CPersistentItem *persistent_item : this->Items) {
		if (
			persistent_item->Type == item->GetType()
			&& persistent_item->Prefix == item->Prefix
			&& persistent_item->Suffix == item->Suffix
			&& persistent_item->Spell == item->Spell
			&& persistent_item->Work == item->Work
			&& persistent_item->Elixir == item->Elixir
			&& persistent_item->Unique == item->Unique
			&& persistent_item->Bound == item->Bound
			&& persistent_item->Identified == item->Identified
			&& this->IsItemEquipped(persistent_item) == item->Container->IsItemEquipped(item)
		) {
			if (persistent_item->Name.empty() || persistent_item->Name == item->Name.c_str()) {
				return persistent_item;
			}
		}
	}
	return nullptr;
}

void CCharacter::UpdateAttributes()
{
	if (this->UnitType == nullptr) {
		return;
	}
	
	for (int i = 0; i < MaxAttributes; ++i) {
		int var = GetAttributeVariableIndex(i);
		this->Attributes[i] = this->UnitType->DefaultStat.Variables[var].Value;
		for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
			if (
				(this->Trait != nullptr && modifier->UpgradeId == this->Trait->GetIndex())
				|| std::find(this->Abilities.begin(), this->Abilities.end(), CUpgrade::Get(modifier->UpgradeId)) != this->Abilities.end()
			) {
				if (modifier->Modifier.Variables[var].Value != 0) {
					this->Attributes[i] += modifier->Modifier.Variables[var].Value;
				}
			}
		}
	}
}

CUnit *CCharacter::GetUnit() const
{
	for (const CPlayer *player : CPlayer::Players) {
		CUnit *hero_unit = player->GetHeroUnit(this);
		
		if (hero_unit != nullptr) {
			return hero_unit;
		}
	}
	
	return nullptr;
}

void CCharacter::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_name_word"), +[](const CCharacter *character){ return character->NameWord; });
	ClassDB::bind_method(D_METHOD("get_extra_name"), &CCharacter::GetExtraName);
	ClassDB::bind_method(D_METHOD("get_family_name"), &CCharacter::GetFamilyName);
	ClassDB::bind_method(D_METHOD("get_family_name_word"), +[](const CCharacter *character){ return character->FamilyNameWord; });
	ClassDB::bind_method(D_METHOD("get_full_name"), &CCharacter::GetFullName);
	ClassDB::bind_method(D_METHOD("is_usable"), &CCharacter::IsUsable);
	ClassDB::bind_method(D_METHOD("get_civilization"), &CCharacter::GetCivilization);
	ClassDB::bind_method(D_METHOD("get_faction"), +[](const CCharacter *character){ return const_cast<CFaction *>(character->GetFaction()); });
	ClassDB::bind_method(D_METHOD("get_unit_type"), +[](const CCharacter *character){ return const_cast<CUnitType *>(character->GetUnitType()); });
	ClassDB::bind_method(D_METHOD("get_level"), &CCharacter::GetLevel);
	ClassDB::bind_method(D_METHOD("get_father"), &CCharacter::GetFather);
	ClassDB::bind_method(D_METHOD("get_mother"), &CCharacter::GetMother);
	ClassDB::bind_method(D_METHOD("get_children"), +[](const CCharacter *character){ return ContainerToGodotArray(character->GetChildren()); });
	ClassDB::bind_method(D_METHOD("get_siblings"), +[](const CCharacter *character){ return ContainerToGodotArray(character->GetSiblings()); });
	ClassDB::bind_method(D_METHOD("get_home_site"), +[](const CCharacter *character){ return const_cast<CSite *>(character->GetHomeSite()); });
	ClassDB::bind_method(D_METHOD("get_deities"), +[](const CCharacter *character){ return ContainerToGodotArray(character->Deities); });
	ClassDB::bind_method(D_METHOD("get_abilities"), +[](const CCharacter *character){ return ContainerToGodotArray(character->Abilities); });
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

CCharacter *GetCustomHero(const std::string &hero_ident)
{
	if (CustomHeroes.find(hero_ident) != CustomHeroes.end()) {
		return CustomHeroes[hero_ident];
	}
	
	for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) { // for backwards compatibility
		if (iterator->second->GetFullName() == hero_ident.c_str()) {
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
	for (CCharacter *character : CCharacter::GetAll()) { //save characters
		SaveHero(character);
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
	old_path += hero->GetFullName().utf8().get_data();
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
		fprintf(fd, "DefineCharacter(\"%s\", {\n", hero->GetIdent().utf8().get_data());
	} else {
		fprintf(fd, "DefineCustomHero(\"%s\", {\n", hero->GetIdent().utf8().get_data());
		fprintf(fd, "\tName = \"%s\",\n", hero->GetName().utf8().get_data());
		if (!hero->GetExtraName().empty()) {
			fprintf(fd, "\tExtraName = \"%s\",\n", hero->GetExtraName().utf8().get_data());
		}
		if (!hero->GetFamilyName().empty()) {
			fprintf(fd, "\tFamilyName = \"%s\",\n", hero->GetFamilyName().utf8().get_data());
		}
		if (hero->GetGender() != nullptr) {
			fprintf(fd, "\tGender = \"%s\",\n", hero->GetGender()->GetIdent().utf8().get_data());
		}
		if (hero->Civilization != nullptr) {
			fprintf(fd, "\tCivilization = \"%s\",\n", hero->Civilization->GetIdent().utf8().get_data());
		}
	}
	if (hero->UnitType != nullptr) {
		fprintf(fd, "\tType = \"%s\",\n", hero->UnitType->Ident.c_str());
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
				fprintf(fd, "\n\t\t\t\"name\", \"%s\",", hero->Items[j]->Name.utf8().get_data());
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
				fprintf(fd, "\"%s\"", hero->QuestsInProgress[j]->Ident.c_str());
				if (j < (hero->QuestsInProgress.size() - 1)) {
					fprintf(fd, ", ");
				}
			}
			fprintf(fd, "},\n");
		}
		if (hero->QuestsCompleted.size() > 0) {
			fprintf(fd, "\tQuestsCompleted = {");
			for (size_t j = 0; j < hero->QuestsCompleted.size(); ++j) {
				fprintf(fd, "\"%s\"", hero->QuestsCompleted[j]->Ident.c_str());
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
	
	CQuest *quest = CQuest::Get(quest_name);
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
	
	CQuest *quest = CQuest::Get(quest_name);
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
		
		CCivilization *civilization = CCivilization::Get(civilization_name);
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
			const CUnitType *new_unit_type = CCivilization::GetCivilizationClassUnitType(hero->Civilization, hero->UnitType->GetClass());
			if (new_unit_type != nullptr) {
				hero->UnitType = new_unit_type;
				hero->Name = new_hero_name.c_str();
				hero->FamilyName = new_hero_family_name.c_str();
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
