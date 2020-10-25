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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "faction.h"

#include "character.h"
#include "civilization.h"
#include "diplomacy_state.h"
#include "faction_tier.h"
#include "government_type.h"
#include "luacallback.h"
#include "player_color.h"
#include "script/condition/and_condition.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void faction::process_title_names(title_name_map &title_names, const sml_data &scope)
{
	scope.for_each_child([&](const sml_data &child_scope) {
		faction::process_title_name_scope(title_names, child_scope);
	});

	scope.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		wyrmgus::government_type government_type = string_to_government_type(key);
		title_names[government_type][faction_tier::none] = value;
	});
}

void faction::process_title_name_scope(title_name_map &title_names, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const wyrmgus::government_type government_type = string_to_government_type(tag);

	scope.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const faction_tier tier = string_to_faction_tier(key);
		title_names[government_type][tier] = value;
	});
}

void faction::process_character_title_name_scope(character_title_name_map &character_title_names, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const character_title title_type = GetCharacterTitleIdByName(tag);

	scope.for_each_child([&](const sml_data &child_scope) {
		faction::process_character_title_name_scope(character_title_names[title_type], child_scope);
	});

	scope.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const wyrmgus::government_type government_type = string_to_government_type(key);
		character_title_names[title_type][government_type][faction_tier::none][gender::none] = value;
	});
}

void faction::process_character_title_name_scope(std::map<wyrmgus::government_type, std::map<faction_tier, std::map<gender, std::string>>> &character_title_names, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const wyrmgus::government_type government_type = string_to_government_type(tag);

	scope.for_each_child([&](const sml_data &child_scope) {
		faction::process_character_title_name_scope(character_title_names[government_type], child_scope);
	});
}

void faction::process_character_title_name_scope(std::map<faction_tier, std::map<gender, std::string>> &character_title_names, const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const faction_tier faction_tier = string_to_faction_tier(tag);

	scope.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();
		const gender gender = string_to_gender(key);
		character_title_names[faction_tier][gender] = value;
	});
}

faction::faction(const std::string &identifier)
	: detailed_data_entry(identifier), default_tier(faction_tier::barony), min_tier(faction_tier::none), max_tier(faction_tier::none), tier(faction_tier::barony), default_government_type(government_type::monarchy), government_type(government_type::monarchy)
{
}

faction::~faction()
{
}

void faction::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "adjective") {
		this->Adjective = value;
	} else if (key == "type") {
		const int faction_type = GetFactionTypeIdByName(value);
		if (faction_type != -1) {
			this->Type = faction_type;
		} else {
			throw std::runtime_error("Faction type \"" + value + "\" doesn't exist.");
		}
	} else if (key == "faction_upgrade") {
		this->FactionUpgrade = value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void faction::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "develops_from") {
		for (const std::string &value : values) {
			faction *other_faction = faction::get(value);
			this->DevelopsFrom.push_back(other_faction);
			other_faction->DevelopsTo.push_back(this);
		}
	} else if (tag == "title_names") {
		faction::process_title_names(this->title_names, scope);
	} else if (tag == "character_title_names") {
		scope.for_each_child([&](const sml_data &child_scope) {
			faction::process_character_title_name_scope(this->character_title_names, child_scope);
		});
	} else if (tag == "preconditions") {
		auto preconditions = std::make_unique<and_condition>();
		database::process_sml_data(preconditions, scope);
		this->preconditions = std::move(preconditions);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition>();
		database::process_sml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void faction::process_sml_dated_scope(const sml_data &scope, const QDateTime &date)
{
	const std::string &tag = scope.get_tag();

	if (tag == "resources") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const sml_operator sml_operator = property.get_operator();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			const int quantity = std::stoi(value);

			if (sml_operator == sml_operator::assignment) {
				this->resources[resource] = quantity;
			} else if (sml_operator == sml_operator::addition) {
				this->resources[resource] += quantity;
			} else if (sml_operator == sml_operator::subtraction) {
				this->resources[resource] -= quantity;
			} else {
				throw std::runtime_error("Invalid faction resource operator: \"" + std::to_string(static_cast<int>(sml_operator)) + "\".");
			}
		});
	} else if (tag == "diplomacy_state") {
		const faction *other_faction = nullptr;
		std::optional<diplomacy_state> state;
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "faction") {
				other_faction = faction::get(value);
			} else if (key == "state") {
				state = string_to_diplomacy_state(value);
			} else {
				throw std::runtime_error("Invalid diplomacy state property: \"" + key + "\".");
			}
		});

		if (other_faction == nullptr) {
			throw std::runtime_error("Diplomacy state has no faction.");
		}

		if (!state.has_value()) {
			throw std::runtime_error("Diplomacy state has no state.");
		}

		const bool is_vassalage = is_vassalage_diplomacy_state(state.value());

		if (is_vassalage) {
			//a faction can only have one overlord, so remove any other vassalage states
			map::remove_value_if(this->diplomacy_states, [](const diplomacy_state state) {
				return is_vassalage_diplomacy_state(state);
			});
		}

		this->diplomacy_states[other_faction] = state.value();
	} else {
		data_entry::process_sml_dated_scope(scope, date);
	}
}

