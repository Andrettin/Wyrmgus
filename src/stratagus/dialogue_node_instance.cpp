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
//      (c) Copyright 2022 by Andrettin
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

#include "dialogue_node_instance.h"

#include "dialogue.h"
#include "dialogue_node.h"
#include "engine_interface.h"
#include "game/game.h"
#include "player/player.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"

namespace wyrmgus {

void dialogue_node_instance::call_option_effect(const int option_index)
{
	game::get()->post_function([this, option_index]() {
		CPlayer *player = CPlayer::GetThisPlayer();

		context ctx;
		ctx.current_player = player;

		if (unit_number != -1) {
			CUnit &unit = unit_manager::get()->GetSlotUnit(unit_number);
			if (!unit.Destroyed) {
				ctx.current_unit = unit.acquire_ref();
			}
		}

		this->node->get_dialogue()->call_node_option_effect(this->node->get_index(), option_index, player, ctx);

		engine_interface::get()->remove_dialogue_node_instance(this);
	});
}

}
