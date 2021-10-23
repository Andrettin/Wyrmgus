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
//      (c) Copyright 2021 by Andrettin
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

class CUnit;

namespace wyrmgus {

class resource;

struct find_resource_result final
{
	bool is_valid() const
	{
		this->resource_pos != QPoint(-1, -1) || this->resource_unit != nullptr;
	}

	QPoint resource_pos = QPoint(-1, -1);
	CUnit *resource_unit = nullptr;
};

class resource_finder final
{
public:
	explicit resource_finder(const CUnit *worker, const CUnit *start_unit, const int range, const wyrmgus::resource *resource, const CUnit *depot)
		: worker(worker), start_unit(start_unit), range(range), resource(resource), depot(depot)
	{
	}

	find_resource_result find();

	const CUnit *get_worker() const
	{
		return this->worker;
	}

	int get_range() const
	{
		return this->range;
	}

	const wyrmgus::resource *get_resource() const
	{
		return this->resource;
	}

	const CUnit *get_depot() const
	{
		return this->depot;
	}

	bool ignores_exploration() const
	{
		return this->ignore_exploration;
	}

	void set_ignore_exploration(const bool value)
	{
		this->ignore_exploration = value;
	}

	bool checks_usage() const
	{
		return this->check_usage;
	}

	void set_check_usage(const bool value)
	{
		this->check_usage = value;
	}

	bool includes_only_harvestable() const
	{
		return this->only_harvestable;
	}

	void set_only_harvestable(const bool value)
	{
		this->only_harvestable = value;
	}

	bool includes_luxury_resources() const
	{
		return this->include_luxury_resources;
	}

	void set_include_luxury_resources(const bool value)
	{
		this->include_luxury_resources = value;
	}

	bool allows_only_same_resource() const
	{
		return this->only_same_resource;
	}

	void set_only_same_resource(const bool value)
	{
		this->only_same_resource = value;
	}

private:
	const CUnit *worker = nullptr;
	const CUnit *start_unit = nullptr; //the unit to use as the starting point for the search
	int range = std::numeric_limits<int>::max();
	const wyrmgus::resource *resource = nullptr;
	const CUnit *depot = nullptr;
	bool ignore_exploration = false;
	bool check_usage = false;
	bool only_harvestable = true;
	bool include_luxury_resources = false;
	bool only_same_resource = false;
};

}
