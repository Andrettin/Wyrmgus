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
//      (c) Copyright 1998-2022 by Lutz Sammer and Andrettin
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

#pragma once

#include "util/enum_converter.h"

namespace wyrmgus {

/**
**  Missile-class this defines how a missile-type reacts.
*/
enum class missile_class {
	none, //missile does nothing
	point_to_point, //missile flies from x,y to x1,y1
	point_to_point_with_hit, //missile flies from x,y to x1,y1 than shows hit animation.
	point_to_point_cycle_once, //missile flies from x,y to x1,y1 and animates ONCE from start to finish and back
	point_to_point_bounce, //missile flies from x,y to x1,y1 than bounces three times.
	stay,                     /// Missile appears at x,y, does it's anim and vanishes.
	cycle_once,                /// Missile appears at x,y, then cycle through the frames once.
	fire,                     /// Missile doesn't move, than checks the source unit for HP.
	hit,                      /// Missile shows the hit points.
	parabolic,                /// Missile flies from x,y to x1,y1 using a parabolic path
	land_mine,                 /// Missile wait on x,y until a non-air unit comes by, the explodes.
	whirlwind,                /// Missile appears at x,y, is whirlwind
	flame_shield,              /// Missile surround x,y
	death_coil,                /// Missile is death coil.
	tracer,                   /// Missile seeks towards to target unit
	clip_to_target, //missile remains clipped to target's current goal and plays his animation once
	continuous,               /// Missile stays and plays it's animation several times
	straight_fly //missile flies from x,y to x1,y1 then continues to fly, until incompatible terrain is detected
};

}

extern template class archimedes::enum_converter<missile_class>;

Q_DECLARE_METATYPE(wyrmgus::missile_class)
