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
//      (c) Copyright 2018-2020 by Andrettin
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
#include "database/named_data_entry.h"
#include "data_type.h"
#include "time/date.h"

class CFaction;
class CHistoricalLocation;
class CUnitType;

namespace stratagus {

class historical_unit : public named_data_entry, public data_type<historical_unit>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(int quantity MEMBER quantity READ get_quantity)
	Q_PROPERTY(int resources_held MEMBER resources_held READ get_resources_held)

public:
	static constexpr const char *class_identifier = "historical_unit";
	static constexpr const char *database_folder = "historical_units";

	historical_unit(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
	{
	}

	~historical_unit();
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void check() const override;

	int get_quantity() const
	{
		return this->quantity;
	}

	int get_resources_held() const
	{
		return this->resources_held;
	}
	
public:
	CUnitType *UnitType = nullptr; //the unit's unit type
	CFaction *Faction = nullptr; //the unit's faction
private:
	int quantity = 1; //how many in-game units does this historical unit result in when applied
public:
	CDate StartDate; //when the unit starts being active
	CDate EndDate; //when the unit ends being active (e.g. when it is disbanded)
private:
	int resources_held = 0; //how much of the unit's resource, if any, does the unit contain
public:
	std::vector<CHistoricalLocation *> HistoricalLocations; //historical locations for the unit
};

}
