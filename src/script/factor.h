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

class CPlayer;
class CUnit;

namespace wyrmgus {

class sml_data;
class sml_property;

template <typename T>
class factor_modifier;

//a scripted factor, i.e. a random chance, weight or score
template <typename scope_type>
class factor
{
public:
	factor();
	explicit factor(const int base_value);
	~factor();

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	int calculate(const scope_type *scope) const;

private:
	int base_value = 0; //the base value for the factor
	std::vector<std::unique_ptr<factor_modifier<scope_type>>> modifiers; //modifiers for the factor
};

extern template class factor<CPlayer>;
extern template class factor<CUnit>;

}
