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
/**@name plane.h - The plane header file. */
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

#ifndef __PLANE_H__
#define __PLANE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CDeityDomain;
class CSchoolOfMagic;
class CSeasonSchedule;
class CSpecies;
class CTimeOfDaySchedule;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CPlane : public DetailedDataElement, public DataType<CPlane>
{
	DATA_TYPE(CPlane, DetailedDataElement)

public:
	static constexpr const char *ClassIdentifier = "plane";

private:
	static inline bool InitializeClass()
	{
		REGISTER_PROPERTY(SeasonSchedule);
		REGISTER_PROPERTY(TimeOfDaySchedule);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();

public:
	static CPlane *Add(const std::string &ident);
	static void Clear();
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;

	Property<CTimeOfDaySchedule *> TimeOfDaySchedule = nullptr;			/// this plane's time of day schedule
	Property<CSeasonSchedule *> SeasonSchedule = nullptr;				/// this plane's season schedule
	std::vector<CDeityDomain *> EmpoweredDeityDomains;					/// deity domains empowered in this plane
	std::vector<CSchoolOfMagic *> EmpoweredSchoolsOfMagic;				/// schools of magic empowered in this plane
	std::vector<CSpecies *> Species;									/// species native to this plane

protected:
	static void _bind_methods();
};

#endif
