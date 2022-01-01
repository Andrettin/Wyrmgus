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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "script/effect/scope_effect.h"

namespace wyrmgus {

template <typename scope_type>
class current_player_effect final : public scope_effect<scope_type, CPlayer>
{
public:
	explicit current_player_effect(const sml_operator effect_operator) : scope_effect<scope_type, CPlayer>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "current_player";
		return class_identifier;
	}

	virtual std::string get_scope_name() const override
	{
		return "Current player";
	}

	virtual const CPlayer *get_scope(const scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope)

		if (ctx.current_player == nullptr) {
			return nullptr;
		}

		return ctx.current_player;
	}

	virtual CPlayer *get_scope(const scope_type *upper_scope, const context &ctx) const override
	{
		Q_UNUSED(upper_scope)

		if (ctx.current_player == nullptr) {
			return nullptr;
		}

		return ctx.current_player;
	}
};

}
