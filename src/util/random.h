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

#pragma once

#include "util/singleton.h"

namespace wyrmgus {

class geocoordinate;

class random final : public singleton<random>
{
public:
	static constexpr unsigned default_seed = 0x87654321;

	template <typename int_type>
	static int_type generate_in_range(std::mt19937 &engine, const int_type min_value, const int_type max_value);

	random()
	{
		this->reset_seed(false);
		this->async_engine.seed(this->random_device());
	}

	unsigned get_seed() const
	{
		return this->seed;
	}

	void set_seed(const unsigned seed)
	{
		this->seed = seed;
		this->engine.seed(seed);
	}
	
	void reset_seed(const bool default_seed)
	{
		if (default_seed) {
			//reset the seed to the default
			this->set_seed(random::default_seed);
		} else {
			this->set_seed(this->random_device());
		}
	}

	template <typename int_type>
	int_type generate(const int_type modulo)
	{
		static_assert(std::is_integral_v<int_type>);
		return this->generate_in_range<int_type>(0, modulo - 1);
	}

	template <typename int_type>
	int_type generate_in_range(const int_type min_value, const int_type max_value)
	{
		return random::generate_in_range<int_type>(this->engine, min_value, max_value);
	}

	QPoint generate_circle_point()
	{
		return QPoint(this->generate(360) - 180, this->generate(360) - 180);
	}

	QColor generate_color()
	{
		return QColor(this->generate(256), this->generate(256), this->generate(256));
	}

	geocoordinate generate_geocoordinate();

	template <typename int_type>
	int_type generate_async(const int_type modulo)
	{
		static_assert(std::is_integral_v<int_type>);
		return this->generate_in_range_async<int_type>(0, modulo - 1);
	}

	template <typename int_type>
	int_type generate_in_range_async(const int_type min_value, const int_type max_value)
	{
		return random::generate_in_range<int_type>(this->async_engine, min_value, max_value);
	}

private:
	std::random_device random_device;
	std::mt19937 engine;
	unsigned seed = random::default_seed;
	std::mt19937 async_engine;
};

extern template int32_t random::generate_in_range<int32_t>(std::mt19937 &, const int32_t, const int32_t);
extern template uint32_t random::generate_in_range<uint32_t>(std::mt19937 &, const uint32_t, const uint32_t);
extern template int64_t random::generate_in_range<int64_t>(std::mt19937 &, const int64_t, const int64_t);
extern template uint64_t random::generate_in_range<uint64_t>(std::mt19937 &, const uint64_t, const uint64_t);

}
