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

#include "action/action_move.h"

#include "ai.h"
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
#include "player/player.h"
#include "script.h"
#include "settings.h"
#include "sound/sound.h"
#include "sound/unit_sound_type.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "video/video.h"

//Wyrmgus start
//std::unique_ptr<COrder> COrder::NewActionMove(const Vec2i &pos)
std::unique_ptr<COrder> COrder::NewActionMove(const Vec2i &pos, int z)
//Wyrmgus end
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

	auto order = std::make_unique<COrder_Move>();

	order->goalPos = pos;
	//Wyrmgus start
	order->MapLayer = z;
	//Wyrmgus end
	return order;
}

void COrder_Move::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)
	
	file.printf("{\"action-move\",");

	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	//Wyrmgus start
//	file.printf(" \"tile\", {%d, %d}", this->goalPos.x, this->goalPos.y);
	file.printf(" \"tile\", {%d, %d},", this->goalPos.x, this->goalPos.y);
	file.printf(" \"map-layer\", %d", this->MapLayer);
	//Wyrmgus end

	file.printf("}");
}

bool COrder_Move::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)
	
	if (!strcmp(value, "range")) {
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

bool COrder_Move::IsValid() const
{
	return true;
}

PixelPos COrder_Move::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	if (this->MapLayer != UI.CurrentMapLayer->ID) {
		return lastScreenPos;
	}
	
	return COrder::Show(vp, lastScreenPos, render_commands);
}

QPoint COrder_Move::get_shown_target_pos(const CViewport &vp) const
{
	return vp.TilePosToScreen_Center(this->goalPos);
}

void COrder_Move::UpdatePathFinderData(PathFinderInput &input)
{
	const Vec2i tileSize(0, 0);
	input.SetGoal(this->goalPos, tileSize, this->MapLayer);

	int distance = this->Range;
	input.SetMaxRange(distance);
	input.SetMinRange(0);
}

/**
**  Unit moves! Generic function called from other actions.
**
**  @param unit  Pointer to unit.
**
**  @return      >0 remaining path length, 0 wait for path, -1
**               reached goal, -2 can't reach the goal.
*/
int DoActionMove(CUnit &unit)
{
	Vec2i posd; // movement in tile.
	int d;

	assert_throw(unit.CanMove());

	if (!unit.Moving && (unit.get_animation_set()->Move != unit.Anim.CurrAnim || !unit.Anim.Wait)) {
		assert_throw(!unit.Anim.Unbreakable);

		// FIXME: So units flying up and down are not affected.
		unit.pixel_offset = QPoint(0, 0);

		UnmarkUnitFieldFlags(unit);
		d = NextPathElement(unit, posd.x, posd.y);
		MarkUnitFieldFlags(unit);
		switch (d) {
			case PF_UNREACHABLE: // Can't reach, stop
				if (unit.Player->AiEnabled) {
					AiCanNotMove(unit);
				}
				unit.Moving = 0;
				unit.reset_step_count();
				return d;
			case PF_REACHED: // Reached goal, stop
				unit.Moving = 0;
				return d;
			case PF_WAIT: // No path, wait
				unit.Wait = 10;

				unit.Moving = 0;
				unit.reset_step_count();
				return d;
			default: // On the way moving
				unit.Moving = 1;
				break;
		}
		
		if (unit.Type->get_domain() == unit_domain::water) { //boat (un)docking?
			//Wyrmgus start
//			const wyrmgus::tile &mf_cur = *Map.Field(unit.Offset);
//			const wyrmgus::tile &mf_next = *Map.Field(unit.tilePos + posd);
			const wyrmgus::tile &mf_cur = *unit.MapLayer->Field(unit.Offset);
			const wyrmgus::tile &mf_next = *unit.MapLayer->Field(unit.tilePos + posd);
			//Wyrmgus end

			if (mf_cur.WaterOnMap() && mf_next.CoastOnMap()) {
				PlayUnitSound(&unit, wyrmgus::unit_sound_type::docking);
			} else if (mf_cur.CoastOnMap() && mf_next.WaterOnMap()) {
				PlayUnitSound(&unit, wyrmgus::unit_sound_type::docking); // undocking
			}
		}
		Vec2i pos = unit.tilePos + posd;

		//Wyrmgus start
//		unit.MoveToXY(pos);
		unit.MoveToXY(pos, unit.MapLayer->ID);
		unit.increment_step_count();
		//Wyrmgus end
		//Wyrmgus start
		PlayUnitSound(&unit, unit_sound_type::step);
		//Wyrmgus end

		// Remove unit from the current selection
		//Wyrmgus start
//		if (unit.Selected && !Map.Field(pos)->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
		if (unit.Selected) {
		//Wyrmgus end
			const tile *target_tile = unit.MapLayer->Field(pos);

			if (!target_tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
				if (IsOnlySelected(unit)) { //  Remove building cursor
					CancelBuildingMode();
				}
				if (!ReplayRevealMap) {
					UnSelectUnit(unit);
					SelectionChanged();
				}
			}
		}

		unit.pixel_offset.setX(-posd.x * defines::get()->get_tile_width());
		unit.pixel_offset.setY(-posd.y * defines::get()->get_tile_height());
		unit.Frame = unit.Type->StillFrame;
		UnitHeadingFromDeltaXY(unit, posd);
	} else {
		posd.x = Heading2X[unit.Direction / NextDirection];
		posd.y = Heading2Y[unit.Direction / NextDirection];
		d = unit.pathFinderData->output.Length + 1;
	}

	unit.pathFinderData->output.Cycles++;// reset have to be manualy controlled by caller.
	int move = UnitShowAnimationScaled(unit, unit.get_animation_set()->Move, DefaultTileMovementCost);

	unit.pixel_offset += QPoint(posd.x * move, posd.y * move);
	
	//Wyrmgus start
	if (abs(unit.get_pixel_offset().x()) > (defines::get()->get_tile_width() * 2) || abs(unit.get_pixel_offset().y()) > (defines::get()->get_tile_height() * 2)) {
		unit.pixel_offset = QPoint(0, 0);
#ifdef DEBUG
		log::log_error("Error in DoActionMove: unit's pixel movement was too big.");
#endif
	}
	//Wyrmgus end

	// Finished move animation, set Moving to 0 so we recalculate the path
	// next frame
	// FIXME: this is broken for subtile movement
	if (!unit.Anim.Unbreakable && !unit.get_pixel_offset().x() && !unit.get_pixel_offset().y()) {
		unit.Moving = 0;
	}

	return d;
}

void COrder_Move::Execute(CUnit &unit)
{
	assert_throw(unit.CanMove());

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
	// FIXME: (mr-russ) Make a reachable goal here with GoalReachable ...

	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			// Some tries to reach the goal
			this->Range++;
			break;
		case PF_REACHED:
			this->Finished = true;
			break;
		default:
			break;
	}
}
