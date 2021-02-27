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

#include "util/geocoordinate.h"

#include "map/equirectangular_map_projection.h"
#include "util/georectangle_util.h"
#include "util/point_util.h"

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_CASE(point_to_geocoordinate_test)
{
    const map_projection *map_projection = equirectangular_map_projection::get();
    const QSize size(384, 192);

    const int min_longitude = -11;
    const int max_longitude = 34;
    const int min_latitude = 45;
    const int max_latitude = 60;
    const QRect georectangle(QPoint(min_longitude, min_latitude), QPoint(max_longitude, max_latitude));
    const QRect unsigned_georectangle = georectangle::to_unsigned_georectangle(georectangle);

    geocoordinate geocoordinate = map_projection->point_to_geocoordinate(QPoint(0, 0), size, unsigned_georectangle);
    BOOST_CHECK(geocoordinate.get_longitude() == min_longitude);
    BOOST_CHECK(geocoordinate.get_latitude() == max_latitude);

    geocoordinate = map_projection->point_to_geocoordinate(QPoint(383, 191), size, unsigned_georectangle);
    BOOST_CHECK(geocoordinate.get_longitude().to_int() == (max_longitude - 1));
    BOOST_CHECK(geocoordinate.get_latitude().to_int() == min_latitude);
}
