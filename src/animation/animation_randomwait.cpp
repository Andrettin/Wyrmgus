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
/**@name animation_randomwait.cpp - The animation RandomWait. */
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
//

#include "stratagus.h"

#include "animation/animation_randomwait.h"

#include "unit/unit.h"
#include "util/string_util.h"
#include "util/util.h"

void CAnimation_RandomWait::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	Assert(unit.Anim.Anim == this);

	const int arg1 = this->min_wait;
	const int arg2 = this->max_wait;

	unit.Anim.Wait = arg1 + SyncRand(arg2 - arg1 + 1);
}

/*
** s = "min_wait max_wait"
*/
void CAnimation_RandomWait::Init(const char *s, lua_State *)
{
	const std::vector<std::string> str_list = wyrmgus::string::split(s, ' ');

	if (str_list.size() >= 2) {
		this->min_wait = std::stoi(str_list.at(0));
		this->max_wait = std::stoi(str_list.at(1));
	} else {
		this->min_wait = 1;
		this->max_wait = std::stoi(str_list.at(0));
	}
}
