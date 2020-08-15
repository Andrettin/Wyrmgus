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
//      (c) Copyright 2020 by Andrettin
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

namespace wyrmgus {

class degree_scaling
{
public:
	explicit degree_scaling(const double min_degree, const double max_degree, const int scale)
		: min_degree(min_degree), max_degree(max_degree), scale(scale)
	{
	}

	double get_min_degree() const
	{
		return this->min_degree;
	}

	double get_max_degree() const
	{
		return this->max_degree;
	}

	double get_length() const
	{
		return this->get_max_degree() - this->get_min_degree();
	}

	bool contains(const double degree) const
	{
		return degree >= this->get_min_degree() && degree <= this->get_max_degree();
	}

	int get_scale() const
	{
		return this->scale;
	}

private:
	double min_degree = 0;
	double max_degree = 0;
	int scale = 100; //in percent
};

}
