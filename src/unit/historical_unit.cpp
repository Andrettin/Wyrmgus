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
#include "item/item.h"
#include "map/historical_location.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Destructor
*/
CHistoricalUnit::~CHistoricalUnit()
{
	for (const CHistoricalLocation *historical_location : this->HistoricalLocations) {
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
	if (key == "start_date") {
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
		this->HistoricalLocations.push_back(historical_location);
	} else if (section->Tag == "historical_owner") {
		CDate date;
		const CFaction *owner_faction = nullptr;
		
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			if (property.Key == "date") {
				std::string value = FindAndReplaceString(property.Value, "_", "-");
				date = CDate::FromString(value);
			} else if (property.Key == "faction") {
				owner_faction = CFaction::Get(property.Value);
			} else {
				fprintf(stderr, "Invalid historical owner property: \"%s\".\n", property.Key.c_str());
			}
		}
		
		this->HistoricalOwners[date] = owner_faction;
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
	if (this->UnitTypes.empty() && this->UnitClasses.empty()) {
		fprintf(stderr, "Historical unit \"%s\" has neither a unit type nor a unit class.\n", this->GetIdent().utf8().get_data());
	}
	
	if (this->HistoricalLocations.empty()) {
		fprintf(stderr, "Historical unit \"%s\" does not have any historical locations.\n", this->GetIdent().utf8().get_data());
	}
	
	this->Initialized = true;
}

void CHistoricalUnit::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_unit_class", "unit_class_ident"), [](CHistoricalUnit *historical_unit, const String &unit_class_ident){
		historical_unit->UnitClasses.clear();
		historical_unit->UnitClasses.push_back(UnitClass::Get(unit_class_ident));
	});
	ClassDB::bind_method(D_METHOD("get_unit_class"), [](const CHistoricalUnit *historical_unit){
		if (!historical_unit->GetUnitClasses().empty()) {
			return const_cast<UnitClass *>(historical_unit->GetUnitClasses().front());
		}
		
		return static_cast<UnitClass *>(nullptr);
	});
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "unit_class"), "set_unit_class", "get_unit_class");
	
	ClassDB::bind_method(D_METHOD("add_to_unit_classes", "unit_class"), [](CHistoricalUnit *historical_unit, const String &unit_class_ident){ historical_unit->UnitClasses.push_back(UnitClass::Get(unit_class_ident)); });
	ClassDB::bind_method(D_METHOD("remove_from_unit_classes", "unit_class_ident"), [](CHistoricalUnit *historical_unit, const String &unit_class_ident){ historical_unit->UnitClasses.erase(std::remove(historical_unit->UnitClasses.begin(), historical_unit->UnitClasses.end(), UnitClass::Get(unit_class_ident)), historical_unit->UnitClasses.end()); });
	ClassDB::bind_method(D_METHOD("get_unit_classes"), [](const CHistoricalUnit *historical_unit){ return VectorToGodotArray(historical_unit->UnitClasses); });
	
	ClassDB::bind_method(D_METHOD("set_unit_type", "unit_type_ident"), [](CHistoricalUnit *historical_unit, const String &unit_type_ident){
		historical_unit->UnitTypes.clear();
		historical_unit->UnitTypes.push_back(CUnitType::Get(unit_type_ident));
	});
	ClassDB::bind_method(D_METHOD("get_unit_type"), [](const CHistoricalUnit *historical_unit){
		if (!historical_unit->GetUnitTypes().empty()) {
			return const_cast<CUnitType *>(historical_unit->GetUnitTypes().front());
		}
		
		return static_cast<CUnitType *>(nullptr);
	});
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "unit_type"), "set_unit_type", "get_unit_type");
	
	ClassDB::bind_method(D_METHOD("add_to_unit_types", "unit_type"), [](CHistoricalUnit *historical_unit, const String &unit_type_ident){ historical_unit->UnitTypes.push_back(CUnitType::Get(unit_type_ident)); });
	ClassDB::bind_method(D_METHOD("remove_from_unit_types", "unit_type_ident"), [](CHistoricalUnit *historical_unit, const String &unit_type_ident){ historical_unit->UnitTypes.erase(std::remove(historical_unit->UnitTypes.begin(), historical_unit->UnitTypes.end(), CUnitType::Get(unit_type_ident)), historical_unit->UnitTypes.end()); });
	ClassDB::bind_method(D_METHOD("get_unit_types"), [](const CHistoricalUnit *historical_unit){ return VectorToGodotArray(historical_unit->UnitTypes); });
	
	ClassDB::bind_method(D_METHOD("set_faction", "faction_ident"), [](CHistoricalUnit *historical_unit, const String &faction_ident){ historical_unit->Faction = CFaction::Get(faction_ident); });
	ClassDB::bind_method(D_METHOD("get_faction"), [](const CHistoricalUnit *historical_unit){ return const_cast<CFaction *>(historical_unit->GetFaction()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "faction"), "set_faction", "get_faction");
	
	ClassDB::bind_method(D_METHOD("set_unique", "unique_ident"), [](CHistoricalUnit *historical_unit, const String &unique_ident){ historical_unit->Unique = GetUniqueItem(unique_ident.utf8().get_data()); });
	ClassDB::bind_method(D_METHOD("get_unique"), &CHistoricalUnit::GetUnique);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "unique"), "set_unique", "get_unique");
	
	ClassDB::bind_method(D_METHOD("set_quantity", "quantity"), [](CHistoricalUnit *historical_unit, const int quantity){ historical_unit->Quantity = quantity; });
	ClassDB::bind_method(D_METHOD("get_quantity"), &CHistoricalUnit::GetQuantity);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quantity"), "set_quantity", "get_quantity");
	
	ClassDB::bind_method(D_METHOD("set_connection_surface_layer", "connection_surface_layer"), [](CHistoricalUnit *historical_unit, const int connection_surface_layer){ historical_unit->ConnectionSurfaceLayer = connection_surface_layer; });
	ClassDB::bind_method(D_METHOD("get_connection_surface_layer"), &CHistoricalUnit::GetConnectionSurfaceLayer);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_surface_layer"), "set_connection_surface_layer", "get_connection_surface_layer");
	
	ClassDB::bind_method(D_METHOD("set_resources_held", "resources_held"), [](CHistoricalUnit *historical_unit, const int resources_held){ historical_unit->ResourcesHeld = resources_held; });
	ClassDB::bind_method(D_METHOD("get_resources_held"), &CHistoricalUnit::GetResourcesHeld);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "resources_held"), "set_resources_held", "get_resources_held");
}
