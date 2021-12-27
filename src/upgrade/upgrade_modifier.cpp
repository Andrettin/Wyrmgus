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
//      (c) Copyright 1999-2021 by Vladi Belperchinov-Shabanski, Jimmy Salmon
//		and Andrettin
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

#include "upgrade/upgrade_modifier.h"

#include "config.h"
#include "game/game.h"
#include "map/map.h"
#include "player/faction.h"
#include "player/player.h"
#include "script/condition/and_condition.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

/// Upgrades modifiers
std::vector<upgrade_modifier *> upgrade_modifier::UpgradeModifiers;

upgrade_modifier::upgrade_modifier(const CUpgrade *upgrade) : upgrade(upgrade)
{
	memset(this->ChangeUnits, 0, sizeof(this->ChangeUnits));
	
	memset(this->ChangeUpgrades, '?', sizeof(this->ChangeUpgrades));
	this->Modifier.Variables.resize(UnitTypeVar.GetNumberVariable());
	this->ModifyPercent = std::make_unique<int[]>(UnitTypeVar.GetNumberVariable());
	memset(this->ModifyPercent.get(), 0, UnitTypeVar.GetNumberVariable() * sizeof(int));
}

std::unique_ptr<upgrade_modifier> upgrade_modifier::duplicate(const CUpgrade *new_upgrade) const
{
	auto modifier = std::make_unique<upgrade_modifier>(new_upgrade);

	modifier->Modifier = this->Modifier;
	memcpy(modifier->ModifyPercent.get(), this->ModifyPercent.get(), sizeof(UnitTypeVar.GetNumberVariable()));
	modifier->SpeedResearch = this->SpeedResearch;
	modifier->infantry_cost_modifier = this->infantry_cost_modifier;
	modifier->cavalry_cost_modifier = this->cavalry_cost_modifier;
	memcpy(modifier->ImproveIncomes, this->ImproveIncomes, sizeof(modifier->ImproveIncomes));
	modifier->UnitStock = this->UnitStock;
	memcpy(modifier->ChangeUnits, this->ChangeUnits, sizeof(modifier->ChangeUnits));
	memcpy(modifier->ChangeUpgrades, this->ChangeUpgrades, sizeof(modifier->ChangeUpgrades));
	modifier->ConvertTo = this->ConvertTo;
	modifier->change_civilization_to = this->change_civilization_to;
	modifier->change_faction_to = this->change_faction_to;
	modifier->removed_upgrades = this->removed_upgrades;
	modifier->unit_types = this->unit_types;
	modifier->unit_classes = this->unit_classes;

	return modifier;
}

void upgrade_modifier::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "infantry_cost_modifier") {
		this->infantry_cost_modifier = std::stoi(value);
	} else if (key == "cavalry_cost_modifier") {
		this->cavalry_cost_modifier = std::stoi(value);
	} else {
		const std::string variable_name = string::snake_case_to_pascal_case(key);

		const int index = UnitTypeVar.VariableNameLookup[variable_name.c_str()]; // variable index
		if (index != -1) { // valid index
			if (string::is_number(value)) {
				this->Modifier.Variables[index].Enable = 1;
				this->Modifier.Variables[index].Value = std::stoi(value);
				this->Modifier.Variables[index].Max = std::stoi(value);
			} else { // error
				throw std::runtime_error("Invalid value (\"" + value + "\") for variable \"" + key + "\" when defining modifier for upgrade \"" + this->get_upgrade()->get_identifier() + "\".");
			}
		} else {
			throw std::runtime_error("Invalid upgrade modifier property: \"" + key + "\".");
		}
	}
}

