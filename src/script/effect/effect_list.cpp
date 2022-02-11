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

#include "stratagus.h"

#include "script/effect/effect_list.h"

#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "script/effect/effect.h"

namespace wyrmgus {

template <typename scope_type>
effect_list<scope_type>::effect_list()
{
}

template <typename scope_type>
effect_list<scope_type>::~effect_list()
{
}

template <typename scope_type>
void effect_list<scope_type>::process_gsml_property(const gsml_property &property)
{
	this->effects.push_back(effect<scope_type>::from_gsml_property(property));
}

template <typename scope_type>
void effect_list<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	this->effects.push_back(effect<scope_type>::from_gsml_scope(scope));
}

template <typename scope_type>
void effect_list<scope_type>::check() const
{
	for (const std::unique_ptr<effect<scope_type>> &effect : this->effects) {
		effect->check();
	}
}

template <typename scope_type>
void effect_list<scope_type>::do_effects(scope_type *scope, const context &ctx) const
{
	for (const std::unique_ptr<effect<scope_type>> &effect : this->effects) {
		effect->do_effect(scope, ctx);
	}
}

template <typename scope_type>
std::string effect_list<scope_type>::get_effects_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
{
	std::string effects_string;
	bool first = true;
	for (const std::unique_ptr<effect<scope_type>> &effect : this->effects) {
		if (effect->is_hidden()) {
			continue;
		}

		const std::string effect_string = effect->get_string(scope, ctx, indent, prefix);
		if (effect_string.empty()) {
			continue;
		}

		if (first) {
			first = false;
		} else {
			effects_string += "\n";
		}

		if (indent > 0) {
			effects_string += std::string(indent, '\t');
		}

		if (!effect_string.starts_with(prefix)) {
			//add prefix if not already prefixed
			effects_string += prefix;
		}
		effects_string += effect_string;
	}
	return effects_string;
}

template <typename scope_type>
void effect_list<scope_type>::add_effect(std::unique_ptr<effect<scope_type>> &&effect)
{
	this->effects.push_back(std::move(effect));
}

template class effect_list<CPlayer>;
template class effect_list<CUnit>;

}
