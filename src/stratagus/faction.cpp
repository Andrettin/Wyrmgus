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

#include "civilization.h"
#include "grand_strategy.h" //for the faction tier string to enum conversion
#include "luacallback.h"
#include "player_color.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace stratagus {

faction::~faction()
{
	for (const auto &kv_pair : this->ForceTemplates) {
		for (size_t i = 0; i < kv_pair.second.size(); ++i) {
			delete kv_pair.second[i];
		}
	}
	
	for (size_t i = 0; i < this->AiBuildingTemplates.size(); ++i) {
		delete this->AiBuildingTemplates[i];
	}

	if (this->Conditions) {
		delete Conditions;
	}
}

void faction::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type") {
		const int faction_type = GetFactionTypeIdByName(value);
		if (faction_type != -1) {
			this->Type = faction_type;
		} else {
			throw std::runtime_error("Faction type \"" + value + "\" doesn't exist.");
		}
	} else if (key == "default_tier") {
		const faction_tier tier = GetFactionTierIdByName(value);
		if (tier != faction_tier::none) {
			this->default_tier = tier;
		} else {
			throw std::runtime_error("Faction tier \"" + value + "\" doesn't exist.");
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
				state = GetDiplomacyStateIdByName(value);
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

	data_entry::initialize();
}

void faction::check() const
{
	if (this->civilization == nullptr) {
		throw std::runtime_error("Faction \"" + this->get_identifier() + "\" has no civilization.");
	}
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

std::vector<CForceTemplate *> faction::GetForceTemplates(const ForceType force_type) const
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

std::vector<CAiBuildingTemplate *> faction::GetAiBuildingTemplates() const
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
		return stratagus::faction::get_all()[ParentFaction]->get_class_upgrade(upgrade_class);
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

QVariantList faction::get_acquired_upgrades_qstring_list() const
{
	return container::to_qvariant_list(this->get_acquired_upgrades());
}

void faction::remove_acquired_upgrade(CUpgrade *upgrade)
{
	vector::remove(this->acquired_upgrades, upgrade);
}

}
