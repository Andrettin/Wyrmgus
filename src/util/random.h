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
//      (c) Copyright 2019-2020 by Andrettin
//
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "util/singleton.h"

namespace stratagus {

class random : public singleton<random>
{
public:
	static constexpr unsigned default_seed = 0x87654321;

	random()
	{
		this->engine = std::mt19937(random::default_seed);
	}

	unsigned get_seed() const
	{
		return this->seed;
	}

	void set_seed(unsigned seed)
	{
		this->seed = seed;
		this->engine.seed(seed);
	}
	
	void reset_seed(const bool default)
	{
		if (default) {
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

private:
	std::random_device random_device;
	std::mt19937 engine;
	unsigned seed = random::default_seed;
};

}
