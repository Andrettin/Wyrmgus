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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "item.h"

#include "character.h"
#include "config.h"
#include "game.h"
#include "network.h"
#include "parameters.h"
#include "player.h"
#include "spells.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unittype.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "util/string_util.h"

#include <cctype>
#include <map>
#include <string>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CUniqueItem *> UniqueItems;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

int GetItemSlotIdByName(const std::string &item_slot)
{
	if (item_slot == "weapon") {
		return WeaponItemSlot;
	} else if (item_slot == "shield") {
		return ShieldItemSlot;
	} else if (item_slot == "helmet") {
		return HelmetItemSlot;
	} else if (item_slot == "armor") {
		return ArmorItemSlot;
	} else if (item_slot == "gloves") {
		return GlovesItemSlot;
	} else if (item_slot == "boots") {
		return BootsItemSlot;
	} else if (item_slot == "belt") {
		return BeltItemSlot;
	} else if (item_slot == "amulet") {
		return AmuletItemSlot;
	} else if (item_slot == "ring") {
		return RingItemSlot;
	} else if (item_slot == "arrows") {
		return ArrowsItemSlot;
	}

	return -1;
}

std::string GetItemSlotNameById(int item_slot)
{
	if (item_slot == WeaponItemSlot) {
		return "weapon";
	} else if (item_slot == ShieldItemSlot) {
		return "shield";
	} else if (item_slot == HelmetItemSlot) {
		return "helmet";
	} else if (item_slot == ArmorItemSlot) {
		return "armor";
	} else if (item_slot == GlovesItemSlot) {
		return "gloves";
	} else if (item_slot == BootsItemSlot) {
		return "boots";
	} else if (item_slot == BeltItemSlot) {
		return "belt";
	} else if (item_slot == AmuletItemSlot) {
		return "amulet";
	} else if (item_slot == RingItemSlot) {
		return "ring";
	} else if (item_slot == ArrowsItemSlot) {
		return "arrows";
	}

	return "";
}

int GetItemClassIdByName(const std::string &item_class)
{
	if (item_class == "dagger") {
		return DaggerItemClass;
	} else if (item_class == "sword") {
		return SwordItemClass;
	} else if (item_class == "thrusting-sword") {
		return ThrustingSwordItemClass;
	} else if (item_class == "axe") {
		return AxeItemClass;
	} else if (item_class == "mace") {
		return MaceItemClass;
	} else if (item_class == "spear") {
		return SpearItemClass;
	} else if (item_class == "bow") {
		return BowItemClass;
	} else if (item_class == "throwing-axe") {
		return ThrowingAxeItemClass;
	} else if (item_class == "javelin") {
		return JavelinItemClass;
	} else if (item_class == "gun") {
		return GunItemClass;
	} else if (item_class == "shield") {
		return ShieldItemClass;
	} else if (item_class == "horn") {
		return HornItemClass;
	} else if (item_class == "helmet") {
		return HelmetItemClass;
	} else if (item_class == "armor") {
		return ArmorItemClass;
	} else if (item_class == "cloak") {
		return CloakItemClass;
	} else if (item_class == "gloves") {
		return GlovesItemClass;
	} else if (item_class == "belt") {
		return BeltItemClass;
	} else if (item_class == "boots") {
		return BootsItemClass;
	} else if (item_class == "amulet") {
		return AmuletItemClass;
	} else if (item_class == "ring") {
		return RingItemClass;
	} else if (item_class == "arrows") {
		return ArrowsItemClass;
	} else if (item_class == "food") {
		return FoodItemClass;
	} else if (item_class == "potion") {
		return PotionItemClass;
	} else if (item_class == "scroll") {
		return ScrollItemClass;
	} else if (item_class == "book") {
		return BookItemClass;
	}

	return -1;
}

std::string GetItemClassNameById(int item_class)
{
	if (item_class == DaggerItemClass) {
		return "dagger";
	} else if (item_class == SwordItemClass) {
		return "sword";
	} else if (item_class == ThrustingSwordItemClass) {
		return "thrusting-sword";
	} else if (item_class == AxeItemClass) {
		return "axe";
	} else if (item_class == MaceItemClass) {
		return "mace";
	} else if (item_class == SpearItemClass) {
		return "spear";
	} else if (item_class == BowItemClass) {
		return "bow";
	} else if (item_class == ThrowingAxeItemClass) {
		return "throwing-axe";
	} else if (item_class == JavelinItemClass) {
		return "javelin";
	} else if (item_class == GunItemClass) {
		return "gun";
	} else if (item_class == ShieldItemClass) {
		return "shield";
	} else if (item_class == HornItemClass) {
		return "horn";
	} else if (item_class == HelmetItemClass) {
		return "helmet";
	} else if (item_class == ArmorItemClass) {
		return "armor";
	} else if (item_class == CloakItemClass) {
		return "cloak";
	} else if (item_class == GlovesItemClass) {
		return "gloves";
	} else if (item_class == BeltItemClass) {
		return "belt";
	} else if (item_class == BootsItemClass) {
		return "boots";
	} else if (item_class == AmuletItemClass) {
		return "amulet";
	} else if (item_class == RingItemClass) {
		return "ring";
	} else if (item_class == ArrowsItemClass) {
		return "arrows";
	} else if (item_class == FoodItemClass) {
		return "food";
	} else if (item_class == PotionItemClass) {
		return "potion";
	} else if (item_class == ScrollItemClass) {
		return "scroll";
	} else if (item_class == BookItemClass) {
		return "book";
	}

	return "";
}

