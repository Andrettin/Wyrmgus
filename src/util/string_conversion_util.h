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

namespace string {

inline bool to_bool(const std::string &str)
{
	if (str == "true" || str == "yes" || str == "1") {
		return true;
	} else if (str == "false" || str == "no" || str == "0") {
		return false;
	}

	throw std::runtime_error("Invalid string used for conversion to boolean: \"" + str + "\".");
}

inline char to_character(const std::string &str)
{
	if (str.size() != 1) {
		throw std::runtime_error("Character string \"" + str + "\" has a string size different than 1.");
	}

	return str.front();
}

template <int N>
inline int fractional_number_string_to_int(const std::string &str)
{
	size_t decimal_point_pos = str.find('.');
	int integer = 0;
	int fraction = 0;
	if (decimal_point_pos != std::string::npos) {
		integer = std::stoi(str.substr(0, decimal_point_pos));
		const size_t decimal_pos = decimal_point_pos + 1;
		const size_t decimal_places = str.length() - decimal_pos;
		fraction = std::stoi(str.substr(decimal_pos, decimal_places));
		if (decimal_places < N) {
			for (int i = static_cast<int>(decimal_places); i < N; ++i) {
				fraction *= 10;
			}
		}
		const bool negative = str.front() == '-';
		if (negative) {
			fraction *= -1;
		}
	} else {
		integer = std::stoi(str);
	}

	for (int i = 0; i < N; ++i) {
		integer *= 10;
	}
	integer += fraction;

	return integer;
}

inline int centesimal_number_string_to_int(const std::string &str)
{
	return fractional_number_string_to_int<2>(str);
}

inline int millesimal_number_string_to_int(const std::string &str)
{
	return fractional_number_string_to_int<3>(str);
}

extern QDateTime to_date(const std::string &date_str);

}
