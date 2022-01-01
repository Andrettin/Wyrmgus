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

#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"

namespace wyrmgus {

class destroy_faction_objective final : public quest_objective
{
public:
	explicit destroy_faction_objective(const wyrmgus::quest *quest) : quest_objective(quest)
	{
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::destroy_faction;
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		const CPlayer *faction_player = GetFactionPlayer(this->get_faction());
		if (faction_player == nullptr || !faction_player->is_alive()) {
			return false;
		}

		return true;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		//if is supposed to destroy a faction, but it is nowhere to be found, fail the quest
		const CPlayer *faction_player = GetFactionPlayer(this->get_faction());
		if (faction_player == nullptr || !faction_player->is_alive()) {
			return std::make_pair(true, "The target no longer exists.");
		}

		return quest_objective::check_failure(player);
	}

	virtual void on_unit_destroyed(const CUnit *unit, player_quest_objective *player_quest_objective) const override
	{
		const CPlayer *faction_player = GetFactionPlayer(this->get_faction());

		if (faction_player == nullptr) {
			return;
		}

		int dying_faction_units = (faction_player == unit->Player) ? 1 : 0;
		dying_faction_units += unit->GetTotalInsideCount(faction_player, true, true);

		if (dying_faction_units == 0 || faction_player->GetUnitCount() > dying_faction_units) {
			return;
		}

		player_quest_objective->increment_counter();
	}
};

}
