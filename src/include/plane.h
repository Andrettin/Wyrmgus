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
/**@name plane.h - The plane headerfile. */
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

#ifndef __PLANE_H__
#define __PLANE_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CDeityDomain;
class CSeasonSchedule;
class CSpecies;
class CTimeOfDaySchedule;

class CPlane
{
public:
	CPlane() :
		ID(-1), TimeOfDaySchedule(nullptr), SeasonSchedule(nullptr)
	{
	}
	
	static CPlane *GetPlane(const std::string &plane_ident);
	static CPlane *GetOrAddPlane(const std::string &plane_ident);
	static void ClearPlanes();
	
	static std::vector<CPlane *> Planes;								/// Planes

	int ID;																/// ID of this plane
	std::string Ident;
	std::string Name;
	std::string Description;
	std::string Background;
	std::string Quote;
	CTimeOfDaySchedule *TimeOfDaySchedule;								/// this plane's time of day schedule
	CSeasonSchedule *SeasonSchedule;									/// this plane's season schedule
	std::vector<CDeityDomain *> EmpoweredDeityDomains;					/// Deity domains empowered in this plane
	std::vector<CSpecies *> Species;									/// Species in this plane
};

//@}

#endif // !__PLANE_H__
