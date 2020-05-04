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

#include "stratagus.h"

#include "character.h"

#include "ai/ai_local.h" //for using AiHelpers
#include "civilization.h"
#include "config.h"
#include "faction.h"
#include "game.h"
#include "iocompat.h"
#include "iolib.h"
#include "item.h"
#include "map/historical_location.h"
#include "map/map_template.h"
#include "map/site.h"
#include "parameters.h"
#include "player.h"
#include "province.h"
#include "quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "spells.h"
#include "time/calendar.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "util/string_util.h"
#include "util/vector_util.h"

std::map<std::string, stratagus::character *> CustomHeroes;
stratagus::character *CurrentCustomHero = nullptr;
bool LoadingPersistentHeroes = false;

namespace stratagus {

void character::clear()
{
	data_type::clear();
	
	for (std::map<std::string, character *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
		delete iterator->second;
	}
	CustomHeroes.clear();
}

character::character(const std::string &identifier) : detailed_data_entry(identifier), CDataType(identifier)
{
	memset(Attributes, 0, sizeof(Attributes));
}

character::~character()
{
	if (this->Conditions) {
		delete Conditions;
	}
	
	for (CPersistentItem *item : this->Items) {
		delete item;
	}
}

void character::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "gender") {
		this->Gender = GetGenderIdByName(value);
	} else {
		data_entry::process_sml_property(property);
	}
}

