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

#include "script/effect/scope_effect_base.h"

namespace wyrmgus {

template <typename upper_scope_type, typename scope_type>
class scope_effect : public scope_effect_base<upper_scope_type, scope_type>
{
public:
	explicit scope_effect(const gsml_operator effect_operator) : scope_effect_base<upper_scope_type, scope_type>(effect_operator)
	{
	}

	virtual scope_type *get_scope(const upper_scope_type *upper_scope) const
	{
		Q_UNUSED(upper_scope);

		return nullptr;
	}

	virtual const scope_type *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const
	{
		Q_UNUSED(ctx);

		return this->get_scope(upper_scope);
	}

	virtual scope_type *get_scope(const upper_scope_type *upper_scope, const context &ctx) const
	{
		Q_UNUSED(ctx);

		return this->get_scope(upper_scope);
	}

	virtual void do_assignment_effect(upper_scope_type *upper_scope, const context &ctx) const override final
	{
		scope_type *new_scope = this->get_scope(upper_scope, ctx);
		this->do_scope_effect(new_scope, ctx);
	}

	virtual const scope_type *get_effects_string_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override final
	{
		return this->get_scope(upper_scope, ctx);
	}
};

}
