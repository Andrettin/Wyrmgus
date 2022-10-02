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

#include "stratagus.h"

#include "player/faction_type.h"

namespace wyrmgus {

template class enum_converter<faction_type>;

template <>
const std::string enum_converter<faction_type>::property_class_identifier = "wyrmgus::faction_type";

template <>
const std::map<std::string, faction_type> enum_converter<faction_type>::string_to_enum_map = {
	{ "none", faction_type::none },
	{ "tribe", faction_type::tribe },
	{ "polity", faction_type::polity },
	{ "minor_tribe", faction_type::minor_tribe },
	{ "notable_house", faction_type::notable_house },
	{ "mercenary_company", faction_type::mercenary_company },
	{ "holy_order", faction_type::holy_order },
	{ "trading_company", faction_type::trading_company }
};

template <>
const bool enum_converter<faction_type>::initialized = enum_converter::initialize();

}