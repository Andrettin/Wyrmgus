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
/**@name animation_randomgoto.cpp - The animation RandomGoto. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#include "animation/animation_randomgoto.h"

#include "animation/animation_sequence.h"
#include "unit/unit.h"
#include "util/assert_util.h"
#include "util/string_util.h"
#include "util/util.h"

void CAnimation_RandomGoto::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	assert_throw(unit.Anim.Anim == this);

	if (SyncRand(100) < this->random) {
		unit.Anim.Anim = this->gotoLabel;
	}
}

/*
**  s : "percent label"
*/
void CAnimation_RandomGoto::Init(const char *s, animation_sequence *sequence)
{
	const std::vector<std::string> str_list = string::split(s, ' ');

	this->random = std::stoi(str_list.at(0));

	const std::string &label = str_list.at(1);
	sequence->find_label_later(&this->gotoLabel, label);
}
