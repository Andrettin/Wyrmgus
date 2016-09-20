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
/**@name action_follow.cpp - The follow action. */
//
//      (c) Copyright 2001-2015 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_follow.h"

#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "iolib.h"
#include "luacallback.h"
#include "missile.h"
#include "pathfinder.h"
#include "script.h"
//Wyrmgus start
#include "tileset.h"
//Wyrmgus end
#include "ui.h"
#include "unit.h"
#include "unit_find.h"
#include "unittype.h"
#include "video.h"

enum {
	State_Init = 0,
	State_Initialized = 1,

	State_TargetReached = 128,
};



/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder *COrder::NewActionFollow(CUnit &dest)
{
	COrder_Follow *order = new COrder_Follow;

	// Destination could be killed.
	// Should be handled in action, but is not possible!
	// Unit::Refs is used as timeout counter.
	if (dest.Destroyed) {
		order->goalPos = dest.tilePos + dest.Type->GetHalfTileSize();
		//Wyrmgus start
		order->MapLayer = dest.MapLayer;
		//Wyrmgus end
	} else {
		order->SetGoal(&dest);
		order->Range = 1;
	}
	return order;
}

/* virtual */ void COrder_Follow::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-follow\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->HasGoal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->GetGoal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	file.printf(" \"state\", %d", this->State);

	file.printf("}");
}

/* virtual */ bool COrder_Follow::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp(value, "state")) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "range")) {
		++j;
		this->Range = LuaToNumber(l, -1, j + 1);
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

/* virtual */ bool COrder_Follow::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_Follow::Show(const CViewport &vp, const PixelPos &lastScreenPos) const
{
	PixelPos targetPos;

	if (this->HasGoal()) {
		//Wyrmgus start
		if (this->GetGoal()->MapLayer != CurrentMapLayer) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.MapToScreenPixelPos(this->GetGoal()->GetMapPixelPosCenter());
	} else {
		//Wyrmgus start
		if (this->MapLayer != CurrentMapLayer) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}
	//Wyrmgus start
//	Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
//	Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
//	Video.FillCircleClip(ColorGreen, targetPos, 3);
	if (Preference.ShowPathlines) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
		Video.FillCircleClip(ColorGreen, targetPos, 3);
	}
	//Wyrmgus end
	return targetPos;
}

/* virtual */ void COrder_Follow::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);

	Vec2i tileSize;
	if (this->HasGoal()) {
		CUnit *goal = this->GetGoal();
		tileSize.x = goal->Type->TileWidth;
		tileSize.y = goal->Type->TileHeight;
		//Wyrmgus start
//		input.SetGoal(goal->tilePos, tileSize);
		input.SetGoal(goal->tilePos, tileSize, goal->MapLayer);
		//Wyrmgus end
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		//Wyrmgus start
//		input.SetGoal(this->goalPos, tileSize);
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
		//Wyrmgus end
	}
}


