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

#include "util/string_conversion_util.h"

#include <boost/test/unit_test.hpp>

using namespace wyrmgus;

BOOST_AUTO_TEST_CASE(string_to_time_test_1)
{
    const QTime time = string::to_time("6.28.45.710");

    BOOST_CHECK(time.hour() == 6);
    BOOST_CHECK(time.minute() == 28);
    BOOST_CHECK(time.second() == 45);
    BOOST_CHECK(time.msec() == 710);
}

BOOST_AUTO_TEST_CASE(string_to_time_test_2)
{
    const QTime time = string::to_time("19.40.5.791");

    BOOST_CHECK(time.hour() == 19);
    BOOST_CHECK(time.minute() == 40);
    BOOST_CHECK(time.second() == 5);
    BOOST_CHECK(time.msec() == 791);
}
