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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "script/effect/effect_list.h"

#include "database/sml_data.h"
#include "database/sml_property.h"
#include "script/effect/effect.h"

namespace wyrmgus {

effect_list::effect_list()
{
}

effect_list::~effect_list()
{
}

void effect_list::process_sml_property(const sml_property &property)
{
	this->effects.push_back(effect::from_sml_property(property));
}

void effect_list::process_sml_scope(const sml_data &scope)
{
	this->effects.push_back(effect::from_sml_scope(scope));
}

void effect_list::check() const
{
	for (const std::unique_ptr<effect> &effect : this->effects) {
		effect->check();
	}
}

void effect_list::do_effects(CPlayer *player) const
{
	for (const std::unique_ptr<effect> &effect : this->effects) {
		effect->do_effect(player);
	}
}

std::string effect_list::get_effects_string(const size_t indent) const
{
	std::string effects_string;
	bool first = true;
	for (const std::unique_ptr<effect> &effect : this->effects) {
		if (effect->is_hidden()) {
			continue;
		}

		const std::string effect_string = effect->get_string();
		if (effect_string.empty()) {
			continue;
		}

		if (first) {
			first = false;
		} else {
			effects_string += "\n";
		}

		if (indent > 0) {
			effects_string += std::string(indent, '\t');
		}

		effects_string += "- ";

		effects_string += effect_string;
	}
	return effects_string;
}

}
