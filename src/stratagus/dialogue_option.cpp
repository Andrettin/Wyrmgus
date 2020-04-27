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
//      (c) Copyright 2015-2020 by Andrettin
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

#include "dialogue_option.h"

#include "database/database.h"
#include "database/sml_data.h"
#include "database/sml_property.h"
#include "luacallback.h"
#include "script/effect/effect_list.h"

namespace stratagus {

dialogue_option::dialogue_option()
{
}

dialogue_option::~dialogue_option()
{
}

void dialogue_option::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "name") {
		this->name = value;
	} else if (key == "tooltip") {
		this->tooltip = value;
	} else {
		throw std::runtime_error("Invalid dialogue option property: \"" + key + "\".");
	}
}

void dialogue_option::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "effects") {
		this->effects = std::make_unique<effect_list>();
		database::process_sml_data(this->effects, scope);
	} else {
		throw std::runtime_error("Invalid dialogue option scope: \"" + tag + "\".");
	}
}

void dialogue_option::do_effects(CPlayer *player) const
{
	if (this->effects != nullptr) {
		this->effects->do_effects(player);
	}

	if (this->lua_effects != nullptr) {
		this->lua_effects->pushPreamble();
		this->lua_effects->run();
	}
}

}
