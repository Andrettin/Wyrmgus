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

#include "util/astronomy_util.h"

#include "util/fractional_int.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(pc_to_ly_test)
{
    const centesimal_int pc("76.28");
    const centesimal_int ly = astronomy::pc_to_ly(pc);

    BOOST_CHECK(ly == centesimal_int("248.67"));
}

BOOST_AUTO_TEST_CASE(ly_to_pc_test)
{
    const centesimal_int ly(382);
    const centesimal_int pc = astronomy::ly_to_pc(ly);

    BOOST_CHECK(pc == centesimal_int("117.17"));
}

BOOST_AUTO_TEST_CASE(au_to_gm_test)
{
    const centesimal_int au(23);
    const int gm = astronomy::au_to_gm(au);

    BOOST_CHECK(gm == 3450);
}

BOOST_AUTO_TEST_CASE(gm_to_au_test)
{
    const int gm = 778;
    const centesimal_int au = astronomy::gm_to_au(gm);

    BOOST_CHECK(au == centesimal_int("5.18"));
}

BOOST_AUTO_TEST_CASE(ra_to_lon_test_1)
{
    const decimillesimal_int ra("6.4792");

    const decimillesimal_int lon = astronomy::ra_to_lon(ra);

    BOOST_CHECK(lon == decimillesimal_int("97.188"));
}

BOOST_AUTO_TEST_CASE(ra_to_lon_test_2)
{
    const decimillesimal_int ra("19.6682");

    const decimillesimal_int lon = astronomy::ra_to_lon(ra);

    BOOST_CHECK(lon == decimillesimal_int("-64.977"));
}
