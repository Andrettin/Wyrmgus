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
/**@name depend.cpp - The dependencies source file */
//
//      (c) Copyright 2000-2018 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Pali Rohár and Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "depend.h"

#include "age.h"
#include "config.h"
#include "map/map_layer.h"
#include "player.h"
#include "religion/deity.h"
#include "script.h"
#include "time/season.h"
#include "translate.h"
#include "trigger.h"
#include "ui/button_action.h"
#include "ui/interface.h"
#include "unit.h"
#include "unittype.h"
#include "upgrade_structs.h"
#include "upgrade.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/// All dependencies map
static std::map<intptr_t, DependRule *> DependencyMap;

/// All predependencies map (predependencies are checked to see whether a button should be displayed at all)
static std::map<intptr_t, DependRule *> PredependencyMap;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
**	@param	rule_type	The rule type of the dependency
**	@param	target		The target of the dependency
*/
void DependRule::ProcessConfigData(const CConfigData *config_data, const int rule_type, const std::string &target)
{
	bool is_predependency = config_data->Tag == "predependency";
	
	bool first_rule_group = true;
	int or_flag = 0;
	
	for (size_t i = 0; i < config_data->Children.size(); ++i) {
		const CConfigData *child_config_data = config_data->Children[i];
		
		if (child_config_data->Tag == "rule_group") {
			if (first_rule_group) {
				first_rule_group = false;
			} else {
				or_flag = 1;
			}
			
			for (size_t j = 0; j < child_config_data->Children.size(); ++j) {
				const CConfigData *grandchild_config_data = child_config_data->Children[j];
				
				if (grandchild_config_data->Tag == "rule") {
					std::string requirement;
					int count = 1;
					int requirement_type = -1;
					
					for (size_t k = 0; k < grandchild_config_data->Properties.size(); ++k) {
						std::string key = grandchild_config_data->Properties[k].first;
						std::string value = grandchild_config_data->Properties[k].second;
						
						if (key == "requirement") {
							value = FindAndReplaceString(value, "_", "-");
							requirement = value;
						} else if (key == "requirement_type") {
							if (value == "unit_type") {
								requirement_type = DependRuleUnitType;
							} else if (value == "upgrade") {
								requirement_type = DependRuleUpgrade;
							} else if (value == "age") {
								requirement_type = DependRuleAge;
							} else if (value == "season") {
								requirement_type = DependRuleSeason;
							}
						} else if (key == "count") {
							count = std::stoi(value);
						} else {
							fprintf(stderr, "Invalid dependency rule property: \"%s\".\n", key.c_str());
						}
					}
					
					if (requirement.empty()) {
						fprintf(stderr, "Dependency rule has no \"requirement\" property.\n");
						continue;
					}
					
					if (requirement_type == -1) {
						fprintf(stderr, "Dependency rule has an invalid \"requirement_type\".\n");
						continue;
					}
					
					AddDependency(rule_type, target, requirement_type, requirement, count, or_flag, is_predependency);
					or_flag = 0;
				} else {
					fprintf(stderr, "Invalid dependency rule group property: \"%s\".\n", grandchild_config_data->Tag.c_str());
				}
			}
		} else {
			fprintf(stderr, "Invalid dependency property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

/**
**	@brief Get the key used for the dependency maps for a given rule
**
**	@param	rule	The rule
**
**	@return	The key for the dependency map
*/
static intptr_t GetDependencyKeyForRule(const DependRule &rule)
{
	intptr_t dependency_key;
	if (rule.Type == DependRuleUnitType) {
		dependency_key = (intptr_t) rule.Kind.UnitType;
	} else if (rule.Type == DependRuleUpgrade) {
		dependency_key = (intptr_t) rule.Kind.Upgrade;
	} else if (rule.Type == DependRuleAge) {
		dependency_key = (intptr_t) rule.Kind.Age;
	} else if (rule.Type == DependRuleSeason) {
		dependency_key = (intptr_t) rule.Kind.Season;
	} else {
		fprintf(stderr, "Invalid rule type for dependency rule: %d.\n", rule.Type);
		return (intptr_t) nullptr;
	}
	return dependency_key;
}

/**
**	@brief Get the dependency node for a given rule
**
**	@param	rule				The rule
**	@param	is_predependency	Whether the rule is a predependency
**
**	@return	The dependency node for the rule
*/
static DependRule *GetDependencyNodeForRule(const DependRule &rule, const bool is_predependency)
{
	intptr_t dependency_key = GetDependencyKeyForRule(rule);
	
	std::map<intptr_t, DependRule *> &dependency_map = is_predependency ? PredependencyMap : DependencyMap;
	
	if (dependency_map.find(dependency_key) != dependency_map.end()) {
		return dependency_map.find(dependency_key)->second;
	}
	
	return nullptr;
}

/**
**	@brief Add a new dependency. If it already exists append to "and" rule
**
**	@param	rule_type			The type of the rule (e.g. unit type or upgrade)
**	@param	target				Target of the dependency
**	@param	required_rule_type	The type of the rule of the requirement
**	@param	required			Requirement of the dependency
**	@param	count				Amount of the required needed
**	@param	or_flag				Start of or rule
**	@param	is_predependency	Whether the dependency is a predependency
*/
void AddDependency(const int rule_type, const std::string &target, const int required_rule_type, const std::string &required, int count, const int or_flag, const bool is_predependency)
{
	DependRule rule;

	//  Setup structure.
	rule.Type = rule_type;
	if (rule.Type == DependRuleUnitType) {
		// target string refers to unit-xxx
		rule.Kind.UnitType = UnitTypeByIdent(target);
		if (!rule.Kind.UnitType) {
			fprintf(stderr, "Unit type \"%s\" doesn't exist.\n", target.c_str());
		}
	} else if (rule.Type == DependRuleUpgrade) {
		// target string refers to upgrade-XXX
		rule.Kind.Upgrade = CUpgrade::Get(target);
		if (!rule.Kind.Upgrade) {
			fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", target.c_str());
		}
	} else if (rule.Type == DependRuleAge) {
		rule.Kind.Age = CAge::GetAge(target);
	} else if (rule.Type == DependRuleSeason) {
		rule.Kind.Season = CSeason::GetSeason(target);
	} else {
		fprintf(stderr, "Invalid rule type for dependency target \"%s\": %d.\n", target.c_str(), rule.Type);
		return;
	}

	//  Find correct dependency slot.
	DependRule *node = GetDependencyNodeForRule(rule, is_predependency);

	if (!node) { //create new entry if not found
		node = new DependRule;
		node->Next = nullptr;
		node->Rule = nullptr;
		node->Type = rule.Type;
		node->Kind = rule.Kind;
		intptr_t dependency_key = GetDependencyKeyForRule(rule);
		if (is_predependency) {
			PredependencyMap[dependency_key] = node;
		} else {
			DependencyMap[dependency_key] = node;
		}
	}

	//  Adjust count.
	if (count < 0 || count > 255) {
		DebugPrint("wrong count '%d' range 0 .. 255\n" _C_ count);
		count = 255;
	}

	DependRule *temp = new DependRule;
	temp->Rule = nullptr;
	temp->Next = nullptr;
	temp->Count = count;
	temp->Type = required_rule_type;
	
	if (temp->Type == -1) {
		if (!strncmp(required.c_str(), "unit-", 5)) {
		// required string refers to unit-xxx
			temp->Type = DependRuleUnitType;
		} else if (!strncmp(required.c_str(), "upgrade-", 8)) {
		// required string refers to upgrade-XXX
			temp->Type = DependRuleUpgrade;
		}
	}

	//  Setup structure.
	if (temp->Type == DependRuleUnitType) {
		temp->Kind.UnitType = UnitTypeByIdent(required);
		if (!temp->Kind.UnitType) {
			fprintf(stderr, "Unit type \"%s\" was set as a dependency for \"%s\", but it doesn't exist.\n", required.c_str(), target.c_str());
			fprintf(stderr, "Unit type \"%s\" doesn't exist.\n", required.c_str());
		}
	} else if (temp->Type == DependRuleUpgrade) {
		temp->Kind.Upgrade = CUpgrade::Get(required);
		if (!temp->Kind.Upgrade) {
			fprintf(stderr, "Upgrade \"%s\" was set as a dependency for \"%s\", but it doesn't exist.\n", required.c_str(), target.c_str());
		}
	} else if (temp->Type == DependRuleAge) {
		temp->Kind.Age = CAge::GetAge(required);
	} else if (temp->Type == DependRuleSeason) {
		temp->Kind.Season = CSeason::GetSeason(required);
	} else {
		fprintf(stderr, "Invalid rule type for dependency requirement \"%s\": %d\n", required.c_str(), temp->Type);
		delete temp;
		return;
	}

	if (or_flag) {
		// move rule to temp->next
		temp->Next = node->Rule;  // insert rule
		node->Rule = temp;
	} else {
		// move rule to temp->rule
		temp->Rule = node->Rule;  // insert rule

		// also Link temp to old "or" list
		if (node->Rule) {
			temp->Next = node->Rule->Next;
		}
		node->Rule = temp;
	}
}

/**
**  Check if this upgrade or unit is available.
**
**  @param player  For this player available.
**  @param rule  .
**
**  @return        True if available, false otherwise.
*/
static bool CheckDependByRule(const CPlayer &player, DependRule &rule, bool ignore_units, bool is_predependency)
{
	//if is making a normal dependency check, do a predependency check first
	if (!is_predependency && !CheckDependByRule(player, rule, ignore_units, true)) {
		return false;
	}
	
	//  Find rule
	const DependRule *node = GetDependencyNodeForRule(rule, is_predependency);

	if (!node) { //entry not found
		return true;
	}

	//  Prove the rules
	node = node->Rule;

	int i;
	while (node) {
		const DependRule *temp = node;
		while (temp) {
			switch (temp->Type) {
				case DependRuleUnitType:
					if (!ignore_units) {
						i = player.HaveUnitTypeByType(*temp->Kind.UnitType);
						if (temp->Count ? i < temp->Count : i) {
							goto try_or;
						}
					}
					break;
				case DependRuleUpgrade:
					i = UpgradeIdAllowed(player, temp->Kind.Upgrade->ID) != 'R';
					if (temp->Count ? i : !i) {
						goto try_or;
					}
					break;
				case DependRuleAge:
					i = player.Age != temp->Kind.Age;
					if (temp->Count ? i : !i) {
						goto try_or;
					}
					break;
				case DependRuleSeason:
					i = Map.MapLayers[player.StartMapLayer]->GetSeason() != temp->Kind.Season;
					if (temp->Count ? i : !i) {
						goto try_or;
					}
					break;
			}
			temp = temp->Rule;
		}
		return true;  // all rules match

try_or:
		node = node->Next;
	}
	return false;  // no rule matches
}

/**
**  Check if this upgrade or unit is available.
**
**  @param unit  For this unit available.
**  @param rule  .
**
**  @return        True if available, false otherwise.
*/
static bool CheckDependByRule(const CUnit &unit, DependRule &rule, bool ignore_units, bool is_predependency)
{
	//if is making a normal dependency check, do a predependency check first
	if (!is_predependency && !CheckDependByRule(unit, rule, ignore_units, true)) {
		return false;
	}
	
	//  Find rule
	const DependRule *node = GetDependencyNodeForRule(rule, is_predependency);
	
	if (!node) {  //entry not found
		return true;
	}

	//  Prove the rules
	node = node->Rule;

	int i;
	while (node) {
		const DependRule *temp = node;
		while (temp) {
			switch (temp->Type) {
				case DependRuleUnitType:
					if (!ignore_units) {
						i = unit.Player->HaveUnitTypeByType(*temp->Kind.UnitType);
						if (temp->Count ? i < temp->Count : i) {
							goto try_or;
						}
					}
					break;
				case DependRuleUpgrade:
					i = UpgradeIdAllowed(*unit.Player, temp->Kind.Upgrade->ID) != 'R' && !unit.GetIndividualUpgrade(temp->Kind.Upgrade);
					if (temp->Count ? i : !i) {
						goto try_or;
					}
					break;
				case DependRuleAge:
					i = unit.Player->Age != temp->Kind.Age;
					if (temp->Count ? i : !i) {
						goto try_or;
					}
					break;
				case DependRuleSeason:
					i = unit.MapLayer->GetSeason() != temp->Kind.Season;
					if (temp->Count ? i : !i) {
						goto try_or;
					}
					break;
			}
			temp = temp->Rule;
		}
		return true;  // all rules matches.

try_or:
		node = node->Next;
	}
	return false;  // no rule matches
}

/**
**  Check if this upgrade or unit is available.
**
**  @param player  For this player available.
**  @param target  Unit or Upgrade.
**
**  @return        True if available, false otherwise.
*/
std::string PrintDependencies(const CPlayer &player, const ButtonAction &button)
{
	DependRule rule;
	std::string rules("");

	//
	//  first have to check, if target is allowed itself
	//
	if (!strncmp(button.ValueStr.c_str(), "unit-", 5)) {
		// target string refers to unit-XXX
		rule.Kind.UnitType = UnitTypeByIdent(button.ValueStr);
		rule.Type = DependRuleUnitType;
	} else if (!strncmp(button.ValueStr.c_str(), "upgrade-", 8)) {
		// target string refers to upgrade-XXX
		rule.Kind.Upgrade = CUpgrade::Get(button.ValueStr);
		rule.Type = DependRuleUpgrade;
	} else if (!strncmp(button.ValueStr.c_str(), "spell-", 6)) {
		// Special case for spells
		if (button.Allowed && IsButtonAllowed(*Selected[0], button) == false) {
			if (!strncmp(button.AllowStr.c_str(), "upgrade-", 8)) {
				rules.insert(0, _("Requirements:\n"));
				rules.append("-");
				rules.append(AllUpgrades[UpgradeIdByIdent(button.AllowStr)]->Name);
				rules.append("\n");
			}
		}
		return rules;
	} else {
		DebugPrint("target '%s' should be unit-type or upgrade\n" _C_ button.ValueStr.c_str());
		return rules;
	}

	//  Find rule
	const DependRule *node = GetDependencyNodeForRule(rule, false);

	if (!node) {  //entry not found
		return rules;
	}

	//  Prove the rules
	node = node->Rule;

	int i;
	while (node) {
		const DependRule *temp = node;
		std::string subrules("");
		while (temp) {
			if (temp->Type == DependRuleUnitType) {
				i = player.HaveUnitTypeByType(*temp->Kind.UnitType);
				if (temp->Count ? i < temp->Count : i) {
					subrules.append("-");
					subrules.append(temp->Kind.UnitType->Name.c_str());
					subrules.append("\n");
				}
			} else if (temp->Type == DependRuleUpgrade) {
				i = UpgradeIdAllowed(player, temp->Kind.Upgrade->ID) != 'R';
				if (temp->Count ? i : !i) {
					subrules.append("-");
					subrules.append(temp->Kind.Upgrade->Name.c_str());
					subrules.append("\n");
				}
			}
			temp = temp->Rule;
		}
		if (subrules.empty()) {
			return subrules;
		}
		rules.clear();
		rules.append(subrules);
		node = node->Next;
	}
	rules.insert(0, _("Requirements:\n"));
	return rules;
}

/**
**	@brief	Check if this upgrade or unit is available.
**
**	@param	rule_type	The type of the rule (e.g. unit type or upgrade)
**	@param	player		For this player available.
**	@param	target		Unit or Upgrade.
**
**	@return	True if available, false otherwise.
*/
bool CheckDependByIdent(const CPlayer &player, const int rule_type, const std::string &target, bool ignore_units, bool is_predependency, bool is_neutral_use)
{
	DependRule rule;

	//
	//  first have to check, if target is allowed itself
	//
	rule.Type = rule_type;
	if (rule_type == DependRuleUnitType) {
		// target string refers to unit-XXX
		rule.Kind.UnitType = UnitTypeByIdent(target);
		
		if (!rule.Kind.UnitType) {
			fprintf(stderr, "Unit type \"%s\" doesn't exist.\n", target.c_str());
			return false;
		}
		
		if (UnitIdAllowed(player, rule.Kind.UnitType->Slot) == 0) {
			return false;
		}
	} else if (rule_type == DependRuleUpgrade) {
		// target string refers to upgrade-XXX
		rule.Kind.Upgrade = CUpgrade::Get(target);

		if (!rule.Kind.Upgrade) {
			fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", target.c_str());
			return false;
		}
		
		if (UpgradeIdAllowed(player, rule.Kind.Upgrade->ID) != 'A' && !((is_predependency || is_neutral_use) && UpgradeIdAllowed(player, rule.Kind.Upgrade->ID) == 'R')) {
			return false;
		}

		if (player.Faction != -1 && PlayerRaces.Factions[player.Faction]->Type == FactionTypeHolyOrder) { // if the player is a holy order, and the upgrade is incompatible with its deity, don't allow it
			if (PlayerRaces.Factions[player.Faction]->HolyOrderDeity) {
				CUpgrade *deity_upgrade = PlayerRaces.Factions[player.Faction]->HolyOrderDeity->DeityUpgrade;
				if (deity_upgrade) {
					for (size_t z = 0; z < rule.Kind.Upgrade->UpgradeModifiers.size(); ++z) {
						if (std::find(rule.Kind.Upgrade->UpgradeModifiers[z]->RemoveUpgrades.begin(), rule.Kind.Upgrade->UpgradeModifiers[z]->RemoveUpgrades.end(), deity_upgrade) != rule.Kind.Upgrade->UpgradeModifiers[z]->RemoveUpgrades.end()) {
							return false;
						}
					}
					for (size_t z = 0; z < deity_upgrade->UpgradeModifiers.size(); ++z) {
						if (std::find(deity_upgrade->UpgradeModifiers[z]->RemoveUpgrades.begin(), deity_upgrade->UpgradeModifiers[z]->RemoveUpgrades.end(), rule.Kind.Upgrade) != deity_upgrade->UpgradeModifiers[z]->RemoveUpgrades.end()) {
							return false;
						}
					}
				}
			}
		}
	} else if (rule.Type == DependRuleAge) {
		rule.Kind.Age = CAge::GetAge(target);
		
		if (!rule.Kind.Age) {
			return false;
		}
	} else if (rule.Type == DependRuleSeason) {
		rule.Kind.Season = CSeason::GetSeason(target);
		
		if (!rule.Kind.Season) {
			return false;
		}
	} else {
		DebugPrint("target '%s' should be unit-type or upgrade\n" _C_ target.c_str());
		return false;
	}

	return CheckDependByRule(player, rule, ignore_units, is_predependency);
}

/**
**  Check if this upgrade or unit is available for a particular unit.
**
**  @param unit	   Whether it is available for this unit.
**  @param target  Unit or Upgrade.
**
**  @return        True if available, false otherwise.
*/
bool CheckDependByIdent(const CUnit &unit, const int rule_type, const std::string &target, bool ignore_units, bool is_predependency)
{
	DependRule rule;

	rule.Type = rule_type;
	//
	//  first have to check, if target is allowed itself
	//
	if (rule.Type == DependRuleUnitType) {
		// target string refers to unit-XXX
		rule.Kind.UnitType = UnitTypeByIdent(target);
		
		if (!rule.Kind.UnitType) {
			fprintf(stderr, "Unit type \"%s\" doesn't exist.\n", target.c_str());
			return false;
		}
		
		if (UnitIdAllowed(*unit.Player, rule.Kind.UnitType->Slot) == 0) {
			return false;
		}
	} else if (rule.Type == DependRuleUpgrade) {
		rule.Kind.Upgrade = CUpgrade::Get(target);
		
		if (!rule.Kind.Upgrade) {
			fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", target.c_str());
			return false;
		}

		if (UpgradeIdAllowed(*unit.Player, rule.Kind.Upgrade->ID) == 'F') {
			return false;
		}
		rule.Type = DependRuleUpgrade;
	} else if (rule.Type == DependRuleAge) {
		rule.Kind.Age = CAge::GetAge(target);
		
		if (!rule.Kind.Age) {
			return false;
		}
	} else if (rule.Type == DependRuleSeason) {
		rule.Kind.Season = CSeason::GetSeason(target);
		
		if (!rule.Kind.Season) {
			return false;
		}
	} else {
		DebugPrint("target '%s' should be unit-type or upgrade\n" _C_ target.c_str());
		return false;
	}
	return CheckDependByRule(unit, rule, ignore_units, is_predependency);
}

/**
**  Check if this upgrade or unit is available.
**
**  @param player  For this player available.
**  @param target  Unit or Upgrade.
**
**  @return        True if available, false otherwise.
*/
bool CheckDependByType(const CPlayer &player, const CUnitType &type, bool ignore_units, bool is_predependency)
{
	if (UnitIdAllowed(player, type.Slot) == 0) {
		return false;
	}
	DependRule rule;

	rule.Kind.UnitType = &type;
	rule.Type = DependRuleUnitType;

	return CheckDependByRule(player, rule, ignore_units, is_predependency);
}

/**
**	@brief Check if this upgrade or unit is available.
**
**	@param	unit	For this unit available.
**	@param	target	Unit or Upgrade.
**
**	@return	True if available, false otherwise.
*/
bool CheckDependByType(const CUnit &unit, const CUnitType &type, bool ignore_units, bool is_predependency)
{
	if (UnitIdAllowed(*unit.Player, type.Slot) == 0) {
		return false;
	}
	DependRule rule;

	rule.Kind.UnitType = &type;
	rule.Type = DependRuleUnitType;
	return CheckDependByRule(unit, rule, ignore_units, is_predependency);
}

/**
**  Initialize unit and upgrade dependencies.
*/
void InitDependencies()
{
}

/**
**	@brief	Clean up unit and upgrade dependencies.
*/
void CleanDependencies()
{
	// Free all dependencies

	for (std::map<intptr_t, DependRule *>::iterator iterator = DependencyMap.begin(); iterator != DependencyMap.end(); ++iterator) {
		DependRule *node = iterator->second;
		while (node) {  // all hash links
			// All or cases

			DependRule *rule = node->Rule;
			while (rule) {
				if (rule) {
					DependRule *temp = rule->Rule;
					while (temp) {
						DependRule *next = temp;
						temp = temp->Rule;
						delete next;
					}
				}
				DependRule *temp = rule;
				rule = rule->Next;
				delete temp;
			}
			DependRule *temp = node;
			node = node->Next;
			delete temp;
		}
	}
	DependencyMap.clear();
	
	for (std::map<intptr_t, DependRule *>::iterator iterator = PredependencyMap.begin(); iterator != PredependencyMap.end(); ++iterator) {
		DependRule *node = iterator->second;
		while (node) {  // all hash links
			// All or cases

			DependRule *rule = node->Rule;
			while (rule) {
				if (rule) {
					DependRule *temp = rule->Rule;
					while (temp) {
						DependRule *next = temp;
						temp = temp->Rule;
						delete next;
					}
				}
				DependRule *temp = rule;
				rule = rule->Next;
				delete temp;
			}
			DependRule *temp = node;
			node = node->Next;
			delete temp;
		}
	}
	PredependencyMap.clear();
}

/*----------------------------------------------------------------------------
--  Ccl part of dependencies
----------------------------------------------------------------------------*/

/**
**  Define a new dependency.
**
**  @param l  Lua state.
*/
static int CclDefineDependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	//  All or rules.
	int or_flag = 0;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		for (int k = 0; k < subargs; ++k) {
			const char *required = LuaToString(l, j + 1, k + 1);
			int count = 1;
			if (k + 1 < subargs) {
				lua_rawgeti(l, j + 1, k + 2);
				if (lua_isnumber(l, -1)) {
					count = LuaToNumber(l, -1);
					++k;
				}
				lua_pop(l, 1);
			}
			AddDependency(!strncmp(target, "unit-", 5) ? DependRuleUnitType : DependRuleUpgrade, target, -1, required, count, or_flag, false);
			or_flag = 0;
		}
		if (j + 1 < args) {
			++j;
			const char *value = LuaToString(l, j + 1);
			if (strcmp(value, "or")) {
				LuaError(l, "not or symbol: %s" _C_ value);
				return 0;
			}
			or_flag = 1;
		}
	}
	return 0;
}

static int CclDefinePredependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	//  All or rules.
	int or_flag = 0;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		for (int k = 0; k < subargs; ++k) {
			const char *required = LuaToString(l, j + 1, k + 1);
			int count = 1;
			if (k + 1 < subargs) {
				lua_rawgeti(l, j + 1, k + 2);
				if (lua_isnumber(l, -1)) {
					count = LuaToNumber(l, -1);
					++k;
				}
				lua_pop(l, 1);
			}
			AddDependency(!strncmp(target, "unit-", 5) ? DependRuleUnitType : DependRuleUpgrade, target, -1, required, count, or_flag, true);
			or_flag = 0;
		}
		if (j + 1 < args) {
			++j;
			const char *value = LuaToString(l, j + 1);
			if (strcmp(value, "or")) {
				LuaError(l, "not or symbol: %s" _C_ value);
				return 0;
			}
			or_flag = 1;
		}
	}
	return 0;
}

