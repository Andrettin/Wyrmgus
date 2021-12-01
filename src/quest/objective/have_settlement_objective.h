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

#include "ai/ai_local.h"
#include "database/defines.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "script/condition/and_condition.h"
#include "unit/unit.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

class have_settlement_objective final : public quest_objective
{
public:
	explicit have_settlement_objective(const std::string &settlement_identifier, const wyrmgus::quest *quest)
		: quest_objective(quest)
	{
		this->settlement = site::get(settlement_identifier);
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::have_settlement;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		return "Own the " + this->settlement->get_game_data()->get_current_cultural_name() + " settlement";
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		if (!player->has_settlement(this->settlement)) {
			const unit_type *town_hall_type = player->get_faction()->get_class_unit_type(defines::get()->get_town_hall_class());

			if (town_hall_type == nullptr) {
				return false;
			}

			if (!player->HasUnitBuilder(town_hall_type) || !check_conditions(town_hall_type, player)) {
				return false;
			}
		}

		return true;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		if (!player->has_settlement(this->settlement)) {
			const unit_type *town_hall_type = player->get_faction()->get_class_unit_type(defines::get()->get_town_hall_class());

			if (town_hall_type == nullptr) {
				return std::make_pair(true, "You can no longer build the required building to take possession of the settlement.");
			}

			if (!player->HasUnitBuilder(town_hall_type) || !check_conditions(town_hall_type, player)) {
				return std::make_pair(true, "You can no longer build the required building to take possession of the settlement.");
			}
		}

		return quest_objective::check_failure(player);
	}

	virtual void update_counter(player_quest_objective *player_quest_objective) const override
	{
		const int count = player_quest_objective->get_player()->has_settlement(this->settlement) ? 1 : 0;
		player_quest_objective->set_counter(count);
	}

	virtual void check_ai(PlayerAi *ai_player, const player_quest_objective *player_quest_objective) const override
	{
		Q_UNUSED(player_quest_objective)

		CPlayer *player = ai_player->Player;

		const site_game_data *settlement_game_data = this->settlement->get_game_data();
		CPlayer *settlement_owner = settlement_game_data->get_owner();

		if (settlement_owner == nullptr) {
			//perform the settlement construction check for the settlement
			ai_player->check_settlement_construction({ this->settlement });
		} else if (settlement_owner != player && !player->has_enemy_stance_with(settlement_owner) && !player->at_war()) {
			if (player->can_declare_war_on(settlement_owner) && player->has_military_advantage_over(settlement_owner)) {
				//declare war on the owner of the settlement if we aren't at war already
				player->set_enemy_diplomatic_stance_with(settlement_owner);
			}
		}
	}

private:
	const site *settlement = nullptr;
};

}
