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

#include "ui/ui.h"
#include "species/species.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "world/plane.h"
#include "world/province.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Add a world
**
**	@param	ident	The world's string identifier
**
**	@return	A newly-created world
*/
CWorld *CWorld::Add(const std::string &ident)
{
	CWorld *world = DataType<CWorld>::Add(ident);
	UI.WorldButtons.resize(CWorld::GetAll().size());
	UI.WorldButtons[world->GetIndex()].X = -1;
	UI.WorldButtons[world->GetIndex()].Y = -1;
	
	return world;
}

/**
**	@brief	Remove the existing worlds
*/
void CWorld::Clear()
{
	UI.WorldButtons.clear();
	
	DataType<CWorld>::Clear();
}

void CWorld::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_plane", "plane"), +[](CWorld *world, const String &ident){ world->Plane = CPlane::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_plane"), &CWorld::GetPlane);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "plane"), "set_plane", "get_plane");
	
	ClassDB::bind_method(D_METHOD("set_time_of_day_schedule", "time_of_day_schedule"), +[](CWorld *world, const String &ident){ world->TimeOfDaySchedule = CTimeOfDaySchedule::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_time_of_day_schedule"), +[](const CWorld *world){ return const_cast<CTimeOfDaySchedule *>(world->GetTimeOfDaySchedule()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "time_of_day_schedule"), "set_time_of_day_schedule", "get_time_of_day_schedule");
	
	ClassDB::bind_method(D_METHOD("set_season_schedule", "season_schedule"), +[](CWorld *world, const String &ident){ world->SeasonSchedule = CSeasonSchedule::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_season_schedule"), +[](const CWorld *world){ return const_cast<CSeasonSchedule *>(world->GetSeasonSchedule()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "season_schedule"), "set_season_schedule", "get_season_schedule");
	
	ClassDB::bind_method(D_METHOD("get_species"), +[](const CWorld *world){ return ContainerToGodotArray(world->Species); });
}
