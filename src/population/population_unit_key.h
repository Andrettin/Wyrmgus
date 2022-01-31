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
//      (c) Copyright 2022 by Andrettin
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

class employment_type;
class population_type;
class population_unit;

//contains the characteristics which make a population unit separate from others in its settlement
struct population_unit_key final
{
public:
	explicit population_unit_key(const population_type *type, const wyrmgus::employment_type *employment_type)
		: type(type), employment_type(employment_type)
	{
	}

	bool operator ==(const population_unit_key &rhs) const
	{
		return this->type == rhs.type && this->employment_type == rhs.employment_type;
	}

	bool operator <(const population_unit_key &rhs) const;

	const population_type *type = nullptr;
	const wyrmgus::employment_type *employment_type = nullptr;
};

}
