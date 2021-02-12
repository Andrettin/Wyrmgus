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
//      (c) Copyright 2019-2021 by Andrettin
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

#include "util/random.h"

#include "util/geocoordinate.h"

#include <boost/random.hpp>

namespace wyrmgus {

template <typename int_type>
int_type random::generate_in_range(std::mt19937 &engine, const int_type min_value, const int_type max_value)
{
	static_assert(std::is_integral_v<int_type>);

	//we have to use the Boost number distribution here since it is portable (has the same result with different compilers), which the standard library's isn't
	boost::random::uniform_int_distribution<int_type> distribution(min_value, max_value);
	int_type result = distribution(engine);

	return result;
}

template int32_t random::generate_in_range<int32_t>(std::mt19937 &, const int32_t, const int32_t);
template uint32_t random::generate_in_range<uint32_t>(std::mt19937 &, const uint32_t, const uint32_t);
template int64_t random::generate_in_range<int64_t>(std::mt19937 &, const int64_t, const int64_t);
template uint64_t random::generate_in_range<uint64_t>(std::mt19937 &, const uint64_t, const uint64_t);

geocoordinate random::generate_geocoordinate()
{
	static constexpr int64_t longitude_size_value = geocoordinate::longitude_size * longitude::divisor;
	static constexpr int64_t latitude_size_value = geocoordinate::latitude_size * latitude::divisor;

	longitude lon = longitude::from_value(this->generate<int64_t>(longitude_size_value) - (longitude_size_value / 2));
	latitude lat = latitude::from_value(this->generate<int64_t>(latitude_size_value) - (latitude_size_value / 2));

	return geocoordinate(std::move(lon), std::move(lat));
}

}