void character::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "deities") {
		for (const std::string &value : values) {
			CDeity *deity = CDeity::GetDeity(value);
			if (deity) {
				this->Deities.push_back(deity);
			}
		}
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void character::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "family_name") {
			this->surname = value;
		} else if (key == "unit_type") {
			CUnitType *unit_type = CUnitType::get(value);
			if (this->get_unit_type() == nullptr || this->get_unit_type() == unit_type || this->get_unit_type()->CanExperienceUpgradeTo(unit_type)) {
				this->unit_type = unit_type;
			}
		} else if (key == "gender") {
			this->Gender = GetGenderIdByName(value);
		} else if (key == "civilization") {
			this->civilization = civilization::get(value);
		} else if (key == "faction") {
			faction *faction = faction::get(value);
			if (!this->Faction) {
				this->Faction = faction;
			}
			this->Factions.push_back(faction);
		} else if (key == "hair_variation") {
			value = FindAndReplaceString(value, "_", "-");
			this->HairVariation = value;
		} else if (key == "trait") {
			CUpgrade *upgrade = CUpgrade::try_get(value);
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
		} else if (key == "father") {
			character *father = character::get(value);
			this->Father = father;
			if (!father->IsParentOf(this->Ident)) { //check whether the character has already been set as a child of the father
				father->Children.push_back(this);
			}
			// see if the father's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
			for (character *sibling : father->Children) {
				if (sibling != this) {
					if (!this->IsSiblingOf(sibling->Ident)) {
						this->Siblings.push_back(sibling);
					}
					if (!sibling->IsSiblingOf(this->Ident)) {
						sibling->Siblings.push_back(this);
					}
				}
			}
		} else if (key == "mother") {
			character *mother = character::get(value);
			this->Mother = mother;
			if (!mother->IsParentOf(this->Ident)) { //check whether the character has already been set as a child of the mother
				mother->Children.push_back(this);
			}
			// see if the mother's other children aren't already included in the character's siblings, and if they aren't, add them (and add the character to the siblings' sibling list, of course)
			for (character *sibling : mother->Children) {
				if (sibling != this) {
					if (!this->IsSiblingOf(sibling->Ident)) {
						this->Siblings.push_back(sibling);
					}
					if (!sibling->IsSiblingOf(this->Ident)) {
						sibling->Siblings.push_back(this);
					}
				}
			}
		} else if (key == "deity") {
			value = FindAndReplaceString(value, "_", "-");
			CDeity *deity = CDeity::GetDeity(value);
			if (deity) {
				this->Deities.push_back(deity);
			}
		} else if (key == "description") {
			this->set_description(value);
		} else if (key == "background") {
			this->set_background(value);
		} else if (key == "quote") {
			this->set_quote(value);
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
		} else if (key == "heroic_icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->HeroicIcon.Name = value;
			this->HeroicIcon.Icon = nullptr;
			this->HeroicIcon.Load();
		} else if (key == "forbidden_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			CUnitType *unit_type = CUnitType::get(value);
			this->ForbiddenUpgrades.push_back(unit_type);
		} else if (key == "ability") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *ability_upgrade = CUpgrade::try_get(value);
			if (ability_upgrade) {
				this->Abilities.push_back(ability_upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "read_work") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->ReadWorks.push_back(upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "consumed_elixir") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->ConsumedElixirs.push_back(upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else {
			fprintf(stderr, "Invalid character property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "historical_location") {
			auto location = std::make_unique<historical_location>();
			location->ProcessConfigData(child_config_data);
			this->HistoricalLocations.push_back(std::move(location));
		} else if (child_config_data->Tag == "historical_title") {
			int title = -1;
			CDate start_date;
			CDate end_date;
			faction *title_faction = nullptr;
				
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
					title_faction = faction::get(value);
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
				
			this->HistoricalTitles.push_back(std::tuple<CDate, CDate, faction *, int>(start_date, end_date, title_faction, title));
		} else if (child_config_data->Tag == "item") {
			CPersistentItem *item = new CPersistentItem;
			item->Owner = this;
			this->Items.push_back(item);
			item->ProcessConfigData(child_config_data);
		} else {
			fprintf(stderr, "Invalid character property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void character::initialize()
{
	if (this->Level < this->get_unit_type()->DefaultStat.Variables[LEVEL_INDEX].Value) {
		this->Level = this->get_unit_type()->DefaultStat.Variables[LEVEL_INDEX].Value;
	}

	if (this->Gender == NoGender) { //if no gender was set so far, have the character be the same gender as the unit type (if the unit type has it predefined)
		if (this->get_unit_type()->DefaultStat.Variables[GENDER_INDEX].Value != 0) {
			this->Gender = this->get_unit_type()->DefaultStat.Variables[GENDER_INDEX].Value;
		}
	}

	//use the character's name for name generation (do this only after setting all properties so that the type, civilization and gender will have been parsed if given
	if (this->get_unit_type() != nullptr && this->get_unit_type()->BoolFlag[FAUNA_INDEX].value) {
		if (!this->get_name().empty()) {
			this->get_unit_type()->PersonalNames[this->Gender].push_back(this->get_name());
		}
	} else if (this->civilization != nullptr) {
		if (!this->get_name().empty()) {
			this->civilization->PersonalNames[this->Gender].push_back(this->get_name());
		}
		if (!this->get_surname().empty()) {
			this->civilization->FamilyNames.push_back(this->get_surname());
		}
	}

	if (this->Trait == nullptr) { //if no trait was set, have the character be the same trait as the unit type (if the unit type has a single one predefined)
		if (this->get_unit_type() != nullptr && this->get_unit_type()->Traits.size() == 1) {
			this->Trait = this->get_unit_type()->Traits[0];
		}
	}

	//check if the abilities are correct for this character's unit type
	if (this->get_unit_type() != nullptr && this->Abilities.size() > 0 && ((int) AiHelpers.LearnableAbilities.size()) > this->get_unit_type()->Slot) {
		int ability_count = (int) this->Abilities.size();
		for (int i = (ability_count - 1); i >= 0; --i) {
			if (!vector::contains(AiHelpers.LearnableAbilities[this->get_unit_type()->Slot], this->Abilities[i])) {
				vector::remove(this->Abilities, this->Abilities[i]);
			}
		}
	}

	if (this->home_settlement != nullptr) {
		this->home_settlement->add_character(this);
	}

	this->GenerateMissingDates();
	this->UpdateAttributes();

	for (const std::unique_ptr<historical_location> &location : this->HistoricalLocations) {
		location->initialize();
	}
}

void character::check() const
{
	if (this->Father != nullptr && this->Father->Gender != MaleGender) {
		throw std::runtime_error("Character \"" + this->Father->get_identifier() + "\" is set to be the biological father of \"" + this->get_identifier() + "\", but isn't male.");
	}

	if (this->Mother != nullptr && this->Mother->Gender != FemaleGender) {
		throw std::runtime_error("Character \"" + this->Mother->get_identifier() + "\" is set to be the biological mother of \"" + this->get_identifier() + "\", but isn't female.");
	}
}

void character::GenerateMissingDates()
{
	if (this->DeathDate.Year == 0 && this->BirthDate.Year != 0) { //if the character is missing a death date so far, give it +60 years after the birth date
		this->DeathDate.Year = this->BirthDate.Year + 60;
		this->DeathDate.Month = this->BirthDate.Month;
		this->DeathDate.Day = this->BirthDate.Day;
	}
	
	if (this->BirthDate.Year == 0 && this->DeathDate.Year != 0) { //if the character is missing a birth date so far, give it 60 years before the death date
		this->BirthDate.Year = this->DeathDate.Year - 60;
		this->BirthDate.Month = this->DeathDate.Month;
		this->BirthDate.Day = this->DeathDate.Day;
	}
	
	if (this->StartDate.Year == 0 && this->BirthDate.Year != 0) { //if the character is missing a start date so far, give it +30 years after the birth date
		this->StartDate.Year = this->BirthDate.Year + 30;
		this->StartDate.Month = this->BirthDate.Month;
		this->StartDate.Day = this->BirthDate.Day;
	}
	
	if (this->BirthDate.Year == 0 && this->StartDate.Year != 0) { //if the character is missing a birth date so far, give it 30 years before the start date
		this->BirthDate.Year = this->StartDate.Year - 30;
		this->BirthDate.Month = this->StartDate.Month;
		this->BirthDate.Day = this->StartDate.Day;
	}
	
	if (this->DeathDate.Year == 0 && this->StartDate.Year != 0) { //if the character is missing a death date so far, give it +30 years after the start date
		this->DeathDate.Year = this->StartDate.Year + 30;
		this->DeathDate.Month = this->StartDate.Month;
		this->DeathDate.Day = this->StartDate.Day;
	}
	
	if (this->StartDate.Year == 0 && this->DeathDate.Year != 0) { //if the character is missing a start date so far, give it 30 years before the death date
		this->StartDate.Year = this->DeathDate.Year - 30;
		this->StartDate.Month = this->DeathDate.Month;
		this->StartDate.Day = this->DeathDate.Day;
	}
}

int character::GetMartialAttribute() const
{
	if ((this->get_unit_type()->get_unit_class() != nullptr && this->get_unit_type()->get_unit_class()->get_identifier() == "thief") || this->get_unit_type()->DefaultStat.Variables[ATTACKRANGE_INDEX].Value > 1) {
		return DexterityAttribute;
	} else {
		return StrengthAttribute;
	}
}

int character::GetAttributeModifier(int attribute) const
{
	return this->Attributes[attribute] - 10;
}

/**
**	@brief	Get the character's religion
**
**	@return	The religion if found, or null otherwise
*/
CReligion *character::GetReligion() const
{
	//get the first religion of the character's first deity, since at present we don't set the religion directly for the character
	
	for (size_t i = 0; i < this->Deities.size(); ++i) {
		if (!this->Deities[i]->Religions.empty()) {
			return this->Deities[i]->Religions[0];
		}
	}
	
	return nullptr;
}

CLanguage *character::GetLanguage() const
{
	return PlayerRaces.get_civilization_language(this->civilization->ID);
}

calendar *character::get_calendar() const
{
	if (this->civilization != nullptr) {
		return this->civilization->get_calendar();
	}
	
	return nullptr;
}

bool character::IsParentOf(const std::string &child_ident) const
{
	for (size_t i = 0; i < this->Children.size(); ++i) {
		if (this->Children[i]->Ident == child_ident) {
			return true;
		}
	}
	return false;
}

bool character::IsChildOf(const std::string &parent_ident) const
{
	if ((this->Father != nullptr && this->Father->Ident == parent_ident) || (this->Mother != nullptr && this->Mother->Ident == parent_ident)) {
		return true;
	}
	return false;
}

bool character::IsSiblingOf(const std::string &sibling_ident) const
{
	for (size_t i = 0; i < this->Siblings.size(); ++i) {
		if (this->Siblings[i]->Ident == sibling_ident) {
			return true;
		}
	}
	return false;
}

bool character::IsItemEquipped(const CPersistentItem *item) const
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

bool character::IsUsable() const
{
	if (this->get_unit_type()->DefaultStat.Variables[GENDER_INDEX].Value != 0 && this->Gender != this->get_unit_type()->DefaultStat.Variables[GENDER_INDEX].Value) {
		return false; // hero not usable if their unit type has a set gender which is different from the hero's (this is because this means that the unit type lacks appropriate graphics for that gender)
	}
	
	return true;
}

bool character::CanAppear(bool ignore_neutral) const
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
bool character::CanWorship() const
{
	if (this->Deity) {
		return false; //the character cannot worship a deity if it is itself a deity
	}
	
	if (this->get_unit_type()->BoolFlag[FAUNA_INDEX].value) {
		return false; //the character cannot worship a deity if it is not sentient
	}
	
	return true;
}

/**
**	@brief	Get whether the character has a major deity in its worshipped deities list
**
**	@return True if the character has a major deity, false otherwise
*/
bool character::HasMajorDeity() const
{
	for (size_t i = 0; i < this->Deities.size(); ++i) {
		if (this->Deities[i]->Major) {
			return true;
		}
	}

	return false;
}

std::string character::GetFullName() const
{
	std::string full_name = this->get_name();
	if (!this->ExtraName.empty()) {
		full_name += " " + this->ExtraName;
	}
	if (!this->get_surname().empty()) {
		full_name += " " + this->get_surname();
	}
	return full_name;
}

IconConfig character::GetIcon() const
{
	if (this->Level >= 3 && this->HeroicIcon.Icon) {
		return this->HeroicIcon;
	} else if (this->Icon.Icon) {
		return this->Icon;
	} else if (!this->HairVariation.empty() && this->get_unit_type()->GetVariation(this->HairVariation) != nullptr && !this->get_unit_type()->GetVariation(this->HairVariation)->Icon.Name.empty()) {
		return this->get_unit_type()->GetVariation(this->HairVariation)->Icon;
	} else {
		return this->get_unit_type()->Icon;
	}
}

CPersistentItem *character::GetItem(CUnit &item) const
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

void character::UpdateAttributes()
{
	if (this->get_unit_type() == nullptr) {
		return;
	}
	
	for (int i = 0; i < MaxAttributes; ++i) {
		int var = GetAttributeVariableIndex(i);
		this->Attributes[i] = this->get_unit_type()->DefaultStat.Variables[var].Value;
		for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
			if (
				(this->Trait != nullptr && modifier->UpgradeId == this->Trait->ID)
				|| std::find(this->Abilities.begin(), this->Abilities.end(), CUpgrade::get_all()[modifier->UpgradeId]) != this->Abilities.end()
			) {
				if (modifier->Modifier.Variables[var].Value != 0) {
					this->Attributes[i] += modifier->Modifier.Variables[var].Value;
				}
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

stratagus::character *GetCustomHero(const std::string &hero_ident)
{
	if (CustomHeroes.find(hero_ident) != CustomHeroes.end()) {
		return CustomHeroes[hero_ident];
	}
	
	for (std::map<std::string, stratagus::character *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) { // for backwards compatibility
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
	for (stratagus::character *character : stratagus::character::get_all()) { //save characters
		SaveHero(character);
	}

	for (std::map<std::string, stratagus::character *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) { //save custom heroes
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

	if (std::filesystem::exists(path)) {
		std::filesystem::remove(path);
	}
}

void SaveHero(stratagus::character *hero)
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
	if (std::filesystem::exists(old_path)) {
		std::filesystem::remove(old_path);
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
		fprintf(fd, "\tName = \"%s\",\n", hero->get_name().c_str());
		if (!hero->ExtraName.empty()) {
			fprintf(fd, "\tExtraName = \"%s\",\n", hero->ExtraName.c_str());
		}
		if (!hero->get_surname().empty()) {
			fprintf(fd, "\tFamilyName = \"%s\",\n", hero->get_surname().c_str());
		}
		if (hero->Gender != NoGender) {
			fprintf(fd, "\tGender = \"%s\",\n", GetGenderNameById(hero->Gender).c_str());
		}
		if (hero->get_civilization()) {
			fprintf(fd, "\tCivilization = \"%s\",\n", hero->get_civilization()->get_identifier().c_str());
		}
	}
	if (hero->get_unit_type() != nullptr) {
		fprintf(fd, "\tType = \"%s\",\n", hero->get_unit_type()->Ident.c_str());
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
	
	fprintf(fd, "})\n\n");
		
	fclose(fd);
}

void SaveCustomHero(const std::string &hero_full_name)
{
	stratagus::character *hero = GetCustomHero(hero_full_name);
	if (!hero) {
		fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
	}
	
	SaveHero(hero);
}

void DeleteCustomHero(const std::string &hero_full_name)
{
	stratagus::character *hero = GetCustomHero(hero_full_name);
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
	if (std::filesystem::exists(path)) {
		std::filesystem::remove(path);
	}
	
	CustomHeroes.erase(hero_full_name);
	delete hero;
}

void SetCurrentCustomHero(const std::string &hero_full_name)
{
	if (!hero_full_name.empty()) {
		stratagus::character *hero = GetCustomHero(hero_full_name);
		if (!hero) {
			fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
		}
		
		CurrentCustomHero = hero;
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
		stratagus::character *hero = GetCustomHero(hero_full_name);
		if (!hero) {
			fprintf(stderr, "Custom hero \"%s\" does not exist.\n", hero_full_name.c_str());
		}

		stratagus::civilization *civilization = stratagus::civilization::get(civilization_name);
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
		if (std::filesystem::exists(path)) {
			std::filesystem::remove(path);
		}

		//now, update the hero
		hero->civilization = civilization;
		CUnitType *new_unit_type = hero->civilization->get_class_unit_type(hero->get_unit_type()->get_unit_class());
		if (new_unit_type != nullptr) {
			hero->unit_type = new_unit_type;
			hero->set_name(new_hero_name);
			hero->surname = new_hero_family_name;
			SaveHero(hero);

			CustomHeroes.erase(hero_full_name);
			CustomHeroes[hero->Ident] = hero;
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
