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
/**@name actions.cpp - The actions. */
//
//      (c) Copyright 1998-2021 by Lutz Sammer, Russell Smith, Jimmy Salmon and Andrettin
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
#include "version.h"

#include "actions.h"

#include "action/action_attack.h"
#include "action/action_board.h"
#include "action/action_build.h"
#include "action/action_built.h"
#include "action/action_defend.h"
#include "action/action_die.h"
#include "action/action_follow.h"
#include "action/action_move.h"
#include "action/action_patrol.h"
#include "action/action_pickup.h"
#include "action/action_repair.h"
#include "action/action_research.h"
#include "action/action_resource.h"
#include "action/action_spellcast.h"
#include "action/action_still.h"
//Wyrmgus start
#include "action/action_trade.h"
//Wyrmgus end
#include "action/action_train.h"
#include "action/action_unload.h"
#include "action/action_upgradeto.h"
#include "action/action_use.h"

#include "animation/animation_die.h"
#include "commands.h"
#include "luacallback.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "missile.h"
#include "pathfinder.h"
#include "player.h"
#include "script.h"
#include "script/condition/condition.h"
#include "spell/spell.h"
#include "time/time_of_day.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_manager.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/random.h"

unsigned SyncHash; /// Hash calculated to find sync failures

CUnit *COrder::get_goal() const
{
	if (this->goal == nullptr) {
		return nullptr;
	}

	return this->goal->get();
}

void COrder::set_goal(CUnit *const new_goal)
{
	this->goal = new_goal->acquire_ref();
}

void COrder::clear_goal()
{
	this->goal.reset();
}

void COrder::UpdatePathFinderData_NotCalled(PathFinderInput &input)
{
	Assert(false); // should not be called.

	// Don't move
	input.SetMinRange(0);
	input.SetMaxRange(0);
	const Vec2i tileSize(0, 0);
	input.SetGoal(input.GetUnit()->tilePos, tileSize, input.GetUnit()->MapLayer->ID);
}

/* virtual */ void COrder::FillSeenValues(CUnit &unit) const
{
	unit.Seen.State = ((Action == UnitAction::UpgradeTo) << 1);
	if (unit.CurrentAction() == UnitAction::Die) {
		unit.Seen.State = 3;
	}
	unit.Seen.cframe = nullptr;
}

bool COrder::OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/)
{
	Q_UNUSED(unit)
	Q_UNUSED(attacker)

	return false;
}


/** Called when unit is killed.
**  warn the AI module.
*/
/* virtual */ void COrder::AiUnitKilled(CUnit &unit)
{
	switch (Action) {
		case UnitAction::Still:
		case UnitAction::Attack:
		case UnitAction::Move:
			break;
		default:
			DebugPrint("FIXME: %d: %d(%s) killed, with order %d!\n" _C_
					   unit.Player->Index _C_ UnitNumber(unit) _C_
					   unit.Type->Ident.c_str() _C_ Action);
			break;
	}
}

/**
**  Call when animation step is "attack"
*/
/* virtual */ void COrder::OnAnimationAttack(CUnit &unit)
{
	//Wyrmgus start
//	if (unit.Type->CanAttack == false) {
	if (unit.CanAttack(false) == false) {
	//Wyrmgus end
		return;
	}
	CUnit *goal = AttackUnitsInRange(unit);

	if (goal != nullptr) {
		const Vec2i invalidPos(-1, -1);

		FireMissile(unit, goal, invalidPos, goal->MapLayer->ID);
		UnHideUnit(unit); // unit is invisible until attacks
	}
	unit.reset_step_count();
	// Fixme : Auto select position to attack ?
}

/**
**  Get goal position
*/
const Vec2i COrder::GetGoalPos() const
{
	const Vec2i invalidPos(-1, -1);
	if (this->has_goal()) {
		return this->get_goal()->tilePos;
	}
	return invalidPos;
}

//Wyrmgus start
/**
**  Get goal map layer
*/
const int COrder::GetGoalMapLayer() const
{
	if (this->has_goal()) {
		return this->get_goal()->MapLayer->ID;
	}
	return 0;
}
//Wyrmgus end

