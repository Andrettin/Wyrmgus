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
//      (c) Copyright 1998-2021 by Vladi Belperchinov-Shabanski, Lutz Sammer,
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

/*
** And when we cast our final spell
** And we meet in our dreams
** A place that no one else can go
** Don't ever let your love die
** Don't ever go breaking this spell
*/

#include "stratagus.h"

#include "spell/spell.h"

#include "actions.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "magic_domain.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile_flag.h"
#include "player/faction.h"
#include "script.h"
#include "sound/sound.h"
#include "spell/spell_action.h"
#include "spell/spell_action_adjust_variable.h"
#include "spell/spell_action_spawn_missile.h"
#include "spell/spell_target_type.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "upgrade/upgrade.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"

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
static std::unique_ptr<Target> NewTargetUnit(CUnit &unit)
{
	return std::make_unique<Target>(wyrmgus::spell_target_type::unit, &unit, unit.tilePos, unit.MapLayer->ID);
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
static bool PassCondition(const CUnit &caster, const wyrmgus::spell &spell, const CUnit *target,
						  const ConditionInfo *condition)
{
	if (caster.Variable[MANA_INDEX].Value < spell.get_mana_cost()) { // Check caster mana.
		return false;
	}
	// check countdown timer
	if (caster.get_spell_cooldown_timer(&spell) > 0) { // Check caster mana.
		return false;
	}
	// Check caster's resources
	if (caster.Player->CheckCosts(spell.get_costs(), false)) {
		return false;
	}
	if (spell.get_target() == wyrmgus::spell_target_type::unit) { // Casting a unit spell without a target.
		if ((!target) || target->IsAlive() == false) {
			return false;
		}
	}
	if (!condition) { // no condition, pass.
		return true;
	}
	
	if (target && !target->Type->CheckUserBoolFlags(condition->BoolFlag.get())) {
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
		if (condition->Variable[i].ExactValue.has_value() &&
			condition->Variable[i].ExactValue != unit->Variable[i].Value) {
			return false;
		}
		if (condition->Variable[i].ExceptValue.has_value() &&
			condition->Variable[i].ExceptValue == unit->Variable[i].Value) {
			return false;
		}
		if (condition->Variable[i].MinValue >= unit->Variable[i].Value) {
			return false;
		}
		if (condition->Variable[i].MaxValue.has_value() &&
			//Wyrmgus start
//			condition->Variable[i].MaxValue <= unit->Variable[i].Value) {
			condition->Variable[i].MaxValue <= unit->GetModifiedVariable(i, VariableAttribute::Value)) {
			//Wyrmgus end
			return false;
		}

		//Wyrmgus start
//		if (condition->Variable[i].MinMax >= unit->Variable[i].Max) {
		if (condition->Variable[i].MinMax >= unit->GetModifiedVariable(i, VariableAttribute::Max)) {
		//Wyrmgus end
			return false;
		}

		//Wyrmgus start
//		if (!unit->Variable[i].Max) {
		if (!unit->GetModifiedVariable(i, VariableAttribute::Max)) {
		//Wyrmgus end
			continue;
		}
		// Percent
		//Wyrmgus start
//		if (condition->Variable[i].MinValuePercent * unit->Variable[i].Max
		if (condition->Variable[i].MinValuePercent * unit->GetModifiedVariable(i, VariableAttribute::Max)
		//Wyrmgus end
			>= 100 * unit->Variable[i].Value) {
			return false;
		}
		//Wyrmgus start
//		if (condition->Variable[i].MaxValuePercent * unit->Variable[i].Max
		if (condition->Variable[i].MaxValuePercent * unit->GetModifiedVariable(i, VariableAttribute::Max)
		//Wyrmgus end
			<= 100 * unit->Variable[i].Value) {
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
			(caster.IsEnemy(*target))) {
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
		if ((condition->ThrustingWeapon == CONDITION_ONLY) ^ wyrmgus::is_thrusting_weapon_item_class(caster.GetCurrentWeaponClass())) {
			return false;
		}
	}
	if (condition->FactionUnit != CONDITION_TRUE) {
		if ((condition->FactionUnit == CONDITION_ONLY) ^ (caster.Type->get_faction() != nullptr)) {
			return false;
		}
	}
	if (condition->civilization_equivalent != nullptr) {
		if (caster.Type->get_civilization() == nullptr || (caster.Type->get_civilization() == condition->civilization_equivalent && (caster.get_character() == nullptr || (caster.get_character()->get_civilization() && caster.get_character()->get_civilization() == condition->civilization_equivalent))) || caster.Type->get_civilization()->get_species() != condition->civilization_equivalent->get_species() || condition->civilization_equivalent->get_class_unit_type(caster.Type->get_unit_class()) == nullptr || (caster.get_character() != nullptr && !caster.get_character()->Custom)) {
			return false;
		}
	}
	if (condition->FactionEquivalent != nullptr) {
		if (caster.Type->get_civilization() == nullptr || caster.Type->get_civilization() != condition->FactionEquivalent->get_civilization() || condition->FactionEquivalent->get_class_unit_type(caster.Type->get_unit_class()) == nullptr|| (caster.get_character() != nullptr && !caster.get_character()->Custom)) {
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

namespace wyrmgus {

spell *spell::add(const std::string &identifier, const wyrmgus::data_module *data_module)
{
	spell *spell = data_type::add(identifier, data_module);
	spell->Slot = spell::get_all().size() - 1;

	return spell;
}

spell::spell(const std::string &identifier) : named_data_entry(identifier)
{
	//Wyrmgus start
	memset(ItemSpell, 0, sizeof(ItemSpell));
	//Wyrmgus end
}

spell::~spell()
{
}

void spell::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "range") {
		if (value == "infinite") {
			this->range = spell::infinite_range;
		} else {
			this->range = std::stoi(value);
		}
	} else if (key == "effects_string") {
		this->effects_string = value;
	} else if (key == "item_spell") {
		const int item_class = static_cast<int>(string_to_item_class(value));
		this->ItemSpell[item_class] = true;
	} else {
		data_entry::process_sml_property(property);
	}
}

void spell::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "magic_domains") {
		for (const std::string &value : values) {
			magic_domain *domain = magic_domain::get(value);
			this->magic_domains.push_back(domain);
			domain->add_spell(this);
		}
	} else if (tag == "actions") {
		scope.for_each_child([&](const sml_data &child_scope) {
			this->actions.push_back(spell_action::from_sml_scope(child_scope));
		});
	} else if (tag == "cast_conditions") {
		if (!this->cast_conditions) {
			this->cast_conditions = std::make_unique<ConditionInfo>();
		}
		database::process_sml_data(this->cast_conditions, scope);
	} else if (tag == "autocast") {
		if (!this->autocast) {
			this->autocast = std::make_unique<AutoCastInfo>();
		}
		database::process_sml_data(this->autocast, scope);
	} else if (tag == "ai_cast") {
		if (!this->ai_cast) {
			this->ai_cast = std::make_unique<AutoCastInfo>();
		}
		database::process_sml_data(this->ai_cast, scope);
	} else if (tag == "resource_costs") {
		scope.for_each_property([&](const wyrmgus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const wyrmgus::resource *resource = resource::get(key);
			this->costs[resource] = std::stoi(value);
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void spell::delete_lua_callbacks()
{
	if (this->autocast != nullptr) {
		this->autocast->PositionAutoCast.reset();
	}

	if (this->ai_cast != nullptr) {
		this->ai_cast->PositionAutoCast.reset();
	}
}

/**
**	@brief	Get the autocast info for the spell
**
**	@param	ai	Whether the spell would be cast by the AI
**
**	@return	The autocast info for the spell if present, or null otherwise
*/
const AutoCastInfo *spell::get_autocast_info(const bool ai) const
{
	if (ai && this->ai_cast) {
		return this->ai_cast.get();
	} else {
		return this->autocast.get();
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
bool spell::CheckAutoCastGenericConditions(const CUnit &caster, const AutoCastInfo *autocast, const bool ignore_combat_status) const
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
bool spell::IsUnitValidAutoCastTarget(const CUnit *target, const CUnit &caster, const AutoCastInfo *autocast, const int max_path_length) const
{
	if (!target || !autocast) {
		return false;
	}

	// Check if unit is in battle
	if (this->get_target() == spell_target_type::unit) {
		if (autocast->Attacker == CONDITION_ONLY) {
			const int react_range = target->GetReactionRange();
			if (
				(
					target->CurrentAction() != UnitAction::Attack
					&& target->CurrentAction() != UnitAction::AttackGround
					&& target->CurrentAction() != UnitAction::SpellCast
					)
				|| target->CurrentOrder()->has_goal() == false
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

	if (this->get_target() == spell_target_type::unit) {
		//if caster is terrified, don't target enemy units
		if (caster.Variable[TERROR_INDEX].Value > 0 && caster.IsEnemy(*target)) {
			return false;
		}
	}

	if (!PassCondition(caster, *this, target, this->get_cast_conditions()) || !PassCondition(caster, *this, target, autocast->get_cast_conditions())) {
		return false;
	}

	//Wyrmgus start
	int range = this->get_range();
	if (!CheckObstaclesBetweenTiles(caster.tilePos, target->tilePos, tile_flag::air_impassable, target->MapLayer->ID)) {
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
std::vector<CUnit *> spell::GetPotentialAutoCastTargets(const CUnit &caster, const AutoCastInfo *autocast) const
{
	std::vector<CUnit *> potential_targets;

	if (!autocast) {
		return potential_targets;
	}

	int range = autocast->Range;
	int min_range = autocast->MinRange;

	if (caster.CurrentAction() == UnitAction::StandGround) {
		range = std::min(range, this->get_range());
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
**	@brief	Check if a spell is available for a given unit
**
**	@param	unit	The unit for whom we want to know if have access to the spell
**
**	@return	True if the spell is available for the unit, and false otherwise
*/
bool spell::IsAvailableForUnit(const CUnit &unit) const
{
	const CUpgrade *dependency_upgrade = this->get_dependency_upgrade();

	//Wyrmgus start
//	return dependency_upgrade == nullptr || UpgradeIdAllowed(player, dependencyId) == 'R';
	return dependency_upgrade == nullptr || unit.GetIndividualUpgrade(dependency_upgrade) > 0 || UpgradeIdAllowed(*unit.Player, dependency_upgrade->get_index()) == 'R';
	//Wyrmgus end
}

bool spell::is_caster_only() const
{
	return this->get_range() == 0 && this->get_target() == spell_target_type::self;
}

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
static std::unique_ptr<Target> SelectTargetUnitsOfAutoCast(CUnit &caster, const wyrmgus::spell &spell)
{
	const AutoCastInfo *autocast = spell.get_autocast_info(caster.Player->AiEnabled);
	Assert(autocast);
	
	if (!spell.CheckAutoCastGenericConditions(caster, autocast)) {
		return nullptr;
	}
	
	const CMapLayer *map_layer = caster.MapLayer;

	if (spell.get_target() == wyrmgus::spell_target_type::self) {
		if (PassCondition(caster, spell, &caster, spell.get_cast_conditions()) && PassCondition(caster, spell, &caster, autocast->get_cast_conditions())) {
			return NewTargetUnit(caster);
		}
	} else if (spell.get_target() == wyrmgus::spell_target_type::position) {
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
			if (CMap::get()->Info->IsPointOnMap(resPos, map_layer)) {
				return std::make_unique<Target>(wyrmgus::spell_target_type::position, nullptr, resPos, map_layer->ID);
			}
		}
	} else if (spell.get_target() == wyrmgus::spell_target_type::unit) {
		std::vector<CUnit *> table = spell.GetPotentialAutoCastTargets(caster, autocast);
		//now select the best unit to target.
		if (!table.empty()) {
			// For the best target???
			if (autocast->PriorityVar != ACP_NOVALUE) {
				std::sort(table.begin(), table.end(), AutoCastPrioritySort(caster, autocast->PriorityVar, autocast->ReverseSort));
				return NewTargetUnit(*table[0]);
			} else { // Use the old behavior
				return NewTargetUnit(*vector::get_random(table));
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
// CanAutoCastSpell, CanCastSpell, AutoCastSpell, CastSpell.
// ****************************************************************************

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
bool CanCastSpell(const CUnit &caster, const wyrmgus::spell &spell, const CUnit *target)
{
	if (spell.get_target() == wyrmgus::spell_target_type::unit && target == nullptr) {
		return false;
	}
	return PassCondition(caster, spell, target, spell.get_cast_conditions());
}

/**
**	@brief	Check if the spell can be auto cast and cast it.
**
**	@param	caster	Unit who can cast the spell.
**  @param	spell	Spell-type pointer.
**
**	@return	1 if spell is casted, 0 if not.
*/
int AutoCastSpell(CUnit &caster, const wyrmgus::spell &spell)
{
	//  Check for mana and cooldown time, trivial optimization.
	if (!caster.CanAutoCastSpell(&spell)) {
		return 0;
	}
	std::unique_ptr<Target> target = SelectTargetUnitsOfAutoCast(caster, spell);
	if (target == nullptr) {
		return 0;
	} else {
		// Save previous order
		std::unique_ptr<COrder> saved_order;
		if (caster.CurrentAction() != UnitAction::Still && caster.CanStoreOrder(caster.CurrentOrder())) {
			saved_order = caster.CurrentOrder()->Clone();
		}
		// Must move before ?
		CommandSpellCast(caster, target->targetPos, target->Unit, spell, FlushCommands, target->MapLayer);
		if (saved_order != nullptr) {
			caster.SavedOrder = std::move(saved_order);
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
int SpellCast(CUnit &caster, const wyrmgus::spell &spell, CUnit *target, const Vec2i &goalPos, CMapLayer *map_layer)
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
	if (spell.get_target() == wyrmgus::spell_target_type::self) {
		pos = caster.tilePos;
		map_layer = caster.MapLayer;
		target = &caster;
	}
	DebugPrint("Spell cast: (%s), %s -> %s (%d,%d)\n" _C_ spell.get_identifier().c_str() _C_
			   caster.Type->get_name().c_str() _C_ target ? target->Type->get_name().c_str() : "none" _C_ pos.x _C_ pos.y);
	if (CanCastSpell(caster, spell, target)) {
		int cont = 1; // Should we recast the spell.
		bool mustSubtractMana = true; // false if action which have their own calculation is present.
		//
		//  Ugly hack, CastAdjustVitals makes it's own mana calculation.
		//
		if (spell.get_sound_when_cast() != nullptr) {
			if (spell.get_target() == wyrmgus::spell_target_type::self) {
				PlayUnitSound(caster, spell.get_sound_when_cast());
			} else {
				PlayGameSound(spell.get_sound_when_cast(), CalculateVolume(false, ViewPointDistance(target ? target->tilePos : goalPos), spell.get_sound_when_cast()->get_range()) * spell.get_sound_when_cast()->VolumePercent / 100);
			}
		} else if (caster.Type->MapSound->Hit.Sound != nullptr) {
			//if the spell has no sound-when-cast designated, use the unit's hit sound instead (if any)
			if (spell.get_target() == wyrmgus::spell_target_type::self) {
				PlayUnitSound(caster, caster.Type->MapSound->Hit.Sound);
			} else {
				PlayGameSound(caster.Type->MapSound->Hit.Sound, CalculateVolume(false, ViewPointDistance(target ? target->tilePos : goalPos), caster.Type->MapSound->Hit.Sound->get_range()) * caster.Type->MapSound->Hit.Sound->VolumePercent / 100);
			}
		}
		
		int modifier = 100;
		if (caster.is_spell_empowered(&spell)) {
			modifier += 100; //empowered spells have double the effect
		}
			
		for (const auto &spell_action : spell.get_actions()) {
			if (spell_action->ModifyManaCaster) {
				mustSubtractMana = false;
			}
			
			cont = cont & spell_action->Cast(caster, spell, target, pos, z, modifier);
		}
		if (mustSubtractMana) {
			caster.Variable[MANA_INDEX].Value -= spell.get_mana_cost();
		}
		caster.Player->subtract_costs(spell.get_costs());
		if (spell.get_cooldown() > 0) {
			caster.set_spell_cooldown_timer(&spell, spell.get_cooldown());
		}
		//
		// Spells like blizzard are casted again.
		// This is sort of confusing, we do the test again, to
		// check if it will be possible to cast again. Otherwise,
		// when you're out of mana the caster will try again ( do the
		// anim but fail in this proc.
		//
		if (spell.repeats_cast() && cont) {
			return CanCastSpell(caster, spell, target);
		}
	}
	//
	// Can't cast, STOP.
	//
	return 0;
}

char StringToCondition(const std::string &str)
{
	if (str == "true") {
		return CONDITION_TRUE;
	} else if (str == "false") {
		return CONDITION_FALSE;
	} else if (str == "only") {
		return CONDITION_ONLY;
	} else {
		throw std::runtime_error("Bad condition result: \"" + str + "\".");
	}
}

ConditionInfo::ConditionInfo()
{
	// Flags are defaulted to 0(CONDITION_TRUE)
	const size_t new_bool_size = UnitTypeVar.GetNumberBoolFlag();

	this->BoolFlag = std::make_unique<char[]>(new_bool_size);
	memset(this->BoolFlag.get(), 0, new_bool_size * sizeof(char));

	this->Variable = std::make_unique<ConditionInfoVariable[]>(UnitTypeVar.GetNumberVariable());
	// Initialize min/max stuff to values with no effect.
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		this->Variable[i].Check = false;
		this->Variable[i].MinValue = -1;
		this->Variable[i].MinMax = -1;
		this->Variable[i].MinValuePercent = -8;
		this->Variable[i].MaxValuePercent = 1024;
	}
}

void ConditionInfo::process_sml_property(const wyrmgus::sml_property &property)
{
	const std::string &key = property.get_key();
	const wyrmgus::sml_operator &property_operator = property.get_operator();
	const std::string &value = property.get_value();

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
		this->civilization_equivalent = wyrmgus::civilization::get(value);
	} else if (key == "faction_equivalent") {
		this->FactionEquivalent = wyrmgus::faction::get(value);
	} else {
		const std::string pascal_case_key = wyrmgus::string::snake_case_to_pascal_case(key);

		int index = UnitTypeVar.VariableNameLookup[pascal_case_key.c_str()];
		if (index != -1) {
			this->Variable[index].Check = true;

			switch (property_operator) {
				case wyrmgus::sml_operator::equality:
					this->Variable[index].ExactValue = std::stoi(value);
					break;
				default:
					throw std::runtime_error("Invalid operator for variable property: \"" + std::to_string(static_cast<int>(property_operator)) + "\".");
			}
			return;
		}

		index = UnitTypeVar.BoolFlagNameLookup[pascal_case_key.c_str()];
		if (index != -1) {
			this->BoolFlag[index] = StringToCondition(value);
		} else {
			throw std::runtime_error("Invalid spell condition property: \"" + key + "\".");
		}
	}
}

void ConditionInfo::process_sml_scope(const wyrmgus::sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	const std::string pascal_case_tag = wyrmgus::string::snake_case_to_pascal_case(tag);

	const int index = UnitTypeVar.VariableNameLookup[pascal_case_tag.c_str()];
	if (index != -1) {
		this->Variable[index].Check = true;

		scope.for_each_property([&](const wyrmgus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

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
				this->Variable[index].ConditionApplyOnCaster = wyrmgus::string::to_bool(value);
			} else {
				throw std::runtime_error("Invalid adjust variable spell action variable property: \"" + key + "\".");
			}
		});
	} else {
		throw std::runtime_error("Invalid spell condition scope: \"" + tag + "\".");
	}
}

void AutoCastInfo::process_sml_property(const wyrmgus::sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

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
		throw std::runtime_error("Invalid autocast info property: \"" + key + "\".");
	}
}

void AutoCastInfo::process_sml_scope(const wyrmgus::sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "priority") {
		scope.for_each_property([&](const wyrmgus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "priority_var") {
				int index = -1;
				if (value == "distance") {
					index = ACP_DISTANCE;
				} else {
					const std::string pascal_case_value = wyrmgus::string::snake_case_to_pascal_case(value);
					index = UnitTypeVar.VariableNameLookup[pascal_case_value.c_str()];
				}

				if (index != -1) {
					this->PriorityVar = index;
				} else {
					throw std::runtime_error("Invalid autocast priority variable value: \"" + value + "\".");
				}
			} else if (key == "reverse_sort") {
				this->ReverseSort = wyrmgus::string::to_bool(value);
			} else {
				throw std::runtime_error("Invalid autocast priority property: \"" + key + "\".");
			}
		});
	} else if (tag == "cast_conditions") {
		if (!this->cast_conditions) {
			this->cast_conditions = std::make_unique<ConditionInfo>();
		}
		wyrmgus::database::process_sml_data(this->cast_conditions, scope);
	} else {
		throw std::runtime_error("Invalid autocast info scope: \"" + tag + "\".");
	}
}
