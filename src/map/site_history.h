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
//      (c) Copyright 2020-2021 by Andrettin
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

class site_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::faction* owner MEMBER owner)
	Q_PROPERTY(QVariantList building_classes READ get_building_classes_qvariant_list)
	Q_PROPERTY(wyrmgus::unit_class* pathway_class MEMBER pathway_class)
	Q_PROPERTY(int population MEMBER population READ get_population)

public:
	explicit site_history(const site *site) : site(site)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;

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

	const unit_class *get_pathway_class() const
	{
		return this->pathway_class;
	}

	int get_population() const
	{
		return this->population;
	}

	const unit_class_map<int> &get_population_groups() const
	{
		return this->population_groups;
	}

private:
	const wyrmgus::site *site = nullptr;
	faction *owner = nullptr;
	std::vector<unit_class *> building_classes; //applied as buildings at scenario start
	unit_class *pathway_class = nullptr;
	int population = 0; //used for creating units at scenario start
	unit_class_map<int> population_groups; //population size for unit classes (represented as indexes)
};

}
