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
//      (c) Copyright 2015-2022 by Andrettin
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

#include "item/unique_item.h"

#include "character.h"
#include "game/game.h"
#include "item/persistent_item.h"
#include "network/network.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "upgrade/upgrade_structs.h"

namespace wyrmgus {

bool unique_item::compare_encyclopedia_entries(const unique_item *lhs, const unique_item *rhs)
{
	const wyrmgus::unit_type *lhs_unit_type = lhs->get_unit_type();
	const wyrmgus::unit_type *rhs_unit_type = rhs->get_unit_type();

	if (lhs_unit_type != rhs_unit_type) {
		if (lhs_unit_type->BoolFlag[ITEM_INDEX].value != rhs_unit_type->BoolFlag[ITEM_INDEX].value) {
			return lhs_unit_type->BoolFlag[ITEM_INDEX].value;
		}

		if (lhs_unit_type->BoolFlag[BUILDING_INDEX].value != rhs_unit_type->BoolFlag[BUILDING_INDEX].value) {
			return lhs_unit_type->BoolFlag[BUILDING_INDEX].value;
		}

		return unit_type::compare_encyclopedia_entries(lhs_unit_type, rhs_unit_type);
	}

	return named_data_entry::compare_encyclopedia_entries(lhs, rhs);
}

void unique_item::initialize()
{
	if (this->set != nullptr) {
		this->set->add_set_unique(this);
	}

	data_entry::initialize();
}

std::string unique_item::get_encyclopedia_text() const
{
	std::string text;

	if (this->get_unit_type() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Type: " + this->get_unit_type()->get_link_string());
	}

	if (this->get_set() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Set: " + this->get_set()->get_name());
	}

	std::string description = this->get_description();
	if (this->get_set() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(description, this->get_set()->get_description());
	}
	if (!description.empty()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Description: " + description);
	}

	if (!this->get_quote().empty()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Quote: " + this->get_quote());
	} else if (this->get_set() != nullptr && !this->get_set()->get_quote().empty()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Quote: " + this->get_set()->get_quote());
	}

	std::string background = this->get_background();
	if (this->get_set() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(background, this->get_set()->get_background());
	}
	if (!background.empty()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Background: " + background);
	}

	std::string notes = this->get_notes();
	if (this->get_set() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(notes, this->get_set()->get_notes());
	}
	if (!notes.empty()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Notes: " + notes);
	}

	if (this->get_magic_level() > 0) {
		named_data_entry::concatenate_encyclopedia_text(text, "Magic Level: " + std::to_string(this->get_magic_level()));
	}

	const std::string effects = GetUniqueItemEffectsString(this->get_identifier());
	if (!effects.empty()) {
		named_data_entry::concatenate_encyclopedia_text(text, "Effects: " + effects);
	}

	return text;
}

icon *unique_item::get_icon() const
{
	if (this->icon != nullptr) {
		return this->icon;
	} else {
		return this->get_unit_type()->get_icon();
	}
}

bool unique_item::can_drop() const
{
	// unique items cannot drop if a persistent hero owns them already, or if there's already one of them in the current scenario; unless it's a character-specific bound item, in which case it can still drop
	if (!IsNetworkGame()) {
		for (const character *character : character::get_all_with_custom()) {
			for (const auto &item : character->get_items()) {
				if (item->get_unique() == this && !item->is_bound()) {
					return false;
				}
			}
		}
	}
	
	if (GameRunning) {
		if (this->get_unit() != nullptr) {
			return false;
		}
	}

	return true;
}

int unique_item::get_magic_level() const
{
	int magic_level = 0;
	
	if (this->get_prefix() != nullptr) {
		magic_level += this->get_prefix()->get_magic_level();
	}
	
	if (this->get_suffix() != nullptr) {
		magic_level += this->get_suffix()->get_magic_level();
	}
	
	if (this->get_set() != nullptr) {
		magic_level += this->get_set()->get_magic_level();
	}
	
	if (this->get_work() != nullptr) {
		magic_level += this->get_work()->get_magic_level();
	}
	
	if (this->get_elixir() != nullptr) {
		magic_level += this->get_elixir()->get_magic_level();
	}
	
	return magic_level;
}

CUnit *unique_item::get_unit() const
{
	for (CUnit *unit : unit_manager::get()->get_units()) {
		if (unit->get_unique() == this && !unit->Bound) {
			return unit;
		}
	}

	return nullptr;
}

}

