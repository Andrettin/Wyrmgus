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

#include "stratagus.h"

#include "script/cheat.h"

#include "player/player.h"
#include "script/context.h"
#include "script/effect/effect_list.h"

namespace wyrmgus {

cheat::cheat(const std::string &identifier) : data_entry(identifier)
{
}

cheat::~cheat()
{
}

void cheat::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "effects") {
		this->effects = std::make_unique<effect_list<CPlayer>>();
		database::process_sml_data(this->effects, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void cheat::check() const
{
	if (this->code.empty()) {
		throw std::runtime_error("Cheat \"" + this->get_identifier() + "\" has no cheat code.");
	}

	if (this->effects != nullptr) {
		this->effects->check();
	}
}

void cheat::do_effects() const
{
	if (this->effects != nullptr) {
		wyrmgus::context ctx;
		ctx.current_player = CPlayer::GetThisPlayer();

		this->effects->do_effects(CPlayer::GetThisPlayer(), ctx);
	}
}

}
