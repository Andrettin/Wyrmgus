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

#include "map/site.h"
#include "player/player.h"
#include "script/condition/condition.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

namespace wyrmgus {

template <typename scope_type>
class unit_type_condition final : public condition<scope_type>
{
public:
	explicit unit_type_condition(const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
	}

	explicit unit_type_condition(const unit_type *unit_type, const int count, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator), unit_type(unit_type), count(count)
	{
	}

	explicit unit_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->unit_type = unit_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "unit_type";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_type") {
			this->unit_type = unit_type::get(value);
		} else if (key == "count") {
			this->count = std::stoi(value);
		} else if (key == "settlement") {
			this->settlement = site::get(value);
		} else {
			throw std::runtime_error("Invalid unit type condition property: \"" + property.get_key() + "\".");
		}
	}

	virtual void check_validity() const override
	{
		if (this->unit_type == nullptr) {
			throw std::runtime_error("\"unit_type\" condition has no unit type.");
		}

		if constexpr (std::is_same_v<scope_type, CUnit>) {
			if (this->count != 1 || this->settlement != nullptr) {
				throw std::runtime_error("\"unit_type\" condition has a count or settlement, despite having a unit as its scope.");
			}
		}
	}

	virtual bool check(const civilization *civilization) const override
	{
		if (this->unit_type->get_preconditions() != nullptr && !this->unit_type->get_preconditions()->check(civilization)) {
			return false;
		}

		if (this->unit_type->get_conditions() != nullptr && !this->unit_type->get_conditions()->check(civilization)) {
			return false;
		}

		return true;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			if (ctx.ignore_units) {
				return true;
			}

			if (this->settlement != nullptr) {
				if (!scope->has_settlement(this->settlement)) {
					return false;
				}

				std::vector<CUnit *> units;
				FindPlayerUnitsByType(*scope, *this->unit_type, units);

				int counter = 0;
				for (const CUnit *unit : units) {
					if (unit->get_settlement() == this->settlement) {
						counter++;

						if (counter >= this->count) {
							return true;
						}
					}
				}

				return false;
			} else {
				return scope->GetUnitTypeCount(this->unit_type) >= this->count;
			}
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			return scope->Type == this->unit_type;
		}
	}

	virtual std::string get_assignment_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent);

		std::string str;

		if constexpr (std::is_same_v<scope_type, CPlayer>) {
			if (this->count > 1) {
				str += std::to_string(this->count) + " ";
			}

			str += condition<scope_type>::get_object_string(this->unit_type, links_allowed) + " ";

			if (this->unit_type->BoolFlag[BUILDING_INDEX].value) {
				str += "building";
			} else {
				str += "unit";
			}

			if (this->count > 1) {
				str += "s";
			}

			if (this->settlement != nullptr) {
				str += " in " + settlement->get_name();
			}
		} else if constexpr (std::is_same_v<scope_type, CUnit>) {
			str = condition<scope_type>::get_object_string(this->unit_type, links_allowed) + " unit type";
		}

		return str;
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr;
	int count = 1; //how many of the unit type are required
	const site *settlement = nullptr; //in which settlement the unit should be located
};

}