int GetItemClassSlot(int item_class)
{
	if (
		item_class == DaggerItemClass
		|| item_class == SwordItemClass
		|| item_class == ThrustingSwordItemClass
		|| item_class == AxeItemClass
		|| item_class == MaceItemClass
		|| item_class == SpearItemClass
		|| item_class == BowItemClass
		|| item_class == ThrowingAxeItemClass
		|| item_class == JavelinItemClass
		|| item_class == GunItemClass
	) {
		return WeaponItemSlot;
	} else if (item_class == ShieldItemClass || item_class == HornItemClass) {
		return ShieldItemSlot;
	} else if (item_class == HelmetItemClass) {
		return HelmetItemSlot;
	} else if (item_class == ArmorItemClass || item_class == CloakItemClass) {
		return ArmorItemSlot;
	} else if (item_class == GlovesItemClass) {
		return GlovesItemSlot;
	} else if (item_class == BootsItemClass) {
		return BootsItemSlot;
	} else if (item_class == BeltItemClass) {
		return BeltItemSlot;
	} else if (item_class == AmuletItemClass) {
		return AmuletItemSlot;
	} else if (item_class == RingItemClass) {
		return RingItemSlot;
	} else if (item_class == ArrowsItemClass) {
		return ArrowsItemSlot;
	}

	return -1;
}

bool IsItemClassConsumable(int item_class)
{
	if (item_class == FoodItemClass || item_class == PotionItemClass || item_class == ScrollItemClass || item_class == BookItemClass) {
		return true;
	}
	
	return false;
}

bool CUniqueItem::CanDrop() const
{
	// unique items cannot drop if a persistent hero owns them already, or if there's already one of them in the current scenario; unless it's a character-specific bound item, in which case it can still drop
	if (!IsNetworkGame()) {
		for (CCharacter *character : CCharacter::Characters) {
			for (CPersistentItem *item : character->Items) {
				if (item->Unique == this && !item->Bound) {
					return false;
				}
			}
		}
		
		for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
			for (CPersistentItem *item : iterator->second->Items) {
				if (item->Unique == this && !item->Bound) {
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

IconConfig CUniqueItem::GetIcon() const
{
	if (this->Icon.Icon) {
		return this->Icon;
	} else {
		return this->Type->Icon;
	}
}

int CUniqueItem::GetMagicLevel() const
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

void CleanUniqueItems()
{
	for (size_t i = 0; i < UniqueItems.size(); ++i) {
		delete UniqueItems[i];
	}
	UniqueItems.clear();
}

CUniqueItem *GetUniqueItem(const std::string &item_ident)
{
	for (size_t i = 0; i < UniqueItems.size(); ++i) {
		if (item_ident == UniqueItems[i]->Ident) {
			return UniqueItems[i];
		}
	}
	for (size_t i = 0; i < UniqueItems.size(); ++i) { // for backwards compatibility, search the name of the unique too
		if (NameToIdent(item_ident) == UniqueItems[i]->Ident) {
			return UniqueItems[i];
		}
	}
	return nullptr;
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CPersistentItem::ProcessConfigData(const CConfigData *config_data)
{
	bool is_equipped = false;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "type") {
			value = FindAndReplaceString(value, "_", "-");
			CUnitType *unit_type = UnitTypeByIdent(value);
			if (unit_type) {
				this->Type = unit_type;
			} else {
				fprintf(stderr, "Unit type \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "prefix") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Prefix = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "suffix") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Suffix = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "spell") {
			value = FindAndReplaceString(value, "_", "-");
			CSpell *spell = CSpell::GetSpell(value);
			if (spell) {
				this->Spell = spell;
			} else {
				fprintf(stderr, "Spell \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "work") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Work = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "elixir") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->Elixir = upgrade;
			} else {
				fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "unique") {
			value = FindAndReplaceString(value, "_", "-");
			CUniqueItem *unique_item = GetUniqueItem(value);
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
			this->Bound = string::to_bool(value);
		} else if (key == "identified") {
			this->Identified = string::to_bool(value);
		} else if (key == "equipped") {
			is_equipped = string::to_bool(value);
		} else {
			fprintf(stderr, "Invalid item property: \"%s\".\n", key.c_str());
		}
	}
	
	if (is_equipped && this->Owner && GetItemClassSlot(this->Type->ItemClass) != -1) {
		this->Owner->EquippedItems[GetItemClassSlot(this->Type->ItemClass)].push_back(this);
	}
}

std::string GetItemEffectsString(const std::string &item_ident)
{
	const CUnitType *item = UnitTypeByIdent(item_ident);

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

std::string GetUniqueItemEffectsString(const std::string &item_ident)
{
	const CUniqueItem *item = GetUniqueItem(item_ident);

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
				|| var == BONUSAGAINSTMOUNTED_INDEX|| var == BONUSAGAINSTBUILDINGS_INDEX || var == BONUSAGAINSTAIR_INDEX || var == BONUSAGAINSTGIANTS_INDEX || var == BONUSAGAINSTDRAGONS_INDEX
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
			
			for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
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
					|| var == BONUSAGAINSTMOUNTED_INDEX|| var == BONUSAGAINSTBUILDINGS_INDEX || var == BONUSAGAINSTAIR_INDEX || var == BONUSAGAINSTGIANTS_INDEX || var == BONUSAGAINSTDRAGONS_INDEX
					|| var == SUPPLY_INDEX
					|| var == ATTACKRANGE_INDEX)
				) {
					continue;
				}
				
				int variable_value = 0;
				int variable_increase = 0;

				for (size_t z = 0; z < item->Set->UpgradeModifiers.size(); ++z) {
					variable_value += item->Set->UpgradeModifiers[z]->Modifier.Variables[var].Value;
					variable_increase += item->Set->UpgradeModifiers[z]->Modifier.Variables[var].Increase;
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
