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
/**@name item.cpp - The item source file. */
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

#include "item/item.h"

#include "character.h"
#include "config.h"
#include "config_operator.h"
#include "game/game.h"
#include "item/item_class.h"
#include "item/unique_item.h"
#include "parameters.h"
#include "spell/spells.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"

#include <ctype.h>

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CPersistentItem::ProcessConfigData(const CConfigData *config_data)
{
	bool is_equipped = false;
	
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		std::string key = property.Key;
		std::string value = property.Value;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "type") {
			const CUnitType *unit_type = CUnitType::Get(value);
			if (unit_type != nullptr) {
				this->Type = unit_type;
			}
		} else if (key == "prefix") {
			value = FindAndReplaceString(value, "_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Prefix = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "suffix") {
			value = FindAndReplaceString(value, "_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Suffix = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "spell") {
			value = FindAndReplaceString(value, "_", "-");
			const CSpell *spell = CSpell::GetSpell(value);
			if (spell) {
				this->Spell = spell;
			} else {
				fprintf(stderr, "Spell \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "work") {
			value = FindAndReplaceString(value, "_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Work = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "elixir") {
			value = FindAndReplaceString(value, "_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Elixir = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "unique") {
			value = FindAndReplaceString(value, "_", "-");
			UniqueItem *unique_item = UniqueItem::Get(value);
			if (unique_item) {
				this->Unique = unique_item;
				this->Name = unique_item->Name;
				if (unique_item->Type != nullptr) {
					this->Type = unique_item->Type;
				} else {
					fprintf(stderr, "Unique item \"%s\" has no type.\n", unique_item->Ident.c_str());
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
			this->Bound = StringToBool(value);
		} else if (key == "identified") {
			this->Identified = StringToBool(value);
		} else if (key == "equipped") {
			is_equipped = StringToBool(value);
		} else {
			fprintf(stderr, "Invalid item property: \"%s\".\n", key.c_str());
		}
	}
	
	if (is_equipped && this->Owner && this->Type->ItemClass->Slot != nullptr) {
		this->Owner->EquippedItems[this->Type->ItemClass->Slot].push_back(this);
	}
}

std::string GetItemEffectsString(const std::string &item_ident)
{
	const CUnitType *item = CUnitType::Get(item_ident);

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
				for (size_t z = 0; z < item->Elixir->UpgradeModifiers.size(); ++z) {
					if (item->Elixir->UpgradeModifiers[z]->Modifier.Variables[var].Value != 0) {
						if (!first_var) {
							item_effects_string += ", ";
						} else {
							first_var = false;
						}
												
						if (IsBooleanVariable(var) && item->Elixir->UpgradeModifiers[z]->Modifier.Variables[var].Value < 0) {
							item_effects_string += "Lose ";
						}
						
						if (!IsBooleanVariable(var)) {
							if (item->Elixir->UpgradeModifiers[z]->Modifier.Variables[var].Value >= 0 && var != HITPOINTHEALING_INDEX) {
								item_effects_string += "+";
							}
							item_effects_string += std::to_string((long long) item->Elixir->UpgradeModifiers[z]->Modifier.Variables[var].Value);
							if (IsPercentageVariable(var)) {
								item_effects_string += "%";
							}
							item_effects_string += " ";
						}
													
						item_effects_string += GetVariableDisplayName(var);
					}
					
					if (item->Elixir->UpgradeModifiers[z]->Modifier.Variables[var].Increase != 0) {
						if (!first_var) {
							item_effects_string += ", ";
						} else {
							first_var = false;
						}
													
						if (item->Elixir->UpgradeModifiers[z]->Modifier.Variables[var].Increase > 0) {
							item_effects_string += "+";
						}
						item_effects_string += std::to_string((long long) item->Elixir->UpgradeModifiers[z]->Modifier.Variables[var].Increase);
						item_effects_string += " ";
													
						item_effects_string += GetVariableDisplayName(var, true);
					}
				}
			}
		}
			
		return item_effects_string;
	}
	
	return "";
}