void upgrade_modifier::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "unit_types") {
		for (const std::string &value : values) {
			this->unit_types.push_back(unit_type::get(value));
		}
	} else if (tag == "unit_classes") {
		for (const std::string &value : values) {
			this->unit_classes.push_back(unit_class::get(value));
		}
	} else if (tag == "removed_upgrades") {
		for (const std::string &value : values) {
			const CUpgrade *removed_upgrade = CUpgrade::get(value);
			this->removed_upgrades.push_back(removed_upgrade);
		}
	} else if (tag == "costs") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			this->Modifier.set_cost(resource, std::stoi(value));
		});
	} else if (tag == "processing_bonus") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			this->Modifier.set_improve_income(resource, std::stoi(value));
		});
	} else {
		const std::string variable_name = string::snake_case_to_pascal_case(tag);
		const int index = UnitTypeVar.VariableNameLookup[variable_name.c_str()]; // variable index
		if (index != -1) { // valid index
			scope.for_each_property([&](const sml_property &property) {
				const std::string &key = property.get_key();
				if (key == "percent_value") {
					this->ModifyPercent[index] = std::stoi(property.get_value());
				} else {
					throw std::runtime_error("Invalid upgrade modifier variable property: \"" + key + "\".");
				}
			});
		} else {
			throw std::runtime_error("Invalid upgrade modifier scope: \"" + tag + "\".");
		}
	}
}

int upgrade_modifier::GetUnitStock(unit_type *unit_type) const
{
	auto find_iterator = this->UnitStock.find(unit_type);
	if (unit_type && find_iterator != this->UnitStock.end()) {
		return find_iterator->second;
	} else {
		return 0;
	}
}

void upgrade_modifier::SetUnitStock(unit_type *unit_type, int quantity)
{
	if (!unit_type) {
		return;
	}

	if (quantity == 0) {
		if (this->UnitStock.contains(unit_type)) {
			this->UnitStock.erase(unit_type);
		}
	} else {
		this->UnitStock[unit_type] = quantity;
	}
}

void upgrade_modifier::ChangeUnitStock(unit_type *unit_type, int quantity)
{
	this->SetUnitStock(unit_type, this->GetUnitStock(unit_type) + quantity);
}

bool upgrade_modifier::applies_to(const unit_type *unit_type) const
{
	if (vector::contains(this->get_unit_types(), unit_type)) {
		return true;
	}

	if (unit_type->get_unit_class() != nullptr && vector::contains(this->get_unit_classes(), unit_type->get_unit_class())) {
		return true;
	}

	return false;
}

bool upgrade_modifier::affects_variable(const int var_index) const
{
	const unit_variable &modifier_variable = this->Modifier.Variables[var_index];

	return modifier_variable.Enable || modifier_variable.Value != 0 || modifier_variable.Max != 0 || modifier_variable.Increase != 0 || this->ModifyPercent[var_index] != 0;
}

