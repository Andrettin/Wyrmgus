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

#include "species/taxonomic_rank.h"

namespace archimedes {

template class enum_converter<taxonomic_rank>;

template <>
const std::string enum_converter<taxonomic_rank>::property_class_identifier = "wyrmgus::taxonomic_rank";

template <>
const std::map<std::string, taxonomic_rank> enum_converter<taxonomic_rank>::string_to_enum_map = {
	{ "species", taxonomic_rank::species },
	{ "genus", taxonomic_rank::genus },
	{ "subtribe", taxonomic_rank::subtribe },
	{ "tribe", taxonomic_rank::tribe },
	{ "subfamily", taxonomic_rank::subfamily },
	{ "family", taxonomic_rank::family },
	{ "superfamily", taxonomic_rank::superfamily },
	{ "infraorder", taxonomic_rank::infraorder },
	{ "suborder", taxonomic_rank::suborder },
	{ "order", taxonomic_rank::order },
	{ "infraclass", taxonomic_rank::infraclass },
	{ "subclass", taxonomic_rank::subclass },
	{ "class", taxonomic_rank::class_rank },
	{ "superclass", taxonomic_rank::superclass },
	{ "infraphylum", taxonomic_rank::infraphylum },
	{ "subphylum", taxonomic_rank::subphylum },
	{ "phylum", taxonomic_rank::phylum },
	{ "superphylum", taxonomic_rank::superphylum },
	{ "infrakingdom", taxonomic_rank::infrakingdom },
	{ "subkingdom", taxonomic_rank::subkingdom },
	{ "kingdom", taxonomic_rank::kingdom },
	{ "domain", taxonomic_rank::domain },
	{ "empire", taxonomic_rank::empire }
};

template <>
const bool enum_converter<taxonomic_rank>::initialized = enum_converter::initialize();

}
