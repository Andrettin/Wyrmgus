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
//      (c) Copyright 2020-2022 by Andrettin
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

namespace wyrmgus::vector {

static inline const std::vector<std::string> empty_string_vector;

template <typename T, typename U>
bool contains(const std::vector<T> &vector, const U &value)
{
	return std::find(vector.begin(), vector.end(), value) != vector.end();
}

template <typename T, typename U>
void merge(std::vector<T> &vector, const U &other_container)
{
	for (const typename U::value_type &element : other_container) {
		vector.push_back(element);
	}
}

template <typename T, typename U>
void merge(std::vector<T> &vector, U &&other_container)
{
	for (typename U::value_type &element : other_container) {
		vector.push_back(std::move(element));
	}
}

template <typename T>
inline void remove(std::vector<T> &vector, const typename std::vector<T>::value_type &element )
{
	vector.erase(std::remove(vector.begin(), vector.end(), element), vector.end());
}

template <typename T>
inline void remove(std::vector<std::unique_ptr<T>> &vector, T *element)
{
	//the element pointer parameter is not const so that those holding only a const pointer can't delete it this way

	for (size_t i = 0; i < vector.size();) {
		if (vector[i].get() == element) {
			vector.erase(vector.begin() + i);
			return; //since we are using unique pointers, the element is necessarily unique, so there is nothing further to do
		} else {
			++i;
		}
	}
}

template <typename T>
inline void remove_one(std::vector<T> &vector, const T &element)
{
	const auto find_iterator = std::find(vector.begin(), vector.end(), element);

	if (find_iterator == vector.end()) {
		throw std::runtime_error("Cannot remove element from vector, as it is not present in it.");
	}

	vector.erase(find_iterator);
}

template <typename T>
inline std::optional<size_t> find_index(const std::vector<T> &vector, const T &element)
{
	const auto find_iterator = std::find(vector.begin(), vector.end(), element);

	if (find_iterator == vector.end()) {
		return std::nullopt;
	}

	return static_cast<size_t>(find_iterator - vector.begin());
}

template <typename T>
T take(std::vector<T> &vector, const size_t index)
{
	T value = std::move(vector[index]);
	vector.erase(vector.begin() + index);
	return value;
}

template <typename T>
T take_front(std::vector<T> &vector)
{
	return vector::take(vector, 0);
}

template <typename T>
T take_back(std::vector<T> &vector)
{
	return vector::take(vector, vector.size() - 1);
}

template <typename T>
std::vector<T> subvector(const std::vector<T> &vector, const size_t pos, const size_t size)
{
	return std::vector<T>(vector.begin() + pos, vector.begin() + pos + size);
}

template <typename T>
std::vector<T> subvector(const std::vector<T> &vector, const size_t pos)
{
	return vector::subvector(vector, pos, vector.size() - pos);
}

template <typename T, typename function_type>
void for_each_until(const std::vector<T> &vector, function_type &function)
{
	//loops through a vector, calling a function, until the function returns true
	for (const T &element : vector) {
		if (function(element) == true) {
			break;
		}
	}
}

template <typename T, typename function_type>
void for_each_unless(const std::vector<T> &vector, function_type &function)
{
	//loops through a vector, calling a function, until the function returns false
	for (const T &element : vector) {
		if (function(element) == false) {
			break;
		}
	}
}

}
