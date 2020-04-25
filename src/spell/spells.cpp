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
/**@name spells.cpp - The spell cast action. */
//
//      (c) Copyright 1998-2020 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Joris Dauphin and Andrettin
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

/*
** And when we cast our final spell
** And we meet in our dreams
** A place that no one else can go
** Don't ever let your love die
** Don't ever go breaking this spell
*/

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "spells.h"

#include "actions.h"
#include "civilization.h"
#include "commands.h"
#include "config.h"
#include "faction.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "sound/sound.h"
#include "spell/spell_adjustvariable.h"
#include "spell/spell_spawnmissile.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "upgrade/upgrade.h"
#include "util/string_util.h"

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

/**
** Define the names and effects of all available spells in the game
*/
std::vector<CSpell *> CSpell::Spells;
std::map<std::string, CSpell *> CSpell::SpellsByIdent;


/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

// ****************************************************************************
// Target constructor
// ****************************************************************************

/**
**  Target constructor for unit.
**
**  @param unit  Target unit.
**
**  @return the new target.
*/
static Target *NewTargetUnit(CUnit &unit)
{
	return new Target(TargetType::Unit, &unit, unit.tilePos, unit.MapLayer->ID);
}

// ****************************************************************************
// Main local functions
// ****************************************************************************

/**
**	@brief	Check the condition.
**
**	@param	caster		Pointer to caster unit.
**	@param	spell		Pointer to the spell to cast.
**	@param	target		Pointer to target unit, or 0 if it is a position spell.
**	@param	goalPos		Position, or {-1, -1} if it is a unit spell.
**	@param	condition	Pointer to condition info.
**	@param	map_layer	Map layer, or null if it is a unit spell.
**
**	@return	True if passed, or false otherwise.
*/
static bool PassCondition(const CUnit &caster, const CSpell &spell, const CUnit *target,
						  const Vec2i &goalPos, const ConditionInfo *condition, const CMapLayer *map_layer)
{
	if (caster.Variable[MANA_INDEX].Value < spell.ManaCost) { // Check caster mana.
		return false;
	}
	// check countdown timer
	if (caster.SpellCoolDownTimers[spell.Slot]) { // Check caster mana.
		return false;
	}
	// Check caster's resources
	if (caster.Player->CheckCosts(spell.Costs, false)) {
		return false;
	}
	if (spell.Target == TargetType::Unit) { // Casting a unit spell without a target.
		if ((!target) || target->IsAlive() == false) {
			return false;
		}
	}
	if (!condition) { // no condition, pass.
		return true;
	}
	
	if (target && !target->Type->CheckUserBoolFlags(condition->BoolFlag)) {
		return false;
	}

	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) { // for custom variables
		const CUnit *unit;

		if (!condition->Variable[i].Check) {
			continue;
		}

		unit = (condition->Variable[i].ConditionApplyOnCaster) ? &caster : target;
		//  Spell should target location and have unit condition.
		if (unit == nullptr) {
			continue;
		}
		if (condition->Variable[i].Enable != CONDITION_TRUE) {
			if ((condition->Variable[i].Enable == CONDITION_ONLY) ^ (unit->Variable[i].Enable)) {
				return false;
			}
		}
		// Value and Max
		if (condition->Variable[i].ExactValue != -1 &&
			condition->Variable[i].ExactValue != unit->Variable[i].Value) {
			return false;
		}
		if (condition->Variable[i].ExceptValue != -1 &&
			condition->Variable[i].ExceptValue == unit->Variable[i].Value) {
			return false;
		}
		if (condition->Variable[i].MinValue >= unit->Variable[i].Value) {
			return false;
		}
		if (condition->Variable[i].MaxValue != -1 &&
			//Wyrmgus start
//			condition->Variable[i].MaxValue <= unit->Variable[i].Value) {
			condition->Variable[i].MaxValue <= unit->GetModifiedVariable(i, VariableValue)) {
			//Wyrmgus end
			return false;
		}

		//Wyrmgus start
//		if (condition->Variable[i].MinMax >= unit->Variable[i].Max) {
		if (condition->Variable[i].MinMax >= unit->GetModifiedVariable(i, VariableMax)) {
		//Wyrmgus end
			return false;
		}

		//Wyrmgus start
//		if (!unit->Variable[i].Max) {
		if (!unit->GetModifiedVariable(i, VariableMax)) {
		//Wyrmgus end
			continue;
		}
		// Percent
		//Wyrmgus start
//		if (condition->Variable[i].MinValuePercent * unit->Variable[i].Max
		if (condition->Variable[i].MinValuePercent * unit->GetModifiedVariable(i, VariableMax)
		//Wyrmgus end
			>= 100 * unit->Variable[i].Value) {
			return false;
		}
		//Wyrmgus start
//		if (condition->Variable[i].MaxValuePercent * unit->Variable[i].Max
		if (condition->Variable[i].MaxValuePercent * unit->GetModifiedVariable(i, VariableMax)
		//Wyrmgus end
			<= 100 * unit->Variable[i].Value) {
			return false;
		}
	}

	if (condition->CheckFunc) {
		condition->CheckFunc->pushPreamble();
		condition->CheckFunc->pushString(spell.Ident);
		condition->CheckFunc->pushInteger(UnitNumber(caster));
		condition->CheckFunc->pushInteger(goalPos.x);
		condition->CheckFunc->pushInteger(goalPos.y);
		condition->CheckFunc->pushInteger((target && target->IsAlive()) ? UnitNumber(*target) : -1);
		condition->CheckFunc->run(1);
		if (condition->CheckFunc->popBoolean() == false) {
			return false;
		}
	}
	if (!target) {
		return true;
	}

	if (condition->Alliance != CONDITION_TRUE) {
		if ((condition->Alliance == CONDITION_ONLY) ^
			// own units could be not allied ?
			(caster.IsAllied(*target) || target->Player == caster.Player)) {
			return false;
		}
	}
	if (condition->Opponent != CONDITION_TRUE) {
		if ((condition->Opponent == CONDITION_ONLY) ^
			(caster.IsEnemy(*target) && 1)) {
			return false;
		}
	}
	if (condition->TargetSelf != CONDITION_TRUE) {
		if ((condition->TargetSelf == CONDITION_ONLY) ^ (&caster == target)) {
			return false;
		}
	}
	//Wyrmgus start
	if (condition->ThrustingWeapon != CONDITION_TRUE) {
		if ((condition->ThrustingWeapon == CONDITION_ONLY) ^ (caster.GetCurrentWeaponClass() == DaggerItemClass || caster.GetCurrentWeaponClass() == SwordItemClass || caster.GetCurrentWeaponClass() == ThrustingSwordItemClass || caster.GetCurrentWeaponClass() == SpearItemClass)) {
			return false;
		}
	}
	if (condition->FactionUnit != CONDITION_TRUE) {
		if ((condition->FactionUnit == CONDITION_ONLY) ^ (caster.Type->Faction != -1)) {
			return false;
		}
	}
	if (condition->civilization_equivalent != -1) {
		if (caster.Type->civilization == -1 || (caster.Type->civilization == condition->civilization_equivalent && (!caster.Character || (caster.Character->civilization && caster.Character->civilization->ID == condition->civilization_equivalent))) || PlayerRaces.Species[caster.Type->civilization] != PlayerRaces.Species[condition->civilization_equivalent] || stratagus::civilization::get_all()[condition->civilization_equivalent]->get_class_unit_type(caster.Type->get_unit_class()) == nullptr || (caster.Character && !caster.Character->Custom)) {
			return false;
		}
	}
	if (condition->FactionEquivalent != nullptr) {
		if (caster.Type->civilization == -1 || caster.Type->civilization != condition->FactionEquivalent->civilization->ID || condition->FactionEquivalent->get_class_unit_type(caster.Type->get_unit_class()) == nullptr|| (caster.Character && !caster.Character->Custom)) {
			return false;
		}
	}
	//Wyrmgus end
	return true;
}