/* virtual */ void COrder_Follow::Execute(CUnit &unit)
{
	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		//Wyrmgus start
//		UnitShowAnimation(unit, unit.Type->Animations->Still);
		UnitShowAnimation(unit, unit.GetAnimations()->Still);
		//Wyrmgus end
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}
	CUnit *goal = this->GetGoal();

	// Reached target
	if (this->State == State_TargetReached) {

		if (!goal || !goal->IsVisibleAsGoal(*unit.Player)) {
			DebugPrint("Goal gone\n");
			this->Finished = true;
			return ;
		}

		// Don't follow after immobile units
		if (goal && goal->CanMove() == false) {
			this->Finished = true;
			return;
		}

		//Wyrmgus start
//		if (goal->tilePos == this->goalPos) {
		if (goal->tilePos == this->goalPos && goal->MapLayer == this->MapLayer) {
		//Wyrmgus end
			// Move to the next order
			if (unit.Orders.size() > 1) {
				this->Finished = true;
				return ;
			}

			unit.Wait = 10;
			if (this->Range > 1) {
				this->Range = 1;
				this->State = State_Init;
			}
			return ;
		}
		this->State = State_Init;
	}
	if (this->State == State_Init) { // first entry
		this->State = State_Initialized;
	}
	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			//Wyrmgus start
			if ((Map.Field(unit.tilePos, unit.MapLayer)->Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeLand) {
				std::vector<CUnit *> table;
				Select(unit.tilePos, unit.tilePos, table);
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitActionStill) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->tilePos : this->goalPos, FlushCommands, this->HasGoal() ? this->GetGoal()->MapLayer : this->MapLayer);
						}
						return;
					}
				}
			}
			//Wyrmgus end
			// Some tries to reach the goal
			this->Range++;
			break;
		case PF_REACHED: {
			if (!goal) { // goal has died
				this->Finished = true;
				return ;
			}
			// Handle Teleporter Units
			// FIXME: BAD HACK
			// goal shouldn't be busy and portal should be alive
			if (goal->Type->BoolFlag[TELEPORTER_INDEX].value && goal->Goal && goal->Goal->IsAlive() && unit.MapDistanceTo(*goal) <= 1) {
				if (!goal->IsIdle()) { // wait
					unit.Wait = 10;
					return;
				}
				// Check if we have enough mana
				if (goal->Goal->Type->TeleportCost > goal->Variable[MANA_INDEX].Value) {
					this->Finished = true;
					return;
				} else {
					goal->Variable[MANA_INDEX].Value -= goal->Goal->Type->TeleportCost;
				}
				// Everything is OK, now teleport the unit
				unit.Remove(NULL);
				if (goal->Type->TeleportEffectIn) {
					goal->Type->TeleportEffectIn->pushPreamble();
					goal->Type->TeleportEffectIn->pushInteger(UnitNumber(unit));
					goal->Type->TeleportEffectIn->pushInteger(UnitNumber(*goal));
					goal->Type->TeleportEffectIn->pushInteger(unit.GetMapPixelPosCenter().x);
					goal->Type->TeleportEffectIn->pushInteger(unit.GetMapPixelPosCenter().y);
					goal->Type->TeleportEffectIn->run();
				}
				unit.tilePos = goal->Goal->tilePos;
				//Wyrmgus start
				unit.MapLayer = goal->Goal->MapLayer;
				//Wyrmgus end
				DropOutOnSide(unit, unit.Direction, NULL);

				// FIXME: we must check if the units supports the new order.
				CUnit &dest = *goal->Goal;
				if (dest.Type->TeleportEffectOut) {
					dest.Type->TeleportEffectOut->pushPreamble();
					dest.Type->TeleportEffectOut->pushInteger(UnitNumber(unit));
					dest.Type->TeleportEffectOut->pushInteger(UnitNumber(dest));
					dest.Type->TeleportEffectOut->pushInteger(unit.GetMapPixelPosCenter().x);
					dest.Type->TeleportEffectOut->pushInteger(unit.GetMapPixelPosCenter().y);
					dest.Type->TeleportEffectOut->run();
				}

				if (dest.NewOrder == NULL
					|| (dest.NewOrder->Action == UnitActionResource && !unit.Type->BoolFlag[HARVESTER_INDEX].value)
					//Wyrmgus start
//					|| (dest.NewOrder->Action == UnitActionAttack && !unit.Type->CanAttack)
					|| (dest.NewOrder->Action == UnitActionAttack && !unit.CanAttack(true))
					//Wyrmgus end
					|| (dest.NewOrder->Action == UnitActionBoard && unit.Type->UnitType != UnitTypeLand)) {
					this->Finished = true;
					return ;
				} else {
					if (dest.NewOrder->HasGoal()) {
						if (dest.NewOrder->GetGoal()->Destroyed) {
							delete dest.NewOrder;
							dest.NewOrder = NULL;
							this->Finished = true;
							return ;
						}
						unit.Orders.insert(unit.Orders.begin() + 1, dest.NewOrder->Clone());
						this->Finished = true;
						return ;
					}
				}
			}
			this->goalPos = goal->tilePos;
			//Wyrmgus start
			this->MapLayer = goal->MapLayer;
			//Wyrmgus end
			this->State = State_TargetReached;
		}
		// FALL THROUGH
		default:
			break;
	}

	// Target destroyed?
	if (goal && !goal->IsVisibleAsGoal(*unit.Player)) {
		DebugPrint("Goal gone\n");
		this->goalPos = goal->tilePos + goal->Type->GetHalfTileSize();
		//Wyrmgus start
		this->MapLayer = goal->MapLayer;
		//Wyrmgus end
		this->ClearGoal();
		goal = NULL;
	}

	if (unit.Anim.Unbreakable) {
		return ;
	}
	// If our leader is dead or stops or attacks:
	// Attack any enemy in reaction range.
	// If don't set the goal, the unit can than choose a
	//  better goal if moving nearer to enemy.
	//Wyrmgus start
//	if (unit.Type->CanAttack
	if (unit.CanAttack()
	//Wyrmgus end
		&& (!goal || goal->CurrentAction() == UnitActionAttack || goal->CurrentAction() == UnitActionStill)) {
		CUnit *target = AttackUnitsInReactRange(unit);
		if (target) {
			// Save current command to come back.
			COrder *savedOrder = NULL;
			if (unit.CanStoreOrder(unit.CurrentOrder())) {
				savedOrder = this->Clone();
			}

			this->Finished = true;
			//Wyrmgus start
//			unit.Orders.insert(unit.Orders.begin() + 1, COrder::NewActionAttack(unit, target->tilePos));
			unit.Orders.insert(unit.Orders.begin() + 1, COrder::NewActionAttack(unit, target->tilePos, target->MapLayer));
			//Wyrmgus end

			if (savedOrder != NULL) {
				unit.SavedOrder = savedOrder;
			}
		}
	}
}

//@}
