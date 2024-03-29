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
/**@name animation_unbreakable.cpp - The animation Unbreakable. */
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

#include "animation/animation_unbreakable.h"

#include "unit/unit.h"
#include "util/assert_util.h"

void CAnimation_Unbreakable::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	assert_log(unit.Anim.Anim == this);
	assert_log(unit.Anim.Unbreakable != this->state);

	unit.Anim.Unbreakable = this->state;
}

void CAnimation_Unbreakable::Init(const char *s, animation_sequence *sequence)
{
	Q_UNUSED(sequence);

	if (!strcmp(s, "begin")) {
		this->state = true;
	} else if (!strcmp(s, "end")) {
		this->state = false;
	} else {
		//LuaError(l, "Unbreakable must be 'begin' or 'end'.  Found: %s" _C_ op2);
	}
}
