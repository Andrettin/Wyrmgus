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

#include "player/player.h"
#include "script/condition/condition.h"
#include "unit/unit_class.h"

namespace wyrmgus {

class can_sustain_unit_class_condition final : public condition<CPlayer>
{
public:
	explicit can_sustain_unit_class_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator)
	{
		this->unit_class = unit_class::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "can_sustain_unit_class";
		return class_identifier;
	}

	virtual bool check_assignment(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const unit_type *unit_type = player->get_class_unit_type(this->unit_class);

		if (unit_type == nullptr) {
			return false;
		}

		const int unit_demand = unit_type->Stats[player->get_index()].Variables[DEMAND_INDEX].Value;

		if (unit_demand > 0) {
			const int available_supply = player->get_supply() - player->get_demand();
			if (available_supply < unit_demand) {
				return false;
			}
		}

		return true;
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);

		return "Can sustain the " + condition::get_object_string(this->unit_class, links_allowed) + " unit class";
	}

private:
	const wyrmgus::unit_class *unit_class = nullptr;
};

}
