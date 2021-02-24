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

#include "character.h"
#include "player.h"
#include "script/effect/effect.h"
#include "unit/unit.h"
#include "util/string_util.h"

namespace wyrmgus {

class remove_character_effect final : public effect<CPlayer>
{
public:
	explicit remove_character_effect(const std::string &character_identifier, const sml_operator effect_operator)
		: effect(effect_operator)
	{
		this->character = character::get(character_identifier);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "remove_character";
		return class_identifier;
	}

	virtual void do_assignment_effect(CPlayer *player) const override
	{
		Q_UNUSED(player)

		CUnit *character_unit = this->character->get_unit();

		if (character_unit != nullptr) {
			character_unit->Remove(nullptr);
			LetUnitDie(*character_unit);
		}
	}

	virtual std::string get_assignment_string() const override
	{
		std::string str = "Remove the " + string::highlight(this->character->get_name()) + " character";
		return str;
	}

private:
	const wyrmgus::character *character = nullptr; //the character to be removed
};

}
