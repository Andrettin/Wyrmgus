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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "stratagus.h"

#include "unit/historical_unit.h"

#include "config.h"
#include "faction.h"
#include "map/historical_location.h"
#include "player.h"
#include "unit/unit_type.h"

namespace stratagus {

historical_unit::historical_unit(const std::string &identifier) : named_data_entry(identifier)
{
}

historical_unit::~historical_unit()
{
}

void historical_unit::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "faction") {
		this->Faction = faction::get(value);
	} else {
		data_entry::process_sml_property(property);
	}
}

void historical_unit::process_sml_dated_scope(const sml_data &scope, const QDateTime &date)
{
	const std::string &tag = scope.get_tag();

	if (tag == "location") {
		auto location = std::make_unique<historical_location>();
		database::get()->process_sml_data(location, scope);
		location->check();
		this->location = std::move(location);
	} else {
		data_entry::process_sml_dated_scope(scope, date);
	}
}

void historical_unit::check() const
{
	if (this->get_unit_type() == nullptr && this->get_unit_class() == nullptr) {
		throw std::runtime_error("Historical unit \"" + this->get_identifier() + "\" has neither a unit type nor a unit class.");
	}
}

void historical_unit::reset_history()
{
	this->location.reset();
	this->active = false;
}

}
