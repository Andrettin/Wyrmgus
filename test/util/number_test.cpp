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

#include "util/number_util.h"
#include "util/util.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(isqrt_test)
{
    BOOST_CHECK(isqrt(1) == 1);
    BOOST_CHECK(isqrt(2) == 1);
    BOOST_CHECK(isqrt(3) == 1);
    BOOST_CHECK(isqrt(4) == 2);
    BOOST_CHECK(isqrt(5) == 2);
    BOOST_CHECK(isqrt(6) == 2);
    BOOST_CHECK(isqrt(7) == 2);
    BOOST_CHECK(isqrt(8) == 2);
    BOOST_CHECK(isqrt(9) == 3);
    BOOST_CHECK(isqrt(10) == 3);
    BOOST_CHECK(isqrt(27) == 5);
    BOOST_CHECK(isqrt(32) == 5);
    BOOST_CHECK(isqrt(64) == 8);
    BOOST_CHECK(isqrt(778) == 27);
    BOOST_CHECK(isqrt(75900) == 275);
}

BOOST_AUTO_TEST_CASE(icbrt_test)
{
    BOOST_CHECK(number::cbrt(1) == 1);
    BOOST_CHECK(number::cbrt(2) == 1);
    BOOST_CHECK(number::cbrt(3) == 1);
    BOOST_CHECK(number::cbrt(4) == 1);
    BOOST_CHECK(number::cbrt(5) == 1);
    BOOST_CHECK(number::cbrt(6) == 1);
    BOOST_CHECK(number::cbrt(7) == 1);
    BOOST_CHECK(number::cbrt(8) == 2);
    BOOST_CHECK(number::cbrt(9) == 2);
    BOOST_CHECK(number::cbrt(10) == 2);
    BOOST_CHECK(number::cbrt(27) == 3);
    BOOST_CHECK(number::cbrt(32) == 3);
    BOOST_CHECK(number::cbrt(64) == 4);
    BOOST_CHECK(number::cbrt(778) == 9);
    BOOST_CHECK(number::cbrt(75900) == 42);
}
