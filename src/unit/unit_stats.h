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
//      (c) Copyright 1999-2022 by Vladi Belperchinov-Shabanski, Jimmy Salmon
//      and Andrettin
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

#include "economy/resource_container.h"
#include "unit/unit_type_container.h"
#include "unit/unit_variable.h"

namespace wyrmgus {

enum class gender;

/**
**  These are the current stats of a unit. Upgraded or downgraded.
*/
class unit_stats final
{
public:
	const unit_stats &operator = (const unit_stats &rhs);

	bool operator == (const unit_stats &rhs) const;
	bool operator != (const unit_stats &rhs) const;

	const resource_map<int> &get_costs() const
	{
		return this->costs;
	}

	int get_cost(const resource *resource) const
	{
		const auto find_iterator = this->costs.find(resource);

		if (find_iterator != this->costs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_cost(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->costs.contains(resource)) {
				this->costs.erase(resource);
			}
		} else {
			this->costs[resource] = quantity;
		}
	}

	void change_cost(const resource *resource, const int quantity)
	{
		this->set_cost(resource, this->get_cost(resource) + quantity);
	}

	int get_time_cost() const;
	std::string get_costs_string() const;

	const resource_map<int> &get_storing() const
	{
		return this->storing;
	}

	int get_storing(const resource *resource) const
	{
		const auto find_iterator = this->storing.find(resource);

		if (find_iterator != this->storing.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_storing(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->storing.contains(resource)) {
				this->storing.erase(resource);
			}
		} else {
			this->storing[resource] = quantity;
		}
	}

	void change_storing(const resource *resource, const int quantity)
	{
		this->set_storing(resource, this->get_storing(resource) + quantity);
	}

	const resource_map<int> &get_incomes() const
	{
		return this->incomes;
	}

	int get_income(const resource *resource) const
	{
		const auto find_iterator = this->incomes.find(resource);

		if (find_iterator != this->incomes.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_income(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->incomes.contains(resource)) {
				this->incomes.erase(resource);
			}
		} else {
			this->incomes[resource] = quantity;
		}
	}

	void change_income(const resource *resource, const int quantity)
	{
		this->set_income(resource, this->get_income(resource) + quantity);
	}

	const resource_map<int> &get_improve_incomes() const
	{
		return this->improve_incomes;
	}

	int get_improve_income(const resource *resource) const
	{
		const auto find_iterator = this->improve_incomes.find(resource);

		if (find_iterator != this->improve_incomes.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_improve_income(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->improve_incomes.contains(resource)) {
				this->improve_incomes.erase(resource);
			}
		} else {
			this->improve_incomes[resource] = quantity;
		}
	}

	void change_improve_income(const resource *resource, const int quantity)
	{
		this->set_improve_income(resource, this->get_improve_income(resource) + quantity);
	}

	const resource_map<int> &get_resource_demands() const
	{
		return this->resource_demands;
	}

	int get_resource_demand(const resource *resource) const
	{
		const auto find_iterator = this->resource_demands.find(resource);

		if (find_iterator != this->resource_demands.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_resource_demand(const resource *resource, const int quantity)
	{
		if (quantity == 0) {
			if (this->resource_demands.contains(resource)) {
				this->resource_demands.erase(resource);
			}
		} else {
			this->resource_demands[resource] = quantity;
		}
	}

	void change_resource_demand(const resource *resource, const int quantity)
	{
		this->set_resource_demand(resource, this->get_resource_demand(resource) + quantity);
	}

	int get_price() const;

	const unit_type_map<int> &get_unit_stocks() const
	{
		return this->unit_stocks;
	}

	int get_unit_stock(const unit_type *unit_type) const
	{
		if (unit_type == nullptr) {
			return 0;
		}

		const auto find_iterator = this->unit_stocks.find(unit_type);
		if (find_iterator != this->unit_stocks.end()) {
			return find_iterator->second;
		} else {
			return 0;
		}
	}

	void set_unit_stock(const unit_type *unit_type, const int quantity)
	{
		if (unit_type == nullptr) {
			return;
		}

		if (quantity <= 0) {
			if (this->unit_stocks.contains(unit_type)) {
				this->unit_stocks.erase(unit_type);
			}
		} else {
			this->unit_stocks[unit_type] = quantity;
		}
	}

	void change_unit_stock(const unit_type *unit_type, const int quantity)
	{
		this->set_unit_stock(unit_type, this->get_unit_stock(unit_type) + quantity);
	}

	gender get_gender() const;

public:
	std::vector<unit_variable> Variables;           /// user defined variable.
private:
	resource_map<int> costs;            /// current costs of the unit
	resource_map<int> storing;          /// storage increasing
	resource_map<int> incomes; //passive resource incomes for the owner player
	resource_map<int> improve_incomes;   /// Gives player an improved income
	resource_map<int> resource_demands;	/// Resource demand
	unit_type_map<int> unit_stocks;	/// Units in stock
};

}
