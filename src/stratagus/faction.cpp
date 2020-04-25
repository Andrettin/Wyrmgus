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

namespace stratagus {

faction::~faction()
{
	for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = this->ForceTemplates.begin(); iterator != this->ForceTemplates.end(); ++iterator) {
		for (size_t i = 0; i < iterator->second.size(); ++i) {
			delete iterator->second[i];
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

	if (tag == "colors") {
		this->Colors.clear(); //remove previously defined colors

		for (const std::string &value : values) {
			const int color = GetPlayerColorIndexByName(value);
			if (color != -1) {
				this->Colors.push_back(color);
			} else {
				throw std::runtime_error("Player color \"" + value + "\" doesn't exist.");
			}
		}
	} else if (tag == "develops_from") {
		for (const std::string &value : values) {
			faction *other_faction = faction::get(value);
			this->DevelopsFrom.push_back(other_faction);
			other_faction->DevelopsTo.push_back(this);
		}
	} else {
		data_entry::process_sml_scope(scope);
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

int faction::GetForceTypeWeight(int force_type) const
{
	if (force_type == -1) {
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

std::vector<CForceTemplate *> faction::GetForceTemplates(int force_type) const
{
	if (force_type == -1) {
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

CUnitType *faction::get_class_unit_type(const unit_class *unit_class) const
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

}
