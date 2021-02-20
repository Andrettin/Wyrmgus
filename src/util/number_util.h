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

#pragma once

namespace wyrmgus::number {

extern std::string to_formatted_string(const int number);

inline std::string to_signed_string(const int number)
{
	std::string number_str;
	if (number >= 0) {
		number_str += "+";
	}
	number_str += number::to_formatted_string(number);
	return number_str;
}

inline int fast_abs(const int value)
{
	const int temp = value >> 31;
	return (value ^ temp) - temp;
}

template <typename number_type>
inline constexpr number_type pow(const number_type base, const number_type exp)
{
	static_assert(std::is_integral_v<number_type>);

	number_type value = 1;

	for (number_type i = 0; i < exp; ++i) {
		value *= base;
	}

	return value;
}

//integer cube root
template <typename number_type>
inline constexpr number_type cbrt(number_type n)
{
	static_assert(std::is_integral_v<number_type>);

	constexpr number_type bit_size = sizeof(number_type) * 8;

	//static_assert((bit_size - 1) % 3 == 0);

	number_type ret = 0;
	number_type b = 0;

	for (int s = bit_size - (bit_size % 3); s >= 0; s -= 3) {
		ret += ret;
		b = 3 * ret * (ret + 1) + 1;
		if ((n >> s) >= b) {
			n -= b << s;
			++ret;
		}
	}

	return ret;
}

}
