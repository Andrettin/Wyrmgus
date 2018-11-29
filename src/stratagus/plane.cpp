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
/**@name plane.cpp - The plane source file. */
//
//      (c) Copyright 2016-2018 by Andrettin
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

#include "plane.h"

#include "ui/ui.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CPlane *> CPlane::Planes;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CPlane *CPlane::GetPlane(const std::string &plane_ident)
{
	for (size_t i = 0; i < Planes.size(); ++i) {
		if (plane_ident == Planes[i]->Ident) {
			return Planes[i];
		}
	}
	
	return nullptr;
}

CPlane *CPlane::GetOrAddPlane(const std::string &plane_ident)
{
	CPlane *plane = GetPlane(plane_ident);
	
	if (!plane) {
		plane = new CPlane;
		plane->Ident = plane_ident;
		plane->ID = Planes.size();
		Planes.push_back(plane);
		UI.PlaneButtons.resize(Planes.size());
		UI.PlaneButtons[plane->ID].X = -1;
		UI.PlaneButtons[plane->ID].Y = -1;
	}
	
	return plane;
}

void CPlane::ClearPlanes()
{
	for (size_t i = 0; i < Planes.size(); ++i) {
		delete Planes[i];
	}
	Planes.clear();
}

//@}
