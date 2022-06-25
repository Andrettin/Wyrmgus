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

#include "game/results_info.h"

#include "game/player_results_info.h"
#include "results.h"
#include "util/container_util.h"

namespace wyrmgus {

results_info::results_info(const GameResults result, std::vector<qunique_ptr<player_results_info>> &&player_results)
	: result(result), player_results(std::move(player_results))
{
}

results_info::~results_info()
{
}

bool results_info::is_victory() const
{
	return this->result == GameVictory;
}

bool results_info::is_defeat() const
{
	return this->result == GameDefeat;
}

bool results_info::is_draw() const
{
	return this->result == GameDraw;
}

QVariantList results_info::get_player_results_qvariant_list() const
{
	return container::to_qvariant_list(this->player_results);
}

}
