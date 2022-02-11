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
#include "unit/unit_class.h"
#include "unit/unit_type.h"

namespace wyrmgus {

class unit_class_condition final : public condition
{
public:
	unit_class_condition() {}

	explicit unit_class_condition(const std::string &value)
	{
		this->unit_class = unit_class::get(value);
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "unit_class") {
			this->unit_class = unit_class::get(value);
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
		if (this->unit_class == nullptr) {
			throw std::runtime_error("\"unit_class\" condition has no unit class.");
		}
	}

	virtual bool check(const civilization *civilization) const override
	{
		if (this->unit_class->get_preconditions() != nullptr && !this->unit_class->get_preconditions()->check(civilization)) {
			return false;
		}

		if (this->unit_class->get_conditions() != nullptr && !this->unit_class->get_conditions()->check(civilization)) {
			return false;
		}

		const unit_type *unit_type = civilization->get_class_unit_type(this->unit_class);

		if (unit_type == nullptr) {
			return false;
		}

		if (unit_type->get_preconditions() != nullptr && !unit_type->get_preconditions()->check(civilization)) {
			return false;
		}

		if (unit_type->get_conditions() != nullptr && !unit_type->get_conditions()->check(civilization)) {
			return false;
		}

		return true;
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units) const override
	{
		Q_UNUSED(ctx)

		if (ignore_units) {
			return true;
		}

		const unit_type *unit_type = player->get_class_unit_type(this->unit_class);

		if (unit_type == nullptr) {
			return this->count == 0;
		}

		if (this->settlement != nullptr) {
			if (!player->has_settlement(this->settlement)) {
				return false;
			}

			std::vector<CUnit *> units;
			FindPlayerUnitsByType(*player, *unit_type, units);

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
			return player->GetUnitTypeCount(unit_type) >= this->count;
		}
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)
		Q_UNUSED(links_allowed)

		std::string str;

		if (this->count > 1) {
			str += std::to_string(this->count) + " ";
		}

		str += string::highlight(this->unit_class->get_name()) + " class ";

		const bool is_building = !this->unit_class->get_unit_types().empty() && this->unit_class->get_unit_types().front()->BoolFlag[BUILDING_INDEX].value;

		if (is_building) {
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

		return str;
	}

private:
	const wyrmgus::unit_class *unit_class = nullptr;
	int count = 1; //how many of the unit type are required
	const site *settlement = nullptr; //in which settlement the unit should be located
};

}
