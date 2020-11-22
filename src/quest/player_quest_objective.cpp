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
//      (c) Copyright 2020 by Andrettin
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

#include "quest/player_quest_objective.h"

#include "faction.h"
#include "player.h"
#include "quest/objective_type.h"
#include "quest/quest_objective.h"
#include "unit/unit.h"
#include "upgrade/upgrade.h"
#include "util/vector_util.h"

namespace wyrmgus {

void player_quest_objective::change_counter(const int change)
{
	this->counter = std::min(this->get_counter() + change, this->get_quest_objective()->get_quantity());
}

void player_quest_objective::update_counter()
{
	const wyrmgus::quest_objective *quest_objective = this->get_quest_objective();

	switch (quest_objective->get_objective_type()) {
		case objective_type::have_resource:
			this->counter = std::min(this->player->get_resource(quest_objective->get_resource(), STORE_BOTH), quest_objective->get_quantity());
			break;
		case objective_type::research_upgrade:
			this->counter = UpgradeIdAllowed(*this->player, quest_objective->get_upgrade()->ID) == 'R' ? 1 : 0;
			break;
		case objective_type::recruit_hero:
			this->counter = this->player->HasHero(quest_objective->get_character()) ? 1 : 0;
			break;
		default:
			break;
	}
}

void player_quest_objective::on_unit_built(const CUnit *unit)
{
	this->get_quest_objective()->on_unit_built(unit, this);
}

void player_quest_objective::on_unit_destroyed(const CUnit *unit)
{
	this->get_quest_objective()->on_unit_destroyed(unit, this);
}

void player_quest_objective::on_resource_gathered(const resource *resource, const int quantity)
{
	this->get_quest_objective()->on_resource_gathered(resource, quantity, this);
}

}
