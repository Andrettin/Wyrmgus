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
/**@name religion.cpp - The religions. */
//
//      (c) Copyright 2018 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "religion.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CReligion *> CReligion::Religions;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CReligion *CReligion::GetReligion(std::string religion_ident)
{
	for (size_t i = 0; i < Religions.size(); ++i) {
		if (religion_ident == Religions[i]->Ident) {
			return Religions[i];
		}
	}
	
	return NULL;
}

void CReligion::ClearReligions()
{
	for (size_t i = 0; i < Religions.size(); ++i) {
		delete Religions[i];
	}
	Religions.clear();
}

//@}
