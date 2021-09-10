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

/**
**  Types for the player
**
**  #player_type::neutral
**
**    This player is controlled by the computer doing nothing.
**
**  #player_type::nobody
**
**    This player is unused. Nobody controls this player.
**
**  #player_type::computer
**
**    This player is controlled by the computer. CPlayer::AiNum
**    selects the AI strategy.
**
**  #player_type::person
**
**    This player is contolled by a person. This can be the player
**    sitting on the local computer or player playing over the
**    network.
**
**  #player_type::rescue_passive
**
**    This player does nothing, the game pieces just sit in the game
**    (being passive)... when a person player moves next to a
**    PassiveRescue unit/building, then it is "rescued" and becomes
**    part of that persons team. If the city center is rescued, than
**    all units of this player are rescued.
**
**  #player_type::rescue_active
**
**    This player is controlled by the computer. CPlayer::AiNum
**    selects the AI strategy. Until it is rescued it plays like
**    an ally. The first person which reaches units of this player,
**    can rescue them. If the city center is rescued, than all units
**    of this player are rescued.
*/
enum class player_type {
	none = 0,
	neutral = 2,        /// neutral
	nobody = 3,        /// unused slot
	computer = 4,       /// computer player
	person = 5,         /// human player
	rescue_passive = 6, /// rescued passive
	rescue_active = 7   /// rescued active
};

inline player_type string_to_player_type(const std::string &str)
{
	if (str == "neutral") {
		return player_type::neutral;
	} else if (str == "nobody") {
		return player_type::nobody;
	} else if (str == "computer") {
		return player_type::computer;
	} else if (str == "person") {
		return player_type::person;
	} else if (str == "rescue_passive") {
		return player_type::rescue_passive;
	} else if (str == "rescue_active") {
		return player_type::rescue_active;
	}

	throw std::runtime_error("Invalid player type: \"" + str + "\".");
}

inline std::string player_type_to_string(const player_type player_type)
{
	switch (player_type) {
		case player_type::neutral:
			return "neutral";
		case player_type::nobody:
			return "nobody";
		case player_type::computer:
			return "computer";
		case player_type::person:
			return "person";
		case player_type::rescue_passive:
			return "rescue_passive";
		case player_type::rescue_active:
			return "rescue_active";
		default:
			break;
	}

	throw std::runtime_error("Invalid player type: \"" + std::to_string(static_cast<int>(player_type)) + "\".");
}

}
