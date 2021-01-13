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

inline std::string to_centesimal_rest_string(const int rest)
{
	std::string rest_str;
	const int abs_rest = std::abs(rest);
	if (abs_rest > 0) {
		rest_str += ".";
		if (abs_rest < 10) {
			rest_str += "0";
		}
		rest_str += std::to_string(abs_rest);
	}
	return rest_str;
}

inline std::string to_centesimal_string(const int number)
{
	std::string number_str = std::to_string(number / 100);
	number_str += number::to_centesimal_rest_string(number % 100);
	return number_str;
}

inline std::string to_signed_centesimal_string(const int number)
{
	std::string number_str = number::to_signed_string(number / 100);
	number_str += number::to_centesimal_rest_string(number % 100);
	return number_str;
}

inline int fast_abs(const int value)
{
	const int temp = value >> 31;
	return (value ^ temp) - temp;
}

}
