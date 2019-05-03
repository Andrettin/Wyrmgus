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
/**@name historical_unit.cpp - The historical unit source file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#include "unit/historical_unit.h"

#include "config.h"
#include "faction.h"
#include "map/historical_location.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Destructor
*/
CHistoricalUnit::~CHistoricalUnit()
{
	for (CHistoricalLocation *historical_location : this->HistoricalLocations) {
		delete historical_location;
	}
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CHistoricalUnit::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "unit_type") {
		this->UnitType = CUnitType::Get(value);
	} else if (key == "faction") {
		this->Faction = CFaction::Get(value);
	} else if (key == "start_date") {
		value = FindAndReplaceString(value, "_", "-");
		this->StartDate = CDate::FromString(value);
	} else if (key == "end_date") {
		value = FindAndReplaceString(value, "_", "-");
		this->EndDate = CDate::FromString(value);
	} else {
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CHistoricalUnit::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "historical_location") {
		CHistoricalLocation *historical_location = new CHistoricalLocation;
		historical_location->ProcessConfigData(section);
			
		if (historical_location->Date.Year == 0 || !historical_location->MapTemplate) {
			delete historical_location;
			return true;
		}
		
		this->HistoricalLocations.push_back(historical_location);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the historical unit
*/
void CHistoricalUnit::Initialize()
{
	if (this->UnitType == nullptr) {
		fprintf(stderr, "Historical unit \"%s\" does not have a unit type.\n", this->GetIdent().utf8().get_data());
	}
	
	if (this->HistoricalLocations.empty()) {
		fprintf(stderr, "Historical unit \"%s\" does not have any historical locations.\n", this->GetIdent().utf8().get_data());
	}
	
	this->Initialized = true;
}

void CHistoricalUnit::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_quantity", "quantity"), [](CHistoricalUnit *historical_unit, const int quantity){ historical_unit->Quantity = quantity; });
	ClassDB::bind_method(D_METHOD("get_quantity"), &CHistoricalUnit::GetQuantity);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quantity"), "set_quantity", "get_quantity");
	
	ClassDB::bind_method(D_METHOD("set_resources_held", "resources_held"), [](CHistoricalUnit *historical_unit, const int resources_held){ historical_unit->ResourcesHeld = resources_held; });
	ClassDB::bind_method(D_METHOD("get_resources_held"), &CHistoricalUnit::GetResourcesHeld);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "resources_held"), "set_resources_held", "get_resources_held");
}
