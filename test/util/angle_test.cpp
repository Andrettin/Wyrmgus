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

#include "stratagus.h"

#include "util/angle_util.h"

#include "util/fractional_int.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(degrees_to_radians_test)
{
    BOOST_CHECK(angle::degrees_to_radians(angle::number_type(180)) == angle::pi);
    BOOST_CHECK(angle::degrees_to_radians(angle::number_type(90)) == angle::number_type("1.5708"));
}

BOOST_AUTO_TEST_CASE(radians_to_degrees_test)
{
    BOOST_CHECK(angle::radians_to_degrees(angle::pi) == 180);
    BOOST_CHECK(angle::radians_to_degrees(angle::number_type("1.5708")) == 90);
}

BOOST_AUTO_TEST_CASE(gudermannian_test)
{
    const angle::number_type y_max = angle::gudermannian_inverse(angle::number_type(85));
    const angle::number_type y_min = angle::gudermannian_inverse(angle::number_type(-85));
    const int height = 1588;
    
    const angle::number_type original_y(615);
    const angle::number_type degrees = angle::gudermannian(y_max - ((original_y / height) * (y_max - y_min)));
    BOOST_CHECK(degrees == angle::number_type("37.4770"));
}