void faction::initialize()
{
	if (this->Type == FactionTypeTribe) {
		this->DefiniteArticle = true;
	}

	std::sort(this->AiBuildingTemplates.begin(), this->AiBuildingTemplates.end(), [](const std::unique_ptr<CAiBuildingTemplate> &a, const std::unique_ptr<CAiBuildingTemplate> &b) {
		return a->get_priority() > b->get_priority();
	});

	for (auto &kv_pair : this->ForceTemplates) {
		std::sort(kv_pair.second.begin(), kv_pair.second.end(), [](const std::unique_ptr<CForceTemplate> &a, const std::unique_ptr<CForceTemplate> &b) {
			return a->Priority > b->Priority;
		});
	}

	if (this->ParentFaction != -1) {
		const faction *parent_faction = faction::get_all()[this->ParentFaction];

		if (this->FactionUpgrade.empty()) { //if the faction has no faction upgrade, inherit that of its parent faction
			this->FactionUpgrade = parent_faction->FactionUpgrade;
		}

		//inherit button icons from parent civilization, for button actions which none are specified
		for (std::map<ButtonCmd, IconConfig>::const_iterator iterator = parent_faction->ButtonIcons.begin(); iterator != parent_faction->ButtonIcons.end(); ++iterator) {
			if (this->ButtonIcons.find(iterator->first) == this->ButtonIcons.end()) {
				this->ButtonIcons[iterator->first] = iterator->second;
			}
		}

		for (std::map<std::string, std::map<CDate, bool>>::const_iterator iterator = parent_faction->HistoricalUpgrades.begin(); iterator != parent_faction->HistoricalUpgrades.end(); ++iterator) {
			if (this->HistoricalUpgrades.find(iterator->first) == this->HistoricalUpgrades.end()) {
				this->HistoricalUpgrades[iterator->first] = iterator->second;
			}
		}
	}

	if (this->get_min_tier() == faction_tier::none) {
		this->min_tier = this->get_default_tier();
	}

	if (this->get_max_tier() == faction_tier::none) {
		this->max_tier = this->get_default_tier();
	}

	data_entry::initialize();
}

void faction::check() const
{
	if (this->civilization == nullptr) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has no civilization.");
	}
}

std::string_view faction::get_title_name(const wyrmgus::government_type government_type, const faction_tier tier) const
{
	if (this->Type != FactionTypePolity) {
		return string::empty_str;
	}

	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator != this->title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	return this->get_civilization()->get_title_name(government_type, tier);
}

std::string_view faction::get_character_title_name(const character_title title_type, const wyrmgus::government_type government_type, const faction_tier tier, const gender gender) const
{
	auto find_iterator = this->character_title_names.find(title_type);
	if (find_iterator != this->character_title_names.end()) {
		auto secondary_find_iterator = find_iterator->second.find(government_type);
		if (secondary_find_iterator == find_iterator->second.end()) {
			secondary_find_iterator = find_iterator->second.find(government_type::none);
		}

		if (secondary_find_iterator != find_iterator->second.end()) {
			auto tertiary_find_iterator = secondary_find_iterator->second.find(tier);
			if (tertiary_find_iterator == secondary_find_iterator->second.end()) {
				tertiary_find_iterator = secondary_find_iterator->second.find(faction_tier::none);
			}

			if (tertiary_find_iterator != secondary_find_iterator->second.end()) {
				auto quaternary_find_iterator = tertiary_find_iterator->second.find(gender);
				if (quaternary_find_iterator == tertiary_find_iterator->second.end()) {
					quaternary_find_iterator = tertiary_find_iterator->second.find(gender::none);
				}

				if (quaternary_find_iterator != tertiary_find_iterator->second.end()) {
					return quaternary_find_iterator->second;
				}
			}
		}
	}

	return this->get_civilization()->get_character_title_name(title_type, this->Type, government_type, tier, gender);
}

