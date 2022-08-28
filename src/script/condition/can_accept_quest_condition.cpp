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

#include "script/condition/can_accept_quest_condition.h"

#include "database/gsml_operator.h"
#include "player/player.h"
#include "quest/quest.h"
#include "util/string_util.h"

namespace wyrmgus {

can_accept_quest_condition::can_accept_quest_condition(const wyrmgus::quest *quest)
	: condition(gsml_operator::assignment), quest(quest)
{
}

can_accept_quest_condition::can_accept_quest_condition(const std::string &value, const gsml_operator condition_operator)
	: condition(condition_operator), quest(quest::get(value))
{
}

bool can_accept_quest_condition::check_assignment(const CPlayer *player, const read_only_context &ctx) const
{
	Q_UNUSED(ctx);

	return player->can_accept_quest(this->quest);
}

std::string can_accept_quest_condition::get_assignment_string(const size_t indent, const bool links_allowed) const
{
	Q_UNUSED(indent);
	Q_UNUSED(links_allowed);

	return "Can accept the " + string::highlight(this->quest->get_name()) + " quest";
}

}