/**
**  Parse order
**
**  @param l      Lua state.
**  @param order  OUT: resulting order.
*/
std::unique_ptr<COrder> CclParseOrder(lua_State *l, CUnit &unit)
{
	const int args = lua_rawlen(l, -1);
	const char *actiontype = LuaToString(l, -1, 1);

	std::unique_ptr<COrder> order;

	if (!strcmp(actiontype, "action-attack")) {
		order = std::make_unique<COrder_Attack>(false);
	} else if (!strcmp(actiontype, "action-attack-ground")) {
		order = std::make_unique<COrder_Attack>(true);
	} else if (!strcmp(actiontype, "action-board")) {
		order = std::make_unique<COrder_Board>();
	} else if (!strcmp(actiontype, "action-build")) {
		order = std::make_unique<COrder_Build>();
	} else if (!strcmp(actiontype, "action-built")) {
		order = std::make_unique<COrder_Built>();
	} else if (!strcmp(actiontype, "action-defend")) {
		order = std::make_unique<COrder_Defend>();
	} else if (!strcmp(actiontype, "action-die")) {
		order = std::make_unique<COrder_Die>();
	} else if (!strcmp(actiontype, "action-follow")) {
		order = std::make_unique<COrder_Follow>();
	} else if (!strcmp(actiontype, "action-move")) {
		order = std::make_unique<COrder_Move>();
	} else if (!strcmp(actiontype, "action-patrol")) {
		order = std::make_unique<COrder_Patrol>();
	} else if (!strcmp(actiontype, "action-pick-up")) {
		order = std::make_unique<COrder_PickUp>();
	} else if (!strcmp(actiontype, "action-repair")) {
		order = std::make_unique<COrder_Repair>();
	} else if (!strcmp(actiontype, "action-research")) {
		order = std::make_unique<COrder_Research>();
	} else if (!strcmp(actiontype, "action-resource")) {
		order = std::make_unique<COrder_Resource>(unit);
	} else if (!strcmp(actiontype, "action-spell-cast")) {
		order = std::make_unique<COrder_SpellCast>();
	} else if (!strcmp(actiontype, "action-stand-ground")) {
		order = std::make_unique<COrder_Still>(true);
	} else if (!strcmp(actiontype, "action-still")) {
		order = std::make_unique<COrder_Still>(false);
	} else if (!strcmp(actiontype, "action-train")) {
		order = std::make_unique<COrder_Train>();
	} else if (!strcmp(actiontype, "action-transform-into")) {
		order = std::make_unique<COrder_TransformInto>();
	} else if (!strcmp(actiontype, "action-upgrade-to")) {
		order = std::make_unique<COrder_UpgradeTo>();
	} else if (!strcmp(actiontype, "action-unload")) {
		order = std::make_unique<COrder_Unload>();
	} else if (!strcmp(actiontype, "action-use")) {
		order = std::make_unique<COrder_Use>();
	//Wyrmgus start
	} else if (!strcmp(actiontype, "action-trade")) {
		order = std::make_unique<COrder_Trade>();
	//Wyrmgus end
	} else {
		LuaError(l, "ParseOrder: Unsupported type: %s" _C_ actiontype);
	}

	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, -1, j + 1);

		if (order->ParseGenericData(l, j, value)) {
			continue;
		} else if (order->ParseSpecificData(l, j, value, unit)) {
			continue;
		} else {
			// This leaves a half initialized unit
			LuaError(l, "ParseOrder: Unsupported tag: %s" _C_ value);
		}
	}

	return order;
}


/*----------------------------------------------------------------------------
--  Actions
----------------------------------------------------------------------------*/

