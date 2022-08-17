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

#include "script/effect/effect.h"
#include "script/effect/effect_list.h"

namespace wyrmgus {

template <typename scope_type>
class for_effect final : public effect<scope_type>
{
public:
	explicit for_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "for";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		if (property.get_key() == "count") {
			this->count = std::stoi(property.get_value());
		} else {
			this->effects.process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->effects.process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		if (this->count <= 1) {
			throw std::runtime_error("\"for\" effect has a count equal to or smaller than 1.");
		}
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		for (int i = 0; i < this->count; ++i) {
			this->effects.do_effects(scope, ctx);
		}
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		std::string str = std::to_string(this->count) + " times:\n";
		str += this->effects.get_effects_string(scope, ctx, indent + 1, prefix);
		return str;
	}

private:
	int count = 0;
	effect_list<scope_type> effects;
};

}
