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
/**@name action_defend.cpp - The defend action. */
//
//      (c) Copyright 2012-2019 by cybermind and Andrettin
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

#include "action/action_defend.h"

#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "pathfinder.h"
#include "script.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unittype.h"
#include "unit/unit_type_type.h"
#include "video.h"

enum {
	State_Init = 0,
	State_MovingToTarget,
	State_Defending
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder *COrder::NewActionDefend(CUnit &dest)
{
	COrder_Defend *order = new COrder_Defend();

	if (dest.Destroyed) {
		order->goalPos = dest.tilePos + dest.GetHalfTileSize();
		order->MapLayer = dest.MapLayer->ID;
	} else {
		order->SetGoal(&dest);
		order->Range = 1;
	}
	return order;
}

/* virtual */ void COrder_Defend::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-defend\",");

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

/* virtual */ bool COrder_Defend::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
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

/* virtual */ bool COrder_Defend::IsValid() const
{
	return true;
}

/* virtual */ PixelPos COrder_Defend::Show(const CViewport &vp, const PixelPos &lastScreenPos) const
{
	PixelPos targetPos;

	if (this->HasGoal()) {
		//Wyrmgus start
		if (this->GetGoal()->MapLayer != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.MapToScreenPixelPos(this->GetGoal()->GetMapPixelPosCenter());
	} else {
		//Wyrmgus start
		if (this->MapLayer != UI.CurrentMapLayer->ID) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}
	//Wyrmgus start
//	Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
//	Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
//	Video.FillCircleClip(ColorOrange, targetPos, 3);
	if (Preference.ShowPathlines) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
		Video.FillCircleClip(ColorOrange, targetPos, 3);
	}
	//Wyrmgus end
	return targetPos;
}

/* virtual */ void COrder_Defend::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);

	Vec2i tileSize;
	if (this->HasGoal()) {
		CUnit *goal = this->GetGoal();
		tileSize = goal->GetTileSize();
		input.SetGoal(goal->tilePos, tileSize, goal->MapLayer->ID);
	} else {
		tileSize.x = 0;
		tileSize.y = 0;
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}


/* virtual */ void COrder_Defend::Execute(CUnit &unit)
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

	if (this->State == State_Init) {
		if (!goal || !goal->IsVisibleAsGoal(*unit.Player)) {
			this->Finished = true;
			return;
		}
		this->State = State_MovingToTarget;
	} else if (this->State == State_Defending) {
		if (!goal || !goal->IsVisibleAsGoal(*unit.Player)) {
			this->Finished = true;
			return;
		}
	}

	if (!unit.Anim.Unbreakable) {
		if (AutoCast(unit) || AutoAttack(unit) || AutoRepair(unit)) {
			return;
		}
	}

	switch (DoActionMove(unit)) {
		case PF_UNREACHABLE:
			//Wyrmgus start
			//if is unreachable and is on a raft, see if the raft can move closer to the enemy
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
			//Wyrmgus end
			// Some tries to reach the goal
			this->Range++;
			break;
		case PF_REACHED: {
			if (!goal || !goal->IsVisibleAsGoal(*unit.Player)) { // goal has died
				this->Finished = true;
				return;
			}

			// Now defend the goal
			this->goalPos = goal->tilePos;
			this->MapLayer = goal->MapLayer->ID;
			this->State = State_Defending;
		}
		default:
			break;
	}

	// Target destroyed?
	if (goal && !goal->IsVisibleAsGoal(*unit.Player)) {
		DebugPrint("Goal gone\n");
		this->goalPos = goal->tilePos + goal->GetHalfTileSize();
		this->MapLayer = goal->MapLayer->ID;
		this->ClearGoal();
		goal = nullptr;
		if (this->State == State_Defending) {
			this->Finished = true;
			return;
		}
	}
}
