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
//      (c) Copyright 2021 by Andrettin
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

#include "util/geocoordinate.h"
#include "util/point_util.h"

#include <boost/test/unit_test.hpp>

using namespace wyrmgus;

BOOST_AUTO_TEST_CASE(geocoordinate_to_circle_edge_point_test_1)
{
    const geocoordinate geocoordinate(-35, 0);
    const QPoint direction_pos = geocoordinate.to_circle_edge_point();

    const QPoint expected_result(-10000, 0);
    BOOST_TEST((direction_pos == expected_result), "direction_pos " + point::to_string(direction_pos) + " does not equal " + point::to_string(expected_result));
}

BOOST_AUTO_TEST_CASE(geocoordinate_to_circle_edge_point_test_2)
{
    const geocoordinate geocoordinate(-90, 45);
    const QPoint direction_pos = geocoordinate.to_circle_edge_point();

    const QPoint expected_result(-7071, -7071);
    BOOST_TEST((direction_pos == expected_result), "direction_pos " + point::to_string(direction_pos) + " does not equal " + point::to_string(expected_result));
}
