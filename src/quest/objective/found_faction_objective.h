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
//      (c) Copyright 2021 by Andrettin
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

class found_faction_objective final : public quest_objective
{
public:
	explicit found_faction_objective(const std::string &faction_identifier, const wyrmgus::quest *quest)
		: quest_objective(quest)
	{
		this->faction = faction::get(faction_identifier);
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::found_faction;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		std::string str = "Found ";

		if (this->faction->uses_definite_article()) {
			str += "the ";
		}

		str += this->faction->get_name();

		return str;
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		const wyrmgus::faction *player_faction = player->get_faction();

		if (player_faction == nullptr || !player_faction->develops_to_faction(this->faction, true)) {
			return false;
		}

		return true;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		const wyrmgus::faction *player_faction = player->get_faction();

		if (player_faction != this->faction) {
			if (player_faction == nullptr || !player_faction->develops_to_faction(this->faction, true)) {
				return std::make_pair(true, "The faction can no longer be founded.");
			}
		}

		return quest_objective::check_failure(player);
	}

	virtual void update_counter(player_quest_objective *player_quest_objective) const override
	{
		const int count = (player_quest_objective->get_player()->get_faction() == this->faction) ? 1 : 0;
		player_quest_objective->set_counter(count);
	}

private:
	const wyrmgus::faction *faction = nullptr;
};

}
