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

#pragma once

namespace wyrmgus {

class geocoordinate;

template <int N>
class fractional_int;

class map_projection
{
public:
	using number_type = fractional_int<4>;

	static map_projection *from_string(const std::string &str);

	virtual number_type latitude_to_scaled_latitude(const number_type &lat) const;
	geocoordinate geocoordinate_to_scaled_geocoordinate(const geocoordinate &geocoordinate) const;

	virtual number_type scaled_latitude_to_latitude(const number_type &scaled_lat) const;
	geocoordinate scaled_geocoordinate_to_geocoordinate(const geocoordinate &scaled_geocoordinate) const;

	int get_latitude_size(const QRect &georectangle) const;

	number_type longitude_per_pixel(const int lon_size, const QSize &size) const;
	number_type latitude_per_pixel(const int lat_size, const QSize &size) const;

	int longitude_to_x(const number_type &longitude, const number_type &lon_per_pixel) const;
	int latitude_to_y(const number_type &latitude, const number_type &lat_per_pixel) const;
	number_type x_to_longitude(const int x, const number_type &lon_per_pixel) const;
	number_type y_to_latitude(const int y, const number_type &lat_per_pixel) const;

	QPoint geocoordinate_to_point(const geocoordinate &geocoordinate, const number_type &lon_per_pixel, const number_type &lat_per_pixel) const;
	QPoint geocoordinate_to_point(const geocoordinate &geocoordinate, const QRect &georectangle, const QSize &area_size) const;

	geocoordinate point_to_geocoordinate(const QPoint &point, const number_type &lon_per_pixel, const number_type &lat_per_pixel) const;
	geocoordinate point_to_geocoordinate(const QPoint &point, const QRect &georectangle, const QSize &area_size) const;
};

}

Q_DECLARE_METATYPE(wyrmgus::map_projection *)
