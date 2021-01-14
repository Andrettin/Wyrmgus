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
//

#include "stratagus.h"

#include "script/effect/delayed_effect_instance.h"

#include "database/sml_data.h"
#include "script/effect/effect_list.h"
#include "unit/unit.h"
#include "unit/unit_ref.h"

namespace wyrmgus {

template <typename scope_type>
delayed_effect_instance<scope_type>::delayed_effect_instance(const effect_list<scope_type> *effects, scope_type *scope, const wyrmgus::context &ctx, const int cycles)
	: effects(effects), context(ctx), remaining_cycles(cycles)
{
	if constexpr (std::is_same_v<scope_type, CUnit>) {
		this->scope = scope->acquire_ref();
	} else {
		this->scope = scope;
	}
}

template <typename scope_type>
scope_type *delayed_effect_instance<scope_type>::get_scope() const
{
	if constexpr (std::is_same_v<scope_type, CUnit>) {
		return this->scope->get();
	} else {
		return this->scope;
	}
}

template <typename scope_type>
void delayed_effect_instance<scope_type>::do_effects()
{
	scope_type *scope = this->get_scope();

	if constexpr (std::is_same_v<scope_type, CUnit>) {
		//ignore the effects if the unit has been destroyed in the meantime
		if (scope->Destroyed) {
			return;
		}
	}

	this->effects->do_effects(scope, this->context);
}

template <typename scope_type>
sml_data delayed_effect_instance<scope_type>::to_sml_data() const
{
	sml_data data;

	//FIXME: write a way to refer to the effects, perhaps requiring that delayed effects are pre-scripted ones
	std::string scope;
	if constexpr (std::is_same_v<scope_type, CPlayer>) {
		scope = std::to_string(this->scope->get_index());
	} else {
		scope = std::to_string(UnitNumber(*this->scope->get()));
	}
	data.add_property("scope", std::move(scope));

	data.add_property("remaining_cycles", std::to_string(this->get_remaining_cycles()));

	data.add_child(this->context.to_sml_data());

	return data;
}

template class delayed_effect_instance<CPlayer>;
template class delayed_effect_instance<CUnit>;

}
