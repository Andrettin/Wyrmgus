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

#include "map/map_projection.h"

#include "map/equirectangular_map_projection.h"
#include "map/mercator_map_projection.h"

#include "util/angle_util.h"
#include "util/geocoordinate.h"
#include "util/point_util.h"

#include <boost/math/constants/constants.hpp>

namespace wyrmgus {

static double half_pi = boost::math::constants::pi<double>() / 2.;

map_projection *map_projection::from_string(const std::string &str)
{
	if (str == "equirectangular") {
		return equirectangular_map_projection::get();
	} else if (str == "mercator") {
		return mercator_map_projection::get();
	} else {
		throw std::runtime_error("Invalid map projection: \"" + str + "\".");
	}
}

map_projection::number_type map_projection::latitude_to_scaled_latitude(const number_type &lat) const
{
	return lat;
}

geocoordinate map_projection::geocoordinate_to_scaled_geocoordinate(const geocoordinate &geocoordinate) const
{
	return wyrmgus::geocoordinate(geocoordinate.get_longitude(), this->latitude_to_scaled_latitude(geocoordinate.get_latitude()));
}

map_projection::number_type map_projection::scaled_latitude_to_latitude(const number_type &scaled_lat) const
{
	return scaled_lat;
}

geocoordinate map_projection::scaled_geocoordinate_to_geocoordinate(const geocoordinate &scaled_geocoordinate) const
{
	return geocoordinate(scaled_geocoordinate.get_longitude(), this->scaled_latitude_to_latitude(scaled_geocoordinate.get_latitude()));
}

int map_projection::get_latitude_size(const QRect &georectangle) const
{
	const latitude top_lat = this->latitude_to_scaled_latitude(latitude(georectangle.top()));
	const latitude bottom_lat = this->latitude_to_scaled_latitude(latitude(georectangle.bottom()));
	return std::abs((top_lat - bottom_lat).to_int());
}

map_projection::number_type map_projection::longitude_per_pixel(const int lon_size, const QSize &size) const
{
	return number_type(lon_size) / size.width();
}

map_projection::number_type map_projection::longitude_per_pixel(const QRect &georectangle, const QSize &size) const
{
	return this->longitude_per_pixel(georectangle.width() - 1, size);
}

map_projection::number_type map_projection::latitude_per_pixel(const int lat_size, const QSize &size) const
{
	return number_type(lat_size) / size.height();
}

map_projection::number_type map_projection::latitude_per_pixel(const QRect &georectangle, const QSize &size) const
{
	const int lat_size = this->get_latitude_size(georectangle);
	return this->latitude_per_pixel(lat_size, size);
}

int map_projection::longitude_to_x(const number_type &longitude, const number_type &lon_per_pixel) const
{
	const number_type x = longitude / lon_per_pixel;
	return x.to_int();
}

int map_projection::latitude_to_y(const number_type &latitude, const number_type &lat_per_pixel) const
{
	const number_type y = latitude * -1 / lat_per_pixel;
	return y.to_int();
}

map_projection::number_type map_projection::x_to_longitude(const int x, const number_type &lon_per_pixel) const
{
	number_type lon = x * lon_per_pixel;
	return lon;
}

map_projection::number_type map_projection::y_to_latitude(const int y, const number_type &lat_per_pixel) const
{
	number_type lat = y * -1 * lat_per_pixel;
	return lat;
}

QPoint map_projection::geocoordinate_to_point(const geocoordinate &geocoordinate, const number_type &lon_per_pixel, const number_type &lat_per_pixel) const
{
	const int x = this->longitude_to_x(geocoordinate.get_longitude(), lon_per_pixel);
	const int y = this->latitude_to_y(geocoordinate.get_latitude(), lat_per_pixel);
	return QPoint(x, y);
}

QPoint map_projection::geocoordinate_to_point(const geocoordinate &geocoordinate, const QRect &georectangle, const QSize &area_size) const
{
	const longitude lon_per_pixel = this->longitude_per_pixel(georectangle, area_size);
	const latitude lat_per_pixel = this->latitude_per_pixel(georectangle, area_size);

	const wyrmgus::geocoordinate origin_geocoordinate(georectangle.bottomLeft());
	const wyrmgus::geocoordinate scaled_origin_geocoordinate = this->geocoordinate_to_scaled_geocoordinate(origin_geocoordinate);
	const QPoint geocoordinate_offset = this->geocoordinate_to_point(scaled_origin_geocoordinate, lon_per_pixel, lat_per_pixel);

	const wyrmgus::geocoordinate scaled_geocoordinate = this->geocoordinate_to_scaled_geocoordinate(geocoordinate);
	return this->geocoordinate_to_point(scaled_geocoordinate, lon_per_pixel, lat_per_pixel) - geocoordinate_offset;
}

geocoordinate map_projection::point_to_geocoordinate(const QPoint &point, const number_type &lon_per_pixel, const number_type &lat_per_pixel) const
{
	longitude lon = this->x_to_longitude(point.x(), lon_per_pixel);
	latitude lat = this->y_to_latitude(point.y(), lat_per_pixel);
	return geocoordinate(std::move(lon), std::move(lat));
}

geocoordinate map_projection::point_to_geocoordinate(const QPoint &point, const QRect &georectangle, const QSize &area_size) const
{
	const longitude lon_per_pixel = this->longitude_per_pixel(georectangle, area_size);
	const latitude lat_per_pixel = this->latitude_per_pixel(georectangle, area_size);

	const geocoordinate origin_geocoordinate(georectangle.bottomLeft());
	const geocoordinate scaled_origin_geocoordinate = this->geocoordinate_to_scaled_geocoordinate(origin_geocoordinate);
	const QPoint geocoordinate_offset = this->geocoordinate_to_point(scaled_origin_geocoordinate, lon_per_pixel, lat_per_pixel);

	const geocoordinate scaled_geocoordinate = this->point_to_geocoordinate(point + geocoordinate_offset, lon_per_pixel, lat_per_pixel);
	return this->scaled_geocoordinate_to_geocoordinate(scaled_geocoordinate);
}

mercator_map_projection::number_type mercator_map_projection::latitude_to_scaled_latitude(const number_type &lat) const
{
	const double lat_radians = angle::degrees_to_radians(lat.to_double());

	const double scaled_lat_radians = std::log(std::tan(((half_pi + lat_radians) / 2)));

	const double scaled_lat_degrees = angle::radians_to_degrees(scaled_lat_radians);
	latitude scaled_lat(scaled_lat_degrees);
	return scaled_lat;
}

mercator_map_projection::number_type mercator_map_projection::scaled_latitude_to_latitude(const number_type &scaled_lat) const
{
	const double scaled_lat_radians = angle::degrees_to_radians(scaled_lat.to_double());

	const double lat_radians = 2 * std::atan(std::exp(scaled_lat_radians)) - half_pi;

	const double lat_degrees = angle::radians_to_degrees(lat_radians);
	latitude lat(lat_degrees);
	return lat;
}

}
