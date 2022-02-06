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
class population_type;
class sml_data;
class sml_property;
struct population_unit_key;

class population_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::population_type* type READ get_type_unconst CONSTANT)
	Q_PROPERTY(wyrmgus::employment_type* employment_type READ get_employment_type_unconst CONSTANT)
	Q_PROPERTY(qint64 population READ get_population_sync NOTIFY population_changed)

public:
	static constexpr int64_t capacity_growth_divisor = 10;
	static constexpr int64_t min_base_growth = 1000; //minimum base growth (used if capacity / capacity_growth_divisor is lower than this; still subject to be reduced further by other factors)

	static bool compare(const population_unit *lhs, const population_unit *rhs);
	
	population_unit()
	{
	}

	explicit population_unit(const population_unit_key &key, const int64_t population);

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	sml_data to_sml_data() const;

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

	int64_t get_population_sync() const
	{
		std::shared_lock<std::shared_mutex> lock(this->mutex);

		return this->get_population();
	}

	void set_population(const int64_t population);

	void change_population(const int64_t change)
	{
		this->set_population(this->get_population() + change);
	}

	int64_t calculate_promotion_quantity(const int64_t promotion_capacity) const;

signals:
	void population_changed();

private:
	const population_type *type = nullptr;
	const wyrmgus::employment_type *employment_type = nullptr;
	int64_t population = 0;
	mutable std::shared_mutex mutex; //mutex for protecting data which is written from the Wyrmgus thread, but which can be read from the Qt thread
};

}
