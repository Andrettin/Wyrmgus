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
//      (c) Copyright 2022 by Andrettin
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

namespace wyrmgus {

class employment_type;
class gsml_data;
class gsml_property;
class population_type;
struct population_unit_key;

class population_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::population_type* type READ get_type_unconst CONSTANT)
	Q_PROPERTY(wyrmgus::employment_type* employment_type READ get_employment_type_unconst CONSTANT)
	Q_PROPERTY(qint64 population READ get_population NOTIFY population_changed)

public:
	static constexpr int64_t capacity_growth_divisor = 10;
	static constexpr int64_t min_base_growth = 1000; //minimum base growth (used if capacity / capacity_growth_divisor is lower than this; still subject to be reduced further by other factors)

	static int64_t calculate_growth_quantity(const int64_t capacity, const int64_t current_population, const bool limit_to_population);
	static int64_t calculate_population_growth_quantity(const int64_t population_growth_capacity, const int64_t current_population);

	static bool compare(const population_unit *lhs, const population_unit *rhs);
	
	population_unit()
	{
	}

	explicit population_unit(const population_unit_key &key, const int64_t population);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	gsml_data to_gsml_data() const;

	const population_type *get_type() const
	{
		return this->type;
	}

private:
	//for the Qt property (pointers there can't be const)
	population_type *get_type_unconst() const
	{
		return const_cast<population_type *>(this->type);
	}

public:
	const wyrmgus::employment_type *get_employment_type() const
	{
		return this->employment_type;
	}

private:
	//for the Qt property (pointers there can't be const)
	wyrmgus::employment_type *get_employment_type_unconst() const
	{
		return const_cast<wyrmgus::employment_type *>(this->employment_type);
	}

public:
	population_unit_key get_key() const;

	int64_t get_population() const
	{
		return this->population;
	}

	void set_population(const int64_t population);

	void change_population(const int64_t change)
	{
		this->set_population(this->get_population() + change);
	}

	int64_t calculate_growth_quantity(const int64_t capacity, const bool limit_to_population) const
	{
		return population_unit::calculate_growth_quantity(capacity, this->get_population(), limit_to_population);
	}

	int64_t calculate_population_growth_quantity(const int64_t population_growth_capacity) const
	{
		return population_unit::calculate_population_growth_quantity(population_growth_capacity, this->get_population());
	}

	int64_t calculate_promotion_quantity(const int64_t promotion_capacity) const
	{
		return this->calculate_growth_quantity(promotion_capacity, true);
	}

signals:
	void population_changed();

private:
	const population_type *type = nullptr;
	const wyrmgus::employment_type *employment_type = nullptr;
	int64_t population = 0;
};

}
