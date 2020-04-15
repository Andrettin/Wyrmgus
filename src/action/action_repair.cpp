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
/**@name action_repair.cpp - The repair action. */
//
//      (c) Copyright 1999-2020 by Vladi Shabanski, Jimmy Salmon and Andrettin
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

#include "action/action_repair.h"

#include "action/action_built.h"
#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "pathfinder.h"
#include "player.h"
#include "script.h"
#include "sound/sound.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_type.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder *COrder::NewActionRepair(CUnit &unit, CUnit &target)
{
	COrder_Repair *order = new COrder_Repair();

	if (target.Destroyed) {
		order->goalPos = target.tilePos + target.GetHalfTileSize();
		order->MapLayer = target.MapLayer->ID;
	} else {
		order->SetGoal(&target);
		order->ReparableTarget = &target;
	}
	return order;
}

//Wyrmgus start
///* static */ COrder *COrder::NewActionRepair(const Vec2i &pos)
/* static */ COrder *COrder::NewActionRepair(const Vec2i &pos, int z)
//Wyrmgus end
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));

	COrder_Repair *order = new COrder_Repair;

	order->goalPos = pos;
	//Wyrmgus start
	order->MapLayer = z;
	//Wyrmgus end
	return order;
}

/* virtual */ void COrder_Repair::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-repair\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	if (this->HasGoal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->GetGoal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	if (this->ReparableTarget != nullptr) {
		file.printf(" \"repair-target\", \"%s\",", UnitReference(this->GetReparableTarget()).c_str());
	}

	file.printf(" \"repaircycle\", %d,", this->RepairCycle);
	file.printf(" \"state\", %d", this->State);

	file.printf("}");
}

/* virtual */ bool COrder_Repair::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp("repaircycle", value)) {
		++j;
		this->RepairCycle = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp("repair-target", value)) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->ReparableTarget = CclGetUnitFromRef(l);
		lua_pop(l, 1);
	} else if (!strcmp("state", value)) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "tile")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->goalPos.x , &this->goalPos.y);
		lua_pop(l, 1);
	//Wyrmgus start
	} else if (!strcmp(value, "map-layer")) {
		++j;
		this->MapLayer = LuaToNumber(l, -1, j + 1);
	//Wyrmgus end
	} else {
		return false;
	}
	return true;
}

/* virtual */ bool COrder_Repair::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_Repair::Show(const CViewport &vp, const PixelPos &lastScreenPos) const
{
	PixelPos targetPos;

	if (this->ReparableTarget != nullptr) {
		if (this->ReparableTarget->MapLayer != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		targetPos = vp.MapToScreenPixelPos(this->ReparableTarget->GetMapPixelPosCenter());
	} else {
		if (this->MapLayer != UI.CurrentMapLayer->ID) {
			return lastScreenPos;
		}
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}

	if (Preference.ShowPathlines) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
		Video.FillCircleClip(ColorYellow, targetPos, 3);
	}

	return targetPos;
}