int faction::GetUpgradePriority(const CUpgrade *upgrade) const
{
	if (!upgrade) {
		fprintf(stderr, "Error in faction::GetUpgradePriority: the upgrade is null.\n");
	}
	
	if (this->UpgradePriorities.find(upgrade) != this->UpgradePriorities.end()) {
		return this->UpgradePriorities.find(upgrade)->second;
	}
	
	return this->civilization->GetUpgradePriority(upgrade);
}

int faction::GetForceTypeWeight(const ForceType force_type) const
{
	if (force_type == ForceType::None) {
		fprintf(stderr, "Error in faction::GetForceTypeWeight: the force_type is -1.\n");
	}
	
	if (this->ForceTypeWeights.find(force_type) != this->ForceTypeWeights.end()) {
		return this->ForceTypeWeights.find(force_type)->second;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetForceTypeWeight(force_type);
	}
	
	return this->civilization->GetForceTypeWeight(force_type);
}

/**
**	@brief	Get the faction's currency
**
**	@return	The faction's currency
*/
CCurrency *faction::GetCurrency() const
{
	if (this->Currency != nullptr) {
		return this->Currency;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetCurrency();
	}
	
	return this->civilization->GetCurrency();
}

bool faction::uses_simple_name() const
{
	return this->simple_name || this->Type != FactionTypePolity;
}

const std::vector<std::unique_ptr<CForceTemplate>> &faction::GetForceTemplates(const ForceType force_type) const
{
	if (force_type == ForceType::None) {
		fprintf(stderr, "Error in faction::GetForceTemplates: the force_type is -1.\n");
	}
	
	if (this->ForceTemplates.find(force_type) != this->ForceTemplates.end()) {
		return this->ForceTemplates.find(force_type)->second;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetForceTemplates(force_type);
	}
	
	return this->civilization->GetForceTemplates(force_type);
}

const std::vector<std::unique_ptr<CAiBuildingTemplate>> &faction::GetAiBuildingTemplates() const
{
	if (this->AiBuildingTemplates.size() > 0) {
		return this->AiBuildingTemplates;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->GetAiBuildingTemplates();
	}
	
	return this->civilization->GetAiBuildingTemplates();
}

const std::vector<std::string> &faction::get_ship_names() const
{
	if (this->ship_names.size() > 0) {
		return this->ship_names;
	}
	
	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->get_ship_names();
	}
	
	return this->civilization->get_ship_names();
}

QStringList faction::get_ship_names_qstring_list() const
{
	return container::to_qstring_list(this->get_ship_names());
}

void faction::remove_ship_name(const std::string &ship_name)
{
	vector::remove_one(this->ship_names, ship_name);
}

unit_type *faction::get_class_unit_type(const unit_class *unit_class) const
{
	if (unit_class == nullptr) {
		return nullptr;
	}

	auto find_iterator = this->class_unit_types.find(unit_class);
	if (find_iterator != this->class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->get_class_unit_type(unit_class);
	}

	return this->civilization->get_class_unit_type(unit_class);
}

bool faction::is_class_unit_type(const unit_type *unit_type) const
{
	return unit_type == this->get_class_unit_type(unit_type->get_unit_class());
}

CUpgrade *faction::get_class_upgrade(const upgrade_class *upgrade_class) const
{
	if (upgrade_class == nullptr) {
		return nullptr;
	}

	auto find_iterator = this->class_upgrades.find(upgrade_class);
	if (find_iterator != this->class_upgrades.end()) {
		return find_iterator->second;
	}

	if (this->ParentFaction != -1) {
		return wyrmgus::faction::get_all()[ParentFaction]->get_class_upgrade(upgrade_class);
	}

	return this->get_civilization()->get_class_upgrade(upgrade_class);
}

const std::vector<CFiller> &faction::get_ui_fillers() const
{
	if (!this->ui_fillers.empty()) {
		return this->ui_fillers;
	}

	if (this->ParentFaction != -1) {
		return faction::get_all()[this->ParentFaction]->get_ui_fillers();
	}

	return this->get_civilization()->get_ui_fillers();
}

void faction::remove_dynasty(const wyrmgus::dynasty *dynasty)
{
	vector::remove(this->dynasties, dynasty);
}

QVariantList faction::get_acquired_upgrades_qstring_list() const
{
	return container::to_qvariant_list(this->get_acquired_upgrades());
}

void faction::remove_acquired_upgrade(CUpgrade *upgrade)
{
	vector::remove(this->acquired_upgrades, upgrade);
}

}
