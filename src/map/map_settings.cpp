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
//      (c) Copyright 2022 by Andrettin
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

#include "map/map_settings.h"

#include "database/database.h"
#include "unit/unit_type.h"

namespace wyrmgus {

void map_settings::process_gsml_property(const gsml_property &property)
{
	database::process_gsml_property_for_object(this, property);
}

void map_settings::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "disabled_unit_types") {
		for (const std::string &value : values) {
			this->disabled_unit_types.insert(unit_type::get(value));
		}
	} else {
		database::process_gsml_scope_for_object(this, scope);
	}
}

qunique_ptr<map_settings> map_settings::duplicate() const
{
	auto settings = make_qunique<map_settings>();

	if (QApplication::instance()->thread() != QThread::currentThread()) {
		settings->moveToThread(QApplication::instance()->thread());
	}

	settings->name = this->name;
	settings->disabled_unit_types = this->disabled_unit_types;

	return settings;
}

std::string map_settings::get_string() const
{
	if (this->disabled_unit_types.empty()) {
		return std::string();
	}

	std::string str;

	str += "Map Settings";

	if (!this->name.empty()) {
		str += " (" + this->name + ")";
	}

	str += ":";

	if (!this->disabled_unit_types.empty()) {
		str += "\n\tDisabled Unit Types:";

		for (const unit_type *unit_type : this->disabled_unit_types) {
			str += "\n\t\t" + unit_type->get_name();
		}
	}

	return str;
}

bool map_settings::is_unit_type_disabled(const unit_type *unit_type) const
{
	return this->disabled_unit_types.contains(unit_type) || unit_type->is_disabled();
}

}
