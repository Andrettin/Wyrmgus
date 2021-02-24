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

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"

namespace wyrmgus {

template <typename upper_scope_type, typename scope_type>
class scope_effect_base : public effect<upper_scope_type>
{
public:
	explicit scope_effect_base(const sml_operator effect_operator) : effect<upper_scope_type>(effect_operator)
	{
		if (effect_operator != sml_operator::assignment) {
			throw std::runtime_error("Scope effects can only have the assignment operator as their operator.");
		}
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		this->effects.process_sml_property(property);
	}

	virtual void process_sml_scope(const sml_data &scope) override final
	{
		this->effects.process_sml_scope(scope);
	}

	virtual void check() const override
	{
		this->effects.check();
	}

	void do_scope_effect(scope_type *scope, const context &ctx) const
	{
		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_scope_name() const = 0;

	virtual const scope_type *get_effects_string_scope(const upper_scope_type *upper_scope) const
	{
		Q_UNUSED(upper_scope)

		return nullptr;
	}

	std::string get_assignment_string(const upper_scope_type *upper_scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override final
	{
		std::string str = this->get_scope_name() + ":\n";
		str += this->effects.get_effects_string(this->get_effects_string_scope(upper_scope), ctx, indent + 1, prefix);
		return str;
	}

private:
	effect_list<scope_type> effects;
};

}
