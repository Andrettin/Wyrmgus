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
//      (c) Copyright 2020-2021 by Andrettin
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

enum class geological_era {
	none,
	devonian,
	carboniferous,
	permian,
	triassic,
	jurassic,
	cretaceous,
	paleocene,
	eocene,
	oligocene,
	miocene,
	pliocene,
	pleistocene,
	holocene
};

inline geological_era string_to_geological_era(const std::string &str)
{
	if (str == "devonian") {
		return geological_era::devonian;
	} else if (str == "carboniferous") {
		return geological_era::carboniferous;
	} else if (str == "permian") {
		return geological_era::permian;
	} else if (str == "triassic") {
		return geological_era::triassic;
	} else if (str == "jurassic") {
		return geological_era::jurassic;
	} else if (str == "cretaceous") {
		return geological_era::cretaceous;
	} else if (str == "paleocene") {
		return geological_era::paleocene;
	} else if (str == "eocene") {
		return geological_era::eocene;
	} else if (str == "oligocene") {
		return geological_era::oligocene;
	} else if (str == "miocene") {
		return geological_era::miocene;
	} else if (str == "pliocene") {
		return geological_era::pliocene;
	} else if (str == "pleistocene") {
		return geological_era::pleistocene;
	} else if (str == "holocene") {
		return geological_era::holocene;
	}

	throw std::runtime_error("Invalid geological_era: \"" + str + "\".");
}

inline std::string geological_era_to_string(const geological_era era)
{
	switch (era) {
		case geological_era::devonian:
			return "devonian";
		case geological_era::carboniferous:
			return "carboniferous";
		case geological_era::permian:
			return "permian";
		case geological_era::triassic:
			return "triassic";
		case geological_era::jurassic:
			return "jurassic";
		case geological_era::cretaceous:
			return "cretaceous";
		case geological_era::paleocene:
			return "paleocene";
		case geological_era::eocene:
			return "eocene";
		case geological_era::oligocene:
			return "oligocene";
		case geological_era::miocene:
			return "miocene";
		case geological_era::pliocene:
			return "pliocene";
		case geological_era::pleistocene:
			return "pleistocene";
		case geological_era::holocene:
			return "holocene";
		default:
			break;
	}

	throw std::runtime_error("Invalid geological_era: \"" + std::to_string(static_cast<int>(era)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::geological_era)
