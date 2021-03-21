//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//           Stratagus - A free fantasy real time strategy game engine
//
/**@name action_patrol.cpp - The patrol action. */
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

#include "action/action_patrol.h"

#include "animation.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder.h"
#include "script.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_type.h"
#include "video/video.h"

//Wyrmgus start
//std::unique_ptr<COrder> COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest)
std::unique_ptr<COrder> COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest, int current_z, int dest_z)
//Wyrmgus end
{
	Assert(CMap::get()->Info.IsPointOnMap(currentPos, current_z));
	Assert(CMap::get()->Info.IsPointOnMap(dest, dest_z));

	auto order = std::make_unique<COrder_Patrol>();

	order->goalPos = dest;
	order->WayPoint = currentPos;
	//Wyrmgus start
	order->MapLayer = dest_z;
	order->WayPointMapLayer = current_z;
	//Wyrmgus end
	return order;
}


void COrder_Patrol::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)
	
	file.printf("{\"action-patrol\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end
	file.printf(" \"range\", %d,", this->Range);

	if (this->WaitingCycle != 0) {
		file.printf(" \"waiting-cycle\", %d,", this->WaitingCycle);
	}
	//Wyrmgus start
//	file.printf(" \"patrol\", {%d, %d}", this->WayPoint.x, this->WayPoint.y);
	file.printf(" \"patrol\", {%d, %d},", this->WayPoint.x, this->WayPoint.y);
	file.printf(" \"patrol-map-layer\", %d", this->WayPointMapLayer);
	//Wyrmgus end
	file.printf("}");
}

bool COrder_Patrol::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)
	
	if (!strcmp(value, "patrol")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->WayPoint.x , &this->WayPoint.y);
		lua_pop(l, 1);
	//Wyrmgus start
	} else if (!strcmp(value, "patrol-map-layer")) {
		++j;
		this->WayPointMapLayer = LuaToNumber(l, -1, j + 1);
	//Wyrmgus end
	} else if (!strcmp(value, "waiting-cycle")) {
		++j;
		this->WaitingCycle = LuaToNumber(l, -1, j + 1);
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

bool COrder_Patrol::IsValid() const
{
	return true;
}

PixelPos COrder_Patrol::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	if (this->MapLayer != UI.CurrentMapLayer->ID) {
		return lastScreenPos;
	}

	const PixelPos pos1 = vp.TilePosToScreen_Center(this->goalPos);
	const PixelPos pos2 = vp.TilePosToScreen_Center(this->WayPoint);

	if (Preference.ShowPathlines) {
		Video.DrawLineClip(ColorGreen, lastScreenPos, pos1, render_commands);
		Video.FillCircleClip(ColorBlue, pos1, 2 * defines::get()->get_scale_factor(), render_commands);
		Video.DrawLineClip(ColorBlue, pos1, pos2, render_commands);
		Video.FillCircleClip(ColorBlue, pos2, 3 * defines::get()->get_scale_factor(), render_commands);
	}

	return pos2;
}

/* virtual */ void COrder_Patrol::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);
	const Vec2i tileSize(0, 0);
	input.SetGoal(this->goalPos, tileSize, this->MapLayer);
}


/* virtual */ void COrder_Patrol::Execute(CUnit &unit)
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

	switch (DoActionMove(unit)) {
		case PF_FAILED:
			this->WaitingCycle = 0;
			break;
		case PF_UNREACHABLE:
			//Wyrmgus start
			if (unit.MapLayer->Field(unit.tilePos)->has_flag(tile_flag::bridge) && !unit.Type->BoolFlag[BRIDGE_INDEX].value && unit.Type->UnitType == UnitTypeType::Land) {
				std::vector<CUnit *> table;
				Select(unit.tilePos, unit.tilePos, table, unit.MapLayer->ID);
				for (size_t i = 0; i != table.size(); ++i) {
					if (!table[i]->Removed && table[i]->Type->BoolFlag[BRIDGE_INDEX].value && table[i]->CanMove()) {
						if (table[i]->CurrentAction() == UnitAction::Still) {
							CommandStopUnit(*table[i]);
							CommandMove(*table[i], this->goalPos, FlushCommands, this->MapLayer);
						}
						return;
					}
				}
			}
			//Wyrmgus end
			// Increase range and try again
			this->WaitingCycle = 1;
			this->Range++;
			break;

		case PF_REACHED:
			this->WaitingCycle = 1;
			this->Range = 0;
			std::swap(this->WayPoint, this->goalPos);
			//Wyrmgus start
			std::swap(this->WayPointMapLayer, this->MapLayer);
			//Wyrmgus end

			break;
		case PF_WAIT:
			// Wait for a while then give up
			this->WaitingCycle++;
			if (this->WaitingCycle == 5) {
				this->WaitingCycle = 0;
				this->Range = 0;
				std::swap(this->WayPoint, this->goalPos);
				//Wyrmgus start
				std::swap(this->WayPointMapLayer, this->MapLayer);
				//Wyrmgus end

				unit.pathFinderData->output.Cycles = 0; //moving counter
			}
			break;
		default: // moving
			this->WaitingCycle = 0;
			break;
	}

	if (!unit.Anim.Unbreakable) {
		if (AutoAttack(unit) || AutoRepair(unit) || AutoCast(unit)) {
			this->Finished = true;
		}
	}
}
