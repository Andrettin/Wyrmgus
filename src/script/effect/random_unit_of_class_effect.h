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
//      (c) Copyright 2021-2022 by Andrettin
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
#include "unit/unit_class.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

class random_unit_of_class_effect final : public scope_effect_base<CPlayer, CUnit>
{
public:
	explicit random_unit_of_class_effect(const sml_operator effect_operator) : scope_effect_base(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_unit_of_class";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_class") {
			this->unit_class = unit_class::get(value);
		} else {
			scope_effect_base::process_sml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->unit_class == nullptr) {
			throw std::runtime_error("\"random_unit_of_class\" effect has no unit class set for it.");
		}

		scope_effect_base::check();
	}

	virtual void do_assignment_effect(CPlayer *player, const context &ctx) const override
	{
		//copy the unit list, as the effects could change the player's list (e.g. by removing a unit)
		std::vector<CUnit *> class_units = player->get_class_units(this->unit_class);
		std::erase_if(class_units, [](const CUnit *class_unit) {
			return class_unit->IsUnusable();
		});

		if (class_units.empty()) {
			return;
		}

		CUnit *unit = vector::get_random(class_units);
		this->do_scope_effect(unit, ctx);
	}

	virtual std::string get_scope_name() const override
	{
		return "Random " + string::highlight(this->unit_class->get_name()) + " unit";
	}

private:
	const wyrmgus::unit_class *unit_class = nullptr;
};

}
