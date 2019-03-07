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
/**@name unit_ptr.cpp - The unit ptr source file. */
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

#include "unit/unit_ptr.h"

#include "unit/unit.h"

CUnitPtr::CUnitPtr(CUnit *u) : unit(u)
{
	if (unit) {
		unit->RefsIncrease();
	}
}

CUnitPtr::CUnitPtr(const CUnitPtr &u) : unit(u.unit)
{
	if (unit) {
		unit->RefsIncrease();
	}
}

void CUnitPtr::Reset()
{
	if (unit) {
		unit->RefsDecrease();
	}
	unit = nullptr;
}

CUnitPtr &CUnitPtr::operator= (CUnit *u)
{
	if (this->unit != u) {
		if (u) {
			u->RefsIncrease();
		}
		if (unit) {
			unit->RefsDecrease();
		}
		unit = u;
	}
	return *this;
}
