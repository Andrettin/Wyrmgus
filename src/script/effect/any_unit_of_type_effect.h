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

#include "player.h"
#include "script/effect/scope_effect_base.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

namespace wyrmgus {

class any_unit_of_type_effect final : public scope_effect_base<CPlayer, CUnit>
{
public:
	explicit any_unit_of_type_effect(const sml_operator effect_operator) : scope_effect_base(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_unit_of_type";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_type") {
			this->unit_type = unit_type::get(value);
		} else {
			scope_effect_base::process_sml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->unit_type == nullptr) {
			throw std::runtime_error("\"any_unit_of_type\" effect has no unit type set for it.");
		}

		scope_effect_base::check();
	}

	virtual void do_assignment_effect(CPlayer *player, const context &ctx) const override
	{
		//copy the unit list, as the effects could change the player's list (e.g. by removing a unit)
		const std::vector<CUnit *> type_units = player->get_type_units(this->unit_type);

		for (CUnit *unit : type_units) {
			if (unit->IsUnusable()) {
				continue;
			}

			this->do_scope_effect(unit, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Any " + string::highlight(this->unit_type->get_name()) + " unit";
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr;
};

}
