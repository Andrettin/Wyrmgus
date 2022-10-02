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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "util/enum_converter.h"

namespace wyrmgus {

enum class hotkey_setup {
	default_setup,
	position_based,
	position_based_except_commands,

	count
};

extern template class enum_converter<hotkey_setup>;

inline std::string get_hotkey_setup_name(const hotkey_setup hotkey_setup)
{
	switch (hotkey_setup) {
		case hotkey_setup::default_setup:
			return "Default";
		case hotkey_setup::position_based:
			return "Position-Based";
		case hotkey_setup::position_based_except_commands:
			return "Position-Based (except Commands)";
		default:
			break;
	}

	throw std::runtime_error("Invalid hotkey setup: \"" + std::to_string(static_cast<int>(hotkey_setup)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::hotkey_setup)
