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

#include "map/equirectangular_map_projection.h"
#include "map/map_projection.h"

#include "util/geocoordinate.h"
#include "util/point_util.h"

namespace wyrmgus {

map_projection *map_projection::from_string(const std::string &str)
{
	if (str == "equirectangular") {
		return equirectangular_map_projection::get();
	} else {
		throw std::runtime_error("Invalid map projection: \"" + str + "\".");
	}
}

map_projection::number_type map_projection::longitude_per_pixel(const int lon_size, const QSize &size) const
{
	return number_type(lon_size) / size.width();
}

map_projection::number_type map_projection::latitude_per_pixel(const int lat_size, const QSize &size) const
{
	return number_type(lat_size) / size.height();
}

int map_projection::unsigned_longitude_to_x(const number_type &unsigned_longitude, const number_type &lon_per_pixel) const
{
	const number_type x = unsigned_longitude / lon_per_pixel;
	return x.to_int();
}

int map_projection::unsigned_latitude_to_y(const number_type &unsigned_latitude, const number_type &lat_per_pixel) const
{
	const number_type y = unsigned_latitude / lat_per_pixel;
	return y.to_int();
}

int map_projection::longitude_to_x(const number_type &longitude, const number_type &lon_per_pixel) const
{
	const number_type x = geocoordinate::longitude_to_unsigned_longitude(longitude);
	return this->unsigned_longitude_to_x(x, lon_per_pixel);
}

int map_projection::latitude_to_y(const number_type &latitude, const number_type &lat_per_pixel) const
{
	const number_type y = geocoordinate::latitude_to_unsigned_latitude(latitude);
	return this->unsigned_latitude_to_y(y, lat_per_pixel);
}

QPoint map_projection::geocoordinate_to_point(const geocoordinate &geocoordinate, const number_type &lon_per_pixel, const number_type &lat_per_pixel) const
{
	const int x = this->longitude_to_x(geocoordinate.get_longitude(), lon_per_pixel);
	const int y = this->latitude_to_y(geocoordinate.get_latitude(), lat_per_pixel);
	return QPoint(x, y);
}

QPoint map_projection::geocoordinate_to_point(const geocoordinate &geocoordinate, const QRect &georectangle, const QSize &area_size) const
{
	const longitude lon_per_pixel = this->longitude_per_pixel(georectangle.width() - 1, area_size);
	const latitude lat_per_pixel = this->latitude_per_pixel(georectangle.height() - 1, area_size);
	const QPoint geocoordinate_offset = this->geocoordinate_to_point(wyrmgus::geocoordinate(georectangle.bottomLeft()), lon_per_pixel, lat_per_pixel);
	return this->geocoordinate_to_point(geocoordinate, lon_per_pixel, lat_per_pixel) - geocoordinate_offset;
}

geocoordinate map_projection::point_to_unsigned_geocoordinate(const QPoint &point, const QSize &area_size, const QRect &unsigned_georectangle) const
{
	const longitude lon_per_pixel = this->longitude_per_pixel(unsigned_georectangle.width() - 1, area_size);
	const latitude lat_per_pixel = this->latitude_per_pixel(unsigned_georectangle.height() - 1, area_size);

	longitude lon = longitude(point.x());
	lon *= lon_per_pixel;
	lon += unsigned_georectangle.x();

	latitude lat = latitude(point.y());
	lat *= lat_per_pixel;
	lat += unsigned_georectangle.y();

	return geocoordinate(lon, lat);
}

geocoordinate map_projection::point_to_geocoordinate(const QPoint &point, const QSize &area_size, const QRect &unsigned_georectangle) const
{
	const geocoordinate unsigned_geocoordinate = this->point_to_unsigned_geocoordinate(point, area_size, unsigned_georectangle);
	return geocoordinate::from_unsigned_geocoordinate(unsigned_geocoordinate);
}

}