static inline void IncreaseVariable(CUnit &unit, int index)
{
	unit.change_variable_value(index, unit.get_variable_increase(index));
	clamp(&unit.Variable[index].Value, 0, unit.Variable[index].Max);
	
	//Wyrmgus start
	if (index == HP_INDEX && unit.Variable[index].Increase < 0 && unit.HasInventory()) {
		unit.HealingItemAutoUse();
	} else if (index == GIVERESOURCE_INDEX && !unit.Type->BoolFlag[INEXHAUSTIBLE_INDEX].value) {
		unit.ChangeResourcesHeld(unit.Variable[index].Increase);
		clamp(&unit.ResourcesHeld, 0, unit.GetModifiedVariable(index, VariableAttribute::Max));
	}
	//Wyrmgus end

	//if variable is HP and increase is negative, unit dies if HP reached 0
	if (index == HP_INDEX && unit.Variable[HP_INDEX].Value <= 0) {
		LetUnitDie(unit);
	}
	
	//Wyrmgus start
	//if variable is resources held and increase is negative, unit dies if resources held reached 0 (only for units which cannot be harvested, as the ones that can be harvested need more complex code for dying)
	if (index == GIVERESOURCE_INDEX && unit.Variable[GIVERESOURCE_INDEX].Increase < 0 && unit.Variable[GIVERESOURCE_INDEX].Value <= 0 && unit.GivesResource && !unit.Type->BoolFlag[CANHARVEST_INDEX].value) {
		LetUnitDie(unit);
	}
	//Wyrmgus end
}

/**
**  Handle things about the unit that decay over time each cycle
**
**  @param unit    The unit that the decay is handled for
*/
static void HandleBuffsEachCycle(CUnit &unit)
{
	// Look if the time to live is over.
	if (unit.TTL && unit.IsAlive() && unit.TTL < GameCycle) {
		DebugPrint("Unit must die %lu %lu!\n" _C_ unit.TTL _C_ GameCycle);

		// Hit unit does some funky stuff...
		--unit.Variable[HP_INDEX].Value;
		if (unit.Variable[HP_INDEX].Value <= 0) {
			LetUnitDie(unit);
			return;
		}
	}

	if (--unit.Threshold < 0) {
		unit.Threshold = 0;
	}

	// decrease spell countdown timers
	for (size_t i = 0; i < unit.Type->Spells.size(); ++i) {
		int spell_id = unit.Type->Spells[i]->Slot;
		if (unit.SpellCoolDownTimers[spell_id] > 0) {
			--unit.SpellCoolDownTimers[spell_id];
		}
	}

	for (const auto &kv_pair : unit.Type->Stats[unit.Player->Index].UnitStock) {
		wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
		const int unit_stock = kv_pair.second;

		if (unit_stock <= 0) {
			continue;
		}

		if (unit.GetUnitStockReplenishmentTimer(unit_type) > 0) {
			unit.ChangeUnitStockReplenishmentTimer(unit_type, -1);
			if (unit.GetUnitStockReplenishmentTimer(unit_type) == 0 && unit.GetUnitStock(unit_type) < unit_stock) { //if timer reached 0, replenish 1 of the stock
				unit.ChangeUnitStock(unit_type, 1);
			}
		}

		//if the unit still has less stock than its max, re-init the unit stock timer
		if (unit.GetUnitStockReplenishmentTimer(unit_type) == 0 && unit.GetUnitStock(unit_type) < unit_stock && check_conditions(unit_type, unit.Player)) {
			unit.SetUnitStockReplenishmentTimer(unit_type, unit_type->Stats[unit.Player->Index].Costs[TimeCost] * 50);
		}
	}
	
	static constexpr std::array SpellEffects = {BLOODLUST_INDEX, HASTE_INDEX, SLOW_INDEX, INVISIBLE_INDEX, UNHOLYARMOR_INDEX, POISON_INDEX, STUN_INDEX, BLEEDING_INDEX, LEADERSHIP_INDEX, BLESSING_INDEX, INSPIRE_INDEX, PRECISION_INDEX, REGENERATION_INDEX, BARKSKIN_INDEX, INFUSION_INDEX, TERROR_INDEX, WITHER_INDEX, DEHYDRATION_INDEX, HYDRATING_INDEX};
	//  decrease spells effects time.
	for (const auto spell_effect : SpellEffects) {
		unit.Variable[spell_effect].Increase = -1;
		IncreaseVariable(unit, spell_effect);
	}
	
	const bool lastStatusIsHidden = unit.Variable[INVISIBLE_INDEX].Value > 0;
	if (lastStatusIsHidden && unit.Variable[INVISIBLE_INDEX].Value == 0) {
		UnHideUnit(unit);
	}
}

