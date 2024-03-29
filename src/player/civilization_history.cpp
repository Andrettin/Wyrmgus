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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "player/civilization_history.h"

#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void civilization_history::remove_acquired_upgrade_class(const upgrade_class *upgrade_class)
{
	vector::remove(this->acquired_upgrade_classes, upgrade_class);
}

void civilization_history::remove_acquired_upgrade(const CUpgrade *upgrade)
{
	vector::remove(this->acquired_upgrades, upgrade);
}

void civilization_history::remove_explored_settlement(const site *settlement)
{
	vector::remove(this->explored_settlements, settlement);
}

}
