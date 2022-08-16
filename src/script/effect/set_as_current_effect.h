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
//      (c) Copyright 2022 by Andrettin
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

namespace wyrmgus {

template <typename scope_type>
class set_as_current_effect final : public effect<scope_type>
{
public:
	explicit set_as_current_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "set_as_current";
		return class_identifier;
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		if (!this->value) {
			return;
		}

		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			ctx.current_player = scope;
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			ctx.current_unit = scope->acquire_ref();
		}
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	bool value = false; //whether to set the scope as the current one in the context
};

}
