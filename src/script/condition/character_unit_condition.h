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

#include "character.h"
#include "script/condition/scope_condition.h"

namespace wyrmgus {

template <typename upper_scope_type>
class character_unit_condition final : public scope_condition<upper_scope_type, CUnit>
{
public:
	explicit character_unit_condition(const gsml_operator condition_operator)
		: scope_condition<upper_scope_type, CUnit>(condition_operator)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "character") {
			this->character = character::get(value);
		} else {
			scope_condition<upper_scope_type, CUnit>::process_gsml_property(property);
		}
	}

	virtual void check_validity() const override
	{
		if (this->character == nullptr) {
			throw std::runtime_error("\"character_unit\" condition has no character set for it.");
		}

		scope_condition<upper_scope_type, CUnit>::check_validity();
	}

	virtual const CUnit *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);
		Q_UNUSED(ctx);

		return this->character->get_unit();
	}

	virtual std::string get_scope_name() const override
	{
		return string::highlight(this->character->get_full_name());
	}

private:
	const wyrmgus::character *character = nullptr;
};

}
