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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "stratagus.h"

#include "unit/historical_unit.h"

#include "item/unique_item.h"
#include "map/historical_location.h"
#include "map/site.h"
#include "player/player.h"
#include "unit/historical_unit_history.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"

namespace wyrmgus {

historical_unit::historical_unit(const std::string &identifier) : named_data_entry(identifier)
{
}

historical_unit::~historical_unit()
{
}

void historical_unit::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "unit_classes") {
		for (const std::string &value : values) {
			this->unit_classes.push_back(unit_class::get(value));
		}

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			unit_class *unit_class = unit_class::get(key);
			const int weight = std::stoi(value);
			for (int i = 0; i < weight; ++i) {
				this->unit_classes.push_back(unit_class);
			}
		});
	} else if (tag == "unit_types") {
		for (const std::string &value : values) {
			this->unit_types.push_back(unit_type::get(value));
		}

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			unit_type *unit_type = unit_type::get(key);
			const int weight = std::stoi(value);
			for (int i = 0; i < weight; ++i) {
				this->unit_types.push_back(unit_type);
			}
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void historical_unit::check() const
{
	if (this->get_unit_types().empty() && this->get_unit_classes().empty() && this->get_unique() == nullptr) {
		throw std::runtime_error("Historical unit \"" + this->get_identifier() + "\" has neither a unit type nor a unit class nor is it a unique.");
	}

	if (this->get_unique() != nullptr && this->get_quantity() > 1) {
		throw std::runtime_error("Historical unit \"" + this->get_identifier() + "\" is a unique, but has a quantity greater than 1.");
	}
}

data_entry_history *historical_unit::get_history_base()
{
	return this->history.get();
}

void historical_unit::reset_history()
{
	this->history = std::make_unique<historical_unit_history>();
}

}
