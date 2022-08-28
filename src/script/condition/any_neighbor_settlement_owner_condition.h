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
#include "script/condition/scope_condition_base.h"
#include "unit/unit.h"
#include "util/vector_util.h"

namespace wyrmgus {

class any_neighbor_settlement_owner_condition final : public scope_condition_base<CUnit, CPlayer>
{
public:
	explicit any_neighbor_settlement_owner_condition(const gsml_operator condition_operator)
		: scope_condition_base(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_neighbor_settlement_owner";
		return class_identifier;
	}

	virtual bool check_assignment(const CUnit *unit, const read_only_context &ctx) const override
	{
		if (unit->get_settlement() == nullptr) {
			return false;
		}

		std::vector<const CPlayer *> players;

		for (const site *border_settlement : unit->get_settlement()->get_game_data()->get_border_settlements()) {
			const CPlayer *settlement_owner = border_settlement->get_game_data()->get_owner();

			if (settlement_owner == nullptr) {
				continue;
			}

			if (vector::contains(players, settlement_owner)) {
				continue;
			}

			players.push_back(settlement_owner);
		}

		for (const CPlayer *player : players) {
			if (player->is_alive() && !player->is_neutral_player()) {
				if (this->check_scope(player, ctx)) {
					return true;
				}
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any neighbor settlement owner";
	}
};

}
