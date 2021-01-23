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

class random : public singleton<random>
{
public:
	static constexpr unsigned default_seed = 0x87654321;

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

	int generate()
	{
		return this->engine();
	}

	int generate(const int modulo)
	{
		return this->generate_in_range(0, modulo - 1);
	}

	int generate_in_range(const int min_value, const int max_value);

	QColor generate_color()
	{
		return QColor(this->generate(256), this->generate(256), this->generate(256));
	}

	int generate_async()
	{
		return this->async_engine();
	}

	int generate_async(const int modulo)
	{
		return this->generate_in_range_async(0, modulo - 1);
	}

	int generate_in_range_async(const int min_value, const int max_value);

private:
	std::random_device random_device;
	std::mt19937 engine;
	unsigned seed = random::default_seed;
	std::mt19937 async_engine;
};

}