/**
**  Modify unit's health according to burn and poison
**
**  @param unit  the unit to operate on
*/
static bool HandleBurnAndPoison(CUnit &unit)
{
	if (unit.Removed || unit.Destroyed || unit.Variable[HP_INDEX].Max == 0
		|| unit.CurrentAction() == UnitAction::Built
		|| unit.CurrentAction() == UnitAction::Die) {
		return false;
	}
	// Burn & poison
	//Wyrmgus start
//	const int hpPercent = (100 * unit.Variable[HP_INDEX].Value) / unit.Variable[HP_INDEX].Max;
	const int hpPercent = (100 * unit.Variable[HP_INDEX].Value) / unit.GetModifiedVariable(HP_INDEX, VariableAttribute::Max);
	//Wyrmgus end
	if (hpPercent <= unit.Type->BurnPercent && unit.Type->BurnDamageRate) {
		//Wyrmgus start
//		HitUnit(NoUnitP, unit, unit.Type->BurnDamageRate);
		HitUnit(NoUnitP, unit, unit.Type->BurnDamageRate, nullptr, false); //a bit too repetitive to show damage every single time the burn effect is applied
		//Wyrmgus end
		return true;
	}
	if (unit.Variable[POISON_INDEX].Value && unit.Type->PoisonDrain) {
		//Wyrmgus start
//		HitUnit(NoUnitP, unit, unit.Type->PoisonDrain);
		HitUnit(NoUnitP, unit, unit.Type->PoisonDrain, nullptr, false); //a bit too repetitive to show damage every single time the poison effect is applied
		//Wyrmgus end
		return true;
	}
	//Wyrmgus start
	if (unit.Variable[BLEEDING_INDEX].Value || unit.Variable[DEHYDRATION_INDEX].Value) {
		HitUnit(NoUnitP, unit, 1, nullptr, false);
		//don't return true since we don't want to stop regeneration (positive or negative) from happening
	}
	//Wyrmgus end
	return false;
}

