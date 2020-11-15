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
//      (c) Copyright 2020 by Andrettin
//
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
