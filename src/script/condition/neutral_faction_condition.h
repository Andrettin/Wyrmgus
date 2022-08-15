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
#include "player/faction.h"
#include "player/player.h"
#include "script/condition/condition.h"
#include "unit/unit.h"

namespace wyrmgus {

class neutral_faction_condition final : public condition<CPlayer>
{
public:
	explicit neutral_faction_condition(const std::string &value)
	{
		this->faction = faction::get(value);
	}

	virtual void check_validity() const override
	{
		assert_throw(this->faction != nullptr);
		assert_throw(this->faction->has_neutral_type());
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const CPlayer *neutral_faction_player = GetFactionPlayer(this->faction);

		if (neutral_faction_player == nullptr) {
			return false;
		}

		for (const CUnit *unit : neutral_faction_player->get_units()) {
			if (!unit->Type->BoolFlag[BUILDING_INDEX].value) {
				continue;
			}

			const site *settlement = unit->get_settlement();
			if (settlement != nullptr && settlement->get_game_data()->get_owner() == player) {
				return true;
			}
		}

		return false;
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)

		return condition::get_object_string(this->faction, links_allowed) + " neutral faction in territory";
	}

private:
	const wyrmgus::faction *faction = nullptr;
};

}
