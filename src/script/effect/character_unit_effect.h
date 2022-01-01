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
#include "script/effect/scope_effect.h"

namespace wyrmgus {

template <typename scope_type>
class character_unit_effect final : public scope_effect<scope_type, CUnit>
{
public:
	explicit character_unit_effect(const sml_operator effect_operator) : scope_effect<scope_type, CUnit>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "character_unit";
		return class_identifier;
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "character") {
			this->character = character::get(value);
		} else {
			scope_effect<scope_type, CUnit>::process_sml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->character == nullptr) {
			throw std::runtime_error("\"character_unit\" effect has no character set for it.");
		}
	}

	virtual std::string get_scope_name() const override
	{
		return string::highlight(this->character->get_full_name());
	}
	
	virtual CUnit *get_scope(const scope_type *upper_scope) const override
	{
		Q_UNUSED(upper_scope)

		CUnit *character_unit = this->character->get_unit();
		return character_unit;
	}

private:
	const wyrmgus::character *character = nullptr;
};

}
