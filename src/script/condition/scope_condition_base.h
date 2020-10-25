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
//      (c) Copyright 2020 by Andrettin
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
//

#pragma once

#include "script/condition/and_condition.h"
#include "script/condition/condition.h"

namespace wyrmgus {

template <typename scope_type>
class scope_condition_base : public condition
{
public:
	virtual void process_sml_property(const sml_property &property) override final
	{
		this->conditions.process_sml_property(property);
	}

	virtual void process_sml_scope(const sml_data &scope) override final
	{
		this->conditions.process_sml_scope(scope);
	}

	virtual bool check(bool ignore_units = false) const = 0;

	virtual bool check(const CPlayer *player, bool ignore_units = false) const override final
	{
		return this->check(ignore_units);
	}

	virtual bool check(const CUnit *unit, bool ignore_units = false) const override final
	{
		return this->check(ignore_units);
	}

	bool check_scope(const CPlayer *player, bool ignore_units = false) const
	{
		return this->conditions.check(player, ignore_units);
	}

	bool check_scope(const CUnit *unit, bool ignore_units = false) const
	{
		return this->conditions.check(unit, ignore_units);
	}

	virtual std::string get_string(const std::string &prefix = "") const override final
	{
		return this->conditions.get_string(prefix);
	}

private:
	and_condition conditions;
};

}
