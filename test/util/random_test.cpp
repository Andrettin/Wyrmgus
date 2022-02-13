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

#include "util/random.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(random_seed_test)
{
    std::vector<unsigned> random_seeds;

    for (size_t i = 0; i < 1000; ++i) {
        random::get()->reset_seed(false);
        random_seeds.push_back(random::get()->get_seed());
    }

    for (size_t i = 0; i < 1000; ++i) {
        for (size_t j = 0; j < 1000; ++j) {
            if (i == j) {
                continue;
            }

            //check if none of the two generated numbers are equal; in principle it is possible that they would be, but in practice we wouldn't expect this to happen
            BOOST_CHECK(random_seeds[i] != random_seeds[j]);
        }
    }
}

BOOST_AUTO_TEST_CASE(random_number_generation_test)
{
    random::get()->reset_seed(false);

    std::vector<int> random_numbers;

    for (size_t i = 0; i < 1000; ++i) {
        random_numbers.push_back(random::get()->generate(std::numeric_limits<int>::max()));
    }

    for (size_t i = 0; i < 1000; ++i) {
        for (size_t j = 0; j < 1000; ++j) {
            if (i == j) {
                continue;
            }

            //check if none of the two generated numbers are equal; in principle it is possible that they would be, but in practice we wouldn't expect this to happen
            BOOST_CHECK(random_numbers[i] != random_numbers[j]);
        }
    }
}
