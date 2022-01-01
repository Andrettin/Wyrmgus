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

namespace wyrmgus {

enum class status_effect {
	bloodlust,
	haste,
	slow,
	invisible,
	unholy_armor,
	poison,
	stun,
	bleeding,
	leadership,
	blessing,
	inspire,
	precision,
	regeneration,
	barkskin,
	infusion,
	terror,
	wither,
	dehydration,
	hydrating
};

inline status_effect string_to_status_effect(const std::string &str)
{
	if (str == "bloodlust") {
		return status_effect::bloodlust;
	} else if (str == "haste") {
		return status_effect::haste;
	} else if (str == "slow") {
		return status_effect::slow;
	} else if (str == "invisible") {
		return status_effect::invisible;
	} else if (str == "unholy_armor") {
		return status_effect::unholy_armor;
	} else if (str == "poison") {
		return status_effect::poison;
	} else if (str == "stun") {
		return status_effect::stun;
	} else if (str == "bleeding") {
		return status_effect::bleeding;
	} else if (str == "leadership") {
		return status_effect::leadership;
	} else if (str == "blessing") {
		return status_effect::blessing;
	} else if (str == "inspire") {
		return status_effect::inspire;
	} else if (str == "precision") {
		return status_effect::precision;
	} else if (str == "regeneration") {
		return status_effect::regeneration;
	} else if (str == "barkskin") {
		return status_effect::barkskin;
	} else if (str == "infusion") {
		return status_effect::infusion;
	} else if (str == "terror") {
		return status_effect::terror;
	} else if (str == "wither") {
		return status_effect::wither;
	} else if (str == "dehydration") {
		return status_effect::dehydration;
	} else if (str == "hydrating") {
		return status_effect::hydrating;
	}

	throw std::runtime_error("Invalid status effect: \"" + str + "\".");
}

inline std::string status_effect_to_string(const status_effect status_effect)
{
	switch (status_effect) {
		case status_effect::bloodlust:
			return "bloodlust";
		case status_effect::haste:
			return "haste";
		case status_effect::slow:
			return "slow";
		case status_effect::invisible:
			return "invisible";
		case status_effect::unholy_armor:
			return "unholy_armor";
		case status_effect::poison:
			return "poison";
		case status_effect::stun:
			return "stun";
		case status_effect::bleeding:
			return "bleeding";
		case status_effect::leadership:
			return "leadership";
		case status_effect::blessing:
			return "blessing";
		case status_effect::inspire:
			return "inspire";
		case status_effect::precision:
			return "precision";
		case status_effect::regeneration:
			return "regeneration";
		case status_effect::barkskin:
			return "barkskin";
		case status_effect::infusion:
			return "infusion";
		case status_effect::terror:
			return "terror";
		case status_effect::wither:
			return "wither";
		case status_effect::dehydration:
			return "dehydration";
		case status_effect::hydrating:
			return "hydrating";
		default:
			break;
	}

	throw std::runtime_error("Invalid status effect: \"" + std::to_string(static_cast<int>(status_effect)) + "\".");
}

inline bool is_status_effect_harmful(const status_effect status_effect)
{
	switch (status_effect) {
		case status_effect::slow:
		case status_effect::poison:
		case status_effect::stun:
		case status_effect::bleeding:
		case status_effect::terror:
		case status_effect::wither:
		case status_effect::dehydration:
			return true;
		default:
			return false;
	}
}

}

Q_DECLARE_METATYPE(wyrmgus::status_effect)
