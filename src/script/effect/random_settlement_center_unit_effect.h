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
#include "script/effect/scope_effect_base.h"
#include "unit/unit.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

class random_settlement_center_unit_effect final : public scope_effect_base<CPlayer, CUnit>
{
public:
	explicit random_settlement_center_unit_effect(const gsml_operator effect_operator)
		: scope_effect_base(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_settlement_center_unit";
		return class_identifier;
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			scope.process(&this->conditions);
		} else {
			scope_effect_base::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(CPlayer *player, context &ctx) const override
	{
		std::vector<CUnit *> potential_units = player->get_town_hall_units();
		vector::merge(potential_units, player->get_type_units(settlement_site_unit_type));

		std::erase_if(potential_units, [this, &ctx](const CUnit *town_hall) {
			return !this->conditions.check(town_hall, ctx);
		});

		if (potential_units.empty()) {
			return;
		}

		CUnit *unit = vector::get_random(potential_units);
		this->do_scope_effect(unit, ctx);
	}

	virtual std::string get_scope_name() const override
	{
		return "Random settlement center";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent, false);
	}

private:
	and_condition<CUnit> conditions;
};

}
