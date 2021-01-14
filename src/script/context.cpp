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
//      (c) Copyright 2021 by Andrettin
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

#include "stratagus.h"

#include "script/context.h"

#include "database/sml_data.h"
#include "player.h"
#include "unit/unit.h"
#include "unit/unit_ref.h"

namespace wyrmgus {

template <bool read_only>
sml_data context_base<read_only>::to_sml_data() const
{
	sml_data data;

	if (this->source_player != nullptr) {
		data.add_property("source_player", std::to_string(this->source_player->get_index()));
	}

	if (this->current_player != nullptr) {
		data.add_property("current_player", std::to_string(this->current_player->get_index()));
	}

	if (this->source_unit != nullptr) {
		data.add_property("source_unit", std::to_string(UnitNumber(*this->source_unit->get())));
	}

	if (this->current_unit != nullptr) {
		data.add_property("current_unit", std::to_string(UnitNumber(*this->current_unit->get())));
	}

	return data;
}

template struct context_base<false>;
template struct context_base<true>;

}
