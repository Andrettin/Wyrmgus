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

#include "persistent_item.h"

#include "character.h"
#include "config.h"
#include "item_slot.h"
#include "spells.h"
#include "unique_item.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "util/string_util.h"

namespace stratagus {

void persistent_item::ProcessConfigData(const CConfigData *config_data)
{
	bool is_equipped = false;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "type") {
			unit_type *unit_type = unit_type::get(value);
			this->Type = unit_type;
		} else if (key == "prefix") {
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->Prefix = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "suffix") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->Suffix = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "spell") {
			spell *spell = spell::get(value);
			this->Spell = spell;
		} else if (key == "work") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->Work = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "elixir") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::try_get(value);
			if (upgrade) {
				this->Elixir = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "unique") {
			unique_item *unique_item = unique_item::try_get(value);
			if (unique_item != nullptr) {
				this->Unique = unique_item;
				this->Name = unique_item->get_name();
				if (unique_item->Type != nullptr) {
					this->Type = unique_item->Type;
				} else {
					fprintf(stderr, "Unique item \"%s\" has no type.\n", unique_item->get_identifier().c_str());
				}
				this->Prefix = unique_item->Prefix;
				this->Suffix = unique_item->Suffix;
				this->Spell = unique_item->Spell;
				this->Work = unique_item->Work;
				this->Elixir = unique_item->Elixir;
			} else {
				fprintf(stderr, "Unique item \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "bound") {
			this->Bound = string::to_bool(value);
		} else if (key == "identified") {
			this->Identified = string::to_bool(value);
		} else if (key == "equipped") {
			is_equipped = string::to_bool(value);
		} else {
			fprintf(stderr, "Invalid item property: \"%s\".\n", key.c_str());
		}
	}
	
	if (is_equipped && this->Owner && get_item_class_slot(this->Type->get_item_class()) != item_slot::none) {
		this->Owner->EquippedItems[static_cast<int>(get_item_class_slot(this->Type->get_item_class()))].push_back(this);
	}
}

}

std::string GetItemEffectsString(const std::string &item_ident)
{
	const stratagus::unit_type *item = stratagus::unit_type::get(item_ident);

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
			|| var == HITPOINTHEALING_INDEX || var == HITPOINTBONUS_INDEX || var == SIGHTRANGE_INDEX || var == DAYSIGHTRANGEBONUS_INDEX || var == NIGHTSIGHTRANGEBONUS_INDEX || var == HP_INDEX || var == MANA_INDEX
			|| var == ATTACKRANGE_INDEX)
		) {
			continue;
		}
					
		if (var != HP_INDEX) { //only for elixirs, equippable items use the hit point bonus variable instead
			if (item->DefaultStat.Variables[var].Enable) {
				if (!first_var) {
					item_effects_string += ", ";
				} else {
					first_var = false;
				}
										
				if (IsBooleanVariable(var) && item->DefaultStat.Variables[var].Value < 0) {
					item_effects_string += "Lose ";
				}
				
				if (!IsBooleanVariable(var)) {
					if (item->DefaultStat.Variables[var].Value >= 0 && var != HITPOINTHEALING_INDEX) {
						item_effects_string += "+";
					}
					item_effects_string += std::to_string((long long) item->DefaultStat.Variables[var].Value);
					if (IsPercentageVariable(var)) {
						item_effects_string += "%";
					}
					item_effects_string += " ";
				}
											
				item_effects_string += GetVariableDisplayName(var);
			}
			
			if (item->DefaultStat.Variables[var].Increase != 0) {
				if (!first_var) {
					item_effects_string += ", ";
				} else {
					first_var = false;
				}
											
				if (item->DefaultStat.Variables[var].Increase > 0) {
					item_effects_string += "+";
				}
				item_effects_string += std::to_string((long long) item->DefaultStat.Variables[var].Increase);
				item_effects_string += " ";
											
				item_effects_string += GetVariableDisplayName(var, true);
			}
		}
		
		if (item->Elixir) {
			for (const auto &modifier : item->Elixir->get_modifiers()) {
				if (modifier->Modifier.Variables[var].Value != 0) {
					if (!first_var) {
						item_effects_string += ", ";
					} else {
						first_var = false;
					}
											
					if (IsBooleanVariable(var) && modifier->Modifier.Variables[var].Value < 0) {
						item_effects_string += "Lose ";
					}
					
					if (!IsBooleanVariable(var)) {
						if (modifier->Modifier.Variables[var].Value >= 0 && var != HITPOINTHEALING_INDEX) {
							item_effects_string += "+";
						}
						item_effects_string += std::to_string(modifier->Modifier.Variables[var].Value);
						if (IsPercentageVariable(var)) {
							item_effects_string += "%";
						}
						item_effects_string += " ";
					}
												
					item_effects_string += GetVariableDisplayName(var);
				}
				
				if (modifier->Modifier.Variables[var].Increase != 0) {
					if (!first_var) {
						item_effects_string += ", ";
					} else {
						first_var = false;
					}
												
					if (modifier->Modifier.Variables[var].Increase > 0) {
						item_effects_string += "+";
					}
					item_effects_string += std::to_string(modifier->Modifier.Variables[var].Increase);
					item_effects_string += " ";
												
					item_effects_string += GetVariableDisplayName(var, true);
				}
			}
		}
	}
			
	return item_effects_string;
}
