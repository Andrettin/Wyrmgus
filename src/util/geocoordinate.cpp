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

#include "util/geocoordinate.h"

#include "util/point_util.h"
#include "util/random.h"

namespace wyrmgus {

void geocoordinate::for_each_random_until(const std::function<bool(const geocoordinate &)> &function)
{
	//call a function for each possible geocoordinate until the function returns true, going through them in a random manner

	static constexpr uint64_t max_potential_longitudes = static_cast<uint64_t>((geocoordinate::max_longitude - geocoordinate::min_longitude).get_value());
	static constexpr uint64_t max_potential_latitudes = static_cast<uint64_t>((geocoordinate::max_latitude - geocoordinate::min_latitude).get_value());
	static constexpr uint64_t max_potential_geocoordinates = max_potential_longitudes * max_potential_latitudes;

	geocoordinate_set checked_geocoordinates;

	while (checked_geocoordinates.size() < max_potential_geocoordinates && checked_geocoordinates.size() < std::numeric_limits<size_t>::max()) {
		const geocoordinate geocoordinate = random::get()->generate_geocoordinate();

		if (checked_geocoordinates.contains(geocoordinate)) {
			continue;
		} else {
			checked_geocoordinates.insert(geocoordinate);
		}

		if (function(geocoordinate)) {
			break;
		}
	}
}

QPoint geocoordinate::to_circle_edge_point() const
{
	const QPoint circle_point = this->to_circle_point();
	return point::get_nearest_circle_edge_point(circle_point, 1 * number_type::divisor);
}

bool geocoordinate_compare::operator()(const geocoordinate &geocoordinate, const wyrmgus::geocoordinate &other_geocoordinate) const
{
	if (geocoordinate.get_longitude() != other_geocoordinate.get_longitude()) {
		return geocoordinate.get_longitude() < other_geocoordinate.get_longitude();
	}

	return geocoordinate.get_latitude() < other_geocoordinate.get_latitude();
}

}
