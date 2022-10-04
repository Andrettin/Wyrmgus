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
//      (c) Copyright 2015-2022 by Andrettin
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

#include "quest/objective_type.h"

namespace archimedes {

template class enum_converter<objective_type>;

template <>
const std::string enum_converter<objective_type>::property_class_identifier = "wyrmgus::objective_type";

template <>
const std::map<std::string, objective_type> enum_converter<objective_type>::string_to_enum_map = {
	{ "gather_resource", objective_type::gather_resource },
	{ "have_resource", objective_type::have_resource },
	{ "build_units", objective_type::build_units },
	{ "destroy_units", objective_type::destroy_units },
	{ "research_upgrade", objective_type::research_upgrade },
	{ "recruit_hero", objective_type::recruit_hero },
	{ "destroy_hero", objective_type::destroy_hero },
	{ "hero_must_survive", objective_type::hero_must_survive },
	{ "destroy_unique", objective_type::destroy_unique },
	{ "destroy_faction", objective_type::destroy_faction },
	{ "found_faction", objective_type::found_faction },
	{ "bring_unit_to_site", objective_type::bring_unit_to_site },
	{ "have_settlement", objective_type::have_settlement },
};

template <>
const bool enum_converter<objective_type>::initialized = enum_converter::initialize();

}
