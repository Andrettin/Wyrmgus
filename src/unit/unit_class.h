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
//      (c) Copyright 2019-2021 by Andrettin
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

namespace wyrmgus {

class condition;
class unit_type;

class unit_class final : public named_data_entry, public data_type<unit_class>
{
	Q_OBJECT

	Q_PROPERTY(bool town_hall MEMBER town_hall READ is_town_hall)
	Q_PROPERTY(bool ship MEMBER ship READ is_ship)

public:
	static constexpr const char *class_identifier = "unit_class";
	static constexpr const char *database_folder = "unit_classes";

	static unit_class *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		unit_class *unit_class = data_type::add(identifier, data_module);
		unit_class->index = unit_class::get_all().size() - 1;
		return unit_class;
	}

	static const std::vector<unit_class *> &get_town_hall_classes()
	{
		return unit_class::town_hall_classes;
	}

private:
	static inline std::vector<unit_class *> town_hall_classes;

public:
	explicit unit_class(const std::string &identifier);
	~unit_class();

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void check() const override;

	int get_index() const
	{
		return this->index;
	}

	bool is_town_hall() const
	{
		return this->town_hall;
	}

	void set_town_hall(const bool town_hall);

	bool is_ship() const
	{
		return this->ship;
	}

	const std::unique_ptr<condition> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition> &get_conditions() const
	{
		return this->conditions;
	}

	const std::vector<unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}

	bool has_unit_type(unit_type *unit_type) const;

	void add_unit_type(unit_type *unit_type)
	{
		this->unit_types.push_back(unit_type);
	}

	void remove_unit_type(unit_type *unit_type);

private:
	int index = -1;
	bool town_hall = false; //whether the building class is a settlement head building class, e.g. a town hall or fortress
	bool ship = false; //whether the unit class is a ship
	std::unique_ptr<condition> preconditions;
	std::unique_ptr<condition> conditions;
	std::vector<unit_type *> unit_types;
};

}