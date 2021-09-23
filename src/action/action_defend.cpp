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
//      (c) Copyright 2012-2021 by cybermind and Andrettin
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

#include "action/action_defend.h"

#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder.h"
#include "script.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionDefend(CUnit &dest)
{
	auto order = std::make_unique<COrder_Defend>();

	if (dest.Destroyed) {
		order->goalPos = dest.tilePos + dest.GetHalfTileSize();
		order->MapLayer = dest.MapLayer->ID;
	} else {
		order->set_goal(&dest);
		order->Range = 1;
	}
	return order;
}

void COrder_Defend::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	file.printf("{\"action-defend\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->has_goal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->get_goal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	file.printf(" \"state\", %d", static_cast<int>(this->state));

	file.printf("}");
}

bool COrder_Defend::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp(value, "state")) {
		++j;
		this->state = static_cast<defend_state>(LuaToNumber(l, -1, j + 1));
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

bool COrder_Defend::IsValid() const
{
	return true;
}

PixelPos COrder_Defend::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
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
		Video.FillCircleClip(ColorOrange, targetPos, 3 * defines::get()->get_scale_factor(), render_commands);
	}

	return targetPos;
}

void COrder_Defend::UpdatePathFinderData(PathFinderInput &input)
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
		input.SetGoal(this->goalPos, tileSize, this->MapLayer);
	}
}

void COrder_Defend::Execute(CUnit &unit)
{
	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		UnitShowAnimation(unit, unit.get_animation_set()->Still.get());
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}
	CUnit *goal = this->get_goal();

	if (this->state == defend_state::init) {
		if (!goal || !goal->IsVisibleAsGoal(*unit.Player)) {
			this->Finished = true;
			return;
		}
		this->state = defend_state::moving_to_target;
	} else if (this->state == defend_state::defending) {
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
			this->state = defend_state::defending;
		}
		default:
			break;
	}

	// Target destroyed?
	if (goal && !goal->IsVisibleAsGoal(*unit.Player)) {
		DebugPrint("Goal gone\n");
		this->goalPos = goal->tilePos + goal->GetHalfTileSize();
		this->MapLayer = goal->MapLayer->ID;
		this->clear_goal();
		goal = nullptr;
		if (this->state == defend_state::defending) {
			this->Finished = true;
			return;
		}
	}
}
