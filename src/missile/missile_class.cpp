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
//      (c) Copyright 2022 by Andrettin
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

#include "missile/missile_class.h"

namespace wyrmgus {

template class enum_converter<missile_class>;

template <>
const std::string enum_converter<missile_class>::property_class_identifier = "wyrmgus::missile_class";

template <>
const std::map<std::string, missile_class> enum_converter<missile_class>::string_to_enum_map = {
	{ "none", missile_class::none },
	{ "point_to_point", missile_class::point_to_point },
	{ "point_to_point_with_hit", missile_class::point_to_point_with_hit },
	{ "point_to_point_cycle_once", missile_class::point_to_point_cycle_once },
	{ "point_to_point_bounce", missile_class::point_to_point_bounce },
	{ "stay", missile_class::stay },
	{ "cycle_once", missile_class::cycle_once },
	{ "fire", missile_class::fire },
	{ "hit", missile_class::hit },
	{ "parabolic", missile_class::parabolic },
	{ "land_mine", missile_class::land_mine },
	{ "whirlwind", missile_class::whirlwind },
	{ "flame_shield", missile_class::flame_shield },
	{ "death_coil", missile_class::death_coil },
	{ "tracer", missile_class::tracer },
	{ "clip_to_target", missile_class::clip_to_target },
	{ "continuous", missile_class::continuous },
	{ "straight_fly", missile_class::straight_fly }
};

template <>
const bool enum_converter<missile_class>::initialized = enum_converter::initialize();

}
