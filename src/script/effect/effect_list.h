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
//      (c) Copyright 2019-2022 by Andrettin
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

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace wyrmgus {

struct context;
struct read_only_context;

template <typename scope_type>
class effect;

template <typename scope_type>
class effect_list final
{
public:
	effect_list();
	~effect_list();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check() const;
	void do_effects(scope_type *scope, context &ctx) const;
	std::string get_effects_string(const scope_type *scope, const read_only_context &ctx, const size_t indent = 0, const std::string &prefix = "") const;
	void add_effect(std::unique_ptr<effect<scope_type>> &&effect);

private:
	std::vector<std::unique_ptr<effect<scope_type>>> effects;
};

extern template class effect_list<CPlayer>;
extern template class effect_list<CUnit>;

}
