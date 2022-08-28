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

#include "character.h"
#include "script/condition/condition.h"
#include "unit/unit.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace wyrmgus {

template <typename scope_type>
class character_condition final : public condition<scope_type>
{
public:
	explicit character_condition(const std::string &value)
	{
		if (string::is_bool(value)) {
			this->is_character = string::to_bool(value);
		} else {
			this->character = character::get(value);
		}
	}

	virtual void check_validity() const override
	{
		if (this->character == nullptr && !this->is_character.has_value()) {
			throw std::runtime_error("\"character\" condition neither has a character, nor is set to relate to any character.");
		}

		if (this->character != nullptr && this->is_character.has_value()) {
			throw std::runtime_error("\"character\" condition has both a character and is set to relate to any character.");
		}
	}

	virtual bool check(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			if (this->is_character.has_value()) {
				return !scope->Heroes.empty() == this->is_character.value();
			} else {
				return scope->HasHero(this->character);
			}
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			if (this->is_character.has_value()) {
				return (scope->get_character() != nullptr) == this->is_character.value();
			} else {
				return scope->get_character() == this->character;
			}
		}
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);

		std::string str;

		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			if (this->is_character.has_value() && this->is_character.value() == false) {
				str += "Does not have";
			} else {
				str += "Has";
			}
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			str += "Is";
		}

		if (this->is_character.has_value()) {
			if constexpr (std::is_same_v<scope_type, CUnit>) {
				if (this->is_character.value() == false) {
					str += " not";
				}
			}

			str += " a";
		} else {
			str += " the " + condition<scope_type>::get_object_string(this->character, links_allowed, this->character->get_full_name());
		}

		str += " character";

		return str;
	}

private:
	const wyrmgus::character *character = nullptr;
	std::optional<bool> is_character;
};

}
