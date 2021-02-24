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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "util/geocoordinate_util.h"

#include "util/degree_scaling.h"

namespace wyrmgus::qgeocoordinate {

double longitude_to_scaled_longitude(const double longitude, const std::vector<std::unique_ptr<degree_scaling>> &degree_scalings)
{
	const double unsigned_longitude = qgeocoordinate::longitude_to_unsigned_longitude(longitude);

	double scaled_longitude = unsigned_longitude;

	for (const auto &scaling : degree_scalings) {
		const double min_lon = scaling->get_min_degree();
		const double max_lon = scaling->get_max_degree();
		const int scale = scaling->get_scale();
		const double longitude_length = scaling->get_length();

		if (scaling->contains(longitude)) {
			const double longitude_diff = longitude - min_lon;
			scaled_longitude += (longitude_diff * scale / 100) - longitude_diff;
		} else if (longitude > max_lon) {
			scaled_longitude += (longitude_length * scale / 100) - longitude_length;
		}
	}

	return scaled_longitude;
}

double latitude_to_scaled_latitude(const double latitude, const std::vector<std::unique_ptr<degree_scaling>> &degree_scalings)
{
	const double unsigned_latitude = qgeocoordinate::latitude_to_unsigned_latitude(latitude);

	double scaled_latitude = unsigned_latitude;

	for (const auto &scaling : degree_scalings) {
		const double min_lat = scaling->get_min_degree();
		const double max_lat = scaling->get_max_degree();
		const int scale = scaling->get_scale();
		const double latitude_length = scaling->get_length();

		if (scaling->contains(latitude)) {
			const double latitude_diff = latitude - min_lat;
			scaled_latitude += (latitude_diff * scale / 100) - latitude_diff;
		} else if (latitude > max_lat) {
			scaled_latitude += (latitude_length * scale / 100) - latitude_length;
		}
	}

	return scaled_latitude;
}

double scaled_longitude_size(const QGeoRectangle &georectangle, const std::vector<std::unique_ptr<degree_scaling>> &degree_scalings)
{
	const double min_lon = georectangle.bottomLeft().longitude();
	const double max_lon = georectangle.topRight().longitude();

	const double min_scaled_longitude = longitude_to_scaled_longitude(min_lon, degree_scalings);
	const double max_scaled_longitude = longitude_to_scaled_longitude(max_lon, degree_scalings);
	
	return max_scaled_longitude - min_scaled_longitude;
}

double scaled_latitude_size(const QGeoRectangle &georectangle, const std::vector<std::unique_ptr<degree_scaling>> &degree_scalings)
{
	const double min_lat = georectangle.bottomLeft().latitude();
	const double max_lat = georectangle.topRight().latitude();

	const double min_scaled_latitude = latitude_to_scaled_latitude(min_lat, degree_scalings);
	const double max_scaled_latitude = latitude_to_scaled_latitude(max_lat, degree_scalings);

	return max_scaled_latitude - min_scaled_latitude;
}

}