class AutoCastPrioritySort
{
public:
	explicit AutoCastPrioritySort(const CUnit &caster, const int var, const bool reverse) :
		caster(caster), variable(var), reverse(reverse) {}
	bool operator()(const CUnit *lhs, const CUnit *rhs) const
	{
		if (variable == ACP_DISTANCE) {
			if (reverse) {
				return lhs->MapDistanceTo(caster) > rhs->MapDistanceTo(caster);
			} else {
				return lhs->MapDistanceTo(caster) < rhs->MapDistanceTo(caster);
			}
		} else {
			if (reverse) {
				return lhs->Variable[variable].Value > rhs->Variable[variable].Value;
			} else {
				return lhs->Variable[variable].Value < rhs->Variable[variable].Value;
			}
		}
	}
private:
	const CUnit &caster;
	const int variable;
	const bool reverse;
};

/**
**	@brief	Spell constructor.
*/
CSpell::CSpell(int slot, const std::string &ident) :
	Slot(slot), Target(), Action(),
	Range(0), ManaCost(0), RepeatCast(0), Stackable(true), CoolDown(0),
	DependencyId(-1), Condition(nullptr),
	AutoCast(nullptr), AICast(nullptr), ForceUseAnimation(false)
{
	this->Ident = ident;

	memset(Costs, 0, sizeof(Costs));
	//Wyrmgus start
	memset(ItemSpell, 0, sizeof(ItemSpell));
	//Wyrmgus end
}

