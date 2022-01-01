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

namespace wyrmgus {

class and_condition final : public condition
{
public:
	and_condition() {}

	explicit and_condition(std::vector<std::unique_ptr<const condition>> &&conditions)
		: conditions(std::move(conditions))
	{
	}

	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void check_validity() const override;

	template <typename scope_type>
	bool check_internal(const scope_type scope) const
	{
		for (const auto &condition : this->conditions) {
			if (!condition->check(scope)) {
				return false;
			}
		}

		return true;
	}

	template <typename scope_type>
	bool check_internal(const scope_type scope, const read_only_context &ctx, const bool ignore_units) const
	{
		for (const auto &condition : this->conditions) {
			if (!condition->check(scope, ctx, ignore_units)) {
				return false;
			}
		}

		return true;
	}

	virtual bool check(const civilization *civilization) const override
	{
		return this->check_internal(civilization);
	}

	virtual bool check(const government_type government_type) const override
	{
		return this->check_internal(government_type);
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units = false) const override
	{
		return this->check_internal(player, ctx, ignore_units);
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units = false) const override
	{
		return this->check_internal(unit, ctx, ignore_units);
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		if (this->conditions.empty()) {
			return std::string();
		}

		if (this->conditions.size() == 1) {
			return this->conditions.front()->get_string(indent, links_allowed);
		}

		std::string str = "All of:\n";
		str += this->get_conditions_string(indent + 1, links_allowed);
		return str;
	}

	std::string get_conditions_string(const size_t indent, const bool links_allowed) const
	{
		return condition::get_conditions_string(this->conditions, indent, links_allowed);
	}

	void add_condition(std::unique_ptr<const condition> &&condition)
	{
		this->conditions.push_back(std::move(condition));
	}

private:
	std::vector<std::unique_ptr<const condition>> conditions; //the conditions of which all should be true
};

}
