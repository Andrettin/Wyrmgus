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

#include "player/player.h"
#include "script/condition/scope_condition_base.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "util/string_util.h"

namespace wyrmgus {

class any_unit_of_class_condition final : public scope_condition_base<CUnit>
{
public:
	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_class") {
			this->unit_class = unit_class::get(value);
		} else {
			scope_condition_base::process_gsml_property(property);
		}
	}

	virtual void check_validity() const override
	{
		if (this->unit_class == nullptr) {
			throw std::runtime_error("\"any_unit_of_class\" condition has no unit class set for it.");
		}

		scope_condition_base::check_validity();
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx) const override
	{
		for (const CUnit *unit : player->get_class_units(this->unit_class)) {
			if (unit->IsUnusable()) {
				continue;
			}

			if (this->check_scope(unit, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx) const override
	{
		return this->check_scope(unit, ctx);
	}

	virtual std::string get_scope_name() const override
	{
		return "Any " + string::highlight(this->unit_class->get_name()) + " unit";
	}

private:
	const wyrmgus::unit_class *unit_class = nullptr;
};

}
