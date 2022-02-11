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

#include "script/effect/effect.h"
#include "script/effect/scripted_effect.h"

namespace wyrmgus {

template <typename scope_type>
class scripted_effect_base;

template <typename scope_type>
class scripted_effect_effect final : public effect<scope_type>
{
public:
	explicit scripted_effect_effect(const std::string &effect_identifier, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		static_assert(std::is_same_v<scope_type, CPlayer> || std::is_same_v<scope_type, CUnit>);

		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			this->scripted_effect = player_scripted_effect::get(effect_identifier);
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			this->scripted_effect = unit_scripted_effect::get(effect_identifier);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "scripted_effect";
		return class_identifier;
	}

	virtual void do_assignment_effect(scope_type *scope, const context &ctx) const override
	{
		this->scripted_effect->get_effects().do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		return this->scripted_effect->get_effects().get_effects_string(scope, ctx, indent, prefix);
	}

private:
	const scripted_effect_base<scope_type> *scripted_effect = nullptr;
};

}
