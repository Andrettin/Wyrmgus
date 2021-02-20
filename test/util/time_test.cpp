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

#include "util/time_util.h"

#include "util/fractional_int.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(time_to_number_test_1)
{
    const QTime time(6, 28, 45, 710);
    const decimillesimal_int n = time::to_number(time);

    BOOST_CHECK(n == decimillesimal_int("6.4792"));
}

BOOST_AUTO_TEST_CASE(time_to_number_test_2)
{
    const QTime time(19, 40, 5, 791);
    const decimillesimal_int n = time::to_number(time);

    BOOST_CHECK(n == decimillesimal_int("19.6682"));
}
