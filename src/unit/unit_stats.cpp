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
//		and Andrettin
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

#include "stratagus.h"

#include "unit/unit_stats.h"

#include "database/defines.h"
#include "economy/resource.h"
#include "unit/unit_type.h"

namespace wyrmgus {

const unit_stats &unit_stats::operator = (const unit_stats &rhs)
{
	this->costs = rhs.costs;
	this->storing = rhs.storing;
	this->incomes = rhs.incomes;
	this->improve_incomes = rhs.improve_incomes;
	this->resource_demands = rhs.resource_demands;
	this->unit_stocks = rhs.unit_stocks;
	this->unit_class_stocks = rhs.unit_class_stocks;
	this->Variables = rhs.Variables;

	return *this;
}

bool unit_stats::operator == (const unit_stats &rhs) const
{
	if (this->costs != rhs.costs) {
		return false;
	}

	if (this->storing != rhs.storing) {
		return false;
	}

	if (this->incomes != rhs.incomes) {
		return false;
	}

	if (this->improve_incomes != rhs.improve_incomes) {
		return false;
	}

	if (this->resource_demands != rhs.resource_demands) {
		return false;
	}

	if (this->unit_stocks != rhs.unit_stocks) {
		return false;
	}

	if (this->unit_class_stocks != rhs.unit_class_stocks) {
		return false;
	}

	for (unsigned int i = 0; i != UnitTypeVar.GetNumberVariable(); ++i) {
		if (this->Variables[i] != rhs.Variables[i]) {
			return false;
		}
	}

	return true;
}

bool unit_stats::operator != (const unit_stats &rhs) const
{
	return !(*this == rhs);
}

int unit_stats::get_time_cost() const
{
	return this->get_cost(defines::get()->get_time_resource());
}

std::string unit_stats::get_costs_string() const
{
	std::string str;

	for (const auto &[resource, cost] : this->get_costs()) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		if (!str.empty()) {
			str += ", ";
		}

		str += std::to_string(cost) + " " + resource->get_name();
	}

	if (this->Variables[DEMAND_INDEX].Value > 0) {
		if (!str.empty()) {
			str += ", ";
		}

		str += std::to_string(this->Variables[DEMAND_INDEX].Value) + " Food";
	}

	return str;
}

int unit_stats::get_price() const
{
	return resource::get_price(this->get_costs());
}

bool unit_stats::has_hired_unit(const unit_type *unit_type) const
{
	if (this->get_unit_stock(unit_type) != 0) {
		return true;
	}

	if (unit_type->get_unit_class() != nullptr && this->get_unit_class_stock(unit_type->get_unit_class()) != 0) {
		return true;
	}

	return false;
}

gender unit_stats::get_gender() const
{
	return static_cast<gender>(this->Variables[GENDER_INDEX].Value);
}

}
