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
//      (c) Copyright 2020 by Andrettin
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

#pragma once

namespace stratagus::geocoordinate {

static constexpr int longitude_size = 360;
static constexpr int latitude_size = 180;
static constexpr int min_longitude = longitude_size / 2 * -1;
static constexpr int max_longitude = longitude_size / 2;
static constexpr int min_latitude = latitude_size / 2 * -1;
static constexpr int max_latitude = latitude_size / 2;
static const QGeoCoordinate min_geocoordinate(geocoordinate::min_latitude, geocoordinate::min_longitude);
static const QGeoCoordinate max_geocoordinate(geocoordinate::max_latitude, geocoordinate::max_longitude);
static const QGeoRectangle default_georect(min_geocoordinate, max_geocoordinate);

inline int longitude_to_x(const double longitude, const double lon_per_pixel)
{
	const double x = (longitude + (geocoordinate::longitude_size / 2)) / lon_per_pixel;

	return static_cast<int>(std::round(x));
}

inline int latitude_to_y(const double latitude, const double lat_per_pixel)
{
	const double y = (latitude * -1 + (geocoordinate::latitude_size / 2)) / lat_per_pixel;

	return static_cast<int>(std::round(y));
}

inline double longitude_per_pixel(const QSize &size)
{
	return geocoordinate::longitude_size / static_cast<double>(size.width());
}

inline double latitude_per_pixel(const QSize &size)
{
	return geocoordinate::latitude_size / static_cast<double>(size.height());
}

}
