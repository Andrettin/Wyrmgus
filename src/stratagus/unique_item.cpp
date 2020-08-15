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

#include "unique_item.h"

#include "character.h"
#include "network.h"
#include "persistent_item.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"

namespace wyrmgus {

bool unique_item::CanDrop() const
{
	// unique items cannot drop if a persistent hero owns them already, or if there's already one of them in the current scenario; unless it's a character-specific bound item, in which case it can still drop
	if (!IsNetworkGame()) {
		for (character *character : character::get_all()) {
			for (const auto &item : character->get_items()) {
				if (item->get_unique() == this && !item->Bound) {
					return false;
				}
			}
		}
		
		for (std::map<std::string, character *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
			for (const auto &item : iterator->second->get_items()) {
				if (item->get_unique() == this && !item->Bound) {
					return false;
				}
			}
		}
	}
	
	if (GameRunning) {
		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			CUnit &unit = **it;
			if (unit.Unique == this && !unit.Bound) {
				return false;
			}
		}
	}

	return true;
}

icon *unique_item::get_icon() const
{
	if (this->icon != nullptr) {
		return this->icon;
	} else {
		return this->Type->Icon.Icon;
	}
}

int unique_item::GetMagicLevel() const
{
	int magic_level = 0;
	
	if (this->Prefix) {
		magic_level += this->Prefix->MagicLevel;
	}
	
	if (this->Suffix) {
		magic_level += this->Suffix->MagicLevel;
	}
	
	if (this->Set) {
		magic_level += this->Set->MagicLevel;
	}
	
	if (this->Work) {
		magic_level += this->Work->MagicLevel;
	}
	
	if (this->Elixir) {
		magic_level += this->Elixir->MagicLevel;
	}
	
	return magic_level;
}

}

