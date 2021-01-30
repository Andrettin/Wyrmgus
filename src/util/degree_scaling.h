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
