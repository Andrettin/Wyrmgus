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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "unit/build_restriction/build_restriction.h"

#include "database/database.h"
#include "unit/build_restriction/add_on_build_restriction.h"
#include "unit/build_restriction/and_build_restriction.h"
#include "unit/build_restriction/distance_build_restriction.h"
#include "unit/build_restriction/on_top_build_restriction.h"
#include "unit/build_restriction/or_build_restriction.h"
#include "unit/build_restriction/surrounded_by_build_restriction.h"
#include "unit/build_restriction/terrain_build_restriction.h"

namespace wyrmgus {

std::unique_ptr<build_restriction> build_restriction::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "terrain") {
		return std::make_unique<terrain_build_restriction>(value);
	} else {
		throw std::runtime_error("Invalid build restriction property: \"" + key + "\".");
	}
}

std::unique_ptr<build_restriction> build_restriction::from_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	std::unique_ptr<build_restriction> building_rule;

	if (tag == "add_on") {
		building_rule = std::make_unique<add_on_build_restriction>();
	} else if (tag == "and") {
		building_rule = std::make_unique<and_build_restriction>();
	} else if (tag == "distance") {
		building_rule = std::make_unique<distance_build_restriction>();
	} else if (tag == "on_top") {
		building_rule = std::make_unique<on_top_build_restriction>();
	} else if (tag == "or") {
		building_rule = std::make_unique<or_build_restriction>();
	} else if (tag == "surrounded_by") {
		building_rule = std::make_unique<surrounded_by_build_restriction>();
	} else {
		throw std::runtime_error("Invalid build restriction scope: \"" + tag + "\".");
	}

	scope.process(building_rule.get());

	return building_rule;
}

void build_restriction::process_gsml_property(const gsml_property &property)
{
	throw std::runtime_error("Invalid build restriction property: \"" + property.get_key() + "\".");
}

void build_restriction::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid build restriction scope: \"" + scope.get_tag() + "\".");
}

}
