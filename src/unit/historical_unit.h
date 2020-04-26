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

class CUnitType;

namespace stratagus {

class faction;
class historical_location;
class unit_class;

class historical_unit : public named_data_entry, public data_type<historical_unit>
{
	Q_OBJECT

	Q_PROPERTY(stratagus::unit_class* unit_class MEMBER unit_class READ get_unit_class)
	Q_PROPERTY(CUnitType* unit_type MEMBER unit_type READ get_unit_type)
	Q_PROPERTY(int quantity MEMBER quantity READ get_quantity)
	Q_PROPERTY(int resources_held MEMBER resources_held READ get_resources_held)
	Q_PROPERTY(bool ai_active MEMBER ai_active READ is_ai_active)
	Q_PROPERTY(bool active MEMBER active READ is_active)
	Q_PROPERTY(stratagus::faction* faction MEMBER faction READ get_faction)

public:
	static constexpr const char *class_identifier = "historical_unit";
	static constexpr const char *database_folder = "historical_units";

	historical_unit(const std::string &identifier);
	~historical_unit();
	
	virtual void process_sml_dated_scope(const sml_data &scope, const QDateTime &date) override;
	virtual void check() const override;
	virtual void reset_history() override;

	unit_class *get_unit_class() const
	{
		return this->unit_class;
	}

	CUnitType *get_unit_type() const
	{
		return this->unit_type;
	}

	int get_quantity() const
	{
		return this->quantity;
	}

	int get_resources_held() const
	{
		return this->resources_held;
	}

	bool is_ai_active() const
	{
		return this->ai_active;
	}

	bool is_active() const
	{
		return this->active;
	}

	faction *get_faction() const
	{
		return this->faction;
	}

	const std::unique_ptr<historical_location> &get_location() const
	{
		return this->location;
	}
	
private:
	unit_class *unit_class = nullptr; //the unit's unit class
	CUnitType *unit_type = nullptr; //the unit's unit type
	int quantity = 1; //how many in-game units does this historical unit result in when applied
	int resources_held = 0; //how much of the unit's resource, if any, does the unit contain
	bool ai_active = true; //whether the unit's AI is active
	bool active = false; //whether the unit is active, i.e. should be applied to the map; used for history
	faction *faction = nullptr; //the unit's faction, used for history
	std::unique_ptr<historical_location> location; //the unit's location, used for history
};

}