/**
**  Handle things about the unit that decay over time each second
**
**  @param unit    The unit that the decay is handled for
*/
static void HandleBuffsEachSecond(CUnit &unit)
{
	//Wyrmgus start
	if (unit.Type->BoolFlag[DECORATION_INDEX].value) {
		return;
	}
	//Wyrmgus end
	
	// User defined variables
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (i == BLOODLUST_INDEX || i == HASTE_INDEX || i == SLOW_INDEX
			|| i == INVISIBLE_INDEX || i == UNHOLYARMOR_INDEX || i == POISON_INDEX || i == STUN_INDEX || i == BLEEDING_INDEX || i == LEADERSHIP_INDEX || i == BLESSING_INDEX || i == INSPIRE_INDEX || i == PRECISION_INDEX || i == REGENERATION_INDEX || i == BARKSKIN_INDEX || i == INFUSION_INDEX || i == TERROR_INDEX || i == WITHER_INDEX || i == DEHYDRATION_INDEX || i == HYDRATING_INDEX) {
			continue;
		}
		if (i == HP_INDEX && HandleBurnAndPoison(unit)) {
			continue;
		}
		//Wyrmgus start
		if (i == HP_INDEX && unit.Variable[REGENERATION_INDEX].Value > 0) {
			unit.Variable[i].Value += 1;
			clamp(&unit.Variable[i].Value, 0, unit.GetModifiedVariable(i, VariableAttribute::Max));
		}
		//Wyrmgus end
		if (unit.Variable[i].Enable && unit.Variable[i].Increase) {
			IncreaseVariable(unit, i);
		}
	}
	
	//Wyrmgus start
	if (unit.IsAlive() && unit.CurrentAction() != UnitAction::Built) {
		//apply auras
		if (unit.Variable[LEADERSHIPAURA_INDEX].Value > 0) {
			unit.ApplyAura(LEADERSHIPAURA_INDEX);
		}
		if (unit.Variable[REGENERATIONAURA_INDEX].Value > 0) {
			unit.ApplyAura(REGENERATIONAURA_INDEX);
		}
		if (unit.Variable[HYDRATINGAURA_INDEX].Value > 0) {
			unit.ApplyAura(HYDRATINGAURA_INDEX);
		}
		
		//apply "-stalk" abilities
		if ((unit.Variable[DESERTSTALK_INDEX].Value > 0 || unit.Variable[FORESTSTALK_INDEX].Value > 0 || unit.Variable[SWAMPSTALK_INDEX].Value > 0) && CMap::Map.Info.IsPointOnMap(unit.tilePos.x, unit.tilePos.y, unit.MapLayer)) {
			if (
				(
					(unit.Variable[DESERTSTALK_INDEX].Value > 0 && (unit.MapLayer->Field(unit.tilePos.x, unit.tilePos.y)->Flags & MapFieldDesert))
					|| (unit.Variable[FORESTSTALK_INDEX].Value > 0 && CMap::Map.TileBordersFlag(unit.tilePos, unit.MapLayer->ID, MapFieldForest))
					|| (unit.Variable[SWAMPSTALK_INDEX].Value > 0 && (unit.MapLayer->Field(unit.tilePos.x, unit.tilePos.y)->Flags & MapFieldMud))
				)
				&& (unit.Variable[INVISIBLE_INDEX].Value > 0 || !unit.IsInCombat())
			) {				
				std::vector<CUnit *> table;
				SelectAroundUnit(unit, 1, table, IsEnemyWithUnit(&unit));
				if (table.size() == 0) { //only apply the -stalk invisibility if the unit is not adjacent to an enemy unit
					unit.Variable[INVISIBLE_INDEX].Enable = 1;
					unit.Variable[INVISIBLE_INDEX].Max = std::max(CYCLES_PER_SECOND + 1, unit.Variable[INVISIBLE_INDEX].Max);
					unit.Variable[INVISIBLE_INDEX].Value = std::max(CYCLES_PER_SECOND + 1, unit.Variable[INVISIBLE_INDEX].Value);
				}
			}
		}
		
		if ( //apply dehydration to an organic unit on a desert tile; only apply dehydration during day-time
			unit.Type->BoolFlag[ORGANIC_INDEX].value
			&& CMap::Map.Info.IsPointOnMap(unit.tilePos, unit.MapLayer)
			&& (unit.MapLayer->Field(unit.tilePos)->Flags & MapFieldDesert)
			&& unit.MapLayer->Field(unit.tilePos)->get_owner() != unit.Player
			&& unit.MapLayer->GetTimeOfDay()
			&& unit.MapLayer->GetTimeOfDay()->is_day()
			&& unit.Variable[HYDRATING_INDEX].Value <= 0
			&& unit.Variable[DEHYDRATIONIMMUNITY_INDEX].Value <= 0
		) {
			unit.Variable[DEHYDRATION_INDEX].Enable = 1;
			unit.Variable[DEHYDRATION_INDEX].Max = std::max(CYCLES_PER_SECOND + 1, unit.Variable[DEHYDRATION_INDEX].Max);
			unit.Variable[DEHYDRATION_INDEX].Value = std::max(CYCLES_PER_SECOND + 1, unit.Variable[DEHYDRATION_INDEX].Value);
		}
	}
	//Wyrmgus end

	//Wyrmgus start
	if (unit.Variable[TERROR_INDEX].Value > 0) { // if unit is terrified, flee at the sight of enemies
		std::vector<CUnit *> table;
		SelectAroundUnit(unit, unit.CurrentSightRange, table, IsAggresiveUnit(), true);
		for (size_t i = 0; i != table.size(); ++i) {
			if (unit.IsEnemy(*table[i])) {
				HitUnit_RunAway(unit, *table[i]);
				break;
			}
		}
	}
	//Wyrmgus end
}

