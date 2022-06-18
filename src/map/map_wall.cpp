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
/**@name map_wall.cpp - The map wall handling. */
//
//      (c) Copyright 1999-2022 by Vladi Shabanski and Andrettin
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

#include "map/map.h"

#include "map/terrain_type.h"
#include "map/tile.h"
#include "player/player.h"
#include "settings.h"
#include "ui/ui.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

/**
** Wall is hit with damage.
**
** @param pos     Map tile-position of wall.
** @param damage  Damage done to wall.
*/
//Wyrmgus start
//void CMap::HitWall(const Vec2i &pos, unsigned damage)
void CMap::HitWall(const Vec2i &pos, unsigned damage, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	const unsigned v = this->Field(pos)->get_value();
	const unsigned v = this->Field(pos, z)->get_value();
	//Wyrmgus end

	if (v <= damage) {
		//Wyrmgus start
//		RemoveWall(pos);
		ClearOverlayTile(pos, z);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		this->Field(pos)->set_value(v - damage);
		this->Field(pos, z)->set_value(v - damage);
//		MapFixWallTile(pos);
		if (this->Field(pos, z)->get_overlay_terrain()->get_unit_type() && this->Field(pos, z)->get_value() <= this->Field(pos, z)->get_overlay_terrain()->get_unit_type()->DefaultStat.Variables[HP_INDEX].Max / 2) {
			this->SetOverlayTerrainDamaged(pos, true, z);
		}
		//Wyrmgus end
	}
}
