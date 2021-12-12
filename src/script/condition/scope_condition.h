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

#include "script/condition/scope_condition_base.h"
#include "unit/unit.h"

namespace wyrmgus {

template <typename scope_type>
class scope_condition : public scope_condition_base<scope_type>
{
public:
	virtual const scope_type *get_scope(const CPlayer *player, const read_only_context &ctx) const = 0;

	virtual const scope_type *get_scope(const CUnit *unit, const read_only_context &ctx) const
	{
		return this->get_scope(unit->Player, ctx);
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units) const override final
	{
		const scope_type *scope = this->get_scope(player, ctx);

		if (scope == nullptr) {
			return false;
		}

		return this->check_scope(scope, ctx, ignore_units);
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units) const override final
	{
		const scope_type *scope = this->get_scope(unit, ctx);
		return this->check_scope(scope, ctx, ignore_units);
	}
};

}
