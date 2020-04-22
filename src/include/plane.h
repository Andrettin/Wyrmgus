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
//      (c) Copyright 2016-2020 by Andrettin
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

#pragma once

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "data_type.h"

class CDeityDomain;
class CSchoolOfMagic;
class CSeasonSchedule;
class CSpecies;
class CTimeOfDaySchedule;

namespace stratagus {

class plane : public detailed_data_entry, public data_type<plane>, public CDataType
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "plane";
	static constexpr const char *database_folder = "planes";

	plane(const std::string &identifier) : detailed_data_entry(identifier), CDataType(identifier)
	{
	}

	virtual void ProcessConfigData(const CConfigData *config_data) override;

	CTimeOfDaySchedule *TimeOfDaySchedule = nullptr; //this plane's time of day schedule
	CSeasonSchedule *SeasonSchedule = nullptr; //this plane's season schedule
	std::vector<CDeityDomain *> EmpoweredDeityDomains; ///deity domains empowered in this plane
	std::vector<CSchoolOfMagic *> EmpoweredSchoolsOfMagic; ///schools of magic empowered in this plane
	std::vector<CSpecies *> Species; ///species in this plane
};

}