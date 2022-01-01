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
/**@name animation_spawnmissile.cpp - The animation SpawnMissile. */
//
//      (c) Copyright 2012-2022 by Joris Dauphin and Andrettin
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

#include "animation/animation_spawnmissile.h"

#include "action/action_attack.h"
#include "action/action_spellcast.h"

#include "actions.h"
#include "database/defines.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "missile.h"
#include "pathfinder.h"
#include "unit/unit.h"
#include "util/assert_util.h"

void CAnimation_SpawnMissile::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	assert_throw(unit.Anim.Anim == this);

	const int startx = ParseAnimInt(unit, this->startXStr);
	const int starty = ParseAnimInt(unit, this->startYStr);
	const int destx = ParseAnimInt(unit, this->destXStr);
	const int desty = ParseAnimInt(unit, this->destYStr);
	const SpawnMissile_Flags flags = (SpawnMissile_Flags)(ParseAnimFlags(unit, this->flagsStr.c_str()));
	const int offsetnum = ParseAnimInt(unit, this->offsetNumStr);
	const CUnit *goal = flags & SM_RelTarget ? unit.CurrentOrder()->get_goal() : &unit;
	const int dir = ((goal->Direction + NextDirection / 2) & 0xFF) / NextDirection;
	const PixelPos moff = goal->Type->MissileOffsets[dir][!offsetnum ? 0 : offsetnum - 1];
	PixelPos start;
	PixelPos dest;
	wyrmgus::missile_type *mtype = wyrmgus::missile_type::try_get(this->missileTypeStr);
	if (mtype == nullptr) {
		return;
	}

	if (!goal || goal->Destroyed) {
		return;
	}
	if ((flags & SM_Pixel)) {
		start.x = goal->tilePos.x * wyrmgus::defines::get()->get_tile_width() + goal->get_pixel_offset().x() + moff.x + startx;
		start.y = goal->tilePos.y * wyrmgus::defines::get()->get_tile_height() + goal->get_pixel_offset().y() + moff.y + starty;
	} else {
		start.x = (goal->tilePos.x + startx) * wyrmgus::defines::get()->get_tile_width() + wyrmgus::defines::get()->get_tile_width() / 2 + moff.x;
		start.y = (goal->tilePos.y + starty) * wyrmgus::defines::get()->get_tile_height() + wyrmgus::defines::get()->get_tile_height() / 2 + moff.y;
	}
	if ((flags & SM_ToTarget)) {
		CUnit *target = goal->CurrentOrder()->get_goal();
		if (!target || target->Destroyed) {
			assert_throw(!mtype->AlwaysFire || mtype->get_range());
			if (!target && mtype->AlwaysFire == false) {
				return;
			}
		}
		if (!target) {
			if (goal->CurrentAction() == UnitAction::StandGround) {
				return;
			} else if (goal->CurrentAction() == UnitAction::Attack || goal->CurrentAction() == UnitAction::AttackGround) {
				COrder_Attack &order = *static_cast<COrder_Attack *>(goal->CurrentOrder());
				dest = CMap::get()->tile_pos_to_map_pixel_pos_center(order.GetGoalPos());
			} else if (goal->CurrentAction() == UnitAction::SpellCast) {
				COrder_SpellCast &order = *static_cast<COrder_SpellCast *>(goal->CurrentOrder());
				dest = CMap::get()->tile_pos_to_map_pixel_pos_center(order.GetGoalPos());
			}
			if (flags & SM_Pixel) {
				dest.x += destx;
				dest.y += desty;
			} else {
				dest.x += destx * wyrmgus::defines::get()->get_tile_width();
				dest.y += desty * wyrmgus::defines::get()->get_tile_height();
			}
		} else if (flags & SM_Pixel) {
			dest.x = target->get_map_pixel_pos_center().x + destx;
			dest.y = target->get_map_pixel_pos_center().y + desty;
		} else {
			dest.x = (target->tilePos.x + destx) * wyrmgus::defines::get()->get_tile_width();
			dest.y = (target->tilePos.y + desty) * wyrmgus::defines::get()->get_tile_height();
			dest += target->get_tile_pixel_size() / 2;
		}
	} else {
		if ((flags & SM_Pixel)) {
			dest.x = goal->get_map_pixel_pos_center().x + destx;
			dest.y = goal->get_map_pixel_pos_center().y + desty;
		} else {
			dest.x = (goal->tilePos.x + destx) * wyrmgus::defines::get()->get_tile_width();
			dest.y = (goal->tilePos.y + desty) * wyrmgus::defines::get()->get_tile_height();
			dest += goal->get_tile_pixel_size() / 2;
		}
	}
	Vec2i destTilePos = CMap::get()->map_pixel_pos_to_tile_pos(dest);
	const int dist = goal->MapDistanceTo(destTilePos, unit.MapLayer->ID);
	if ((flags & SM_Ranged) && !(flags & SM_Pixel)
		&& dist > goal->GetModifiedVariable(ATTACKRANGE_INDEX)
		&& dist < goal->Type->MinAttackRange) {
	} else {
		Missile *missile = MakeMissile(*mtype, start, dest, unit.MapLayer->ID);
		if (flags & SM_SetDirection) {
			PixelPos posd;
			posd.x = Heading2X[goal->Direction / NextDirection];
			posd.y = Heading2Y[goal->Direction / NextDirection];
			missile->MissileNewHeadingFromXY(posd);
		}
		if (flags & SM_Damage) {
			missile->SourceUnit = unit.acquire_ref();
		}
		CUnit *target = goal->CurrentOrder()->get_goal();
		if (flags & SM_ToTarget && target && target->IsAlive()) {
			missile->TargetUnit = target->acquire_ref();
		}
	}
}

/*
**  s = "missileType startX startY destX destY [flag1[.flagN]] [missileoffset]"
*/
void CAnimation_SpawnMissile::Init(const char *s, lua_State *)
{
	const std::string str(s);
	const size_t len = str.size();

	size_t begin = 0;
	size_t end = str.find(' ', begin);
	this->missileTypeStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->startXStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->startYStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->destXStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->destYStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->flagsStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->offsetNumStr.assign(str, begin, end - begin);
}