/**
**	@brief	Spell destructor.
*/
CSpell::~CSpell()
{
	for (std::vector<SpellActionType *>::iterator act = Action.begin(); act != Action.end(); ++act) {
		delete *act;
	}
	Action.clear();

	delete Condition;
	//
	// Free Autocast.
	//
	delete AutoCast;
	delete AICast;
}

/**
**	@brief	Get a spell
**
**	@param	ident	The spell's string identifier
**
**	@return	The spell if found, or null otherwise
*/
CSpell *CSpell::GetSpell(const std::string &ident, const bool should_find)
{
	std::map<std::string, CSpell *>::const_iterator find_iterator = SpellsByIdent.find(ident);
	
	if (find_iterator != SpellsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid spell: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a spell
**
**	@param	ident	The spell's string identifier
**
**	@return	The spell if found, otherwise a new spell is created and returned
*/
CSpell *CSpell::GetOrAddSpell(const std::string &ident)
{
	CSpell *spell = GetSpell(ident, false);
	
	if (!spell) {
		spell = new CSpell(Spells.size(), ident);
		for (CUnitType *unit_type : CUnitType::get_all()) { // adjust array for casters that have already been defined
			if (unit_type->AutoCastActive) {
				char *newc = new char[(Spells.size() + 1) * sizeof(char)];
				memcpy(newc, unit_type->AutoCastActive, Spells.size() * sizeof(char));
				delete[] unit_type->AutoCastActive;
				unit_type->AutoCastActive = newc;
				unit_type->AutoCastActive[Spells.size()] = 0;
			}
		}
		Spells.push_back(spell);
		SpellsByIdent[ident] = spell;
	}
	
	return spell;
}

/**
**	@brief	Remove the existing spells
*/
void CSpell::ClearSpells()
{
	DebugPrint("Cleaning spells.\n");
	for (size_t i = 0; i < Spells.size(); ++i) {
		delete Spells[i];
	}
	Spells.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CSpell::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "mana_cost") {
			this->ManaCost = std::stoi(value);
		} else if (key == "cooldown") {
			this->CoolDown = std::stoi(value);
		} else if (key == "range") {
			if (value == "infinite") {
				this->Range = INFINITE_RANGE;
			} else {
				this->Range = std::stoi(value);
			}
		} else if (key == "repeat_cast") {
			this->RepeatCast = string::to_bool(value);
		} else if (key == "stackable") {
			this->Stackable = string::to_bool(value);
		} else if (key == "force_use_animation") {
			this->ForceUseAnimation = string::to_bool(value);
		} else if (key == "target") {
			if (value == "self") {
				this->Target = TargetType::Self;
			} else if (value == "unit") {
				this->Target = TargetType::Unit;
			} else if (value == "position") {
				this->Target = TargetType::Position;
			} else {
				fprintf(stderr, "Invalid spell target type: \"%s\".\n", value.c_str());
			}
		} else if (key == "sound_when_cast") {
			value = FindAndReplaceString(value, "_", "-");
			this->SoundWhenCast.Name = value;
			this->SoundWhenCast.MapSound();
			
			//check the sound
			if (!this->SoundWhenCast.Sound) {
				this->SoundWhenCast.Name.clear();
			}
		} else if (key == "depend_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			this->DependencyId = UpgradeIdByIdent(value);
			if (this->DependencyId == -1) {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
			}
		} else if (key == "item_spell") {
			value = FindAndReplaceString(value, "_", "-");
			const int item_class = GetItemClassIdByName(value);
			if (item_class != -1) {
				this->ItemSpell[item_class] = true;
			}
		} else {
			fprintf(stderr, "Invalid spell property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "actions") {
			for (const CConfigData *grandchild_config_data : child_config_data->Children) {
				SpellActionType *spell_action = nullptr;
				
				if (grandchild_config_data->Tag == "adjust_variable") {
					spell_action = new Spell_AdjustVariable;
				} else if (grandchild_config_data->Tag == "spawn_missile") {
					spell_action = new Spell_SpawnMissile;
				} else {
					fprintf(stderr, "Invalid spell action type: \"%s\".\n", grandchild_config_data->Tag.c_str());
				}
				
				spell_action->ProcessConfigData(grandchild_config_data);
				this->Action.push_back(spell_action);
			}
		} else if (child_config_data->Tag == "condition") {
			if (!this->Condition) {
				this->Condition = new ConditionInfo;
			}
			this->Condition->ProcessConfigData(child_config_data);
		} else if (child_config_data->Tag == "autocast") {
			if (!this->AutoCast) {
				this->AutoCast = new AutoCastInfo();
			}
			this->AutoCast->ProcessConfigData(child_config_data);
		} else if (child_config_data->Tag == "ai_cast") {
			if (!this->AICast) {
				this->AICast = new AutoCastInfo();
			}
			this->AICast->ProcessConfigData(child_config_data);
		} else if (child_config_data->Tag == "resource_cost") {
			int resource = -1;
			int cost = 0;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "resource") {
					value = FindAndReplaceString(value, "_", "-");
					resource = GetResourceIdByName(value.c_str());
					if (resource == -1) {
						fprintf(stderr, "Invalid resource: \"%s\".\n", value.c_str());
					}
				} else if (key == "cost") {
					cost = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid resource cost property: \"%s\".\n", key.c_str());
				}
			}
			
			if (resource == -1) {
				fprintf(stderr, "Resource cost has no resource.\n");
				continue;
			}
			
			if (cost == 0) {
				fprintf(stderr, "Resource cost has no valid cost amount.\n");
				continue;
			}
			
			this->Costs[resource] = cost;
		} else {
			fprintf(stderr, "Invalid spell property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

/**
**	@brief	Get the autocast info for the spell
**
**	@param	ai	Whether the spell would be cast by the AI
**
**	@return	The autocast info for the spell if present, or null otherwise
*/
const AutoCastInfo *CSpell::GetAutoCastInfo(const bool ai) const
{
	if (ai && this->AICast) {
		return this->AICast;
	} else {
		return this->AutoCast;
	}
}

/**
**	@brief	Check whether the given caster passes the spell's generic autocast conditions
**
**	@param	caster		The caster
**	@param	autocast	The autocast information for the spell
**
**	@return	True if the generic conditions to autocast the spell are fulfilled, or false otherwise
*/
bool CSpell::CheckAutoCastGenericConditions(const CUnit &caster, const AutoCastInfo *autocast, const bool ignore_combat_status) const
{
	if (!autocast) {
		return false;
	}
	
	if (!ignore_combat_status && autocast->Combat != CONDITION_TRUE) {
		if ((autocast->Combat == CONDITION_ONLY) ^ (caster.IsInCombat())) {
			return false;
		}
	}
	
	return true;
}

/**
**	@brief	Get whether the given unit is a valid autocast target
**
**	@param	target			The potential target for the spell
**	@param	caster			The caster for the spell
**	@param	autocast		The autocast information for the spell
**	@param	max_path_length	The maximum length the caster may move to the target; 0 by default, which means any length is accepted
**
**	@return	True if the generic conditions to autocast the spell are fulfilled, or false otherwise
*/
bool CSpell::IsUnitValidAutoCastTarget(const CUnit *target, const CUnit &caster, const AutoCastInfo *autocast, const int max_path_length) const
{
	if (!target || !autocast) {
		return false;
	}
	
	// Check if unit is in battle
	if (this->Target == TargetType::Unit) {
		if (autocast->Attacker == CONDITION_ONLY) {
			const int react_range = target->GetReactionRange();
			if (
				(
					target->CurrentAction() != UnitAction::Attack
					&& target->CurrentAction() != UnitAction::AttackGround
					&& target->CurrentAction() != UnitAction::SpellCast
				)
				|| target->CurrentOrder()->HasGoal() == false
				|| target->MapDistanceTo(target->CurrentOrder()->GetGoalPos(), target->CurrentOrder()->GetGoalMapLayer()) > react_range
			) {
				return false;
			}
		}
	}
	
	// Check for corpse
	if (autocast->Corpse == CONDITION_ONLY) {
		if (target->CurrentAction() != UnitAction::Die) {
			return false;
		}
	} else if (autocast->Corpse == CONDITION_FALSE) {
		if (target->CurrentAction() == UnitAction::Die || target->IsAlive() == false) {
			return false;
		}
	}
	
	//Wyrmgus start
	if (this->Target == TargetType::Unit) {
		//if caster is terrified, don't target enemy units
		if (caster.Variable[TERROR_INDEX].Value > 0 && caster.IsEnemy(*target)) {
			return false;
		}
	}
	//Wyrmgus end
	
	if (!PassCondition(caster, *this, target, caster.tilePos, this->Condition, target->MapLayer) || !PassCondition(caster, *this, target, caster.tilePos, autocast->Condition, target->MapLayer)) {
		return false;
	}

	//Wyrmgus start
	int range = this->Range;
	if (!CheckObstaclesBetweenTiles(caster.tilePos, target->tilePos, MapFieldAirUnpassable, target->MapLayer->ID)) {
		range = 1; //if there are e.g. dungeon walls between the caster and the target, the unit reachable check must see if the target is reachable with a range of 1 instead of the spell's normal range (to make sure the spell can be cast; spells can't be cast through dungeon walls)
	}

	//pathfinding is expensive performance-wise, so we leave this check for last
	if (!UnitReachable(caster, *target, range, max_path_length)) {
		return false;
	}
	//Wyrmgus end

	return true;
}

/**
**	@brief	Check whether the given caster passes the spell's generic autocast conditions
**
**	@param	caster		The caster
**	@param	autocast	The autocast information for the spell
**
**	@return	True if the generic conditions to autocast the spell are fulfilled, or false otherwise
*/
std::vector<CUnit *> CSpell::GetPotentialAutoCastTargets(const CUnit &caster, const AutoCastInfo *autocast) const
{
	std::vector<CUnit *> potential_targets;
	
	if (!autocast) {
		return potential_targets;
	}
	
	int range = autocast->Range;
	int min_range = autocast->MinRange;

	if (caster.CurrentAction() == UnitAction::StandGround) {
		range = std::min(range, this->Range);
	}
	
	//select all units around the caster
	SelectAroundUnit(caster, range, potential_targets, OutOfMinRange(min_range, caster.tilePos, caster.MapLayer->ID));
	
	//check each unit to see if it is a possible target
	int n = 0;
	for (size_t i = 0; i != potential_targets.size(); ++i) {
		if (this->IsUnitValidAutoCastTarget(potential_targets[i], caster, autocast, caster.GetReactionRange() * 8)) {
			potential_targets[n++] = potential_targets[i];
		}
	}
	
	potential_targets.resize(n);

	return potential_targets;
}

/**
**	@brief	Select the target for the autocast.
**
**	@param	caster	Unit who would cast the spell.
**	@param	spell	Spell-type pointer.
**
**	@return	Target* chosen target or Null if spell can't be cast.
**	@todo FIXME: should be global (for AI) ???
**	@todo FIXME: write for position target.
*/
static Target *SelectTargetUnitsOfAutoCast(CUnit &caster, const CSpell &spell)
{
	const AutoCastInfo *autocast = spell.GetAutoCastInfo(caster.Player->AiEnabled);
	Assert(autocast);
	
	if (!spell.CheckAutoCastGenericConditions(caster, autocast)) {
		return nullptr;
	}
	
	const Vec2i &pos = caster.tilePos;
	CMapLayer *map_layer = caster.MapLayer;
	int range = autocast->Range;
	int minRange = autocast->MinRange;

	if (caster.CurrentAction() == UnitAction::StandGround) {
		range = std::min(range, spell.Range);
	}

	// Select all units around the caster
	std::vector<CUnit *> table;
	SelectAroundUnit(caster, range, table, OutOfMinRange(minRange, caster.tilePos, caster.MapLayer->ID));

	if (spell.Target == TargetType::Self) {
		if (PassCondition(caster, spell, &caster, caster.tilePos, spell.Condition, map_layer) && PassCondition(caster, spell, &caster, caster.tilePos, autocast->Condition, map_layer)) {
			return NewTargetUnit(caster);
		}
	} else if (spell.Target == TargetType::Position) {
		if (!autocast->PositionAutoCast) {
			return nullptr;
		}
		
		std::vector<CUnit *> table = spell.GetPotentialAutoCastTargets(caster, autocast);
		
		if (!table.empty()) {
			if (autocast->PriorityVar != ACP_NOVALUE) {
				std::sort(table.begin(), table.end(), AutoCastPrioritySort(caster, autocast->PriorityVar, autocast->ReverseSort));
			}
			std::vector<int> array(table.size() + 1);
			for (size_t i = 1; i < array.size(); ++i) {
				array[i] = UnitNumber(*table[i - 1]);
			}
			array[0] = UnitNumber(caster);
			autocast->PositionAutoCast->pushPreamble();
			autocast->PositionAutoCast->pushIntegers(array);
			autocast->PositionAutoCast->run(2);
			Vec2i resPos(autocast->PositionAutoCast->popInteger(), autocast->PositionAutoCast->popInteger());
			if (CMap::Map.Info.IsPointOnMap(resPos, map_layer)) {
				Target *target = new Target(TargetType::Position, nullptr, resPos, map_layer->ID);
				return target;
			}
		}
	} else if (spell.Target == TargetType::Unit) {
		std::vector<CUnit *> table = spell.GetPotentialAutoCastTargets(caster, autocast);
		//now select the best unit to target.
		if (!table.empty()) {
			// For the best target???
			if (autocast->PriorityVar != ACP_NOVALUE) {
				std::sort(table.begin(), table.end(), AutoCastPrioritySort(caster, autocast->PriorityVar, autocast->ReverseSort));
				return NewTargetUnit(*table[0]);
			} else { // Use the old behavior
				return NewTargetUnit(*table[SyncRand() % table.size()]);
			}
		}
	} else {
		// Something is wrong
		DebugPrint("Spell is screwed up, unknown target type\n");
		Assert(0);
		return nullptr;
	}

	return nullptr; // cannot autocast the spell
}

// ****************************************************************************
// Public spell functions
// ****************************************************************************

// ****************************************************************************
// Constructor and destructor
// ****************************************************************************

/**
** Spells constructor, inits spell id's and sounds
*/
void InitSpells()
{
}

// ****************************************************************************
// CanAutoCastSpell, CanCastSpell, AutoCastSpell, CastSpell.
// ****************************************************************************

/**
**	@brief	Check if a spell is available for a given unit
**
**	@param	unit	The unit for whom we want to know if have access to the spell
**
**	@return	True if the spell is available for the unit, and false otherwise
*/
bool CSpell::IsAvailableForUnit(const CUnit &unit) const
{
	const int dependencyId = this->DependencyId;

	//Wyrmgus start
//	return dependencyId == -1 || UpgradeIdAllowed(player, dependencyId) == 'R';
	return dependencyId == -1 || unit.GetIndividualUpgrade(CUpgrade::get_all()[dependencyId]) > 0 || UpgradeIdAllowed(*unit.Player, dependencyId) == 'R';
	//Wyrmgus end
}

/**
**	@brief	Check if unit can cast the spell.
**
**	@param	caster	Unit that casts the spell
**	@param	spell	Spell-type pointer
**	@param	target	Target unit that spell is addressed to
**	@param	goalPos	Coord of target spot when/if target does not exist
**	@param	map_layer	Map layer of the target spot when/if target does not exist
**
**	@return	True if the spell should/can casted, false if not
**	@note	caster must know the spell, and spell must be researched.
*/
bool CanCastSpell(const CUnit &caster, const CSpell &spell,
				  const CUnit *target, const Vec2i &goalPos, const CMapLayer *map_layer)
{
	if (spell.Target == TargetType::Unit && target == nullptr) {
		return false;
	}
	return PassCondition(caster, spell, target, goalPos, spell.Condition, map_layer);
}

/**
**	@brief	Check if the spell can be auto cast and cast it.
**
**	@param	caster	Unit who can cast the spell.
**  @param	spell	Spell-type pointer.
**
**	@return	1 if spell is casted, 0 if not.
*/
int AutoCastSpell(CUnit &caster, const CSpell &spell)
{
	//  Check for mana and cooldown time, trivial optimization.
	if (!caster.CanAutoCastSpell(&spell)) {
		return 0;
	}
	Target *target = SelectTargetUnitsOfAutoCast(caster, spell);
	if (target == nullptr) {
		return 0;
	} else {
		// Save previous order
		COrder *savedOrder = nullptr;
		if (caster.CurrentAction() != UnitAction::Still && caster.CanStoreOrder(caster.CurrentOrder())) {
			savedOrder = caster.CurrentOrder()->Clone();
		}
		// Must move before ?
		CommandSpellCast(caster, target->targetPos, target->Unit, spell, FlushCommands, target->MapLayer, true);
		delete target;
		if (savedOrder != nullptr) {
			caster.SavedOrder = savedOrder;
		}
	}
	return 1;
}

/**
** Spell cast!
**
** @param caster    Unit that casts the spell
** @param spell     Spell-type pointer
** @param target    Target unit that spell is addressed to
** @param goalPos   coord of target spot when/if target does not exist
**
** @return          !=0 if spell should/can continue or 0 to stop
*/
int SpellCast(CUnit &caster, const CSpell &spell, CUnit *target, const Vec2i &goalPos, CMapLayer *map_layer)
{
	Vec2i pos = goalPos;
	int z = map_layer ? map_layer->ID : 0;

	caster.Variable[INVISIBLE_INDEX].Value = 0;// unit is invisible until attacks // FIXME: Must be configurable
	if (target) {
		pos = target->tilePos;
		map_layer = target->MapLayer;
	}
	//
	// For TargetSelf, you target.... YOURSELF
	//
	if (spell.Target == TargetType::Self) {
		pos = caster.tilePos;
		map_layer = caster.MapLayer;
		target = &caster;
	}
	DebugPrint("Spell cast: (%s), %s -> %s (%d,%d)\n" _C_ spell.Ident.c_str() _C_
			   caster.Type->get_name().c_str() _C_ target ? target->Type->get_name().c_str() : "none" _C_ pos.x _C_ pos.y);
	if (CanCastSpell(caster, spell, target, pos, map_layer)) {
		int cont = 1; // Should we recast the spell.
		bool mustSubtractMana = true; // false if action which have their own calculation is present.
		//
		//  Ugly hack, CastAdjustVitals makes it's own mana calculation.
		//
		if (spell.SoundWhenCast.Sound) {
			if (spell.Target == TargetType::Self) {
				PlayUnitSound(caster, spell.SoundWhenCast.Sound);
			} else {
				//Wyrmgus start
//				PlayGameSound(spell.SoundWhenCast.Sound, CalculateVolume(false, ViewPointDistance(target ? target->tilePos : goalPos), spell.SoundWhenCast.Sound->Range));
				PlayGameSound(spell.SoundWhenCast.Sound, CalculateVolume(false, ViewPointDistance(target ? target->tilePos : goalPos), spell.SoundWhenCast.Sound->range) * spell.SoundWhenCast.Sound->VolumePercent / 100);
				//Wyrmgus end
			}
		//Wyrmgus start
		} else if (caster.Type->MapSound.Hit.Sound) { //if the spell has no sound-when-cast designated, use the unit's hit sound instead (if any)
			if (spell.Target == TargetType::Self) {
				PlayUnitSound(caster, caster.Type->MapSound.Hit.Sound);
			} else {
				PlayGameSound(caster.Type->MapSound.Hit.Sound, CalculateVolume(false, ViewPointDistance(target ? target->tilePos : goalPos), caster.Type->MapSound.Hit.Sound->range) * caster.Type->MapSound.Hit.Sound->VolumePercent / 100);
			}
		//Wyrmgus end
		}
		
		int modifier = 100;
		if (caster.IsSpellEmpowered(&spell)) {
			modifier += 100; //empowered spells have double the effect
		}
			
		for (std::vector<SpellActionType *>::const_iterator act = spell.Action.begin();
			 act != spell.Action.end(); ++act) {
			if ((*act)->ModifyManaCaster) {
				mustSubtractMana = false;
			}
			
			cont = cont & (*act)->Cast(caster, spell, target, pos, z, modifier);
		}
		if (mustSubtractMana) {
			caster.Variable[MANA_INDEX].Value -= spell.ManaCost;
		}
		caster.Player->SubCosts(spell.Costs);
		caster.SpellCoolDownTimers[spell.Slot] = spell.CoolDown;
		//
		// Spells like blizzard are casted again.
		// This is sort of confusing, we do the test again, to
		// check if it will be possible to cast again. Otherwise,
		// when you're out of mana the caster will try again ( do the
		// anim but fail in this proc.
		//
		if (spell.RepeatCast && cont) {
			return CanCastSpell(caster, spell, target, pos, map_layer);
		}
	}
	//
	// Can't cast, STOP.
	//
	return 0;
}

static char StringToCondition(const std::string &str)
{
	if (str == "true") {
		return CONDITION_TRUE;
	} else if (str == "false") {
		return CONDITION_FALSE;
	} else if (str == "only") {
		return CONDITION_ONLY;
	} else {
		fprintf(stderr, "Bad condition result: \"%s\".\n", str.c_str());
		return -1;
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void ConditionInfo::ProcessConfigData(const CConfigData *config_data)
{
	// Flags are defaulted to 0(CONDITION_TRUE)
	size_t new_bool_size = UnitTypeVar.GetNumberBoolFlag();

	this->BoolFlag = new char[new_bool_size];
	memset(this->BoolFlag, 0, new_bool_size * sizeof(char));

	this->Variable = new ConditionInfoVariable[UnitTypeVar.GetNumberVariable()];
	// Initialize min/max stuff to values with no effect.
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		this->Variable[i].Check = false;
		this->Variable[i].ExactValue = -1;
		this->Variable[i].ExceptValue = -1;
		this->Variable[i].MinValue = -1;
		this->Variable[i].MaxValue = -1;
		this->Variable[i].MinMax = -1;
		this->Variable[i].MinValuePercent = -8;
		this->Variable[i].MaxValuePercent = 1024;
	}
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "alliance") {
			this->Alliance = StringToCondition(value);
		} else if (key == "opponent") {
			this->Opponent = StringToCondition(value);
		} else if (key == "self") {
			this->TargetSelf = StringToCondition(value);
		} else if (key == "thrusting_weapon") {
			this->ThrustingWeapon = StringToCondition(value);
		} else if (key == "faction_unit") {
			this->FactionUnit = StringToCondition(value);
		} else if (key == "civilization_equivalent") {
			value = FindAndReplaceString(value, "_", "-");
			const stratagus::civilization *civilization = stratagus::civilization::get(value);
			this->civilization_equivalent = civilization->ID;
		} else if (key == "faction_equivalent") {
			stratagus::faction *faction = stratagus::faction::get(value);
			this->FactionEquivalent = faction;
		} else {
			key = string::snake_case_to_pascal_case(key);
			
			int index = UnitTypeVar.BoolFlagNameLookup[key.c_str()];
			if (index != -1) {
				this->BoolFlag[index] = StringToCondition(value);
			} else {
				fprintf(stderr, "Invalid spell condition property: \"%s\".\n", key.c_str());
			}
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		std::string tag = child_config_data->Tag;
		tag = string::snake_case_to_pascal_case(tag);
		
		int index = UnitTypeVar.VariableNameLookup[tag.c_str()];
		if (index != -1) {
			this->Variable[index].Check = true;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "enable") {
					this->Variable[index].Enable = StringToCondition(value);
				} else if (key == "exact_value") {
					this->Variable[index].ExactValue = std::stoi(value);
				} else if (key == "except_value") {
					this->Variable[index].ExceptValue = std::stoi(value);
				} else if (key == "min_value") {
					this->Variable[index].MinValue = std::stoi(value);
				} else if (key == "max_value") {
					this->Variable[index].MaxValue = std::stoi(value);
				} else if (key == "min_max") {
					this->Variable[index].MinMax = std::stoi(value);
				} else if (key == "min_value_percent") {
					this->Variable[index].MinValuePercent = std::stoi(value);
				} else if (key == "max_value_percent") {
					this->Variable[index].MaxValuePercent = std::stoi(value);
				} else if (key == "condition_apply_on_caster") {
					this->Variable[index].ConditionApplyOnCaster = string::to_bool(value);
				} else {
					fprintf(stderr, "Invalid adjust variable spell action variable property: \"%s\".\n", key.c_str());
				}
			}
		} else {
			fprintf(stderr, "Invalid spell condition property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void AutoCastInfo::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "range") {
			this->Range = std::stoi(value);
		} else if (key == "min_range") {
			this->MinRange = std::stoi(value);
		} else if (key == "combat") {
			this->Combat = StringToCondition(value);
		} else if (key == "attacker") {
			this->Attacker = StringToCondition(value);
		} else if (key == "corpse") {
			this->Corpse = StringToCondition(value);
		} else {
			fprintf(stderr, "Invalid autocast property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "priority") {
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "priority_var") {
					int index = -1;
					if (value == "distance") {
						index = ACP_DISTANCE;
					} else {
						value = string::snake_case_to_pascal_case(value);
						index = UnitTypeVar.VariableNameLookup[value.c_str()];
					}
					
					if (index != -1) {
						this->PriorityVar = index;
					} else {
						fprintf(stderr, "Invalid autocast priority variable value: \"%s\".\n", value.c_str());
					}
				} else if (key == "reverse_sort") {
					this->ReverseSort = string::to_bool(value);
				} else {
					fprintf(stderr, "Invalid autocast priority property: \"%s\".\n", key.c_str());
				}
			}
		} else if (child_config_data->Tag == "condition") {
			if (!this->Condition) {
				this->Condition = new ConditionInfo;
			}
			this->Condition->ProcessConfigData(child_config_data);
		} else {
			fprintf(stderr, "Invalid autocast property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}
