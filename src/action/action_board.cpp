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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "stratagus.h"

#include "action/action_board.h"

#include "animation.h"
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder.h"
#include "player/player.h"
#include "player/player_type.h"
#include "script.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionBoard(CUnit &unit)
{
	auto order = std::make_unique<COrder_Board>();

	order->set_goal(&unit);
	order->Range = 1;

	return order;
}

void COrder_Board::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)
	
	file.printf("{\"action-board\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->has_goal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->get_goal()).c_str());
	}
	file.printf(" \"state\", %d", static_cast<int>(this->state));

	file.printf("}");
}

bool COrder_Board::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)
	
	if (!strcmp("state", value)) {
		++j;
		this->state = static_cast<board_state>(LuaToNumber(l, -1, j + 1));
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

bool COrder_Board::IsValid() const
{
	return this->has_goal() && this->get_goal()->IsAliveOnMap();
}

PixelPos COrder_Board::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	PixelPos targetPos;

	if (this->has_goal()) {
		//Wyrmgus start
		if (this->get_goal()->MapLayer != UI.CurrentMapLayer) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.scaled_map_to_screen_pixel_pos(this->get_goal()->get_scaled_map_pixel_pos_center());
	} else {
		//Wyrmgus start
		if (this->MapLayer != UI.CurrentMapLayer->ID) {
			return lastScreenPos;
		}
		//Wyrmgus end
		targetPos = vp.TilePosToScreen_Center(this->goalPos);
	}

	if (preferences::get()->are_pathlines_enabled()) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, 2 * defines::get()->get_scale_factor(), render_commands);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(ColorGreen, targetPos, 3 * defines::get()->get_scale_factor(), render_commands);
	}

	return targetPos;
}

void COrder_Board::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);

	Vec2i tileSize;
	if (this->has_goal()) {
		CUnit *goal = this->get_goal();
		tileSize = goal->get_tile_size();
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

	const CUnit *trans = this->get_goal();

	if (!trans || !CanTransport(*trans, unit)) {
		// FIXME: destination destroyed??
		unit.Wait = 6;
		return false;
	}

	//Wyrmgus start
//	if (!trans->IsVisibleAsGoal(*unit.Player)) {
	if (!trans->IsVisibleAsGoal(*unit.Player) && unit.Player->get_type() != player_type::neutral) { // neutral units continue waiting for the transporter even if it is not visible
	//Wyrmgus end
		DebugPrint("Transporter Gone\n");
		this->clear_goal();
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
	this->state = board_state::init;
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
	CUnit *transporter = order.get_goal();

	assert_throw(transporter != nullptr);

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

void COrder_Board::Execute(CUnit &unit)
{
	switch (this->state) {
		// Wait for transporter
		case board_state::wait_for_transporter:
			if (this->WaitForTransporter(unit)) {
				this->state = board_state::enter_transporter;
			} else {
				UnitShowAnimation(unit, unit.get_animation_set()->Still.get());
			}
			break;

		case board_state::enter_transporter: {
			EnterTransporter(unit, *this);
			this->Finished = true;
			return ;
		}
		case board_state::init:
			if (unit.Wait) {
				unit.Wait--;
				return;
			}
			this->state = 1;
		// FALL THROUGH
		default: { // Move to transporter
			if (unit.CanMove() && this->state <= board_state::move_to_transporter_max) {
				const int pathRet = MoveToTransporter(unit);
				// FIXME: if near transporter wait for enter
				if (pathRet) {
					if (pathRet == PF_UNREACHABLE) {
						//Wyrmgus start
						//if is unreachable and is on a raft, see if the raft can move closer to the "transporter"
						if (unit.MapLayer->Field(unit.tilePos)->has_flag(tile_flag::bridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->get_domain() == unit_domain::land) {
							std::vector<CUnit *> table;
							Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
							for (size_t i = 0; i != table.size(); ++i) {
								if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
									if (table[i]->CurrentAction() == UnitAction::Still) {
										CommandStopUnit(*table[i]);
										CommandMove(*table[i], this->has_goal() ? this->get_goal()->tilePos : this->goalPos, FlushCommands, this->has_goal() ? this->get_goal()->MapLayer->ID : this->MapLayer);
									}
									return;
								}
							}
						}
						//Wyrmgus end
						if (++this->state == board_state::move_to_transporter_max) {
							this->Finished = true;
							return;
						} else {
							// Try with a bigger range.
							this->Range++;
							this->state--;
						}
					} else if (pathRet == PF_REACHED) {
						this->state = board_state::wait_for_transporter;
					}
				}
			//Wyrmgus start
			} else if (!unit.CanMove()) { // if the unit can't move, go directly to the state of waiting for the transporter
				this->state = board_state::wait_for_transporter;
			//Wyrmgus end
			}
			break;
		}
	}
}
