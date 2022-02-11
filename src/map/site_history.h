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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "database/data_entry_history.h"
#include "unit/unit_class_container.h"

namespace wyrmgus {

class faction;
class site;
class unit_class;
class unit_type;

class site_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::faction* owner MEMBER owner)
	Q_PROPERTY(QVariantList building_classes READ get_building_classes_qvariant_list)
	Q_PROPERTY(wyrmgus::unit_type* neutral_building_type MEMBER neutral_building_type)
	Q_PROPERTY(wyrmgus::unit_class* pathway_class MEMBER pathway_class)
	Q_PROPERTY(qint64 population MEMBER population READ get_population)

public:
	explicit site_history(const site *site) : site(site)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const faction *get_owner() const
	{
		return this->owner;
	}

	const std::vector<unit_class *> &get_building_classes() const
	{
		return this->building_classes;
	}

	QVariantList get_building_classes_qvariant_list() const;
	Q_INVOKABLE void add_building_class(unit_class *building_class);
	Q_INVOKABLE void remove_building_class(unit_class *building_class);

	const unit_type *get_neutral_building_type() const
	{
		return this->neutral_building_type;
	}

	const unit_class *get_pathway_class() const
	{
		return this->pathway_class;
	}

	int64_t get_population() const
	{
		return this->population;
	}

	void set_population(const int64_t population)
	{
		this->population = population;
	}

	void change_population(const int64_t change)
	{
		this->population += change;
	}

	unit_class_map<int64_t> &get_population_groups()
	{
		return this->population_groups;
	}

	int64_t get_group_population(const unit_class *unit_class) const
	{
		const auto find_iterator = this->population_groups.find(unit_class);
		if (find_iterator != this->population_groups.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_group_population(const unit_class *unit_class, const int64_t population)
	{
		this->population_groups[unit_class] = population;
	}

	void change_group_population(const unit_class *unit_class, const int64_t change)
	{
		this->set_group_population(unit_class, this->get_group_population(unit_class) + change);
	}

private:
	const wyrmgus::site *site = nullptr;
	faction *owner = nullptr;
	std::vector<unit_class *> building_classes; //applied as buildings at scenario start
	unit_type *neutral_building_type = nullptr; //the building applied on this site at scenario start if it has no owner
	unit_class *pathway_class = nullptr;
	int64_t population = 0; //used for creating units at scenario start
	unit_class_map<int64_t> population_groups; //population size for unit classes
};

}
