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

#include "player/player.h"
#include "script/condition/scope_condition_base.h"

namespace wyrmgus {

template <typename upper_scope_type>
class any_player_condition final : public scope_condition_base<upper_scope_type, CPlayer>
{
public:
	bool check(const read_only_context &ctx) const
	{
		for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
			if (player->is_alive() && !player->is_neutral_player()) {
				if (this->check_scope(player.get(), ctx)) {
					return true;
				}
			}
		}

		return false;
	}

	virtual bool check(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);

		return this->check(ctx);
	}

	virtual std::string get_scope_name() const override
	{
		return "Any player";
	}
};

}
