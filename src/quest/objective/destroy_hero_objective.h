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

#include "quest/objective/destroy_unit_objective_base.h"
#include "quest/objective_type.h"

namespace wyrmgus {

class destroy_hero_objective final : public destroy_unit_objective_base
{
public:
	explicit destroy_hero_objective(const std::string &character_identifier, const wyrmgus::quest *quest)
		: destroy_unit_objective_base(quest)
	{
		this->character = character::get(character_identifier);
	}

	virtual objective_type get_objective_type() const override
	{
		return objective_type::destroy_hero;
	}

	virtual std::string generate_objective_string(const CPlayer *player) const override
	{
		Q_UNUSED(player)

		return "Kill " + this->character->get_full_name();
	}

	virtual bool is_quest_acceptance_allowed(const CPlayer *player) const override
	{
		if (this->character->CanAppear()) {
			//if the character "can appear" it doesn't already exist, and thus can't be destroyed
			return false;
		}

		return destroy_unit_objective_base::is_quest_acceptance_allowed(player);
	}

	virtual std::pair<bool, std::string> check_failure(const CPlayer *player) const override
	{
		if (this->character->CanAppear()) {
			//if is supposed to destroy a character, but it is nowhere to be found, fail the quest
			return std::make_pair(true, "The target no longer exists.");
		}

		return destroy_unit_objective_base::check_failure(player);
	}

	virtual bool is_objective_unit(const CUnit *unit) const override
	{
		if (unit->get_character() == nullptr || this->character != unit->get_character()) {
			return false;
		}

		return destroy_unit_objective_base::is_objective_unit(unit);
	}

private:
	const wyrmgus::character *character = nullptr;
};

}