std::string GetUniqueItemEffectsString(const std::string &item_ident)
{
	const wyrmgus::unique_item *item = wyrmgus::unique_item::try_get(item_ident);

	if (item) {
		std::string item_effects_string;

		bool first_var = true;

		for (size_t var = 0; var < UnitTypeVar.GetNumberVariable(); ++var) {
			switch (var) {
				case BASICDAMAGE_INDEX:
				case PIERCINGDAMAGE_INDEX:
				case THORNSDAMAGE_INDEX:
				case FIREDAMAGE_INDEX:
				case COLDDAMAGE_INDEX:
				case ARCANEDAMAGE_INDEX:
				case LIGHTNINGDAMAGE_INDEX:
				case AIRDAMAGE_INDEX:
				case EARTHDAMAGE_INDEX:
				case WATERDAMAGE_INDEX:
				case ACIDDAMAGE_INDEX:
				case SHADOW_DAMAGE_INDEX:
				case ARMOR_INDEX:
				case FIRERESISTANCE_INDEX:
				case COLDRESISTANCE_INDEX:
				case ARCANERESISTANCE_INDEX:
				case LIGHTNINGRESISTANCE_INDEX:
				case AIRRESISTANCE_INDEX:
				case EARTHRESISTANCE_INDEX:
				case WATERRESISTANCE_INDEX:
				case ACIDRESISTANCE_INDEX:
				case SHADOW_RESISTANCE_INDEX:
				case HACKRESISTANCE_INDEX:
				case PIERCERESISTANCE_INDEX:
				case BLUNTRESISTANCE_INDEX:
				case ACCURACY_INDEX:
				case EVASION_INDEX:
				case SPEED_INDEX:
				case CHARGEBONUS_INDEX:
				case BACKSTAB_INDEX:
				case HITPOINTHEALING_INDEX:
				case HITPOINTBONUS_INDEX:
				case MANA_RESTORATION_INDEX:
				case MANA_INDEX:
				case SIGHTRANGE_INDEX:
				case DAYSIGHTRANGEBONUS_INDEX:
				case NIGHTSIGHTRANGEBONUS_INDEX:
				case GIVERESOURCE_INDEX:
				case TIMEEFFICIENCYBONUS_INDEX:
				case RESEARCHSPEEDBONUS_INDEX:
				case GARRISONEDRANGEBONUS_INDEX:
				case RAIL_SPEED_BONUS_INDEX:
				case KNOWLEDGEMAGIC_INDEX:
				case KNOWLEDGEWARFARE_INDEX:
				case KNOWLEDGEMINING_INDEX:
				case BONUS_AGAINST_INFANTRY_INDEX:
				case BONUSAGAINSTMOUNTED_INDEX:
				case BONUSAGAINSTBUILDINGS_INDEX:
				case BONUSAGAINSTAIR_INDEX:
				case BONUSAGAINSTGIANTS_INDEX:
				case BONUSAGAINSTDRAGONS_INDEX:
				case SUPPLY_INDEX:
				case ETHEREALVISION_INDEX:
				case ATTACKRANGE_INDEX:
					break;
				default:
					continue;
			}

			int variable_value = 0;
			int variable_increase = 0;
			if (item->get_unit_type()->BoolFlag[ITEM_INDEX].value && item->get_work() == nullptr && item->get_elixir() == nullptr) {
				variable_value = item->get_unit_type()->DefaultStat.Variables[var].Value;
				variable_increase = item->get_unit_type()->DefaultStat.Variables[var].Increase;
			}

			if (var == GIVERESOURCE_INDEX && item->get_resources_held() != 0) {
				variable_value = item->get_resources_held();
			}

			for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
				if (
					(item->get_prefix() != nullptr && modifier->get_upgrade() == item->get_prefix())
					|| (item->get_suffix() != nullptr && modifier->get_upgrade() == item->get_suffix())
					|| (item->get_work() != nullptr && modifier->get_upgrade() == item->get_work())
					|| (item->get_elixir() != nullptr && modifier->get_upgrade() == item->get_elixir())
				) {
					variable_value += modifier->Modifier.Variables[var].Value;
					variable_increase += modifier->Modifier.Variables[var].Increase;
				}
			}

			if ((item->get_unit_type()->BoolFlag[ITEM_INDEX].value && item->get_unit_type()->DefaultStat.Variables[var].Enable && item->get_work() == nullptr && item->get_elixir() == nullptr) || variable_value != 0) {
				if (!first_var) {
					item_effects_string += ", ";
				} else {
					first_var = false;
				}

				if (IsBooleanVariable(var) && variable_value < 0) {
					item_effects_string += "Lose ";
				}

				if (!IsBooleanVariable(var)) {
					if (variable_value >= 0 && var != HITPOINTHEALING_INDEX && var != MANA_RESTORATION_INDEX && var != GIVERESOURCE_INDEX) {
						item_effects_string += "+";
					}
					item_effects_string += std::to_string(variable_value);
					if (IsPercentageVariable(var)) {
						item_effects_string += "%";
					}
					item_effects_string += " ";
				}

				item_effects_string += GetVariableDisplayName(var);
			}

			if (variable_increase != 0) {
				if (!first_var) {
					item_effects_string += ", ";
				} else {
					first_var = false;
				}

				if (variable_increase > 0) {
					item_effects_string += "+";
				}
				item_effects_string += std::to_string(variable_increase);
				item_effects_string += " ";

				item_effects_string += GetVariableDisplayName(var, true);
			}
		}

		if (item->get_set() != nullptr) {
			for (size_t var = 0; var < UnitTypeVar.GetNumberVariable(); ++var) {
				switch (var) {
				case BASICDAMAGE_INDEX:
				case PIERCINGDAMAGE_INDEX:
				case THORNSDAMAGE_INDEX:
				case FIREDAMAGE_INDEX:
				case COLDDAMAGE_INDEX:
				case ARCANEDAMAGE_INDEX:
				case LIGHTNINGDAMAGE_INDEX:
				case AIRDAMAGE_INDEX:
				case EARTHDAMAGE_INDEX:
				case WATERDAMAGE_INDEX:
				case ACIDDAMAGE_INDEX:
				case SHADOW_DAMAGE_INDEX:
				case ARMOR_INDEX:
				case FIRERESISTANCE_INDEX:
				case COLDRESISTANCE_INDEX:
				case ARCANERESISTANCE_INDEX:
				case LIGHTNINGRESISTANCE_INDEX:
				case AIRRESISTANCE_INDEX:
				case EARTHRESISTANCE_INDEX:
				case WATERRESISTANCE_INDEX:
				case ACIDRESISTANCE_INDEX:
				case SHADOW_RESISTANCE_INDEX:
				case HACKRESISTANCE_INDEX:
				case PIERCERESISTANCE_INDEX:
				case BLUNTRESISTANCE_INDEX:
				case ACCURACY_INDEX:
				case EVASION_INDEX:
				case SPEED_INDEX:
				case CHARGEBONUS_INDEX:
				case BACKSTAB_INDEX:
				case HITPOINTHEALING_INDEX:
				case HITPOINTBONUS_INDEX:
				case MANA_RESTORATION_INDEX:
				case MANA_INDEX:
				case SIGHTRANGE_INDEX:
				case DAYSIGHTRANGEBONUS_INDEX:
				case NIGHTSIGHTRANGEBONUS_INDEX:
				case GIVERESOURCE_INDEX:
				case TIMEEFFICIENCYBONUS_INDEX:
				case RESEARCHSPEEDBONUS_INDEX:
				case GARRISONEDRANGEBONUS_INDEX:
				case RAIL_SPEED_BONUS_INDEX:
				case KNOWLEDGEMAGIC_INDEX:
				case KNOWLEDGEWARFARE_INDEX:
				case KNOWLEDGEMINING_INDEX:
				case BONUS_AGAINST_INFANTRY_INDEX:
				case BONUSAGAINSTMOUNTED_INDEX:
				case BONUSAGAINSTBUILDINGS_INDEX:
				case BONUSAGAINSTAIR_INDEX:
				case BONUSAGAINSTGIANTS_INDEX:
				case BONUSAGAINSTDRAGONS_INDEX:
				case SUPPLY_INDEX:
				case ETHEREALVISION_INDEX:
				case ATTACKRANGE_INDEX:
					break;
				default:
					continue;
				}

				int variable_value = 0;
				int variable_increase = 0;

				for (const auto &modifier : item->get_set()->get_modifiers()) {
					variable_value += modifier->Modifier.Variables[var].Value;
					variable_increase += modifier->Modifier.Variables[var].Increase;
				}

				if (variable_value != 0) {
					if (!first_var) {
						item_effects_string += ", ";
					} else {
						first_var = false;
					}

					if (IsBooleanVariable(var) && variable_value < 0) {
						item_effects_string += "Lose ";
					}

					if (!IsBooleanVariable(var)) {
						if (variable_value >= 0 && var != HITPOINTHEALING_INDEX && var != MANA_RESTORATION_INDEX && var != GIVERESOURCE_INDEX) {
							item_effects_string += "+";
						}
						item_effects_string += std::to_string(variable_value);
						if (IsPercentageVariable(var)) {
							item_effects_string += "%";
						}
						item_effects_string += " ";
					}

					item_effects_string += GetVariableDisplayName(var);
					item_effects_string += " (Set Bonus)";
				}

				if (variable_increase != 0) {
					if (!first_var) {
						item_effects_string += ", ";
					} else {
						first_var = false;
					}

					if (variable_increase > 0) {
						item_effects_string += "+";
					}
					item_effects_string += std::to_string(variable_increase);
					item_effects_string += " ";

					item_effects_string += GetVariableDisplayName(var, true);
					item_effects_string += " (Set Bonus)";
				}
			}
		}

		return item_effects_string;
	}

	return "";
}
