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

#include "item/unique_item.h"
#include "script/condition/scope_condition.h"
#include "util/string_util.h"

namespace wyrmgus {

template <typename upper_scope_type>
class unique_unit_condition final : public scope_condition<upper_scope_type, CUnit>
{
public:
	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unique") {
			this->unique = unique_item::get(value);
		} else {
			scope_condition<upper_scope_type, CUnit>::process_gsml_property(property);
		}
	}

	virtual void check_validity() const override
	{
		if (this->unique == nullptr) {
			throw std::runtime_error("\"unique_unit\" condition has no unique set for it.");
		}

		scope_condition<upper_scope_type, CUnit>::check_validity();
	}

	virtual const CUnit *get_scope(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(upper_scope);
		Q_UNUSED(ctx);

		return this->unique->get_unit();
	}

	virtual std::string get_scope_name() const override
	{
		return string::highlight(this->unique->get_name());
	}

private:
	const unique_item *unique = nullptr;
};

}
