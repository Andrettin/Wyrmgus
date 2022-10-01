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

#include "util/enum_converter.h"

namespace wyrmgus {

enum class government_type {
	none,
	tribe,
	monarchy,
	republic,
	theocracy
};

extern template class enum_converter<government_type>;

inline std::string get_government_type_name(const government_type government_type)
{
	switch (government_type) {
		case government_type::none:
			return "None";
		case government_type::tribe:
			return "Tribe";
		case government_type::monarchy:
			return "Monarchy";
		case government_type::republic:
			return "Republic";
		case government_type::theocracy:
			return "Theocracy";
		default:
			break;
	}

	throw std::runtime_error("Invalid government type: \"" + std::to_string(static_cast<int>(government_type)) + "\".");
}

inline bool can_government_type_have_dynasty(const government_type government_type)
{
	switch (government_type) {
		case government_type::tribe:
		case government_type::monarchy:
			return true;
		default:
			return false;
	}
}

}

Q_DECLARE_METATYPE(wyrmgus::government_type)
