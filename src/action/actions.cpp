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
/**@name actions.cpp - The actions source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Russell Smith, Jimmy Salmon and Andrettin
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

#include <time.h>

#include "stratagus.h"
#include "include/version.h"

#include "action/actions.h"

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
#include "dependency/dependency.h"
#include "luacallback.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "missile/missile.h"
#include "pathfinder/pathfinder.h"
#include "player.h"
#include "script.h"
#include "spell/spells.h"
#include "time/time_of_day.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"

#include <core/math/random_number_generator.h>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

uint64_t SyncHash; /// Hash calculated to find sync failures

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

COrder::~COrder()
{
	Goal.Reset();
}

void COrder::SetGoal(CUnit *const new_goal)
{
	Goal = new_goal;
}

void COrder::ClearGoal()
{
	Goal.Reset();
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
	unit.Seen.State = ((Action == UnitActionUpgradeTo) << 1);
	if (unit.CurrentAction() == UnitActionDie) {
		unit.Seen.State = 3;
	}
	unit.Seen.CFrame = nullptr;
}

/* virtual */ bool COrder::OnAiHitUnit(CUnit &unit, CUnit *attacker, int /*damage*/)
{
	return false;
}


/** Called when unit is killed.
**  warn the AI module.
*/
/* virtual */ void COrder::AiUnitKilled(CUnit &unit)
{
	switch (Action) {
		case UnitActionStill:
		case UnitActionAttack:
		case UnitActionMove:
			break;
		default:
			DebugPrint("FIXME: %i: %d(%s) killed, with order %d!\n" _C_
					   unit.Player->GetIndex() _C_ UnitNumber(unit) _C_
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
	unit.StepCount = 0;
	// Fixme : Auto select position to attack ?
}

/**
**  Get goal position
*/
/* virtual */ const Vec2i COrder::GetGoalPos() const
{
	const Vec2i invalidPos(-1, -1);
	if (this->HasGoal()) {
		return this->GetGoal()->tilePos;
	}
	return invalidPos;
}

//Wyrmgus start
/**
**  Get goal map layer
*/
/* virtual */ const int COrder::GetGoalMapLayer() const
{
	if (this->HasGoal()) {
		return this->GetGoal()->MapLayer->ID;
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
void CclParseOrder(lua_State *l, CUnit &unit, COrderPtr *orderPtr)
{
	const int args = lua_rawlen(l, -1);
	const char *actiontype = LuaToString(l, -1, 1);

	if (!strcmp(actiontype, "action-attack")) {
		*orderPtr = new COrder_Attack(false);
	} else if (!strcmp(actiontype, "action-attack-ground")) {
		*orderPtr = new COrder_Attack(true);
	} else if (!strcmp(actiontype, "action-board")) {
		*orderPtr = new COrder_Board;
	} else if (!strcmp(actiontype, "action-build")) {
		*orderPtr = new COrder_Build;
	} else if (!strcmp(actiontype, "action-built")) {
		*orderPtr = new COrder_Built;
	} else if (!strcmp(actiontype, "action-defend")) {
		*orderPtr = new COrder_Defend;
	} else if (!strcmp(actiontype, "action-die")) {
		*orderPtr = new COrder_Die;
	} else if (!strcmp(actiontype, "action-follow")) {
		*orderPtr = new COrder_Follow;
	} else if (!strcmp(actiontype, "action-move")) {
		*orderPtr = new COrder_Move;
	} else if (!strcmp(actiontype, "action-patrol")) {
		*orderPtr = new COrder_Patrol;
	} else if (!strcmp(actiontype, "action-pick-up")) {
		*orderPtr = new COrder_PickUp;
	} else if (!strcmp(actiontype, "action-repair")) {
		*orderPtr = new COrder_Repair;
	} else if (!strcmp(actiontype, "action-research")) {
		*orderPtr = new COrder_Research;
	} else if (!strcmp(actiontype, "action-resource")) {
		*orderPtr = new COrder_Resource(unit);
	} else if (!strcmp(actiontype, "action-spell-cast")) {
		*orderPtr = new COrder_SpellCast;
	} else if (!strcmp(actiontype, "action-stand-ground")) {
		*orderPtr = new COrder_Still(true);
	} else if (!strcmp(actiontype, "action-still")) {
		*orderPtr = new COrder_Still(false);
	} else if (!strcmp(actiontype, "action-train")) {
		*orderPtr = new COrder_Train;
	} else if (!strcmp(actiontype, "action-transform-into")) {
		*orderPtr = new COrder_TransformInto;
	} else if (!strcmp(actiontype, "action-upgrade-to")) {
		*orderPtr = new COrder_UpgradeTo;
	} else if (!strcmp(actiontype, "action-unload")) {
		*orderPtr = new COrder_Unload;
	} else if (!strcmp(actiontype, "action-use")) {
		*orderPtr = new COrder_Use;
	//Wyrmgus start
	} else if (!strcmp(actiontype, "action-trade")) {
		*orderPtr = new COrder_Trade;
	//Wyrmgus end
	} else {
		LuaError(l, "ParseOrder: Unsupported type: %s" _C_ actiontype);
	}

	COrder &order = **orderPtr;

	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, -1, j + 1);

		if (order.ParseGenericData(l, j, value)) {
			continue;
		} else if (order.ParseSpecificData(l, j, value, unit)) {
			continue;
		} else {
			// This leaves a half initialized unit
			LuaError(l, "ParseOrder: Unsupported tag: %s" _C_ value);
		}
	}
}


/*----------------------------------------------------------------------------
--  Actions
----------------------------------------------------------------------------*/

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
		unit.HandleBuffsEachSecond();
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
		snprintf(buf, sizeof(buf), "log_of_stratagus_%i.log", CPlayer::GetThisPlayer()->GetIndex());
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
	fprintf(logf, "%d %s %d P%i Refs %d: %llX %d,%d %d,%d\n",
			UnitNumber(unit), unit.Type ? unit.Type->Ident.c_str() : "unit-killed",
			!unit.Orders.empty() ? unit.CurrentAction() : -1,
			unit.Player ? unit.Player->GetIndex() : -1, unit.Refs, RNG->get_seed(),
			unit.tilePos.x, unit.tilePos.y, unit.IX, unit.IY);
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

		// Handle each cycle buffs
		unit.HandleBuffsEachCycle();
		// Unit could be dead after TTL kill
		if (unit.Destroyed) {
			continue;
		}

		try {
			unit.HandleUnitAction();
		} catch (AnimationDie_Exception &) {
			AnimationDie_OnCatch(unit);
		}

		if (EnableUnitDebug) {
			DumpUnitInfo(unit);
		}
		// Calculate some hash.
		SyncHash = (SyncHash << 5) | (SyncHash >> 27);
		SyncHash ^= unit.Orders.empty() == false ? unit.CurrentAction() << 18 : 0;
		SyncHash ^= unit.Refs << 3;
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
			CUnitType *spawned_type = unit.Type->SpawnUnits[i];
			int spawned_type_demand = spawned_type->Stats[unit.Player->GetIndex()].Variables[DEMAND_INDEX].Value;
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
	// Unit list may be modified during loop... so make a copy
	std::vector<CUnit *> table(UnitManager.begin(), UnitManager.end());

	// Check for things that only happen every second
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
