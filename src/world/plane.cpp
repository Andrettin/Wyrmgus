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

#include "world/plane.h"

#include "config.h"
#include "religion/deity_domain.h"
#include "school_of_magic.h"
#include "species/species.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "ui/ui.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Add a plane
**
**	@param	ident	The plane's string identifier
**
**	@return	A newly-created plane
*/
CPlane *CPlane::Add(const std::string &ident)
{
	CPlane *plane = DataType<CPlane>::Add(ident);
	UI.PlaneButtons.resize(CPlane::GetAll().size());
	UI.PlaneButtons[plane->GetIndex()].X = -1;
	UI.PlaneButtons[plane->GetIndex()].Y = -1;
	
	return plane;
}

/**
**	@brief	Remove the existing planes
*/
void CPlane::Clear()
{
	UI.PlaneButtons.clear();
	
	DataType<CPlane>::Clear();
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CPlane::ProcessConfigDataProperty(const String &key, String value)
{
	if (key == "empowered_deity_domain") {
		const CDeityDomain *deity_domain = CDeityDomain::Get(value);
		if (deity_domain != nullptr) {
			this->EmpoweredDeityDomains.insert(deity_domain);
		}
	} else if (key == "empowered_school_of_magic") {
		const CSchoolOfMagic *school_of_magic = CSchoolOfMagic::Get(value);
		if (school_of_magic != nullptr) {
			this->EmpoweredSchoolsOfMagic.insert(school_of_magic);
		}
	} else {
		return false;
	}
	
	return true;
}

void CPlane::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_time_of_day_schedule", "ident"), +[](CPlane *plane, const String &ident){
		plane->TimeOfDaySchedule = CTimeOfDaySchedule::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_time_of_day_schedule"), +[](const CPlane *plane){ return const_cast<CTimeOfDaySchedule *>(plane->GetTimeOfDaySchedule()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "time_of_day_schedule"), "set_time_of_day_schedule", "get_time_of_day_schedule");

	ClassDB::bind_method(D_METHOD("set_season_schedule", "ident"), +[](CPlane *plane, const String &ident){
		plane->SeasonSchedule = CSeasonSchedule::Get(ident);
	});
	ClassDB::bind_method(D_METHOD("get_season_schedule"), +[](const CPlane *plane){ return const_cast<CSeasonSchedule *>(plane->GetSeasonSchedule()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "season_schedule"), "set_season_schedule", "get_season_schedule");

	ClassDB::bind_method(D_METHOD("get_species"), +[](const CPlane *plane){ return ContainerToGodotArray(plane->Species); });
}
