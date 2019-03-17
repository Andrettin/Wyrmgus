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
/**@name action_board.cpp - The board action. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_board.h"

#include "animation/animation.h"
#include "commands.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "pathfinder/pathfinder.h"
#include "player.h"
#include "script.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "video/video.h"

enum {
	State_Init = 0,
	State_MoveToTransporterMax = 200, // Range from previous
	State_WaitForTransporter = 201,
	State_EnterTransporter = 202
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/* static */ COrder *COrder::NewActionBoard(CUnit &unit)
{
	COrder_Board *order = new COrder_Board;

	order->SetGoal(&unit);
	order->Range = 1;

	return order;
}

/* virtual */ void COrder_Board::Save(CFile &file, const CUnit &unit) const
{
	file.printf("{\"action-board\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->HasGoal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->GetGoal()).c_str());
	}
	file.printf(" \"state\", %d", this->State);

	file.printf("}");
}

/* virtual */ bool COrder_Board::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	if (!strcmp("state", value)) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp("range", value)) {
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

/* virtual */ bool COrder_Board::IsValid() const
{
	return this->HasGoal() && this->GetGoal()->IsAliveOnMap();
}

/* virtual */ PixelPos COrder_Board::Show(const CViewport &vp, const PixelPos &lastScreenPos) const
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
//	Video.FillCircleClip(ColorGreen, targetPos, 3);
	if (Preference.ShowPathlines) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, 2);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos);
		Video.FillCircleClip(ColorGreen, targetPos, 3);
	}
	//Wyrmgus end
	return targetPos;
}

/* virtual */ void COrder_Board::UpdatePathFinderData(PathFinderInput &input)
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
		//Wyrmgus start
//		input.SetGoal(this->goalPos, tileSize);
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
		//Wyrmgus end
	}
}


/**
**  Move to transporter.
**
**  @param unit  Pointer to unit, moving to transporter.
**
**  @return      >0 remaining path length, 0 wait for path, -1
**               reached goal, -2 can't reach the goal.
*/
int COrder_Board::MoveToTransporter(CUnit &unit)
{
	const Vec2i oldPos = unit.tilePos;
	const int res = DoActionMove(unit);

	// We have to reset a lot, or else they will circle each other and stuff.
	if (oldPos != unit.tilePos) {
		this->Range = 1;
	}
	return res;
}

/**
**  Wait for transporter.
**
**  @param unit  Pointer to unit.
**
**  @return      True if ship arrived/present, False otherwise.
*/
bool COrder_Board::WaitForTransporter(CUnit &unit)
{

	if (unit.Wait) {
		unit.Wait--;
		return false;
	}

	const CUnit *trans = this->GetGoal();

	if (!trans || !CanTransport(*trans, unit)) {
		// FIXME: destination destroyed??
		unit.Wait = 6;
		return false;
	}

	//Wyrmgus start
//	if (!trans->IsVisibleAsGoal(*unit.Player)) {
	if (!trans->IsVisibleAsGoal(*unit.Player) && unit.Player->Type != PlayerNeutral) { // neutral units continue waiting for the transporter even if it is not visible
	//Wyrmgus end
		DebugPrint("Transporter Gone\n");
		this->ClearGoal();
		unit.Wait = 6;
		return false;
	}

	if (unit.MapDistanceTo(*trans) == 1) {
		// enter transporter
		return true;
	}

	// FIXME: any enemies in range attack them, while waiting.

	// n0b0dy: This means we have to search with a smaller range.
	// It happens only when you reach the shore,and the transporter
	// is not there. The unit searches with a big range, so it thinks
	// it's there. This is why we reset the search. The transporter
	// should be a lot closer now, so it's not as bad as it seems.
	this->State = State_Init;
	this->Range = 1;
	// Uhh wait a bit.
	unit.Wait = 10;

	return false;
}

/**
**  Enter the transporter.
**
**  @param unit  Pointer to unit.
*/
static void EnterTransporter(CUnit &unit, COrder_Board &order)
{
	CUnit *transporter = order.GetGoal();

	Assert(transporter != nullptr);

	if (!transporter->IsVisibleAsGoal(*unit.Player)) {
		DebugPrint("Transporter gone\n");
		return;
	}

	if (transporter->BoardCount < transporter->Type->MaxOnBoard) {
		// Place the unit inside the transporter.
		unit.Remove(transporter);
		transporter->BoardCount += unit.Type->BoardSize;
		unit.Boarded = 1;
		transporter->UpdateContainerAttackRange();
		if (!unit.Player->AiEnabled) {
			// Don't make anything funny after going out of the transporter.
			CommandStopUnit(unit);
		}

		if (IsOnlySelected(*transporter)) {
			SelectedUnitChanged();
		}
		return;
	}
	DebugPrint("No free slot in transporter\n");
}

/* virtual */ void COrder_Board::Execute(CUnit &unit)
{
	switch (this->State) {
		// Wait for transporter
		case State_WaitForTransporter:
			if (this->WaitForTransporter(unit)) {
				this->State = State_EnterTransporter;
			} else {
				//Wyrmgus start
//				UnitShowAnimation(unit, unit.Type->Animations->Still);
				UnitShowAnimation(unit, unit.GetAnimations()->Still);
				//Wyrmgus end
			}
			break;

		case State_EnterTransporter: {
			EnterTransporter(unit, *this);
			this->Finished = true;
			return ;
		}
		case State_Init:
			if (unit.Wait) {
				unit.Wait--;
				return;
			}
			this->State = 1;
		// FALL THROUGH
		default: { // Move to transporter
			//Wyrmgus start
//			if (this->State <= State_MoveToTransporterMax) {
			if (unit.CanMove() && this->State <= State_MoveToTransporterMax) {
			//Wyrmgus end
				const int pathRet = MoveToTransporter(unit);
				// FIXME: if near transporter wait for enter
				if (pathRet) {
					if (pathRet == PF_UNREACHABLE) {
						//Wyrmgus start
						//if is unreachable and is on a raft, see if the raft can move closer to the "transporter"
						if ((unit.MapLayer->Field(unit.tilePos)->Flags & MapFieldBridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeLand) {
							std::vector<CUnit *> table;
							Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
							for (size_t i = 0; i != table.size(); ++i) {
								if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
									if (table[i]->CurrentAction() == UnitActionStill) {
										CommandStopUnit(*table[i]);
										CommandMove(*table[i], this->HasGoal() ? this->GetGoal()->tilePos : this->goalPos, FlushCommands, this->HasGoal() ? this->GetGoal()->MapLayer->ID : this->MapLayer);
									}
									return;
								}
							}
						}
						//Wyrmgus end
						if (++this->State == State_MoveToTransporterMax) {
							this->Finished = true;
							return;
						} else {
							// Try with a bigger range.
							this->Range++;
							this->State--;
						}
					} else if (pathRet == PF_REACHED) {
						this->State = State_WaitForTransporter;
					}
				}
			//Wyrmgus start
			} else if (!unit.CanMove()) { // if the unit can't move, go directly to the state of waiting for the transporter
				this->State = State_WaitForTransporter;
			//Wyrmgus end
			}
			break;
		}
	}
}