/**
**  Handle the action of a unit.
**
**  @param unit  Pointer to handled unit.
*/
static void HandleUnitAction(CUnit &unit)
{
	// If current action is breakable proceed with next one.
	if (!unit.Anim.Unbreakable) {
		if (unit.CriticalOrder != nullptr) {
			unit.CriticalOrder->Execute(unit);
			unit.CriticalOrder.reset();
		}

		if (unit.Orders[0]->Finished && unit.Orders[0]->Action != UnitAction::Still
			&& unit.Orders.size() == 1) {

			unit.Orders[0] = COrder::NewActionStill();
			if (IsOnlySelected(unit)) { // update display for new action
				SelectedUnitChanged();
			}
		}

		// o Look if we have a new order and old finished.
		// o Or the order queue should be flushed.
		if ((unit.Orders[0]->Action == UnitAction::StandGround || unit.Orders[0]->Finished)
			&& unit.Orders.size() > 1) {
			if (unit.Removed && unit.Orders[0]->Action != UnitAction::Board) { // FIXME: johns I see this as an error
				DebugPrint("Flushing removed unit\n");
				// This happens, if building with ALT+SHIFT.
				return;
			}

			unit.Orders.erase(unit.Orders.begin());

			unit.Wait = 0;
			if (IsOnlySelected(unit)) { // update display for new action
				SelectedUnitChanged();
			}
		}
	}
	unit.Orders[0]->Execute(unit);
}

template <typename UNITP_ITERATOR>
static void UnitActionsEachSecond(UNITP_ITERATOR begin, UNITP_ITERATOR end)
{
	for (UNITP_ITERATOR it = begin; it != end; ++it) {
		CUnit &unit = **it;

		if (unit.Destroyed) {
			continue;
		}

		// OnEachSecond callback
		if (unit.Type->OnEachSecond  && unit.IsUnusable(false) == false) {
			unit.Type->OnEachSecond->pushPreamble();
			unit.Type->OnEachSecond->pushInteger(UnitNumber(unit));
			unit.Type->OnEachSecond->run();
		}

		// 1) Blink flag.
		if (unit.Blink) {
			--unit.Blink;
		}
		// 2) Buffs...
		HandleBuffsEachSecond(unit);
	}
}

template <typename UNITP_ITERATOR>
static void UnitActionsEachFiveSeconds(UNITP_ITERATOR begin, UNITP_ITERATOR end)
{
	for (UNITP_ITERATOR it = begin; it != end; ++it) {
		CUnit &unit = **it;

		if (unit.Destroyed) {
			continue;
		}

		//if the unit is garrisoned within a building that provides garrison training, increase its XP
		if (unit.Container && unit.Container->Type->BoolFlag[GARRISONTRAINING_INDEX].value) {
			unit.ChangeExperience(1);
		}
	}
}

static void DumpUnitInfo(CUnit &unit)
{
	// Dump the unit to find the network sync bugs.
	static FILE *logf = nullptr;

	if (!logf) {
		time_t now;
		char buf[256];

		//TODO should the filename be changed to reflect the new engine name?
		snprintf(buf, sizeof(buf), "log_of_stratagus_%d.log", CPlayer::GetThisPlayer()->Index);
		logf = fopen(buf, "wb");
		if (!logf) {
			return ;
		}
		fprintf(logf, "; Log file generated by " NAME " Version " VERSION "\n");
		time(&now);
		fprintf(logf, ";\tDate: %s", ctime(&now));
		fprintf(logf, ";\tMap: %s\n\n", CMap::Map.Info.Description.c_str());
	}

	fprintf(logf, "%lu: ", GameCycle);
	fprintf(logf, "%d %s %d P%d Refs %d: %X %d,%d %d,%d\n",
			UnitNumber(unit), unit.Type ? unit.Type->Ident.c_str() : "unit-killed",
			!unit.Orders.empty() ? static_cast<int>(unit.CurrentAction()) : -1,
			unit.Player ? unit.Player->Index : -1, unit.get_ref_count(), wyrmgus::random::get()->get_seed(),
			unit.tilePos.x, unit.tilePos.y, unit.get_pixel_offset().x(), unit.get_pixel_offset().y());
#if 0
	SaveUnit(unit, logf);
#endif
	fflush(nullptr);
}

