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
//      (c) Copyright 2021-2022 by Andrettin
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
#include "map/mercator_map_projection.h"
#include "util/georectangle.h"
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

static void check_point_to_geocoordinate(const map_projection *map_projection)
{
    const QSize size(384, 192);

    const geocoordinate::number_type min_longitude(-11);
    const geocoordinate::number_type max_longitude(34);
    const geocoordinate::number_type min_latitude(45);
    const geocoordinate::number_type max_latitude(60);

    const georectangle georectangle(geocoordinate(min_longitude, min_latitude), geocoordinate(max_longitude, max_latitude));

    geocoordinate geocoordinate = map_projection->point_to_geocoordinate(QPoint(0, 0), georectangle, size);
    geocoordinate::number_type lon = geocoordinate.get_longitude();
    BOOST_CHECK((lon - min_longitude).abs() <= 1);
    geocoordinate::number_type lat = geocoordinate.get_latitude();
    BOOST_CHECK((lat - max_latitude).abs() <= 1);

    geocoordinate = map_projection->point_to_geocoordinate(QPoint(384, 192), georectangle, size);
    lon = geocoordinate.get_longitude();
    BOOST_CHECK((lon - max_longitude).abs() <= 1);
    lat = geocoordinate.get_latitude();
    BOOST_CHECK((lat - min_latitude).abs() <= 1);
}

BOOST_AUTO_TEST_CASE(equirectangular_point_to_geocoordinate_test)
{
    const map_projection *map_projection = equirectangular_map_projection::get();
    check_point_to_geocoordinate(map_projection);
}

BOOST_AUTO_TEST_CASE(mercator_point_to_geocoordinate_test)
{
    const map_projection *map_projection = mercator_map_projection::get();
    check_point_to_geocoordinate(map_projection);
}

BOOST_AUTO_TEST_CASE(georectangle_conversion_test)
{
    const geocoordinate::number_type min_longitude(-11);
    const geocoordinate::number_type max_longitude(34);
    const geocoordinate::number_type min_latitude(45);
    const geocoordinate::number_type max_latitude(60);

    const georectangle georectangle(geocoordinate(min_longitude, min_latitude), geocoordinate(max_longitude, max_latitude));

    const QGeoRectangle qgeorectangle = georectangle.to_qgeorectangle();
    const wyrmgus::georectangle converted_georectangle = georectangle::from_qgeorectangle(qgeorectangle);
    BOOST_CHECK(georectangle == converted_georectangle);
}
