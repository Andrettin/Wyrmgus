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

#include <util/fractional_int.h>

class CPlayer;
class CUnit;

namespace wyrmgus {

class and_condition;
class gsml_data;
class gsml_property;

//a modifier for a factor, e.g. a random chance or weight
template <typename scope_type>
class factor_modifier
{
public:
	factor_modifier();
	~factor_modifier();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check_validity() const;

	const centesimal_int &get_factor() const
	{
		return this->factor;
	}

	bool is_additive() const
	{
		return this->additive;
	}

	bool check_conditions(const scope_type *scope) const;

private:
	centesimal_int factor; //the factor of the modifier itself
	bool additive = false; //whether the modifier is additive instead of multiplicative
	std::unique_ptr<and_condition> conditions; //conditions for whether the modifier is to be applied
};

extern template class factor_modifier<CPlayer>;
extern template class factor_modifier<CUnit>;

}
