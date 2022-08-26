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

#include "script/effect/accept_quest_effect.h"

#include "player/player.h"
#include "quest/quest.h"
#include "util/string_util.h"

namespace wyrmgus {

accept_quest_effect::accept_quest_effect(wyrmgus::quest *quest)
	: accept_quest_effect(quest, gsml_operator::assignment)
{
}

accept_quest_effect::accept_quest_effect(const std::string &quest_identifier, const gsml_operator effect_operator)
	: accept_quest_effect(quest::get(quest_identifier), effect_operator)
{
}

void accept_quest_effect::do_assignment_effect(CPlayer *player) const
{
	player->accept_quest(this->quest);
}

std::string accept_quest_effect::get_assignment_string() const
{
	return "Receive the " + string::highlight(this->quest->get_name()) + " quest";
}

}
