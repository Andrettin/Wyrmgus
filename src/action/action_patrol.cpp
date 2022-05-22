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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "animation/animation.h"
#include "animation/animation_set.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "database/defines.h"
#include "database/preferences.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "pathfinder/pathfinder.h"
#include "script.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "video/renderer.h"
#include "video/video.h"

//Wyrmgus start
//std::unique_ptr<COrder> COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest)
std::unique_ptr<COrder> COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest, int current_z, int dest_z)
//Wyrmgus end
{
	assert_throw(CMap::get()->Info->IsPointOnMap(currentPos, current_z));
	assert_throw(CMap::get()->Info->IsPointOnMap(dest, dest_z));

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

	if (preferences::get()->are_pathlines_enabled()) {
		render_commands.push_back([lastScreenPos, pos1, pos2](renderer *renderer) {
			renderer->draw_line(lastScreenPos, pos1, CVideo::GetRGBA(ColorGreen));

			renderer->fill_circle(pos1, (2 * preferences::get()->get_scale_factor()).to_int(), CVideo::GetRGBA(ColorBlue));

			renderer->draw_line(pos1, pos2, CVideo::GetRGBA(ColorBlue));

			renderer->fill_circle(pos2, (3 * preferences::get()->get_scale_factor()).to_int(), CVideo::GetRGBA(ColorBlue));
		});
	}

	return pos2;
}

void COrder_Patrol::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(this->Range);
	const Vec2i tileSize(0, 0);
	input.SetGoal(this->goalPos, tileSize, this->MapLayer);
}

void COrder_Patrol::Execute(CUnit &unit)
{
	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		UnitShowAnimation(unit, unit.get_animation_set()->Still);
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