/**
**  Get the dependency.
**
**  @todo not written.
**
**  @param l  Lua state.
*/
static int CclGetDependency(lua_State *l)
{
	DebugPrint("FIXME: write this %p\n" _C_(void *)l);

	return 0;
}

/**
**  Checks if dependencies are met.
**
**  @return true if the dependencies are met.
**
**  @param l  Lua state.
**  Argument 1: player
**  Argument 2: object which we want to check the dependencies of
*/
static int CclCheckDependency(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const char *object = LuaToString(l, 2);
	lua_pop(l, 1);
	const int plynr = TriggerGetPlayer(l);
	if (plynr == -1) {
		LuaError(l, "bad player: %i" _C_ plynr);
	}
	CPlayer &player = Players[plynr];

	lua_pushboolean(l, CheckDependByIdent(player, !strncmp(object, "unit-", 5) ? DependRuleUnitType : DependRuleUpgrade, object));
	return 1;
}

/**
**  Register CCL features for dependencies.
*/
void DependenciesCclRegister()
{
	lua_register(Lua, "DefineDependency", CclDefineDependency);
	//Wyrmgus start
	lua_register(Lua, "DefinePredependency", CclDefinePredependency);
	//Wyrmgus end
	lua_register(Lua, "GetDependency", CclGetDependency);
	lua_register(Lua, "CheckDependency", CclCheckDependency);
}

//@}
