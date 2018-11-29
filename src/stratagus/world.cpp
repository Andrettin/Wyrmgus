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
/**@name world.cpp - The world source file. */
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

#include "world.h"

#include "province.h"
#include "ui/ui.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CWorld *> CWorld::Worlds;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CWorld *CWorld::GetWorld(std::string world_ident)
{
	for (size_t i = 0; i < Worlds.size(); ++i) {
		if (world_ident == Worlds[i]->Ident) {
			return Worlds[i];
		}
	}
	
	return nullptr;
}

CWorld *CWorld::GetOrAddWorld(std::string world_ident)
{
	CWorld *world = GetWorld(world_ident);
	
	if (!world) {
		world = new CWorld;
		world->Ident = world_ident;
		world->ID = Worlds.size();
		Worlds.push_back(world);
		UI.WorldButtons.resize(Worlds.size());
		UI.WorldButtons[world->ID].X = -1;
		UI.WorldButtons[world->ID].Y = -1;
	}
	
	return world;
}

void CWorld::ClearWorlds()
{
	for (size_t i = 0; i < Worlds.size(); ++i) {
		for (size_t j = 0; j < Worlds[i]->Provinces.size(); ++j) {
			delete Worlds[i]->Provinces[j];
		}
		Worlds[i]->Provinces.clear();
		
		delete Worlds[i];
	}
	Worlds.clear();
}

//@}
