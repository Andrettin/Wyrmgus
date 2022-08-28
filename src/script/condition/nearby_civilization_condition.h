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

#include "player/civilization.h"
#include "script/condition/condition.h"

namespace wyrmgus {

class nearby_civilization_condition final : public condition<CUnit>
{
public:
	explicit nearby_civilization_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->civilization = civilization::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "nearby_civilization";
		return class_identifier;
	}

	virtual bool check_assignment(const CUnit *unit, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (unit->get_settlement() == nullptr) {
			return false;
		}

		if (unit->get_settlement()->get_game_data()->get_owner() != nullptr && unit->get_settlement()->get_game_data()->get_owner()->get_civilization() == this->civilization) {
			return true;
		}

		for (const CUnit *building : unit->get_settlement()->get_game_data()->get_buildings()) {
			if (building->Player->get_civilization() == this->civilization) {
				return true;
			}

			if (building->Player->is_neutral_player() && building->Type->get_civilization() == this->civilization) {
				return true;
			}
		}

		for (const site *border_settlement : unit->get_settlement()->get_game_data()->get_border_settlements()) {
			const CPlayer *settlement_owner = border_settlement->get_game_data()->get_owner();

			if (settlement_owner != nullptr && settlement_owner->get_civilization() == this->civilization) {
				return true;
			}

			for (const CUnit *building : border_settlement->get_game_data()->get_buildings()) {
				if (building->Player->get_civilization() == this->civilization) {
					return true;
				}

				if (building->Player->is_neutral_player() && building->Type->get_civilization() == this->civilization) {
					return true;
				}
			}
		}

		return false;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);

		return "Nearby " + condition::get_object_string(this->civilization, links_allowed) + " civilization presence";
	}

private:
	const wyrmgus::civilization *civilization = nullptr;
};

}