void upgrade_modifier::apply_to_player(CPlayer *player, const int multiplier) const
{
	assert_throw(player != nullptr);
	assert_throw(multiplier != 0);

	if (this->SpeedResearch != 0) {
		player->SpeedResearch += this->SpeedResearch * multiplier;
	}

	if (this->get_infantry_cost_modifier() != 0) {
		player->change_infantry_cost_modifier(this->get_infantry_cost_modifier() * multiplier);
	}

	if (this->get_cavalry_cost_modifier() != 0) {
		player->change_cavalry_cost_modifier(this->get_cavalry_cost_modifier() * multiplier);
	}

	if (multiplier > 0) {
		if (GameRunning) {
			if (this->change_civilization_to != nullptr && this->change_civilization_to != player->get_civilization()) {
				player->set_civilization(this->change_civilization_to);
			}

			if (this->change_faction_to != nullptr && (this->change_faction_to->get_civilization() != player->get_civilization() || this->change_faction_to != player->get_faction())) {
				if (this->change_faction_to->get_civilization() != player->get_civilization()) {
					player->set_civilization(this->change_faction_to->get_civilization());
				}

				player->set_faction(this->change_faction_to);
			}
		}
	}

	for (int z = 0; z < UpgradeMax; ++z) {
		// allow/forbid upgrades for player; only if upgrade is not acquired

		if (this->ChangeUpgrades[z] == '?') {
			continue;
		}

		// FIXME: check if modify is allowed

		if (player->Allow.Upgrades[z] != 'R') {
			if (this->ChangeUpgrades[z] == 'A') {
				if (multiplier > 0) {
					player->Allow.Upgrades[z] = 'A';
				} else {
					player->Allow.Upgrades[z] = 'F';
				}
			}

			if (this->ChangeUpgrades[z] == 'F') {
				if (multiplier > 0) {
					player->Allow.Upgrades[z] = 'F';
				} else {
					player->Allow.Upgrades[z] = 'A';
				}
			}

			// we can even have upgrade acquired w/o costs
			if (this->ChangeUpgrades[z] == 'R') {
				if (multiplier > 0) {
					player->Allow.Upgrades[z] = 'R';
				} else {
					player->Allow.Upgrades[z] = 'A';
				}
			}
		}
	}

	if (multiplier > 0) {
		for (const CUpgrade *removed_upgrade : this->get_removed_upgrades()) {
			if (player->has_upgrade(removed_upgrade)) {
				player->lose_upgrade(removed_upgrade);
			}
		}
	}

	const int player_index = player->get_index();

	for (unit_type *unit_type : unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		CUnitStats &stat = unit_type->Stats[player_index];

		//Wyrmgus start
		if (stat.Variables.empty()) {
			//unit type's stats not initialized
			break;
		}
		//Wyrmgus end

		//add/remove allowed units
		//FIXME: check if modify is allowed
		player->Allow.Units[unit_type->Slot] += this->ChangeUnits[unit_type->Slot] * multiplier;

		if (!this->applies_to(unit_type)) {
			continue;
		}

		std::vector<CUnit *> unitupgrade;
		FindPlayerUnitsByType(*player, *unit_type, unitupgrade);

		//if a unit type's supply is going to be changed, we need to update the player's supply accordingly
		if (this->Modifier.Variables[SUPPLY_INDEX].Value != 0) {
			for (CUnit *unit : unitupgrade) {
				if (unit->IsAlive()) {
					player->Supply += this->Modifier.Variables[SUPPLY_INDEX].Value * multiplier;
				}
			}
		}

		//if a unit type's demand is going to be changed, we need to update the player's demand accordingly
		if (this->Modifier.Variables[DEMAND_INDEX].Value != 0) {
			for (CUnit *unit : unitupgrade) {
				if (unit->IsAlive()) {
					player->Demand += this->Modifier.Variables[DEMAND_INDEX].Value * multiplier;
				}
			}
		}

		//upgrade costs
		for (const auto &[resource, cost] : this->Modifier.get_costs()) {
			stat.change_cost(resource, cost * multiplier);
		}

		for (const auto &[resource, storing] : this->Modifier.get_storing()) {
			stat.change_storing(resource, storing * multiplier);
		}

		for (const auto &[resource, quantity] : this->Modifier.get_improve_incomes()) {
			if (stat.get_improve_income(resource) == 0) {
				stat.set_improve_income(resource, resource->get_default_income());
			}

			const int old_processing_bonus = stat.get_improve_income(resource);
			const int processing_bonus_change = quantity * multiplier;

			stat.change_improve_income(resource, processing_bonus_change);

			if (processing_bonus_change >= 0) {
				//update player's income
				if (!unitupgrade.empty()) {
					player->set_income(resource, std::max(player->get_income(resource), stat.get_improve_income(resource)));
				}
			} else {
				//if this was the highest improve income, search for another
				if (player->get_income(resource) != 0 && old_processing_bonus >= player->get_income(resource)) {
					int m = resource->get_default_income();

					for (int k = 0; k < player->GetUnitCount(); ++k) {
						const CUnit &player_unit = player->GetUnit(k);
						if (!player_unit.IsAlive()) {
							continue;
						}

						m = std::max(m, player_unit.Type->Stats[player_index].get_improve_income(resource));
					}

					player->set_income(resource, m);
				}
			}
		}

		for (const auto &[resource, quantity] : this->Modifier.get_resource_demands()) {
			stat.change_resource_demand(resource, quantity * multiplier);
		}

		for (const auto &[stock_unit_type, unit_stock] : this->Modifier.get_unit_stocks()) {
			stat.change_unit_stock(stock_unit_type, unit_stock *multiplier);
		}

		int varModified = 0;
		for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
			const unit_variable &modifier_variable = this->Modifier.Variables[j];
			const int modify_percent = this->ModifyPercent[j];

			varModified |= modifier_variable.Value
				| modifier_variable.Max
				| modifier_variable.Increase
				| modifier_variable.Enable
				| modify_percent;

			unit_variable &stat_variable = stat.Variables[j];

			stat_variable.Enable |= modifier_variable.Enable;

			const int effective_modify_percent = modify_percent * multiplier;

			if (effective_modify_percent != 0) {
				if (j != MANA_INDEX || effective_modify_percent < 0) {
					if (multiplier < 0) {
						//need to calculate it a bit differently to "undo" the modification
						stat_variable.Value = stat_variable.Value * 100 / (100 + modify_percent);
					} else {
						stat_variable.Value += stat_variable.Value * modify_percent / 100;
					}
				}

				if (multiplier < 0) {
					stat_variable.Max = stat_variable.Max * 100 / (100 + modify_percent);
				} else {
					stat_variable.Max += stat_variable.Max * modify_percent / 100;
				}
			} else {
				const int effective_modifier_value = modifier_variable.Value * multiplier;

				if (j != MANA_INDEX || effective_modifier_value < 0) {
					stat_variable.Value += effective_modifier_value;
				}

				stat_variable.Max += modifier_variable.Max * multiplier;
				stat_variable.Increase += modifier_variable.Increase * multiplier;
			}

			if (!is_potentially_negative_variable(j)) {
				stat_variable.Max = std::max(stat_variable.Max, 0);
			}

			//Wyrmgus start
//			stat_variable.Value = std::clamp(stat_variable.Value, 0, stat_variable.Max);
			if (stat_variable.Max > 0) {
				stat_variable.Value = std::clamp(stat_variable.Value, 0, stat_variable.Max);
			}
			//Wyrmgus end
		}

		if (this->Modifier.Variables[TRADECOST_INDEX].Value != 0) {
			const int trade_cost_change = this->Modifier.Variables[TRADECOST_INDEX].Value * multiplier;

			if (trade_cost_change <= 0) {
				if (!unitupgrade.empty()) {
					player->set_trade_cost(std::min(player->get_trade_cost(), stat.Variables[TRADECOST_INDEX].Value));
				}
			} else {
				if ((stat.Variables[TRADECOST_INDEX].Value + trade_cost_change) <= player->get_trade_cost()) {
					int m = DefaultTradeCost;

					for (int k = 0; k < player->GetUnitCount(); ++k) {
						if (player->GetUnit(k).Type != nullptr) {
							m = std::min(m, player->GetUnit(k).Type->Stats[player_index].Variables[TRADECOST_INDEX].Value);
						}
					}

					player->set_trade_cost(m);
				}
			}
		}

		unitupgrade.clear();

		FindUnitsByType(*unit_type, unitupgrade, true);

		//and now modify ingame units

		if (varModified) {
			for (CUnit *unit : unitupgrade) {
				if (unit->Player != player) {
					continue;
				}

				//Wyrmgus start
				if (
					(this->get_upgrade()->is_weapon() && unit->EquippedItems[static_cast<int>(item_slot::weapon)].size() > 0)
					|| (this->get_upgrade()->is_shield() && unit->EquippedItems[static_cast<int>(item_slot::shield)].size() > 0)
					|| (this->get_upgrade()->is_boots() && unit->EquippedItems[static_cast<int>(item_slot::boots)].size() > 0)
					|| (this->get_upgrade()->is_arrows() && unit->EquippedItems[static_cast<int>(item_slot::arrows)].size() > 0)
				) {
					//if the unit already has an item equipped of the same equipment type as this upgrade, don't apply the modifier to it
					continue;
				}

				if (unit->get_character() != nullptr && this->get_upgrade()->get_deity() != nullptr) {
					//heroes choose their own deities
					continue;
				}
				//Wyrmgus end

				this->apply_to_unit(unit, multiplier);
			}
		}

		//Wyrmgus start
		for (CUnit *unit : unitupgrade) {
			if (unit->Player != player) {
				continue;
			}

			//add or remove starting abilities from the unit if the upgrade enabled/disabled them
			for (const CUpgrade *ability_upgrade : unit->Type->StartingAbilities) {
				if (!unit->GetIndividualUpgrade(ability_upgrade) && check_conditions(ability_upgrade, unit)) {
					IndividualUpgradeAcquire(*unit, ability_upgrade);
				} else if (unit->GetIndividualUpgrade(ability_upgrade) && !check_conditions(ability_upgrade, unit)) {
					IndividualUpgradeLost(*unit, ability_upgrade);
				}
			}

			//change variation if the current one has become forbidden
			const unit_type_variation *current_variation = unit->GetVariation();
			if (current_variation != nullptr) {
				if (!unit->can_have_variation(current_variation)) {
					unit->ChooseVariation();
				}
			}
			for (int i = 0; i < MaxImageLayers; ++i) {
				const unit_type_variation *current_layer_variation = unit->GetLayerVariation(i);
				if (current_layer_variation != nullptr) {
					if (!unit->can_have_variation(current_layer_variation)) {
						unit->ChooseVariation(nullptr, false, i);
					}
				}
			}
			unit->UpdateButtonIcons();
		}
		//Wyrmgus end

		if (this->ConvertTo != nullptr) {
			if (multiplier > 0) {
				ConvertUnitTypeTo(*player, *unit_type, *this->ConvertTo);
			} else {
				ConvertUnitTypeTo(*player, *this->ConvertTo, *unit_type);
			}
		}
	}
}

