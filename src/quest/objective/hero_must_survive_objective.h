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

#include "character.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"

namespace wyrmgus {

class hero_must_survive_objective final : public quest_objective
{
public:
	explicit hero_must_survive_objective(const std::string &character_identifier, const wyrmgus::quest *quest)
		: quest_objective(quest)
	{
		this->character = character::get(character_identifier);
	}

	virtual void check() const override
	{
		if (this->character == nullptr) {
			throw std::runtime_error("Hero must survive quest objective has no character set for it.");
		}
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::hero_must_survive;
	}

	virtual int get_quantity() const override
	{
		//always 0
		return 0;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		return this->character->get_full_name() + " must survive";
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		if (!player->HasHero(this->character)) {
			return false;
		}

		return true;
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		if (!player->HasHero(this->character)) {
			return std::make_pair(true, "A hero necessary for the quest has died.");
		}

		return quest_objective::check_failure(player);
	}

private:
	const wyrmgus::character *character = nullptr;
};

}
