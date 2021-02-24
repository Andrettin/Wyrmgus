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

#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "unit/unit.h"

namespace wyrmgus {

class destroy_unit_objective_base : public quest_objective
{
public:
	explicit destroy_unit_objective_base(const wyrmgus::quest *quest) : quest_objective(quest)
	{
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		if (this->get_faction() != nullptr) {
			const CPlayer *faction_player = GetFactionPlayer(this->get_faction());
			if (faction_player == nullptr || !faction_player->is_alive()) {
				return false;
			}

			if (this->get_settlement() != nullptr && !faction_player->has_settlement(this->get_settlement())) {
				return false;
			}
		}

		return true;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		if (this->get_faction() != nullptr) {
			const CPlayer *faction_player = GetFactionPlayer(this->get_faction());
			if (faction_player == nullptr || !faction_player->is_alive()) {
				return std::make_pair(true, "The target no longer exists.");
			}

			if (this->get_settlement() != nullptr && !faction_player->has_settlement(this->get_settlement())) {
				return std::make_pair(true, "The target no longer exists.");
			}
		}

		return quest_objective::check_failure(player);
	}

	virtual bool is_objective_unit(const CUnit *unit) const
	{
		if (this->get_faction() != nullptr && this->get_faction() != unit->Player->get_faction()) {
			return false;
		}

		return true;
	}

	virtual void on_unit_destroyed(const CUnit *unit, player_quest_objective *player_quest_objective) const override final
	{
		if (!this->is_objective_unit(unit)) {
			return;
		}

		player_quest_objective->increment_counter();
	}
};

}
