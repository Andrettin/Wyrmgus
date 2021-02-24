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
//      (c) Copyright 1998-2021 by Lutz Sammer and Andrettin
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

inline missile_class string_to_missile_class(const std::string &str)
{
	if (str == "none") {
		return missile_class::none;
	} else if (str == "point_to_point") {
		return missile_class::point_to_point;
	} else if (str == "point_to_point_with_hit") {
		return missile_class::point_to_point_with_hit;
	} else if (str == "point_to_point_cycle_once") {
		return missile_class::point_to_point_cycle_once;
	} else if (str == "point_to_point_bounce") {
		return missile_class::point_to_point_bounce;
	} else if (str == "stay") {
		return missile_class::stay;
	} else if (str == "cycle_once") {
		return missile_class::cycle_once;
	} else if (str == "fire") {
		return missile_class::fire;
	} else if (str == "hit") {
		return missile_class::hit;
	} else if (str == "parabolic") {
		return missile_class::parabolic;
	} else if (str == "land_mine") {
		return missile_class::land_mine;
	} else if (str == "whirlwind") {
		return missile_class::whirlwind;
	} else if (str == "flame_shield") {
		return missile_class::flame_shield;
	} else if (str == "death_coil") {
		return missile_class::death_coil;
	} else if (str == "tracer") {
		return missile_class::tracer;
	} else if (str == "clip_to_target") {
		return missile_class::clip_to_target;
	} else if (str == "continuous") {
		return missile_class::continuous;
	} else if (str == "straight_fly") {
		return missile_class::straight_fly;
	}

	throw std::runtime_error("Invalid missile class: \"" + str + "\".");
}

inline std::string missile_class_to_string(const missile_class missile_class)
{
	switch (missile_class) {
		case missile_class::none:
			return "none";
		case missile_class::point_to_point:
			return "point_to_point";
		case missile_class::point_to_point_with_hit:
			return "point_to_point_with_hit";
		case missile_class::point_to_point_cycle_once:
			return "point_to_point_cycle_once";
		case missile_class::point_to_point_bounce:
			return "point_to_point_bounce";
		case missile_class::stay:
			return "stay";
		case missile_class::cycle_once:
			return "cycle_once";
		case missile_class::fire:
			return "fire";
		case missile_class::hit:
			return "hit";
		case missile_class::parabolic:
			return "parabolic";
		case missile_class::land_mine:
			return "land_mine";
		case missile_class::whirlwind:
			return "whirlwind";
		case missile_class::flame_shield:
			return "flame_shield";
		case missile_class::death_coil:
			return "death_coil";
		case missile_class::tracer:
			return "tracer";
		case missile_class::clip_to_target:
			return "clip_to_target";
		case missile_class::continuous:
			return "continuous";
		case missile_class::straight_fly:
			return "straight_fly";
		default:
			break;
	}

	throw std::runtime_error("Invalid missile class: \"" + std::to_string(static_cast<int>(missile_class)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::missile_class)
