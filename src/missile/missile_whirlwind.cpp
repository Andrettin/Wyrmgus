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
/**@name missile_whirlwind.cpp - The missile Whirlwind. */
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

#include "stratagus.h"

#include "missile.h"

#include "database/defines.h"
#include "map/map.h"
#include "map/map_info.h"
#include "util/assert_util.h"
#include "util/util.h"

/**
**  Whirlwind controller
**
**  @todo do it more configurable.
*/
void MissileWhirlwind::Action()
{
	// Animate, move.
	if (!this->AnimWait--) {
		if (this->NextMissileFrame(1, 0)) {
			this->SpriteFrame = 0;
			PointToPointMissile(*this);
		}
		this->AnimWait = this->Type->get_sleep();
	}
	this->Wait = 1;

	// Center of the tornado
	const PixelPos pixelCenter = this->position + this->Type->get_frame_size() / 2;
	const PixelPos centerOffset(wyrmgus::defines::get()->get_tile_width() / 2, wyrmgus::defines::get()->get_tile_height());
	const Vec2i center = CMap::get()->map_pixel_pos_to_tile_pos(pixelCenter + centerOffset);

	//Wyrmgus start
	assert_throw(this->Type->AttackSpeed != 0);
//	if (!(this->TTL % CYCLES_PER_SECOND / 10)) {
	if (!(this->TTL % CYCLES_PER_SECOND / this->Type->AttackSpeed)) { //AttackSpeed is by default 10
	//Wyrmgus end
		this->MissileHit();
	}
	// Changes direction every 3 seconds (approx.)
	if (!(this->TTL % 100)) { // missile has reached target unit/spot
		Vec2i newPos;

		do {
			// find new destination in the map
			newPos.x = center.x + SyncRand(5) - 2;
			newPos.y = center.y + SyncRand(5) - 2;
		} while (!CMap::get()->Info->IsPointOnMap(newPos, this->MapLayer));
		this->destination = CMap::get()->tile_pos_to_map_pixel_pos_center(newPos);
		this->source = this->position;
		this->State = 0;
		DebugPrint("Whirlwind new direction: %d, %d, TTL: %d\n" _C_
				   this->destination.x _C_ this->destination.y _C_ this->TTL);
	}
}
