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
//

#pragma once

#include "map/site.h"
#include "player.h"
#include "script/condition/condition.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

namespace wyrmgus {

class unit_type_condition final : public condition
{
public:
	unit_type_condition() {}

	explicit unit_type_condition(const unit_type *unit_type, const int count) : unit_type(unit_type), count(count) {}

	explicit unit_type_condition(const std::string &value)
	{
		this->unit_type = unit_type::get(value);
	}

	virtual void process_sml_property(const sml_property &property) override
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
	}

	virtual bool check(const CPlayer *player, const bool ignore_units) const override
	{
		if (ignore_units) {
			return true;
		}

		if (this->settlement != nullptr) {
			if (!player->has_settlement(this->settlement)) {
				return false;
			}

			std::vector<CUnit *> units;
			FindPlayerUnitsByType(*player, *this->unit_type, units);

			int counter = 0;
			for (const CUnit *unit : units) {
				if (unit->settlement == this->settlement) {
					counter++;

					if (counter >= this->count) {
						return true;
					}
				}
			}

			return false;
		} else {
			return player->GetUnitTypeCount(this->unit_type) >= this->count;
		}
	}

	virtual std::string get_string(const size_t indent) const override
	{
		Q_UNUSED(indent)

		std::string str = string::highlight(this->unit_type->get_name());

		if (this->count > 1) {
			str += '(' + std::to_string(this->count) + ')';
		}

		if (this->settlement != nullptr) {
			str += " in " + settlement->get_name();
		}

		return str;
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr;
	int count = 1; //how many of the unit type are required
	const site *settlement = nullptr; //in which settlement the unit should be located
};

}
