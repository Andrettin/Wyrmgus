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

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "unit/unit.h"

namespace wyrmgus {

class level_check_effect final : public effect<CUnit>
{
public:
	explicit level_check_effect(const gsml_operator effect_operator) : effect<CUnit>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "level_check";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "level") {
			this->level = std::stoi(value);
		} else {
			effect<CUnit>::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "success") {
			database::process_gsml_data(this->success_effects, scope);
		} else if (tag == "failure") {
			database::process_gsml_data(this->failure_effects, scope);
		} else {
			effect<CUnit>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(CUnit *unit, context &ctx) const override
	{
		if (unit->LevelCheck(this->level)) {
			this->success_effects.do_effects(unit, ctx);
		} else {
			this->failure_effects.do_effects(unit, ctx);
		}
	}

	virtual std::string get_assignment_string(const CUnit *unit, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = "Level check (" + std::to_string(this->level) + "):\n";
		str += std::string(indent + 1, '\t') + "Success:\n";
		str += this->success_effects.get_effects_string(unit, ctx, indent + 2, prefix);
		str += "\n" + std::string(indent + 1, '\t') + "Failure:\n";
		str += this->failure_effects.get_effects_string(unit, ctx, indent + 2, prefix);
		return str;
	}

private:
	int level = 0;
	effect_list<CUnit> success_effects;
	effect_list<CUnit> failure_effects;
};

}
