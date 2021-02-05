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
//      (c) Copyright 2021 by Andrettin
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

class sml_data;
class sml_property;
class world;

class landmass final
{
public:
	explicit landmass(const size_t index, const world *world = nullptr) : index(index), world(world)
	{
	}

	size_t get_index() const
	{
		return this->index;
	}

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	sml_data to_sml_data() const;

	const std::vector<const landmass *> &get_border_landmasses() const
	{
		return this->border_landmasses;
	}

	bool borders_landmass(const landmass *landmass) const;

	void add_border_landmass(const landmass *landmass)
	{
		this->border_landmasses.push_back(landmass);
	}

	const wyrmgus::world *get_world() const
	{
		return this->world;
	}

private:
	const size_t index = 0;
	std::vector<const landmass *> border_landmasses; //"landmasses" which border this one
	const wyrmgus::world *world = nullptr; //the world to which this landmass belongs
};

}
