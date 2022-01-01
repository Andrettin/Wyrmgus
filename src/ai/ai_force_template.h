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
//      (c) Copyright 2015-2022 by Andrettin
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

struct lua_State;

static int CclDefineCivilization(lua_State *l);
static int CclDefineFaction(lua_State *l);

namespace wyrmgus {

class sml_data;
class sml_property;
class unit_class;
enum class ai_force_type;

class ai_force_template final
{
public:
	ai_force_template();

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	void check() const;

	ai_force_type get_force_type() const
	{
		return this->force_type;
	}

	int get_priority() const
	{
		return this->priority;
	}

	int get_weight() const
	{
		return this->weight;
	}

	const std::vector<std::pair<const unit_class *, int>> &get_units() const
	{
		return this->units;
	}

	void add_unit(const unit_class *unit_class, const int quantity)
	{
		this->units.push_back(std::pair<const wyrmgus::unit_class *, int>(unit_class, quantity));
	}

private:
	ai_force_type force_type;
	int priority = 100;
	int weight = 1;
	std::vector<std::pair<const unit_class *, int>> units;	/// vector containing each unit class belonging to the force template, and the respective quantity

	friend int ::CclDefineCivilization(lua_State *l);
	friend int ::CclDefineFaction(lua_State *l);
};


}
