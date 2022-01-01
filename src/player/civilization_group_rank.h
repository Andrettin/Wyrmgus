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

namespace wyrmgus {

enum class civilization_group_rank {
	none,
	subgroup,
	group,
	supergroup
};

inline civilization_group_rank string_to_civilization_group_rank(const std::string &str)
{
	if (str == "subgroup") {
		return civilization_group_rank::subgroup;
	} else if (str == "group") {
		return civilization_group_rank::group;
	} else if (str == "supergroup") {
		return civilization_group_rank::supergroup;
	}

	throw std::runtime_error("Invalid civilization group rank: \"" + str + "\".");
}

inline std::string civilization_group_rank_to_string(const civilization_group_rank rank)
{
	switch (rank) {
		case civilization_group_rank::subgroup:
			return "subgroup";
		case civilization_group_rank::group:
			return "group";
		case civilization_group_rank::supergroup:
			return "supergroup";
		default:
			break;
	}

	throw std::runtime_error("Invalid civilization group rank: \"" + std::to_string(static_cast<int>(rank)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::civilization_group_rank)
