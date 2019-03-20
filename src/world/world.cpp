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
//      (c) Copyright 2016-2019 by Andrettin
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

#include "world/world.h"

#include "config.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "ui/ui.h"
#include "world/plane.h"
#include "world/province.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CWorld *> CWorld::Worlds;
std::map<std::string, CWorld *> CWorld::WorldsByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a world
**
**	@param	ident			The world's string identifier
**	@param	should_find		Whether it is an error if the world could not be found; this is true by default
**
**	@return	The world if found, or null otherwise
*/
CWorld *CWorld::GetWorld(const std::string &ident, const bool should_find)
{
	std::map<std::string, CWorld *>::const_iterator find_iterator = WorldsByIdent.find(ident);
	
	if (find_iterator != WorldsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid world: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a world
**
**	@param	ident	The world's string identifier
**
**	@return	The world if found, or a newly-created one otherwise
*/
CWorld *CWorld::GetOrAddWorld(const std::string &ident)
{
	CWorld *world = GetWorld(ident, false);
	
	if (!world) {
		world = new CWorld;
		world->Ident = ident;
		world->Index = Worlds.size();
		Worlds.push_back(world);
		WorldsByIdent[ident] = world;
		UI.WorldButtons.resize(Worlds.size());
		UI.WorldButtons[world->Index].X = -1;
		UI.WorldButtons[world->Index].Y = -1;
	}
	
	return world;
}

/**
**	@brief	Remove the existing worlds
*/
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
	WorldsByIdent.clear();
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CWorld::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "name") {
		this->Name = value;
	} else if (key == "description") {
		this->Description = value;
	} else if (key == "background") {
		this->Background = value;
	} else if (key == "quote") {
		this->Quote = value;
	} else if (key == "plane") {
		value = FindAndReplaceString(value, "_", "-");
		this->Plane = CPlane::GetPlane(value);
	} else if (key == "time_of_day_schedule") {
		value = FindAndReplaceString(value, "_", "-");
		this->TimeOfDaySchedule = CTimeOfDaySchedule::Get(value);
	} else if (key == "season_schedule") {
		value = FindAndReplaceString(value, "_", "-");
		this->SeasonSchedule = CSeasonSchedule::Get(value);
	} else {
		return false;
	}

	return true;
}
