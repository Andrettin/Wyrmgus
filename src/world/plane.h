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

#include <set>

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
	GDCLASS(CPlane, DetailedDataElement)

public:
	static constexpr const char *ClassIdentifier = "plane";

	static CPlane *Add(const std::string &ident);
	static void Clear();
	
	virtual bool ProcessConfigDataProperty(const String &key, String value) override;
	
	const CTimeOfDaySchedule *GetTimeOfDaySchedule() const { return this->TimeOfDaySchedule; }
	
	const CSeasonSchedule *GetSeasonSchedule() const { return this->SeasonSchedule; }
	
	const std::set<const CDeityDomain *> &GetEmpoweredDeityDomains() const { return this->EmpoweredDeityDomains; }
	
	const std::set<const CSchoolOfMagic *> &GetEmpoweredSchoolsOfMagic() const { return this->EmpoweredSchoolsOfMagic; }

	void AddSpecies(const CSpecies *species) { this->Species.insert(species); }

private:
	const CTimeOfDaySchedule *TimeOfDaySchedule = nullptr;	/// this plane's time of day schedule
	const CSeasonSchedule *SeasonSchedule = nullptr;		/// this plane's season schedule
	std::set<const CDeityDomain *> EmpoweredDeityDomains;		/// deity domains empowered in this plane
	std::set<const CSchoolOfMagic *> EmpoweredSchoolsOfMagic;	/// schools of magic empowered in this plane
	std::set<const CSpecies *> Species;							/// species native to this plane

protected:
	static void _bind_methods();
};

#endif
