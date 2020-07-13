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

#include "util/geocoordinate_util.h"

#include "util/georectangle_scaling.h"

namespace stratagus::geocoordinate {

double longitude_to_scaled_longitude(const double longitude, const std::vector<std::unique_ptr<georectangle_scaling>> &georectangle_scalings)
{
	const double unsigned_longitude = geocoordinate::longitude_to_unsigned_longitude(longitude);

	double scaled_longitude = unsigned_longitude;

	for (const auto &scaling : georectangle_scalings) {
		const QGeoRectangle &georectangle = scaling->get_georectangle();
		const double min_longitude = georectangle.bottomLeft().longitude();
		const double max_longitude = georectangle.topRight().longitude();
		const int scale = scaling->get_scale();

		if (longitude >= min_longitude && longitude <= max_longitude) {
			const double longitude_diff = longitude - min_longitude;
			scaled_longitude += (longitude_diff * scale / 100) - longitude_diff;
		} else if (longitude > max_longitude) {
			scaled_longitude += (georectangle.width() * scale / 100) - georectangle.width();
		}
	}

	return scaled_longitude;
}

double latitude_to_scaled_latitude(const double latitude, const std::vector<std::unique_ptr<georectangle_scaling>> &georectangle_scalings)
{
	const double unsigned_latitude = geocoordinate::latitude_to_unsigned_latitude(latitude);

	double scaled_latitude = unsigned_latitude;

	for (const auto &scaling : georectangle_scalings) {
		const QGeoRectangle &georectangle = scaling->get_georectangle();
		const double min_latitude = georectangle.bottomLeft().latitude();
		const double max_latitude = georectangle.topRight().latitude();
		const int scale = scaling->get_scale();

		if (latitude >= min_latitude && latitude <= max_latitude) {
			const double latitude_diff = latitude - min_latitude;
			scaled_latitude += (latitude_diff * scale / 100) - latitude_diff;
		} else if (latitude > max_latitude) {
			scaled_latitude += (georectangle.height() * scale / 100) - georectangle.height();
		}
	}

	return scaled_latitude;
}

double scaled_longitude_size(const QGeoRectangle &georectangle, const std::vector<std::unique_ptr<georectangle_scaling>> &georectangle_scalings)
{
	const double min_longitude = georectangle.bottomLeft().longitude();
	const double max_longitude = georectangle.topRight().longitude();

	const double min_scaled_longitude = longitude_to_scaled_longitude(min_longitude, georectangle_scalings);
	const double max_scaled_longitude = longitude_to_scaled_longitude(max_longitude, georectangle_scalings);
	
	return max_scaled_longitude - min_scaled_longitude;
}

double scaled_latitude_size(const QGeoRectangle &georectangle, const std::vector<std::unique_ptr<georectangle_scaling>> &georectangle_scalings)
{
	const double min_latitude = georectangle.bottomLeft().latitude();
	const double max_latitude = georectangle.topRight().latitude();

	const double min_scaled_latitude = latitude_to_scaled_latitude(min_latitude, georectangle_scalings);
	const double max_scaled_latitude = latitude_to_scaled_latitude(max_latitude, georectangle_scalings);

	return max_scaled_latitude - min_scaled_latitude;
}

}
