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
//      (c) Copyright 2020 by Andrettin
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
//

#pragma once

namespace wyrmgus {

enum class faction_type {
	none,
	tribe,
	polity,
	mercenary_company,
	holy_order,
	trading_company,
	
	count
};

inline faction_type string_to_faction_type(const std::string &str)
{
	if (str == "none") {
		return faction_type::none;
	} else if (str == "tribe") {
		return faction_type::tribe;
	} else if (str == "polity") {
		return faction_type::polity;
	} else if (str == "mercenary_company") {
		return faction_type::mercenary_company;
	} else if (str == "holy_order") {
		return faction_type::holy_order;
	} else if (str == "trading_company") {
		return faction_type::trading_company;
	}

	throw std::runtime_error("Invalid faction type: \"" + str + "\".");
}

inline std::string faction_type_to_string(const faction_type type)
{
	switch (type) {
		case faction_type::none:
			return "none";
		case faction_type::tribe:
			return "tribe";
		case faction_type::polity:
			return "polity";
		case faction_type::mercenary_company:
			return "mercenary_company";
		case faction_type::holy_order:
			return "holy_order";
		case faction_type::trading_company:
			return "trading_company";
		default:
			break;
	}

	throw std::runtime_error("Invalid faction type: \"" + std::to_string(static_cast<int>(type)) + "\".");
}

inline bool is_faction_type_neutral(const faction_type type)
{
	switch (type) {
		case faction_type::mercenary_company:
		case faction_type::holy_order:
		case faction_type::trading_company:
			return true;
		default:
			return false;
	}
}

inline std::string get_faction_type_name(const faction_type type)
{
	switch (type) {
		case faction_type::none:
			return "None";
		case faction_type::tribe:
			return "Tribe";
		case faction_type::polity:
			return "Polity";
		case faction_type::mercenary_company:
			return "Mercenary Company";
		case faction_type::holy_order:
			return "Holy Order";
		case faction_type::trading_company:
			return "Trading Company";
		default:
			break;
	}

	throw std::runtime_error("Invalid faction type: \"" + std::to_string(static_cast<int>(type)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::faction_type)
