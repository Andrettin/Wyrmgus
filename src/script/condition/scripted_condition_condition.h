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

#include "script/condition/condition.h"
#include "script/condition/scripted_condition.h"

namespace wyrmgus {

template <typename scope_type>
class scripted_condition_condition final : public condition<scope_type>
{
public:
	static const scripted_condition_base<scope_type> *get_scripted_condition(const std::string &identifier)
	{
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			return player_scripted_condition::get(identifier);
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			return unit_scripted_condition::get(identifier);
		}
	}

	explicit scripted_condition_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->scripted_condition = scripted_condition_condition::get_scripted_condition(value);
	}

	virtual bool check(const civilization *civilization) const override
	{
		return this->scripted_condition->get_conditions()->check(civilization);
	}

	virtual bool check(const government_type government_type) const override
	{
		return this->scripted_condition->get_conditions()->check(government_type);
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		return this->scripted_condition->get_conditions()->check(scope, ctx);
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		return this->scripted_condition->get_conditions()->get_string(indent, links_allowed);
	}

private:
	const wyrmgus::scripted_condition_base<scope_type> *scripted_condition = nullptr;
};

}
