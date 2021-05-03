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
//      (c) Copyright 2015-2021 by Andrettin
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

namespace wyrmgus {

enum class objective_type {
	gather_resource,
	have_resource,
	build_units,
	destroy_units,
	research_upgrade,
	recruit_hero,
	destroy_hero,
	hero_must_survive,
	destroy_unique,
	destroy_faction,
	found_faction
};

inline objective_type string_to_objective_type(const std::string &str)
{
	if (str == "gather_resource") {
		return objective_type::gather_resource;
	} else if (str == "have_resource") {
		return objective_type::have_resource;
	} else if (str == "build_units") {
		return objective_type::build_units;
	} else if (str == "destroy_units") {
		return objective_type::destroy_units;
	} else if (str == "research_upgrade") {
		return objective_type::research_upgrade;
	} else if (str == "recruit_hero") {
		return objective_type::recruit_hero;
	} else if (str == "destroy_hero") {
		return objective_type::destroy_hero;
	} else if (str == "hero_must_survive") {
		return objective_type::hero_must_survive;
	} else if (str == "destroy_unique") {
		return objective_type::destroy_unique;
	} else if (str == "destroy_faction") {
		return objective_type::destroy_faction;
	} else if (str == "found_faction") {
		return objective_type::found_faction;
	}

	throw std::runtime_error("Invalid objective type: \"" + str + "\".");
}

inline std::string objective_type_to_string(const objective_type objective_type)
{
	switch (objective_type) {
		case objective_type::gather_resource:
			return "gather_resource";
		case objective_type::have_resource:
			return "have_resource";
		case objective_type::build_units:
			return "build_units";
		case objective_type::destroy_units:
			return "destroy_units";
		case objective_type::research_upgrade:
			return "research_upgrade";
		case objective_type::recruit_hero:
			return "recruit_hero";
		case objective_type::destroy_hero:
			return "destroy_hero";
		case objective_type::hero_must_survive:
			return "hero_must_survive";
		case objective_type::destroy_unique:
			return "destroy_unique";
		case objective_type::destroy_faction:
			return "destroy_faction";
		case objective_type::found_faction:
			return "found_faction";
		default:
			break;
	}

	throw std::runtime_error("Invalid objective type: \"" + std::to_string(static_cast<int>(objective_type)) + "\".");
}

}
