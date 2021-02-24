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

#include "stratagus.h"

#include "species/taxon.h"

#include "species/taxonomic_rank.h"

namespace wyrmgus {

taxon::taxon(const std::string &identifier) : taxon_base(identifier), rank(taxonomic_rank::none)
{
}

void taxon::check() const
{
	if (this->get_rank() == taxonomic_rank::none) {
		throw std::runtime_error("Taxon \"" + this->get_identifier() + "\" has no rank.");
	}

	if (this->get_rank() == taxonomic_rank::species) {
		throw std::runtime_error("Taxon \"" + this->get_identifier() + "\" has \"species\" as its taxonomic rank.");
	}

	if (this->get_supertaxon() != nullptr && this->get_rank() >= this->get_supertaxon()->get_rank()) {
		throw std::runtime_error("The rank of taxon \"" + this->get_identifier() + "\" is greater than or equal to that of its supertaxon.");
	}
}

}
