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

#include "player/player.h"
#include "script/condition/scope_condition_base.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

namespace wyrmgus {

class any_unit_of_type_condition final : public scope_condition_base<CUnit>
{
public:
	virtual void process_sml_property(const sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_type") {
			this->unit_type = unit_type::get(value);
		} else {
			scope_condition_base::process_sml_property(property);
		}
	}

	virtual void check_validity() const override
	{
		if (this->unit_type == nullptr) {
			throw std::runtime_error("\"any_unit_of_type\" condition has no unit type set for it.");
		}

		scope_condition_base::check_validity();
	}

	virtual bool check(const CPlayer *player, const bool ignore_units) const override
	{
		for (const CUnit *unit : player->get_type_units(this->unit_type)) {
			if (unit->IsUnusable()) {
				continue;
			}

			if (this->check_scope(unit, ignore_units)) {
				return true;
			}
		}

		return false;
	}

	virtual bool check(const CUnit *unit, const bool ignore_units) const override
	{
		return this->check_scope(unit, ignore_units);
	}

	virtual std::string get_scope_name() const override
	{
		return "Any " + string::highlight(this->unit_type->get_name()) + " unit";
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr;
};

}
