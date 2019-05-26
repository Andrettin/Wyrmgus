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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "animation/animation_randomwait.h"

#include "unit/unit.h"

/* virtual */ void CAnimation_RandomWait::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	Assert(unit.Anim.Anim == this);

	const int arg1 = this->MinWait;
	const int arg2 = this->MaxWait;

	unit.Anim.Wait = arg1 + SyncRand() % (arg2 - arg1 + 1);
}

/*
** s = "minWait MaxWait"
*/
/* virtual */ void CAnimation_RandomWait::Init(const char *s, lua_State *)
{
	
	const std::string str(s);
	const size_t len = str.size();

	std::string min_wait_str;
	size_t begin = 0;
	size_t end = str.find(' ', begin);
	min_wait_str.assign(str, begin, end - begin);
	this->MinWait = std::stoi(min_wait_str);

	std::string max_wait_str;
	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	max_wait_str.assign(str, begin, end - begin);
	this->MaxWait = std::stoi(max_wait_str);
}
