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

#include "script/effect/set_flag_effect.h"

#include "player/player.h"
#include "player/player_flag.h"
#include "util/string_util.h"

namespace wyrmgus {

set_flag_effect::set_flag_effect(const player_flag *flag)
	: set_flag_effect(flag, gsml_operator::assignment)
{
}

set_flag_effect::set_flag_effect(const std::string &flag_identifier, const gsml_operator effect_operator)
	: set_flag_effect(player_flag::get(flag_identifier), effect_operator)
{
}

void set_flag_effect::do_assignment_effect(CPlayer *player) const
{
	player->set_flag(this->flag);
}

std::string set_flag_effect::get_assignment_string() const
{
	return "Set the " + string::highlight(this->flag->get_name()) + " flag";
}

}