template <typename UNITP_ITERATOR>
static void UnitActionsEachCycle(UNITP_ITERATOR begin, UNITP_ITERATOR end)
{
	for (UNITP_ITERATOR it = begin; it != end; ++it) {
		CUnit &unit = **it;

		if (unit.Destroyed) {
			continue;
		}

		if (!ReplayRevealMap && unit.Selected && !unit.IsVisible(*CPlayer::GetThisPlayer())) {
			UnSelectUnit(unit);
			SelectionChanged();
		}

		// OnEachCycle callback
		if (unit.Type->OnEachCycle && unit.IsUnusable(false) == false) {
			unit.Type->OnEachCycle->pushPreamble();
			unit.Type->OnEachCycle->pushInteger(UnitNumber(unit));
			unit.Type->OnEachCycle->run();
		}

		// Handle each cycle buffs
		HandleBuffsEachCycle(unit);
		// Unit could be dead after TTL kill
		if (unit.Destroyed) {
			continue;
		}

		try {
			HandleUnitAction(unit);
		} catch (AnimationDie_Exception &) {
			AnimationDie_OnCatch(unit);
		}

		if (EnableUnitDebug) {
			DumpUnitInfo(unit);
		}
		// Calculate some hash.
		SyncHash = (SyncHash << 5) | (SyncHash >> 27);
		SyncHash ^= unit.Orders.empty() == false ? static_cast<int>(unit.CurrentAction()) << 18 : 0;
		SyncHash ^= unit.get_ref_count() << 3;
	}
}

//Wyrmgus start
template <typename UNITP_ITERATOR>
static void UnitActionsEachMinute(UNITP_ITERATOR begin, UNITP_ITERATOR end)
{
	for (UNITP_ITERATOR it = begin; it != end; ++it) {
		CUnit &unit = **it;

		if (unit.Destroyed) {
			continue;
		}

		unit.UpdateSoldUnits();
		
		for (size_t i = 0; i < unit.Type->SpawnUnits.size(); ++i) {
			wyrmgus::unit_type *spawned_type = unit.Type->SpawnUnits[i];
			int spawned_type_demand = spawned_type->Stats[unit.Player->Index].Variables[DEMAND_INDEX].Value;
			if ((GameCycle % (CYCLES_PER_MINUTE * spawned_type_demand)) == 0) { //the quantity of minutes it takes to spawn the unit depends on the unit's supply demand
				if ((unit.Player->GetUnitTypeCount(spawned_type) * spawned_type_demand) >= (unit.Player->GetUnitTypeCount(unit.Type) * 5)) { //max limit reached
					continue;
				}
				CUnit *spawned_unit = MakeUnit(*spawned_type, unit.Player);
				DropOutOnSide(*spawned_unit, spawned_unit->Direction, &unit);
			}
		}
	}
}
//Wyrmgus end

/**
**  Update the actions of all units each game cycle/second.
*/
void UnitActions()
{
	const bool isASecondCycle = !(GameCycle % CYCLES_PER_SECOND);

	//unit list may be modified during loop... so make a copy
	std::vector<CUnit *> table = wyrmgus::unit_manager::get()->get_units();

	//check for things that only happen every second
	if (isASecondCycle) {
		UnitActionsEachSecond(table.begin(), table.end());
	}
	
	if ((GameCycle % (CYCLES_PER_SECOND * 5)) == 0) {
		UnitActionsEachFiveSeconds(table.begin(), table.end());
	}
	// Do all actions
	UnitActionsEachCycle(table.begin(), table.end());
	
	//Wyrmgus start
	if ((GameCycle % CYCLES_PER_MINUTE) == 0) {
		UnitActionsEachMinute(table.begin(), table.end());
	}
	//Wyrmgus end
}
