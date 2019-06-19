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
//      (c) Copyright 2012 by Joris Dauphin
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

#include "animation/animation_spawnmissile.h"

#include "action/action_attack.h"
#include "action/action_spellcast.h"

#include "action/actions.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "missile/missile.h"
#include "missile/missile_type.h"
#include "pathfinder/pathfinder.h"
#include "unit/unit.h"

/* virtual */ void CAnimation_SpawnMissile::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	Assert(unit.Anim.Anim == this);

	const int startx = this->StartX;
	const int starty = this->StartY;
	const int destx = this->DestX;
	const int desty = this->DestY;
	const SpawnMissile_Flags flags = (SpawnMissile_Flags)(ParseAnimFlags(unit, this->flagsStr.c_str()));
	const int offsetnum = this->OffsetNum;
	const CUnit *goal = flags & SM_RelTarget ? unit.CurrentOrder()->GetGoal() : &unit;
	const int dir = ((goal->Direction + NextDirection / 2) & 0xFF) / NextDirection;
	const PixelPos moff = goal->GetType()->MissileOffsets[dir][!offsetnum ? 0 : offsetnum - 1];
	PixelPos start;
	PixelPos dest;
	MissileType *mtype = MissileTypeByIdent(this->missileTypeStr);
	if (mtype == nullptr) {
		return;
	}

	if (!goal || goal->Destroyed) {
		return;
	}
	if ((flags & SM_Pixel)) {
		start.x = goal->GetTilePos().x * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).x + goal->GetPixelOffset().x + moff.x + startx;
		start.y = goal->GetTilePos().y * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).y + goal->GetPixelOffset().y + moff.y + starty;
	} else {
		start.x = (goal->GetTilePos().x + startx) * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).x + CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).x / 2 + moff.x;
		start.y = (goal->GetTilePos().y + starty) * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).y + CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).y / 2 + moff.y;
	}
	if ((flags & SM_ToTarget)) {
		CUnit *target = goal->CurrentOrder()->GetGoal();
		if (!target || target->Destroyed) {
			Assert(!mtype->AlwaysFire || mtype->Range);
			if (!target && mtype->AlwaysFire == false) {
				return;
			}
		}
		if (!target) {
			if (goal->CurrentAction() == UnitActionStandGround) {
				return;
			} else if (goal->CurrentAction() == UnitActionAttack || goal->CurrentAction() == UnitActionAttackGround) {
				COrder_Attack &order = *static_cast<COrder_Attack *>(goal->CurrentOrder());
				dest = CMap::Map.TilePosToMapPixelPos_Center(order.GetGoalPos(), goal->GetMapLayer());
			} else if (goal->CurrentAction() == UnitActionSpellCast) {
				COrder_SpellCast &order = *static_cast<COrder_SpellCast *>(goal->CurrentOrder());
				dest = CMap::Map.TilePosToMapPixelPos_Center(order.GetGoalPos(), goal->GetMapLayer());
			}
			if (flags & SM_Pixel) {
				dest.x += destx;
				dest.y += desty;
			} else {
				dest.x += destx * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).x;
				dest.y += desty * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).y;
			}
		} else if (flags & SM_Pixel) {
			dest.x = target->GetMapPixelPosCenter().x + destx;
			dest.y = target->GetMapPixelPosCenter().y + desty;
		} else {
			dest.x = (target->GetTilePos().x + destx) * CMap::Map.GetMapLayerPixelTileSize(target->GetMapLayer()->GetIndex()).x;
			dest.y = (target->GetTilePos().y + desty) * CMap::Map.GetMapLayerPixelTileSize(target->GetMapLayer()->GetIndex()).y;
			dest += target->GetTilePixelSize() / 2;
		}
	} else {
		if ((flags & SM_Pixel)) {
			dest.x = goal->GetMapPixelPosCenter().x + destx;
			dest.y = goal->GetMapPixelPosCenter().y + desty;
		} else {
			dest.x = (goal->GetTilePos().x + destx) * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).x;
			dest.y = (goal->GetTilePos().y + desty) * CMap::Map.GetMapLayerPixelTileSize(goal->GetMapLayer()->GetIndex()).y;
			dest += goal->GetTilePixelSize() / 2;
		}
	}
	Vec2i destTilePos = CMap::Map.MapPixelPosToTilePos(dest, unit.GetMapLayer()->GetIndex());
	const int dist = goal->MapDistanceTo(destTilePos, unit.GetMapLayer()->GetIndex());
	if ((flags & SM_Ranged) && !(flags & SM_Pixel)
		&& dist > goal->GetModifiedVariable(ATTACKRANGE_INDEX)
		&& dist < goal->GetType()->MinAttackRange) {
	} else {
		Missile *missile = MakeMissile(*mtype, start, dest, unit.GetMapLayer()->GetIndex());
		if (flags & SM_SetDirection) {
			PixelPos posd;
			posd.x = Heading2X[goal->Direction / NextDirection];
			posd.y = Heading2Y[goal->Direction / NextDirection];
			missile->MissileNewHeadingFromXY(posd);
		}
		if (flags & SM_Damage) {
			missile->SourceUnit = &unit;
		}
		CUnit *target = goal->CurrentOrder()->GetGoal();
		if (flags & SM_ToTarget && target && target->IsAlive()) {
			missile->TargetUnit = target;
		}
	}
}

/*
**  s = "missileType startX startY destX destY [flag1[.flagN]] [missileoffset]"
*/
/* virtual */ void CAnimation_SpawnMissile::Init(const char *s, lua_State *)
{
	const std::string str(s);
	const size_t len = str.size();

	size_t begin = 0;
	size_t end = str.find(' ', begin);
	this->missileTypeStr.assign(str, begin, end - begin);

	std::string start_x_str;
	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	start_x_str.assign(str, begin, end - begin);
	this->StartX = std::stoi(start_x_str);

	std::string start_y_str;
	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	start_y_str.assign(str, begin, end - begin);
	this->StartY = std::stoi(start_y_str);

	std::string dest_x_str;
	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	dest_x_str.assign(str, begin, end - begin);
	this->DestX = std::stoi(dest_x_str);

	std::string dest_y_str;
	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	dest_y_str.assign(str, begin, end - begin);
	this->DestY = std::stoi(dest_y_str);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->flagsStr.assign(str, begin, end - begin);

	std::string offset_num_str;
	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	offset_num_str.assign(str, begin, end - begin);
	this->OffsetNum = std::stoi(offset_num_str);
}
