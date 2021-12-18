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
//      (c) Copyright 2021 by Andrettin
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

#include "language/name_variant.h"

#include "language/word.h"

namespace wyrmgus {

std::string get_name_variant_string(const name_variant &name_variant)
{
	std::string name_string = std::visit([](auto &&name_value) {
		using name_type = std::decay_t<decltype(name_value)>;

		static_assert(std::is_same_v<name_type, std::string> || std::is_same_v<name_type, const word *>, "Invalid name variant type.");

		if constexpr (std::is_same_v<name_type, std::string>) {
			return name_value;
		} else if constexpr (std::is_same_v<name_type, const word *>) {
			return name_value->get_anglicized_name();
		}
	}, name_variant);

	return name_string;
}

}
