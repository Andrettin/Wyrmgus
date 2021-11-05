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
//      (c) Copyright 2015-2021 by Andrettin
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

#include "stratagus.h"

#include "character.h"

#include "ai/ai_local.h" //for using AiHelpers
#include "character_history.h"
#include "character_title.h"
#include "config.h"
#include "engine_interface.h"
#include "game/game.h"
#include "gender.h"
#include "iocompat.h"
#include "iolib.h"
#include "item/persistent_item.h"
#include "item/unique_item.h"
#include "map/historical_location.h"
#include "map/map_template.h"
#include "map/site.h"
#include "parameters.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "province.h"
#include "religion/deity.h"
#include "script/condition/and_condition.h"
#include "sound/unitsound.h"
#include "species/species.h"
#include "spell/spell.h"
#include "text_processor.h"
#include "time/calendar.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/string_util.h"
#include "util/util.h"
#include "util/vector_util.h"

bool LoadingPersistentHeroes = false;

namespace wyrmgus {

void character::clear()
{
	data_type::clear();
	
	character::custom_heroes.clear();
	character::custom_heroes_by_identifier.clear();
}

bool character::compare_encyclopedia_entries(const character *lhs, const character *rhs)
{
	const wyrmgus::civilization *lhs_civilization = lhs->get_civilization();
	const wyrmgus::civilization *rhs_civilization = rhs->get_civilization();

	if (lhs_civilization != rhs_civilization) {
		if (lhs_civilization == nullptr || rhs_civilization == nullptr) {
			return lhs_civilization == nullptr;
		}

		return lhs_civilization->get_name() < rhs_civilization->get_name();
	}

	if (lhs->get_default_faction() != rhs->get_default_faction()) {
		if (lhs->get_default_faction() == nullptr || rhs->get_default_faction() == nullptr) {
			return lhs->get_default_faction() == nullptr;
		}

		return lhs->get_default_faction()->get_name() < rhs->get_default_faction()->get_name();
	}

	return lhs->get_full_name() < rhs->get_full_name();
}

void character::create_custom_hero(const std::string &name, const std::string &surname, wyrmgus::civilization *civilization, wyrmgus::unit_type *unit_type, CUpgrade *trait, const std::string &variation_identifier)
{
	std::string identifier = "custom_" + string::lowered(name);
	if (!surname.empty()) {
		identifier += "_" + string::lowered(surname);
	}

	std::string identifier_suffix;
	int suffix_number = 1;

	while (character::get_custom_hero(identifier + identifier_suffix) != nullptr) {
		++suffix_number;
		identifier_suffix = "_" + std::to_string(suffix_number);
	}

	identifier += identifier_suffix;

	auto hero = make_qunique<character>(identifier);
	hero->moveToThread(QApplication::instance()->thread());

	hero->Custom = true;

	hero->set_name(name);
	hero->surname = surname;
	hero->civilization = civilization;

	hero->unit_type = unit_type;
	hero->level = hero->get_unit_type()->DefaultStat.Variables[LEVEL_INDEX].Value;

	hero->trait = trait;
	hero->variation = variation_identifier;

	if (hero->get_gender() == gender::none) {
		//if no gender was set, have the hero be the same gender as the unit type (if the unit type has it predefined)
		if (hero->get_unit_type() != nullptr && hero->get_unit_type()->get_gender() != gender::none) {
			hero->gender = hero->get_unit_type()->get_gender();
		}
	}

	hero->save();

	character::custom_heroes.push_back(hero.get());
	character::custom_heroes_by_identifier[identifier] = std::move(hero);

	emit engine_interface::get()->custom_heroes_changed();
}

void character::remove_custom_hero(character *custom_hero)
{
	//delete hero save file
	const std::filesystem::path filepath = custom_hero->get_save_filepath();
	if (std::filesystem::exists(filepath)) {
		std::filesystem::remove(filepath);
	}

	vector::remove(character::custom_heroes, custom_hero);
	character::custom_heroes_by_identifier.erase(custom_hero->get_identifier());

	emit engine_interface::get()->custom_heroes_changed();
}

bool character::is_name_valid_for_custom_hero(const std::string &name)
{
	for (const char c : name) {
		switch (c) {
			case '\n':
			case '\\':
			case '\"':
			case '\t':
			case '\0':
				return false;
			default:
				break;
		}
	}

	if (name.find_first_not_of(' ') == std::string::npos) {
		return false; //name contains only spaces
	}

	return true;
}

character::character(const std::string &identifier)
	: detailed_data_entry(identifier), CDataType(identifier), gender(gender::none)
{
	memset(Attributes, 0, sizeof(Attributes));
}

character::~character()
{
}

void character::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition>();
		database::process_sml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "deities") {
		for (const std::string &value : values) {
			wyrmgus::deity *deity = deity::get(value);
			this->Deities.push_back(deity);
		}
	} else if (tag == "forbidden_upgrades") {
		for (const std::string &value : values) {
			wyrmgus::unit_type *unit_type = unit_type::get(value);
			this->ForbiddenUpgrades.push_back(unit_type);
		}
	} else if (tag == "base_abilities") {
		for (const std::string &value : values) {
			CUpgrade *ability = CUpgrade::get(value);
			this->base_abilities.push_back(ability);
		}
	} else if (tag == "abilities") {
		for (const std::string &value : values) {
			CUpgrade *ability = CUpgrade::get(value);
			this->abilities.push_back(ability);
		}
	} else if (tag == "default_items") {
		scope.for_each_element([&](const sml_property &property) {
			const wyrmgus::unit_type *unit_type = unit_type::get(property.get_key());
			const int quantity = std::stoi(property.get_value());

			for (int i = 0; i < quantity; ++i) {
				auto item = std::make_unique<persistent_item>(unit_type, this);
				this->default_items.push_back(std::move(item));
			}
		}, [&](const sml_data &child_scope) {
			const wyrmgus::unit_type *unit_type = unit_type::get(child_scope.get_tag());

			auto item = std::make_unique<persistent_item>(unit_type, this);
			database::process_sml_data(item, child_scope);
			this->default_items.push_back(std::move(item));
		});
	} else if (tag == "items") {
		scope.for_each_child([&](const sml_data &child_scope) {
			wyrmgus::unit_type *unit_type = unit_type::try_get(child_scope.get_tag());

			if (unit_type == nullptr) {
				fprintf(stderr, "Unit type \"%s\" doesn't exist.\n", child_scope.get_tag().c_str());
			}

			auto item = std::make_unique<persistent_item>(unit_type, this);
			database::process_sml_data(item, child_scope);
			this->add_item(std::move(item));
		});
	} else if (tag == "sounds") {
		if (this->sound_set == nullptr) {
			this->sound_set = std::make_unique<unit_sound_set>();
		}

		database::process_sml_data(this->sound_set, scope);
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
			wyrmgus::unit_type *unit_type = unit_type::get(value);
			if (this->get_unit_type() == nullptr || this->get_unit_type() == unit_type || this->get_unit_type()->CanExperienceUpgradeTo(unit_type)) {
				this->unit_type = unit_type;
			}
		} else if (key == "gender") {
			this->gender = wyrmgus::string_to_gender(value);
		} else if (key == "civilization") {
			this->civilization = civilization::get(value);
		} else if (key == "faction") {
			if (this->default_faction != nullptr) {
				throw std::runtime_error("Character \"" + this->get_identifier() + "\" already has a faction.");
			}

			this->default_faction = faction::get(value);
		} else if (key == "hair_variation") {
			value = FindAndReplaceString(value, "_", "-");
			this->variation = value;
		} else if (key == "trait") {
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->trait = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "level") {
			this->level = std::stoi(value);
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
			this->father = father;
		} else if (key == "mother") {
			character *mother = character::get(value);
			this->mother = mother;
		} else if (key == "deity") {
			wyrmgus::deity *deity = deity::get(value);
			this->Deities.push_back(deity);
		} else if (key == "description") {
			this->set_description(value);
		} else if (key == "background") {
			this->set_background(value);
		} else if (key == "quote") {
			this->set_quote(value);
		} else if (key == "icon") {
			this->icon = icon::get(value);
		} else if (key == "heroic_icon") {
			this->heroic_icon = icon::get(value);
		} else if (key == "forbidden_upgrade") {
			wyrmgus::unit_type *unit_type = unit_type::get(value);
			this->ForbiddenUpgrades.push_back(unit_type);
		} else if (key == "ability") {
			CUpgrade *ability_upgrade = CUpgrade::try_get(value);
			if (ability_upgrade) {
				this->abilities.push_back(ability_upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "read_work") {
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->ReadWorks.push_back(upgrade);
			} else {
				fprintf(stderr, "Upgrade \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "consumed_elixir") {
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
			character_title title = character_title::none;
			CDate start_date;
			CDate end_date;
			wyrmgus::faction *title_faction = nullptr;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "title") {
					title = string_to_character_title(value);
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
			
			if (title == character_title::none) {
				fprintf(stderr, "Historical title has no title.\n");
				continue;
			}
			
			if (!title_faction) {
				fprintf(stderr, "Historical title has no faction.\n");
				continue;
			}
			
			//don't put in the faction's historical data if a blank year was given
			if (start_date.Year != 0 && end_date.Year != 0) {
				title_faction->HistoricalMinisters[std::make_tuple(start_date, end_date, title)] = this;
			}
				
			this->HistoricalTitles.push_back(std::make_tuple(start_date, end_date, title_faction, title));
		} else if (child_config_data->Tag == "item") {
			auto item = std::make_unique<persistent_item>(this);
			item->ProcessConfigData(child_config_data);
			this->add_item(std::move(item));
		} else {
			fprintf(stderr, "Invalid character property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void character::initialize()
{
	if (this->get_unit_type() != nullptr) {
		if (this->level < this->get_unit_type()->DefaultStat.Variables[LEVEL_INDEX].Value) {
			this->level = this->get_unit_type()->DefaultStat.Variables[LEVEL_INDEX].Value;
		}
	}

	if (this->level < this->get_base_level()) {
		this->level = this->get_base_level();
	}

	if (this->get_gender() == gender::none && this->get_unit_type() != nullptr) { //if no gender was set so far, have the character be the same gender as the unit type (if the unit type has it predefined)
		if (this->get_unit_type()->get_gender() != gender::none) {
			this->gender = this->get_unit_type()->get_gender();
		}
	}

	if (this->get_father() != nullptr) {
		this->get_father()->add_child(this);
	}

	if (this->get_mother() != nullptr) {
		this->get_mother()->add_child(this);
	}

	for (character *child : this->get_children()) {
		if (this->get_gender() == gender::male) {
			child->father = this;
		} else {
			child->mother = this;
		}
	}

	if (this->get_deity() != nullptr) {
		if (this->get_name().empty()) {
			this->set_name(this->get_deity()->get_name());
		}

		if (this->get_description().empty()) {
			this->set_description(this->get_deity()->get_description());
		}

		if (this->get_background().empty()) {
			this->set_background(this->get_deity()->get_background());
		}

		if (this->get_quote().empty()) {
			this->set_quote(this->get_deity()->get_quote());
		}
	}

	//use the character's name for name generation (do this only after setting all properties so that the type, civilization and gender will have been parsed if given
	if (this->get_unit_type() != nullptr && this->get_unit_type()->BoolFlag[FAUNA_INDEX].value && this->get_unit_type()->get_species() != nullptr) {
		wyrmgus::species *species = this->get_unit_type()->get_species();
		if (!species->is_initialized()) {
			species->initialize();
		}

		if (!this->get_name().empty()) {
			this->unit_type->get_species()->add_specimen_name(this->get_gender(), this->get_name());
		}
	} else if (this->civilization != nullptr) {
		if (!this->civilization->is_initialized()) {
			this->civilization->initialize();
		}

		if (!this->get_name().empty()) {
			this->civilization->add_personal_name(this->get_gender(), this->get_name());
		}
		if (!this->get_surname().empty()) {
			this->civilization->add_surname(this->get_surname());
		}
	}

	if (this->get_trait() == nullptr) { //if no trait was set, have the character be the same trait as the unit type (if the unit type has a single one predefined)
		if (this->get_unit_type() != nullptr && this->get_unit_type()->get_traits().size() == 1) {
			this->trait = this->get_unit_type()->get_traits().at(0);
		}
	}

	std::map<const CUpgrade *, int> ability_counts;
	for (const CUpgrade *ability : this->get_abilities()) {
		ability_counts[ability]++;
	}

	//add base abilities if not already present in the saved abilities
	for (const CUpgrade *ability : this->get_base_abilities()) {
		int &count = ability_counts[ability];
		if (count > 0) {
			count--;
		} else {
			this->abilities.push_back(ability);
		}
	}

	//check if the abilities are correct for this character's unit type
	if (this->get_unit_type() != nullptr && this->get_abilities().size() > 0 && static_cast<int>(AiHelpers.LearnableAbilities.size()) > this->get_unit_type()->Slot) {
		int ability_count = static_cast<int>(this->get_abilities().size());
		for (int i = (ability_count - 1); i >= 0; --i) {
			if (!vector::contains(AiHelpers.LearnableAbilities[this->get_unit_type()->Slot], this->abilities[i])) {
				vector::remove(this->abilities, this->abilities[i]);
			}
		}
	}

	if (this->home_settlement != nullptr) {
		this->home_settlement->add_character(this);
	}

	if (this->civilization != nullptr) {
		this->civilization->add_character(this);
	}

	if (this->default_faction != nullptr) {
		this->default_faction->add_character(this);
	}

	for (const std::unique_ptr<persistent_item> &default_item : this->default_items) {
		default_item->initialize();
	}

	for (const std::unique_ptr<persistent_item> &item : this->items) {
		item->initialize();
	}

	if (this->items.empty() && !this->default_items.empty()) {
		for (const std::unique_ptr<persistent_item> &default_item : this->default_items) {
			this->add_item(default_item->duplicate());
		}
	}

	for (const auto &item : this->get_items()) {
		if (!item->is_equipped()) {
			continue;
		}

		if (item->get_item_slot() != item_slot::none) {
			this->EquippedItems[static_cast<int>(item->get_item_slot())].push_back(item.get());
		} else {
			fprintf(stderr, "Item \"%s\" cannot be equipped, as it belongs to no item slot.\n", item->get_unit_type()->get_identifier().c_str());
		}
	}

	this->GenerateMissingDates();
	this->UpdateAttributes();

	if (this->sound_set != nullptr) {
		this->sound_set->map_sounds();
	}

	for (const std::unique_ptr<historical_location> &location : this->HistoricalLocations) {
		location->initialize();
	}

	data_entry::initialize();
}

void character::check() const
{
	if (this->get_father() != nullptr && this->get_father()->get_gender() != gender::male) {
		throw std::runtime_error("Character \"" + this->get_father()->get_identifier() + "\" is set to be the biological father of \"" + this->get_identifier() + "\", but isn't male.");
	}

	if (this->get_mother() != nullptr && this->get_mother()->get_gender() != gender::female) {
		throw std::runtime_error("Character \"" + this->get_mother()->get_identifier() + "\" is set to be the biological mother of \"" + this->get_identifier() + "\", but isn't female.");
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

data_entry_history *character::get_history_base()
{
	return this->history.get();
}

void character::reset_history()
{
	//use the home settlement as the default location
	const site *default_location_site = this->home_settlement;

	this->history = std::make_unique<character_history>(this->default_faction, default_location_site);
}

std::string character::get_encyclopedia_text() const
{
	std::string text;

	if (this->get_civilization() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Civilization: " + this->get_civilization()->get_link_string());
	}

	if (this->get_default_faction() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Faction: " + this->get_default_faction()->get_link_string());
	}

	if (this->get_unit_type() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Type: " + this->get_unit_type()->get_link_string());
	}

	named_data_entry::concatenate_encyclopedia_text(text, this->get_encyclopedia_genealogical_text());

	if (!this->Deities.empty()) {
		std::string deities_text;
		for (const wyrmgus::deity *deity : this->Deities) {
			if (!deities_text.empty()) {
				deities_text += ", ";
			}

			deities_text += deity->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, "Deities: " + deities_text);
	}

	named_data_entry::concatenate_encyclopedia_text(text, "Level: " + std::to_string(this->get_level()));

	if (!this->get_abilities().empty()) {
		std::map<const CUpgrade *, int> ability_counts;

		for (const CUpgrade *ability : this->get_abilities()) {
			ability_counts[ability]++;
		}

		std::set<const CUpgrade *> written_abilities;

		std::string abilities_text;
		for (const CUpgrade *ability : this->get_abilities()) {
			if (written_abilities.contains(ability)) {
				continue;
			}

			if (!abilities_text.empty()) {
				abilities_text += ", ";
			}

			abilities_text += ability->get_name();
			if (ability_counts[ability] > 1) {
				abilities_text += " (x" + std::to_string(ability_counts[ability]) + ")";
			}

			written_abilities.insert(ability);
		}

		named_data_entry::concatenate_encyclopedia_text(text, "Acquired Abilities: " + abilities_text);
	}

	named_data_entry::concatenate_encyclopedia_text(text, detailed_data_entry::get_encyclopedia_text());

	return text;
}

std::string character::get_encyclopedia_genealogical_text() const
{
	std::string text;

	const wyrmgus::character *father = this->get_father();
	if (father != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Father: " + (father->is_deity() ? father->get_deity()->get_link_string() : father->get_link_string()));
	}

	const wyrmgus::character *mother = this->get_mother();
	if (this->get_mother() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Mother: " + (mother->is_deity() ? mother->get_deity()->get_link_string() : mother->get_link_string()));
	}

	if (!this->get_children().empty()) {
		std::string children_text;
		for (const wyrmgus::character *child : this->get_children()) {
			if (!children_text.empty()) {
				children_text += ", ";
			}

			children_text += child->is_deity() ? child->get_deity()->get_link_string() : child->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, "Children: " + children_text);
	}

	return text;
}

void character::save() const
{
	const std::filesystem::path filepath = this->get_save_filepath();

	const std::filesystem::path folder_path = filepath.parent_path();
	database::ensure_path_exists(folder_path);

	std::string old_identifier = this->get_identifier();
	string::replace(old_identifier, '_', '-');

	std::filesystem::path old_filepath = filepath;
	old_filepath.replace_filename(old_identifier + ".lua");

	if (std::filesystem::exists(old_filepath)) {
		std::filesystem::remove(old_filepath);
	}

	FILE *fd = fopen(path::to_string(filepath).c_str(), "w");
	if (!fd) {
		log::log_error("Cannot open file \"" + path::to_string(filepath) + "\" for writing.");
		return;
	}

	if (!this->Custom) {
		fprintf(fd, "DefineCharacter(\"%s\", {\n", this->get_identifier().c_str());
	} else {
		fprintf(fd, "DefineCustomHero(\"%s\", {\n", this->get_identifier().c_str());
		fprintf(fd, "\tName = \"%s\",\n", this->get_name().c_str());
		if (!this->ExtraName.empty()) {
			fprintf(fd, "\tExtraName = \"%s\",\n", this->ExtraName.c_str());
		}
		if (!this->get_surname().empty()) {
			fprintf(fd, "\tFamilyName = \"%s\",\n", this->get_surname().c_str());
		}
		if (this->get_gender() != gender::none) {
			fprintf(fd, "\tGender = \"%s\",\n", gender_to_string(this->get_gender()).c_str());
		}
		if (this->get_civilization()) {
			fprintf(fd, "\tCivilization = \"%s\",\n", this->get_civilization()->get_identifier().c_str());
		}
		if (!this->get_description().empty()) {
			fprintf(fd, "\tDescription = \"%s\",\n", string::escaped(this->get_description()).c_str());
		}
	}
	if (this->get_unit_type() != nullptr) {
		fprintf(fd, "\tType = \"%s\",\n", this->get_unit_type()->get_identifier().c_str());
	}
	if (this->Custom) {
		if (this->get_trait() != nullptr) {
			fprintf(fd, "\tTrait = \"%s\",\n", this->get_trait()->get_identifier().c_str());
		}
		if (!this->get_variation().empty()) {
			fprintf(fd, "\tVariation = \"%s\",\n", this->get_variation().c_str());
		}
	}
	if (this->get_level() != 0) {
		fprintf(fd, "\tLevel = %d,\n", this->get_level());
	}
	if (this->ExperiencePercent != 0) {
		fprintf(fd, "\tExperiencePercent = %d,\n", this->ExperiencePercent);
	}
	if (this->get_abilities().size() > 0) {
		fprintf(fd, "\tAbilities = {");
		for (size_t j = 0; j < this->get_abilities().size(); ++j) {
			fprintf(fd, "\"%s\"", this->get_abilities()[j]->get_identifier().c_str());
			if (j < (this->get_abilities().size() - 1)) {
				fprintf(fd, ", ");
			}
		}
		fprintf(fd, "},\n");
	}
	if (this->Custom && this->Deities.size() > 0) {
		fprintf(fd, "\tDeities = {");
		for (size_t j = 0; j < this->Deities.size(); ++j) {
			fprintf(fd, "\"%s\"", this->Deities[j]->get_identifier().c_str());
			if (j < (this->Deities.size() - 1)) {
				fprintf(fd, ", ");
			}
		}
		fprintf(fd, "},\n");
	}
	if (this->ReadWorks.size() > 0) {
		fprintf(fd, "\tReadWorks = {");
		for (size_t j = 0; j < this->ReadWorks.size(); ++j) {
			fprintf(fd, "\"%s\"", this->ReadWorks[j]->get_identifier().c_str());
			if (j < (this->ReadWorks.size() - 1)) {
				fprintf(fd, ", ");
			}
		}
		fprintf(fd, "},\n");
	}
	if (this->ConsumedElixirs.size() > 0) {
		fprintf(fd, "\tConsumedElixirs = {");
		for (size_t j = 0; j < this->ConsumedElixirs.size(); ++j) {
			fprintf(fd, "\"%s\"", this->ConsumedElixirs[j]->get_identifier().c_str());
			if (j < (this->ConsumedElixirs.size() - 1)) {
				fprintf(fd, ", ");
			}
		}
		fprintf(fd, "},\n");
	}
	if (!this->get_items().empty()) {
		fprintf(fd, "\tItems = {");
		for (size_t j = 0; j < this->get_items().size(); ++j) {
			const auto &item = this->get_items()[j];
			fprintf(fd, "\n\t\t{");
			fprintf(fd, "\n\t\t\t\"type\", \"%s\",", item->get_unit_type()->Ident.c_str());
			if (item->Prefix != nullptr) {
				fprintf(fd, "\n\t\t\t\"prefix\", \"%s\",", item->Prefix->get_identifier().c_str());
			}
			if (item->Suffix != nullptr) {
				fprintf(fd, "\n\t\t\t\"suffix\", \"%s\",", item->Suffix->get_identifier().c_str());
			}
			if (item->Spell != nullptr) {
				fprintf(fd, "\n\t\t\t\"spell\", \"%s\",", item->Spell->get_identifier().c_str());
			}
			if (item->Work != nullptr) {
				fprintf(fd, "\n\t\t\t\"work\", \"%s\",", item->Work->get_identifier().c_str());
			}
			if (item->Elixir != nullptr) {
				fprintf(fd, "\n\t\t\t\"elixir\", \"%s\",", item->Elixir->get_identifier().c_str());
			}
			if (!item->get_name().empty()) {
				fprintf(fd, "\n\t\t\t\"name\", \"%s\",", item->get_name().c_str());
			}
			if (item->get_unique() != nullptr) { // affixes, name and etc. will be inherited from the unique item, but we set those previous characteristics for unique items anyway, so that if a unique item no longer exists in the game's code (i.e. if it is from a mod that has been deactivated) the character retains an item with the same affixes, name and etc., even though it will no longer be unique
				fprintf(fd, "\n\t\t\t\"unique\", \"%s\",", item->get_unique()->get_identifier().c_str());
			}
			if (item->is_bound()) {
				fprintf(fd, "\n\t\t\t\"bound\", true,");
			}
			if (!item->is_identified()) {
				fprintf(fd, "\n\t\t\t\"identified\", false,");
			}
			if (this->is_item_equipped(item.get())) {
				fprintf(fd, "\n\t\t\t\"equipped\", true");
			}
			fprintf(fd, "\n\t\t}");
			if (j < (this->get_items().size() - 1)) {
				fprintf(fd, ",");
			}
		}
		fprintf(fd, "\n\t},\n");
	}

	fprintf(fd, "})\n\n");

	fclose(fd);
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

religion *character::get_religion() const
{
	//get the first religion of the character's first deity, since at present we don't set the religion directly for the character
	
	for (const wyrmgus::deity *deity : this->Deities) {
		if (!deity->get_religions().empty()) {
			return deity->get_religions().front();
		}
	}
	
	return nullptr;
}

const language *character::get_language() const
{
	if (this->civilization != nullptr) {
		return this->civilization->get_language();
	}

	return nullptr;
}

calendar *character::get_calendar() const
{
	if (this->civilization != nullptr) {
		return this->civilization->get_calendar();
	}
	
	return nullptr;
}

void character::add_item(std::unique_ptr<persistent_item> &&item)
{
	this->items.push_back(std::move(item));
}

void character::remove_item(persistent_item *item)
{
	vector::remove(this->items, item);
}

persistent_item *character::get_item(const CUnit *item_unit)
{
	return const_cast<persistent_item *>(const_cast<const character *>(this)->get_item(item_unit));
}

const persistent_item *character::get_item(const CUnit *item_unit) const
{
	for (const auto &item : this->items) {
		if (item->get_unit_type() == item_unit->Type && item->Prefix == item_unit->Prefix && item->Suffix == item_unit->Suffix && item->Spell == item_unit->Spell && item->Work == item_unit->Work && item->Elixir == item_unit->Elixir && item->get_unique() == item_unit->get_unique() && item->is_bound() == item_unit->Bound && item->is_identified() == item_unit->Identified && this->is_item_equipped(item.get()) == item_unit->Container->IsItemEquipped(item_unit)) {
			if (item->get_name().empty() || item->get_name() == item_unit->Name) {
				return item.get();
			}
		}
	}

	return nullptr;
}

bool character::is_item_equipped(const persistent_item *item) const
{
	const item_slot item_slot = item->get_item_slot();
	
	if (item_slot == item_slot::none) {
		return false;
	}
	
	if (vector::contains(this->EquippedItems[static_cast<int>(item_slot)], item)) {
		return true;
	}
	
	return false;
}

bool character::IsUsable() const
{
	if (this->get_unit_type() == nullptr) {
		return false;
	}

	if (this->get_unit_type()->get_gender() != gender::none && this->get_gender() != this->get_unit_type()->get_gender()) {
		//hero not usable if their unit type has a set gender which is different from the hero's (this is because this means that the unit type lacks appropriate graphics for that gender)
		return false;
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
	if (this->is_deity()) {
		return false; //the character cannot worship a deity if it is itself a deity
	}
	
	if (this->get_unit_type()->BoolFlag[FAUNA_INDEX].value) {
		return false; //the character cannot worship a deity if it is not sentient
	}
	
	return true;
}

bool character::HasMajorDeity() const
{
	for (const wyrmgus::deity *deity : this->Deities) {
		if (deity->is_major()) {
			return true;
		}
	}

	return false;
}

std::string character::get_full_name() const
{
	std::string full_name = this->get_name();

	if (!this->ExtraName.empty()) {
		full_name += " " + this->ExtraName;
	} else if (!this->get_surname().empty()) {
		full_name += " " + this->get_surname();
	}

	return full_name;
}

void character::set_variation(const std::string &variation)
{
	this->variation = FindAndReplaceString(variation, "_", "-");
}

icon *character::get_icon() const
{
	if (this->get_level() >= 3 && this->heroic_icon != nullptr) {
		return this->heroic_icon;
	} else if (this->icon != nullptr) {
		return this->icon;
	}
	
	if (this->get_unit_type() != nullptr) {
		if (!this->get_variation().empty() && this->get_unit_type()->GetVariation(this->get_variation()) != nullptr && !this->get_unit_type()->GetVariation(this->get_variation())->Icon.Name.empty()) {
			return this->get_unit_type()->GetVariation(this->get_variation())->Icon.Icon;
		} else {
			return this->get_unit_type()->get_icon();
		}
	}

	return nullptr;
}

void character::add_child(character *child)
{
	if (vector::contains(this->get_children(), child)) {
		return;
	}

	this->children.push_back(child);
}

void character::UpdateAttributes()
{
	if (this->get_unit_type() == nullptr) {
		return;
	}
	
	for (int i = 0; i < MaxAttributes; ++i) {
		int var = GetAttributeVariableIndex(i);
		this->Attributes[i] = this->get_unit_type()->DefaultStat.Variables[var].Value;
		for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
			if (
				(this->get_trait() != nullptr && modifier->UpgradeId == this->get_trait()->ID)
				|| vector::contains(this->abilities, CUpgrade::get_all()[modifier->UpgradeId])
			) {
				if (modifier->Modifier.Variables[var].Value != 0) {
					this->Attributes[i] += modifier->Modifier.Variables[var].Value;
				}
			}
		}
	}
}

void character::remove_ability(const CUpgrade *ability)
{
	vector::remove(this->abilities, ability);
}

text_processing_context character::get_text_processing_context() const
{
	text_processing_context ctx;
	ctx.faction = this->get_default_faction();
	return ctx;
}

CUnit *character::get_unit() const
{
	for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
		for (CUnit *character_unit : player->Heroes) {
			if (character_unit->get_character() == this) {
				return character_unit;
			}
		}
	}

	return nullptr;
}

std::filesystem::path character::get_save_filepath() const
{
	std::filesystem::path filepath = parameters::get()->GetUserDirectory();

	if (!GameName.empty()) {
		filepath /= GameName;
	}

	filepath /= "heroes";

	if (this->Custom) {
		filepath /= "custom";
	}

	filepath /= this->get_identifier() + ".lua";

	filepath.make_preferred();

	return filepath;
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

void SaveHeroes()
{
	for (const character *character : character::get_all()) { //save characters
		character->save();
	}

	for (const character *hero : character::get_custom_heroes()) { //save custom heroes
		hero->save();
	}
			
	//see if the old heroes.lua save file is present, and if so, delete it
	std::string path = parameters::get()->GetUserDirectory();

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
