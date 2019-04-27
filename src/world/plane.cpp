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
bool CPlane::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "empowered_deity_domain") {
		CDeityDomain *deity_domain = CDeityDomain::Get(value);
		if (deity_domain) {
			this->EmpoweredDeityDomains.push_back(deity_domain);
		}
	} else if (key == "empowered_school_of_magic") {
		CSchoolOfMagic *school_of_magic = CSchoolOfMagic::Get(value);
		if (school_of_magic) {
			this->EmpoweredSchoolsOfMagic.push_back(school_of_magic);
		}
	} else {
		return false;
	}
	
	return true;
}

void CPlane::_bind_methods()
{
	BIND_PROPERTIES();
	
	ClassDB::bind_method(D_METHOD("get_species"), [](const CPlane *plane){ return VectorToGodotArray(plane->Species); });
}
