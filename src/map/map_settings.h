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

#pragma once

#include "unit/unit_type_container.h"
#include "upgrade/upgrade_container.h"
#include "util/qunique_ptr.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace wyrmgus {

class map_settings final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(std::string name MEMBER name)

public:
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	qunique_ptr<map_settings> duplicate() const;

	std::string get_string() const;

	bool is_unit_type_disabled(const unit_type *unit_type) const;

	const upgrade_set &get_starting_upgrades() const
	{
		return this->starting_upgrades;
	}

private:
	std::string name;
	unit_type_set disabled_unit_types;
	upgrade_set starting_upgrades;
};

}
