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
//      (c) Copyright 2018-2021 by Andrettin
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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "data_type.h"

namespace wyrmgus {

class faction;
class historical_location;
class historical_unit_history;
class unique_item;
class unit_class;
class unit_type;

class historical_unit final : public named_data_entry, public data_type<historical_unit>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::unit_class* unit_class READ get_unit_class WRITE set_unit_class)
	Q_PROPERTY(wyrmgus::unit_type* unit_type READ get_unit_type WRITE set_unit_type)
	Q_PROPERTY(wyrmgus::unique_item* unique MEMBER unique)
	Q_PROPERTY(int repeat_count MEMBER repeat_count READ get_repeat_count)
	Q_PROPERTY(int quantity MEMBER quantity READ get_quantity)
	Q_PROPERTY(int resources_held MEMBER resources_held READ get_resources_held)
	Q_PROPERTY(bool ai_active MEMBER ai_active READ is_ai_active)
	Q_PROPERTY(int ttl MEMBER ttl READ get_ttl)

public:
	static constexpr const char *class_identifier = "historical_unit";
	static constexpr const char *database_folder = "historical_units";

	explicit historical_unit(const std::string &identifier);
	~historical_unit();
	
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	const historical_unit_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	const std::vector<unit_class *> &get_unit_classes() const
	{
		return this->unit_classes;
	}

	unit_class *get_unit_class() const
	{
		if (!this->get_unit_classes().empty()) {
			return this->get_unit_classes().front();
		}

		return nullptr;
	}

	void set_unit_class(unit_class *unit_class)
	{
		this->unit_classes.clear();
		this->unit_classes.push_back(unit_class);
	}

	const std::vector<unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}

	unit_type *get_unit_type() const
	{
		if (!this->get_unit_types().empty()) {
			return this->get_unit_types().front();
		}

		return nullptr;
	}

	void set_unit_type(unit_type *unit_type)
	{
		this->unit_types.clear();
		this->unit_types.push_back(unit_type);
	}

	const unique_item *get_unique() const
	{
		return this->unique;
	}

	int get_repeat_count() const
	{
		return this->repeat_count;
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

	int get_ttl() const
	{
		return this->ttl;
	}
	
private:
	std::vector<unit_class *> unit_classes; //the unit's possible unit classes
	std::vector<unit_type *> unit_types; //the unit's possible unit types
	unique_item *unique = nullptr;
	int repeat_count = 1; //how many times should this historical unit be applied in the game; this differs from the quantity field in that if the unit has a random position, then each time it is applied that will be done in a different location
	int quantity = 1; //how many in-game units does this historical unit result in when applied
	int resources_held = 0; //how much of the unit's resource, if any, does the unit contain
	bool ai_active = true; //whether the unit's AI is active
	int ttl = 0; //the TTL (time to live, in cycles) of the unit, useful for revealers
	std::unique_ptr<historical_unit_history> history;
};

}
