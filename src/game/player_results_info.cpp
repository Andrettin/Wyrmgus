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

#include "game/player_results_info.h"

#include "economy/resource.h"
#include "player/diplomacy_state.h"

namespace wyrmgus {

bool player_results_info::is_ally() const
{
	return this->diplomacy_state.has_value() && this->diplomacy_state.value() == diplomacy_state::allied;
}

bool player_results_info::is_enemy() const
{
	return this->diplomacy_state.has_value() && this->diplomacy_state.value() == diplomacy_state::enemy;
}

int player_results_info::get_resource_count(const QString &resource_identifier) const
{
	const resource *resource = resource::get(resource_identifier.toStdString());

	const auto find_iterator = this->resource_counts.find(resource);

	if (find_iterator != this->resource_counts.end()) {
		return find_iterator->second;
	}

	return 0;
}

}