void upgrade_modifier::apply_to_unit(CUnit *unit, const int multiplier) const
{
	assert_throw(unit != nullptr);
	assert_throw(multiplier != 0);

	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (!this->affects_variable(i)) {
			continue;
		}

		switch (i) {
			case SIGHTRANGE_INDEX:
			case DAYSIGHTRANGEBONUS_INDEX:
			case NIGHTSIGHTRANGEBONUS_INDEX:
			case ETHEREALVISION_INDEX:
				if (!unit->Removed && !SaveGameLoading) {
					MapUnmarkUnitSight(*unit);
				}
				break;
			default:
				break;
		}

		unit_variable &unit_variable = unit->Variable[i];
		const wyrmgus::unit_variable &modifier_variable = this->Modifier.Variables[i];
		const int modify_percent = this->ModifyPercent[i];

		unit_variable.Enable |= modifier_variable.Enable;

		const int effective_modify_percent = modify_percent * multiplier;
		const int effective_modifier_value = modifier_variable.Value * multiplier;

		const int old_value = unit_variable.Value;

		if (effective_modify_percent) {
			if (i != MANA_INDEX || effective_modify_percent < 0) {
				if (multiplier < 0) {
					//need to calculate it a bit differently to "undo" the modification
					unit_variable.Value = unit_variable.Value * 100 / (100 + modify_percent);
				} else {
					unit_variable.Value += unit_variable.Value * modify_percent / 100;
				}
			}

			if (multiplier < 0) {
				unit_variable.Max = unit_variable.Max * 100 / (100 + modify_percent);
			} else {
				unit_variable.Max += unit_variable.Max * modify_percent / 100;
			}
		} else {
			if (i != MANA_INDEX || effective_modifier_value < 0) {
				unit_variable.Value += effective_modifier_value;
			}
			unit_variable.Increase += modifier_variable.Increase * multiplier;
		}

		unit_variable.Max += modifier_variable.Max * multiplier;

		if (!is_potentially_negative_variable(i)) {
			unit_variable.Max = std::max(unit_variable.Max, 0);
		}

		if (unit_variable.Max > 0) {
			unit_variable.Value = std::clamp(unit_variable.Value, 0, unit_variable.Max);
		}

		switch (i) {
			case SIGHTRANGE_INDEX:
			case DAYSIGHTRANGEBONUS_INDEX:
			case NIGHTSIGHTRANGEBONUS_INDEX:
			case ETHEREALVISION_INDEX:
				//if the sight range was modified, we need to change the unit to the new range, as otherwise the counters get confused
				if (!unit->Removed && !SaveGameLoading) {
					UpdateUnitSightRange(*unit);
					MapMarkUnitSight(*unit);
				}
				break;
			default:
				break;
		}

		unit->on_variable_changed(i, unit_variable.Value - old_value);
	}

	for (const auto &[stock_unit_type, unit_stock] : this->Modifier.get_unit_stocks()) {
		const int effective_unit_stock = unit_stock * multiplier;

		if (effective_unit_stock < 0) {
			unit->ChangeUnitStock(stock_unit_type, effective_unit_stock);
		}
	}
}

}
