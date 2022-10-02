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

#include "species/geological_era.h"

namespace wyrmgus {

template class enum_converter<geological_era>;

template <>
const std::string enum_converter<geological_era>::property_class_identifier = "wyrmgus::geological_era";

template <>
const std::map<std::string, geological_era> enum_converter<geological_era>::string_to_enum_map = {
	{ "devonian", geological_era::devonian },
	{ "carboniferous", geological_era::carboniferous },
	{ "permian", geological_era::permian },
	{ "triassic", geological_era::triassic },
	{ "jurassic", geological_era::jurassic },
	{ "cretaceous", geological_era::cretaceous },
	{ "paleocene", geological_era::paleocene },
	{ "eocene", geological_era::eocene },
	{ "oligocene", geological_era::oligocene },
	{ "eocene", geological_era::eocene },
	{ "miocene", geological_era::miocene },
	{ "pliocene", geological_era::pliocene },
	{ "pleistocene", geological_era::pleistocene },
	{ "holocene", geological_era::holocene }
};

template <>
const bool enum_converter<geological_era>::initialized = enum_converter::initialize();

}