/* virtual */ void COrder_Repair::UpdatePathFinderData(PathFinderInput &input)
{
	const CUnit &unit = *input.GetUnit();

	input.SetMinRange(0);
	input.SetMaxRange(ReparableTarget != nullptr ? unit.Type->RepairRange : 0);

	Vec2i tileSize;
	if (ReparableTarget != nullptr) {
		tileSize = ReparableTarget->GetTileSize();
		input.SetGoal(ReparableTarget->tilePos, tileSize, ReparableTarget->MapLayer->ID);
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}

bool SubRepairCosts(const CUnit &unit, CPlayer &player, CUnit &goal)
{
	int RepairCosts[MaxCosts];

	// Check if enough resources are available
	for (int i = 1; i < MaxCosts; ++i) {
		RepairCosts[i] = goal.Type->RepairCosts[i] *
						 (goal.CurrentAction() == UnitAction::Built ? ResourcesMultiBuildersMultiplier : 1);
	}
	for (int i = 1; i < MaxCosts; ++i) {
		if (!player.CheckResource(i, RepairCosts[i])) {
			player.Notify(NotifyYellow, unit.tilePos,
						  //Wyrmgus start
						  unit.MapLayer->ID,
//						  _("We need more %s for repair!"), DefaultResourceNames[i].c_str());
						  _("We need more %s to repair!"), DefaultResourceNames[i].c_str());
						  //Wyrmgus end
			return true;
		}
	}

	// Subtract the resources
	player.SubCosts(RepairCosts);
	return false;
}

/**
**  Repair a unit.
**
**  @param unit  unit repairing
**  @param goal  unit being repaired
**
**  @return true when action is finished/canceled.
*/
bool COrder_Repair::RepairUnit(const CUnit &unit, CUnit &goal)
{
	CPlayer &player = *unit.Player;

	if (goal.CurrentAction() == UnitAction::Built) {
		COrder_Built &order = *static_cast<COrder_Built *>(goal.CurrentOrder());

		order.ProgressHp(goal, 100 * this->RepairCycle);
		this->RepairCycle = 0;
		if (ResourcesMultiBuildersMultiplier && SubRepairCosts(unit, player, goal)) {
			return true;
		}
		return false;
	}
	//Wyrmgus start
//	if (goal.Variable[HP_INDEX].Value >= goal.Variable[HP_INDEX].Max) {
	if (goal.Variable[HP_INDEX].Value >= goal.GetModifiedVariable(HP_INDEX, VariableMax)) {
	//Wyrmgus end
		return true;
	}

	Assert(goal.Stats->Variables[HP_INDEX].Max);

	if (SubRepairCosts(unit, player, goal)) {
		return true;
	}

	goal.Variable[HP_INDEX].Value += goal.Type->RepairHP;
	//Wyrmgus start
//	if (goal.Variable[HP_INDEX].Value >= goal.Variable[HP_INDEX].Max) {
	if (goal.Variable[HP_INDEX].Value >= goal.GetModifiedVariable(HP_INDEX, VariableMax)) {
	//Wyrmgus end
		//Wyrmgus start
//		goal.Variable[HP_INDEX].Value = goal.Variable[HP_INDEX].Max;
		goal.Variable[HP_INDEX].Value = goal.GetModifiedVariable(HP_INDEX, VariableMax);
		//Wyrmgus end
		return true;
	}
	return false;
}

/**
**  Animate unit repair
**
**  @param unit  Unit, for that the repair animation is played.
*/
static void AnimateActionRepair(CUnit &unit)
{
	//Wyrmgus start
//	UnitShowAnimation(unit, unit.Type->Animations->Repair);
	UnitShowAnimation(unit, unit.GetAnimations()->Repair);
	//Wyrmgus end
}

/* virtual */ void COrder_Repair::Execute(CUnit &unit)
{
	Assert(this->ReparableTarget == this->GetGoal());

	switch (this->State) {
		case 0:
			this->State = 1;
		// FALL THROUGH
		case 1: { // Move near to target.
			// FIXME: RESET FIRST!! Why? We move first and than check if
			// something is in sight.
			int err = DoActionMove(unit);
			if (!unit.Anim.Unbreakable) {
				// No goal: if meeting damaged building repair it.
				CUnit *goal = this->GetGoal();

				if (goal) {
					if (!goal->IsVisibleAsGoal(*unit.Player)) {
						DebugPrint("repair target gone.\n");
						this->goalPos = goal->tilePos + goal->GetHalfTileSize();
						this->MapLayer = goal->MapLayer->ID;
						ReparableTarget = nullptr;
						this->ClearGoal();
						goal = nullptr;
					}
				} else if (unit.Player->AiEnabled) {
					// Ai players workers should stop if target is killed
					err = -1;
				}

				// Have reached target? FIXME: could use return value
				if (goal && unit.MapDistanceTo(*goal) <= unit.Type->RepairRange
					//Wyrmgus start
//					&& goal->Variable[HP_INDEX].Value < goal->Variable[HP_INDEX].Max) {
					&& goal->Variable[HP_INDEX].Value < goal->GetModifiedVariable(HP_INDEX, VariableMax)) {
					//Wyrmgus end
					this->State = 2;
					this->RepairCycle = 0;
					//Wyrmgus start
//					const Vec2i dir = goal->tilePos + goal->Type->GetHalfTileSize() - unit.tilePos;
					const Vec2i dir = PixelSize(PixelSize(goal->tilePos) * stratagus::defines::get()->get_tile_size()) + goal->GetHalfTilePixelSize() - PixelSize(PixelSize(unit.tilePos) * stratagus::defines::get()->get_tile_size()) - unit.GetHalfTilePixelSize();
					//Wyrmgus end
					UnitHeadingFromDeltaXY(unit, dir);
				} else if (err < 0) {
					//Wyrmgus start
					//if is unreachable and is on a raft, see if the raft can move closer
					if (err == PF_UNREACHABLE) {
						if ((unit.MapLayer->Field(unit.tilePos)->Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeType::Land) {
							std::vector<CUnit *> table;
							Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
							for (size_t i = 0; i != table.size(); ++i) {
								if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
									if (table[i]->CurrentAction() == UnitAction::Still) {
										CommandStopUnit(*table[i]);
										CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->tilePos : this->goalPos, FlushCommands, this->HasGoal() ? this->GetGoal()->MapLayer->ID : this->MapLayer);
									}
									return;
								}
							}
						}
					}
					//Wyrmgus end
					this->Finished = true;
					return ;
				}
			}
			break;
		}
		case 2: {// Repair the target.
			AnimateActionRepair(unit);
			this->RepairCycle++;
			if (unit.Anim.Unbreakable) {
				return ;
			}
			CUnit *goal = this->GetGoal();

			if (goal) {
				if (!goal->IsVisibleAsGoal(*unit.Player)) {
					DebugPrint("repair goal is gone\n");
					this->goalPos = goal->tilePos + goal->GetHalfTileSize();
					this->MapLayer = goal->MapLayer->ID;
					// FIXME: should I clear this here?
					this->ClearGoal();
					ReparableTarget = nullptr;
					goal = nullptr;
				} else {
					const int dist = unit.MapDistanceTo(*goal);

					if (dist <= unit.Type->RepairRange) {
						if (RepairUnit(unit, *goal)) {
							this->Finished = true;
							return ;
						}
					} else if (dist > unit.Type->RepairRange) {
						// If goal has move, chase after it
						this->State = 0;
					}
				}
			}
			// Target is fine, choose new one.
			//Wyrmgus start
//			if (!goal || goal->Variable[HP_INDEX].Value >= goal->Variable[HP_INDEX].Max) {
			if (!goal || goal->Variable[HP_INDEX].Value >= goal->GetModifiedVariable(HP_INDEX, VariableMax)) {
			//Wyrmgus end
				this->Finished = true;
				return ;
			}
			// FIXME: automatic repair
		}
		break;
	}
}
