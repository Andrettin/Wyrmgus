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
	virtual bool check(const civilization *civilization) const override;
	virtual bool check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units = false) const override;
	virtual bool check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units = false) const override;

	std::string get_string(const size_t indent, const bool links_allowed, const bool add_own_string) const
	{
		if (this->conditions.size() == 1) {
			return this->conditions.front()->get_string(indent, links_allowed);
		}

		std::string str;

		if (add_own_string) {
			str += "All of these must be true:";
		}

		str += "\n";
		str += this->get_conditions_string(indent + 1, links_allowed);

		return str;
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		if (this->conditions.size() == 1) {
			return this->conditions.front()->get_string(indent, links_allowed);
		}

		std::string str = "All of these must be true:\n";
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
