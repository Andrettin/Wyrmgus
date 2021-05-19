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

#include "map/site_game_data.h"
#include "player.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "quest/player_quest_objective.h"
#include "unit/unit.h"

namespace wyrmgus {

class bring_unit_to_site_objective final : public quest_objective
{
public:
	explicit bring_unit_to_site_objective(const wyrmgus::quest *quest) : quest_objective(quest)
	{
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::bring_unit_to_site;
	}

	virtual void process_sml_property(const wyrmgus::sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "character") {
			this->character = character::get(value);
		} else if (key == "site") {
			this->site = site::get(value);
		} else {
			quest_objective::process_sml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->character == nullptr) {
			throw std::runtime_error("Bring unit to site quest objective has no character set for it.");
		}

		if (this->site == nullptr) {
			throw std::runtime_error("Bring unit to site quest objective has no site set for it.");
		}
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		std::string str = "Bring ";
		str += this->character->get_full_name();
		str += " to ";
		str += this->site->get_game_data()->get_current_cultural_name();

		const site_game_data *site_data = site->get_game_data();
		const CUnit *site_unit = site_data->get_site_unit();

		if (site_unit != nullptr) {
			str += " (" + site_unit->get_type_name() + ")";
		}

		return str;
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		if (!player->HasHero(this->character)) {
			return false;
		}

		const site_game_data *site_data = this->site->get_game_data();
		if (!site_data->is_on_map()) {
			return false;
		}

		return true;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		if (!player->HasHero(this->character)) {
			return std::make_pair(true, "You no longer have the required hero.");
		}

		return quest_objective::check_failure(player);
	}

	virtual void update_counter(player_quest_objective *player_quest_objective) const override
	{
		int count = 0;

		const CUnit *character_unit = this->character->get_unit();
		if (character_unit != nullptr && character_unit->is_near_site(this->site)) {
			count = 1;
		}

		player_quest_objective->set_counter(count);
	}

private:
	const wyrmgus::character *character = nullptr;
	const wyrmgus::site *site = nullptr;
};

}
