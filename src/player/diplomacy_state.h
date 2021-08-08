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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

enum class diplomacy_state {
	allied, //ally with opponent
	neutral, //don't attack be neutral
	enemy, //attack opponent
	overlord, //become overlord of another player
	vassal,	//become vassal of another player
	personal_union_overlord,
	personal_union_vassal,
	crazy //ally and attack opponent
};

inline diplomacy_state string_to_diplomacy_state(const std::string &str)
{
	if (str == "peace" || str == "neutral") {
		return diplomacy_state::neutral;
	} else if (str == "war" || str == "enemy") {
		return diplomacy_state::enemy;
	} else if (str == "alliance" || str == "allied") {
		return diplomacy_state::allied;
	} else if (str == "overlord") {
		return diplomacy_state::overlord;
	} else if (str == "vassal") {
		return diplomacy_state::vassal;
	} else if (str == "personal_union_overlord") {
		return diplomacy_state::personal_union_overlord;
	} else if (str == "personal_union_vassal") {
		return diplomacy_state::personal_union_vassal;
	} else if (str == "crazy") {
		return diplomacy_state::crazy;
	}

	throw std::runtime_error("Invalid diplomacy state: \"" + str + "\".");
}

inline std::string diplomacy_state_to_string(const diplomacy_state state)
{
	switch (state) {
		case diplomacy_state::neutral:
			return "peace";
		case diplomacy_state::enemy:
			return "war";
		case diplomacy_state::allied:
			return "alliance";
		case diplomacy_state::overlord:
			return "overlord";
		case diplomacy_state::vassal:
			return "vassal";
		case diplomacy_state::personal_union_overlord:
			return "personal_union_overlord";
		case diplomacy_state::personal_union_vassal:
			return "personal_union_vassal";
		case diplomacy_state::crazy:
			return "crazy";
		default:
			break;
	}

	throw std::runtime_error("Invalid diplomacy state: \"" + std::to_string(static_cast<int>(state)) + "\".");
}

inline bool is_vassalage_diplomacy_state(const diplomacy_state state)
{
	switch (state) {
		case diplomacy_state::vassal:
		case diplomacy_state::personal_union_vassal:
			return true;
		default:
			return false;
	}
}

}

Q_DECLARE_METATYPE(wyrmgus::diplomacy_state)
