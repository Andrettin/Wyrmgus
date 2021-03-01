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

#include "economy/resource.h"

#include "util/astronomy_util.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(mass_multiplier_test)
{
    int multiplier = resource::get_mass_multiplier(873139800, astronomy::zg_per_jovian_mass);
    BOOST_CHECK(multiplier == 71);

    multiplier = resource::get_mass_multiplier(2258774700, astronomy::zg_per_jovian_mass);
    BOOST_CHECK(multiplier == 100);

    multiplier = resource::get_mass_multiplier(2847195000, astronomy::zg_per_jovian_mass);
    BOOST_CHECK(multiplier == 120);
}
