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

class CConfigData;

namespace wyrmgus {

template <typename scope_type>
class or_condition final : public condition<scope_type>
{
public:
	explicit or_condition(const gsml_operator condition_operator);
	explicit or_condition(std::vector<std::unique_ptr<const condition<scope_type>>> &&conditions);

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "or";
		return class_identifier;
	}

	virtual void ProcessConfigDataSection(const CConfigData *section) override;

	virtual void process_gsml_property(const gsml_property &property) override
	{
		this->conditions.push_back(condition<scope_type>::from_gsml_property(property));
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->conditions.push_back(condition<scope_type>::from_gsml_scope(scope));
	}

	virtual void check_validity() const override
	{
		for (const auto &condition : this->conditions) {
			condition->check_validity();
		}
	}

	template <typename checked_scope_type>
	bool check_internal(const checked_scope_type scope) const
	{
		for (const auto &condition : this->conditions) {
			if (condition->check(scope)) {
				return true;
			}
		}

		return false;
	}

	template <typename checked_scope_type>
	bool check_internal(const checked_scope_type scope, const read_only_context &ctx) const
	{
		for (const auto &condition : this->conditions) {
			if (condition->check(scope, ctx)) {
				return true;
			}
		}

		return false;
	}

	virtual bool check(const civilization *civilization) const override
	{
		return this->check_internal(civilization);
	}

	virtual bool check(const government_type government_type) const override
	{
		return this->check_internal(government_type);
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		return this->check_internal(scope, ctx);
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		if (this->conditions.size() == 1) {
			return this->conditions.front()->get_string(indent, links_allowed);
		}

		std::string str = "One of:\n";
		str += condition<scope_type>::get_conditions_string(this->conditions, indent + 1, links_allowed);
		return str;
	}

private:
	std::vector<std::unique_ptr<const condition<scope_type>>> conditions; //the condition of which one should be true
};

}