std::string GetUniqueItemEffectsString(const std::string &item_ident)
{
	const wyrmgus::unique_item *item = wyrmgus::unique_item::try_get(item_ident);

	if (item) {
		std::string item_effects_string;

		bool first_var = true;

		for (size_t var = 0; var < UnitTypeVar.GetNumberVariable(); ++var) {
			if (
				!(var == BASICDAMAGE_INDEX || var == PIERCINGDAMAGE_INDEX || var == THORNSDAMAGE_INDEX
					|| var == FIREDAMAGE_INDEX || var == COLDDAMAGE_INDEX || var == ARCANEDAMAGE_INDEX || var == LIGHTNINGDAMAGE_INDEX
					|| var == AIRDAMAGE_INDEX || var == EARTHDAMAGE_INDEX || var == WATERDAMAGE_INDEX || var == ACIDDAMAGE_INDEX
					|| var == ARMOR_INDEX || var == FIRERESISTANCE_INDEX || var == COLDRESISTANCE_INDEX || var == ARCANERESISTANCE_INDEX || var == LIGHTNINGRESISTANCE_INDEX
					|| var == AIRRESISTANCE_INDEX || var == EARTHRESISTANCE_INDEX || var == WATERRESISTANCE_INDEX || var == ACIDRESISTANCE_INDEX
					|| var == HACKRESISTANCE_INDEX || var == PIERCERESISTANCE_INDEX || var == BLUNTRESISTANCE_INDEX
					|| var == ACCURACY_INDEX || var == EVASION_INDEX || var == SPEED_INDEX || var == CHARGEBONUS_INDEX || var == BACKSTAB_INDEX
					|| var == HITPOINTHEALING_INDEX || var == HITPOINTBONUS_INDEX
					|| var == SIGHTRANGE_INDEX || var == DAYSIGHTRANGEBONUS_INDEX || var == NIGHTSIGHTRANGEBONUS_INDEX
					|| var == GIVERESOURCE_INDEX || var == TIMEEFFICIENCYBONUS_INDEX || var == RESEARCHSPEEDBONUS_INDEX || var == GARRISONEDRANGEBONUS_INDEX
					|| var == KNOWLEDGEMAGIC_INDEX || var == KNOWLEDGEWARFARE_INDEX || var == KNOWLEDGEMINING_INDEX
					|| var == BONUSAGAINSTMOUNTED_INDEX || var == BONUSAGAINSTBUILDINGS_INDEX || var == BONUSAGAINSTAIR_INDEX || var == BONUSAGAINSTGIANTS_INDEX || var == BONUSAGAINSTDRAGONS_INDEX
					|| var == SUPPLY_INDEX || var == ETHEREALVISION_INDEX
					|| var == ATTACKRANGE_INDEX)
				) {
				continue;
			}

			int variable_value = 0;
			int variable_increase = 0;
			if (item->Type->BoolFlag[ITEM_INDEX].value && item->Work == nullptr && item->Elixir == nullptr) {
				variable_value = item->Type->DefaultStat.Variables[var].Value;
				variable_increase = item->Type->DefaultStat.Variables[var].Increase;
			}

			if (var == GIVERESOURCE_INDEX && item->ResourcesHeld != 0) {
				variable_value = item->ResourcesHeld;
			}

			for (const wyrmgus::upgrade_modifier *modifier : wyrmgus::upgrade_modifier::UpgradeModifiers) {
				if (
					(item->Prefix != nullptr && modifier->UpgradeId == item->Prefix->ID)
					|| (item->Suffix != nullptr && modifier->UpgradeId == item->Suffix->ID)
					|| (item->Work != nullptr && modifier->UpgradeId == item->Work->ID)
					|| (item->Elixir != nullptr && modifier->UpgradeId == item->Elixir->ID)
					) {
					variable_value += modifier->Modifier.Variables[var].Value;
					variable_increase += modifier->Modifier.Variables[var].Increase;
				}
			}

			if ((item->Type->BoolFlag[ITEM_INDEX].value && item->Type->DefaultStat.Variables[var].Enable && item->Work == nullptr && item->Elixir == nullptr) || variable_value != 0) {
				if (!first_var) {
					item_effects_string += ", ";
				} else {
					first_var = false;
				}

				if (IsBooleanVariable(var) && variable_value < 0) {
					item_effects_string += "Lose ";
				}

				if (!IsBooleanVariable(var)) {
					if (variable_value >= 0 && var != HITPOINTHEALING_INDEX && var != GIVERESOURCE_INDEX) {
						item_effects_string += "+";
					}
					item_effects_string += std::to_string((long long) variable_value);
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
				item_effects_string += std::to_string((long long) variable_increase);
				item_effects_string += " ";

				item_effects_string += GetVariableDisplayName(var, true);
			}
		}

		if (item->Set) {
			for (size_t var = 0; var < UnitTypeVar.GetNumberVariable(); ++var) {
				if (
					!(var == BASICDAMAGE_INDEX || var == PIERCINGDAMAGE_INDEX || var == THORNSDAMAGE_INDEX
						|| var == FIREDAMAGE_INDEX || var == COLDDAMAGE_INDEX || var == ARCANEDAMAGE_INDEX || var == LIGHTNINGDAMAGE_INDEX
						|| var == AIRDAMAGE_INDEX || var == EARTHDAMAGE_INDEX || var == WATERDAMAGE_INDEX || var == ACIDDAMAGE_INDEX
						|| var == ARMOR_INDEX || var == FIRERESISTANCE_INDEX || var == COLDRESISTANCE_INDEX || var == ARCANERESISTANCE_INDEX || var == LIGHTNINGRESISTANCE_INDEX
						|| var == AIRRESISTANCE_INDEX || var == EARTHRESISTANCE_INDEX || var == WATERRESISTANCE_INDEX || var == ACIDRESISTANCE_INDEX
						|| var == HACKRESISTANCE_INDEX || var == PIERCERESISTANCE_INDEX || var == BLUNTRESISTANCE_INDEX
						|| var == ACCURACY_INDEX || var == EVASION_INDEX || var == SPEED_INDEX || var == CHARGEBONUS_INDEX || var == BACKSTAB_INDEX
						|| var == HITPOINTHEALING_INDEX || var == HITPOINTBONUS_INDEX
						|| var == SIGHTRANGE_INDEX || var == DAYSIGHTRANGEBONUS_INDEX || var == NIGHTSIGHTRANGEBONUS_INDEX
						|| var == GIVERESOURCE_INDEX || var == TIMEEFFICIENCYBONUS_INDEX || var == RESEARCHSPEEDBONUS_INDEX || var == GARRISONEDRANGEBONUS_INDEX
						|| var == KNOWLEDGEMAGIC_INDEX || var == KNOWLEDGEWARFARE_INDEX || var == KNOWLEDGEMINING_INDEX
						|| var == BONUSAGAINSTMOUNTED_INDEX || var == BONUSAGAINSTBUILDINGS_INDEX || var == BONUSAGAINSTAIR_INDEX || var == BONUSAGAINSTGIANTS_INDEX || var == BONUSAGAINSTDRAGONS_INDEX
						|| var == SUPPLY_INDEX
						|| var == ATTACKRANGE_INDEX)
					) {
					continue;
				}

				int variable_value = 0;
				int variable_increase = 0;

				for (const auto &modifier : item->Set->get_modifiers()) {
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
						if (variable_value >= 0 && var != HITPOINTHEALING_INDEX && var != GIVERESOURCE_INDEX) {
							item_effects_string += "+";
						}
						item_effects_string += std::to_string((long long) variable_value);
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
					item_effects_string += std::to_string((long long) variable_increase);
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
