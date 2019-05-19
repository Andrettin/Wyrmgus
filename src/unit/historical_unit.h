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
/**@name historical_unit.h - The historical unit header file. */
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

#ifndef __HISTORICAL_UNIT_H__
#define __HISTORICAL_UNIT_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "time/date.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFaction;
class CHistoricalLocation;
class CUnitType;
class UnitClass;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CHistoricalUnit : public DataElement, public DataType<CHistoricalUnit>
{
	DATA_TYPE(CHistoricalUnit, DataElement)
	
public:
	~CHistoricalUnit();
	
public:	
	static constexpr const char *ClassIdentifier = "historical_unit";
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	const std::vector<const UnitClass *> &GetUnitClasses() const
	{
		return this->UnitClasses;
	}
	
	const std::vector<const CUnitType *> &GetUnitTypes() const
	{
		return this->UnitTypes;
	}
	
	const CFaction *GetFaction() const
	{
		return this->Faction;
	}
	
	int GetQuantity() const
	{
		return this->Quantity;
	}
	
	const CDate &GetStartDate() const
	{
		return this->StartDate;
	}
	
	const CDate &GetEndDate() const
	{
		return this->EndDate;
	}
	
	int GetResourcesHeld() const
	{
		return this->ResourcesHeld;
	}
	
	const std::vector<const CHistoricalLocation *> &GetHistoricalLocations() const
	{
		return this->HistoricalLocations;
	}
	
	const std::map<CDate, const CFaction *> &GetHistoricalOwners() const
	{
		return this->HistoricalOwners;
	}
	
private:
	std::vector<const UnitClass *> UnitClasses;	/// the unit's potential classes
	std::vector<const CUnitType *> UnitTypes;	/// the unit's potential unit types
	const CFaction *Faction = nullptr;		/// the unit's faction
	int Quantity = 1;	/// how many in-game units does this historical unit result in when applied
	CDate StartDate;	/// when the unit starts being active
	CDate EndDate;		/// when the unit ceases being active (e.g. when it is disbanded)
	int ResourcesHeld = 0;	/// how much of the unit's resource, if any, does the unit contain
	std::vector<const CHistoricalLocation *> HistoricalLocations;	/// historical locations for the unit
	std::map<CDate, const CFaction *> HistoricalOwners;				/// historical owners for the unit

protected:
	static void _bind_methods();
};

#endif
