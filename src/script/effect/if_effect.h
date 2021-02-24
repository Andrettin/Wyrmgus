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

#include "script/condition/and_condition.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"

namespace wyrmgus {

template <typename scope_type>
class if_effect final : public effect<scope_type>
{
public:
	explicit if_effect(const sml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "if";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		this->effects.process_sml_property(property);
	}

	virtual void process_sml_scope(const sml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			database::process_sml_data(this->conditions, scope);
		} else {
			this->effects.process_sml_scope(scope);
		}
	}

	virtual void do_assignment_effect(scope_type *scope, const context &ctx) const override
	{
		if (!this->conditions.check(scope)) {
			return;
		}

		this->effects.do_effects(scope, ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = "If:\n";
		str += this->conditions.get_conditions_string(indent + 1);
		str += "\n" + std::string(indent, '\t') + "Then:\n";
		return str + this->effects.get_effects_string(scope, ctx, indent + 1, prefix);
	}

private:
	and_condition conditions;
	effect_list<scope_type> effects;
};

}
