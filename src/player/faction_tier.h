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

#pragma once

namespace wyrmgus {

enum class faction_tier {
	none,
	barony,
	viscounty,
	county,
	marquisate,
	duchy,
	grand_duchy,
	kingdom,
	empire,
	
	count
};

inline faction_tier string_to_faction_tier(const std::string &str)
{
	if (str == "none") {
		return faction_tier::none;
	} else if (str == "barony") {
		return faction_tier::barony;
	} else if (str == "viscounty") {
		return faction_tier::viscounty;
	} else if (str == "county") {
		return faction_tier::county;
	} else if (str == "marquisate") {
		return faction_tier::marquisate;
	} else if (str == "duchy") {
		return faction_tier::duchy;
	} else if (str == "grand_duchy") {
		return faction_tier::grand_duchy;
	} else if (str == "kingdom") {
		return faction_tier::kingdom;
	} else if (str == "empire") {
		return faction_tier::empire;
	}

	throw std::runtime_error("Invalid faction tier: \"" + str + "\".");
}

inline std::string faction_tier_to_string(const faction_tier tier)
{
	switch (tier) {
		case faction_tier::none:
			return "none";
		case faction_tier::barony:
			return "barony";
		case faction_tier::viscounty:
			return "viscounty";
		case faction_tier::county:
			return "county";
		case faction_tier::marquisate:
			return "marquisate";
		case faction_tier::duchy:
			return "duchy";
		case faction_tier::grand_duchy:
			return "grand_duchy";
		case faction_tier::kingdom:
			return "kingdom";
		case faction_tier::empire:
			return "empire";
		default:
			break;
	}

	throw std::runtime_error("Invalid faction tier: \"" + std::to_string(static_cast<int>(tier)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::faction_tier)
