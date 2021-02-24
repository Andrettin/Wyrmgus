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

#pragma once

#include "util/random.h"

namespace wyrmgus::vector {

template <typename T>
inline const T &get_random(const std::vector<T> &vector)
{
	return vector[random::get()->generate(vector.size())];
}

template <typename T>
inline const T &get_random_async(const std::vector<T> &vector)
{
	return vector[random::get()->generate_async(vector.size())];
}

template <typename T>
inline T take_random(std::vector<T> &vector)
{
	const size_t index = random::get()->generate(vector.size());
	T element = std::move(vector[index]);
	vector.erase(vector.begin() + index);
	return element;
}

template <typename T, typename function_type>
inline void process_randomly(std::vector<T> &vector, const function_type &function)
{
	while (!vector.empty()) {
		T element = vector::take_random(vector);
		function(std::move(element));
	}
}

template <typename T, typename function_type>
inline void process_randomly(const std::vector<T> &vector, const function_type &function)
{
	std::vector<T> vector_copy = vector;
	vector::process_randomly(vector_copy, function);
}

}
