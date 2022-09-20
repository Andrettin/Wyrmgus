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

#include "script/context.h"

class CPlayer;
class CUnit;

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace wyrmgus {

class dialogue;
class unit_ref;

template <typename scope_type>
class scripted_effect_base;

template <typename scope_type>
class delayed_effect_instance final
{
public:
	//use a unit ref if this is a unit, to ensure it remains valid
	using scope_ptr = std::conditional_t<std::is_same_v<scope_type, CUnit>, std::shared_ptr<unit_ref>, scope_type *>;

	delayed_effect_instance()
	{
	}

	explicit delayed_effect_instance(const scripted_effect_base<scope_type> *scripted_effect, scope_type *scope, const context &ctx, const int cycles);
	explicit delayed_effect_instance(const dialogue *dialogue, scope_type *scope, const context &ctx, const int cycles);

private:
	explicit delayed_effect_instance(scope_type *scope, const context &ctx, const int cycles);

public:
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	scope_type *get_scope() const;

	int get_remaining_cycles() const
	{
		return this->remaining_cycles;
	}

	void decrement_remaining_cycles()
	{
		--this->remaining_cycles;
	}

	void do_effects();

private:
	const scripted_effect_base<scope_type> *scripted_effect = nullptr;
	const wyrmgus::dialogue *dialogue = nullptr;
	scope_ptr scope = nullptr;
	wyrmgus::context context;
	int remaining_cycles = 0;
};

extern template class delayed_effect_instance<CPlayer>;
extern template class delayed_effect_instance<CUnit>;

}
