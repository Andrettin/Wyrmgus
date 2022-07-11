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

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/scope_condition.h"
#include "unit/unit.h"

namespace wyrmgus {

class settlement_owner_condition final : public scope_condition<CPlayer>
{
public:
	virtual const CPlayer *get_scope(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(player)
		Q_UNUSED(ctx)

		return nullptr;
	}

	virtual const CPlayer *get_scope(const CUnit *unit, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx)

		if (unit->get_settlement() != nullptr) {
			return unit->get_settlement()->get_game_data()->get_owner();
		}

		return nullptr;
	}

	virtual std::string get_scope_name() const override
	{
		return "Settlement owner";
	}
};

}
