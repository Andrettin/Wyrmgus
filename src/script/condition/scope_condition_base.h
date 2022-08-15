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

#include "script/condition/and_condition.h"
#include "script/condition/condition.h"

namespace wyrmgus {

template <typename upper_scope_type, typename scope_type>
class scope_condition_base : public condition<upper_scope_type>
{
public:
	virtual void process_gsml_property(const gsml_property &property) override
	{
		this->conditions.process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override final
	{
		this->conditions.process_gsml_scope(scope);
	}

	virtual void check_validity() const override
	{
		this->conditions.check_validity();
	}

	bool check_scope(const scope_type *scope, const read_only_context &ctx) const
	{
		return this->conditions.check(scope, ctx);
	}

	virtual std::string get_scope_name() const = 0;

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override final
	{
		std::string str = this->get_scope_name() + ":\n";
		str += this->conditions.get_conditions_string(indent + 1, links_allowed);
		return str;
	}

private:
	and_condition<scope_type> conditions;
};

}
