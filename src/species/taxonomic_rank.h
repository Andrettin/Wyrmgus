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

enum class taxonomic_rank {
	none,
	species,
	genus,
	subtribe,
	tribe,
	subfamily,
	family,
	superfamily,
	infraorder,
	suborder,
	order,
	infraclass,
	subclass,
	class_rank,
	superclass,
	infraphylum,
	subphylum,
	phylum,
	superphylum,
	infrakingdom,
	subkingdom,
	kingdom,
	domain,
	empire
};

inline taxonomic_rank string_to_taxonomic_rank(const std::string &str)
{
	if (str == "species") {
		return taxonomic_rank::species;
	} else if (str == "genus") {
		return taxonomic_rank::genus;
	} else if (str == "subtribe") {
		return taxonomic_rank::subtribe;
	} else if (str == "tribe") {
		return taxonomic_rank::tribe;
	} else if (str == "subfamily") {
		return taxonomic_rank::subfamily;
	} else if (str == "family") {
		return taxonomic_rank::family;
	} else if (str == "superfamily") {
		return taxonomic_rank::superfamily;
	} else if (str == "infraorder") {
		return taxonomic_rank::infraorder;
	} else if (str == "suborder") {
		return taxonomic_rank::suborder;
	} else if (str == "order") {
		return taxonomic_rank::order;
	} else if (str == "infraclass") {
		return taxonomic_rank::infraclass;
	} else if (str == "subclass") {
		return taxonomic_rank::subclass;
	} else if (str == "class") {
		return taxonomic_rank::class_rank;
	} else if (str == "superclass") {
		return taxonomic_rank::superclass;
	} else if (str == "infraphylum") {
		return taxonomic_rank::infraphylum;
	} else if (str == "subphylum") {
		return taxonomic_rank::subphylum;
	} else if (str == "phylum") {
		return taxonomic_rank::phylum;
	} else if (str == "superphylum") {
		return taxonomic_rank::superphylum;
	} else if (str == "infrakingdom") {
		return taxonomic_rank::infrakingdom;
	} else if (str == "subkingdom") {
		return taxonomic_rank::subkingdom;
	} else if (str == "kingdom") {
		return taxonomic_rank::kingdom;
	} else if (str == "domain") {
		return taxonomic_rank::domain;
	} else if (str == "empire") {
		return taxonomic_rank::empire;
	}

	throw std::runtime_error("Invalid taxonomic rank: \"" + str + "\".");
}

inline std::string taxonomic_rank_to_string(const taxonomic_rank rank)
{
	switch (rank) {
		case taxonomic_rank::species:
			return "species";
		case taxonomic_rank::genus:
			return "genus";
		case taxonomic_rank::subtribe:
			return "subtribe";
		case taxonomic_rank::tribe:
			return "tribe";
		case taxonomic_rank::subfamily:
			return "subfamily";
		case taxonomic_rank::family:
			return "family";
		case taxonomic_rank::superfamily:
			return "superfamily";
		case taxonomic_rank::infraorder:
			return "infraorder";
		case taxonomic_rank::suborder:
			return "suborder";
		case taxonomic_rank::order:
			return "order";
		case taxonomic_rank::infraclass:
			return "infraclass";
		case taxonomic_rank::subclass:
			return "subclass";
		case taxonomic_rank::class_rank:
			return "class";
		case taxonomic_rank::superclass:
			return "superclass";
		case taxonomic_rank::infraphylum:
			return "infraphylum";
		case taxonomic_rank::subphylum:
			return "subphylum";
		case taxonomic_rank::phylum:
			return "phylum";
		case taxonomic_rank::superphylum:
			return "superphylum";
		case taxonomic_rank::infrakingdom:
			return "infrakingdom";
		case taxonomic_rank::subkingdom:
			return "subkingdom";
		case taxonomic_rank::kingdom:
			return "kingdom";
		case taxonomic_rank::domain:
			return "domain";
		case taxonomic_rank::empire:
			return "empire";
		default:
			break;
	}

	throw std::runtime_error("Invalid taxonomic rank: \"" + std::to_string(static_cast<int>(rank)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::taxonomic_rank)
