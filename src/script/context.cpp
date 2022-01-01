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

#include "stratagus.h"

#include "script/context.h"

#include "database/sml_data.h"
#include "player/player.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_ref.h"

namespace wyrmgus {

template <bool read_only>
void context_base<read_only>::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "source_player") {
		const int index = std::stoi(value);
		this->source_player = CPlayer::Players[index].get();
	} else if (key == "current_player") {
		const int index = std::stoi(value);
		this->current_player = CPlayer::Players[index].get();
	} else if (key == "source_unit") {
		const int slot = std::stoi(value);
		this->source_unit = unit_manager::get()->GetSlotUnit(slot).acquire_ref();
	} else if (key == "current_unit") {
		const int slot = std::stoi(value);
		this->current_unit = unit_manager::get()->GetSlotUnit(slot).acquire_ref();
	} else {
		throw std::runtime_error("Invalid context property: \"" + key + "\".");
	}
}

template <bool read_only>
void context_base<read_only>::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid context scope: \"" + scope.get_tag() + "\".");
}

template <bool read_only>
sml_data context_base<read_only>::to_sml_data(const std::string &tag) const
{
	sml_data data(tag);

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

context context::from_scope(CUnit *unit)
{
	context ctx;
	ctx.current_unit = unit->acquire_ref();
	ctx.current_player = unit->Player;
	return ctx;
}

read_only_context read_only_context::from_scope(const CUnit *unit)
{
	read_only_context ctx;
	ctx.current_unit = unit->acquire_ref();
	ctx.current_player = unit->Player;
	return ctx;
}

}
