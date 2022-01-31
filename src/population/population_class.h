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

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/resource_container.h"

namespace wyrmgus {

class population_class final : public named_data_entry, public data_type<population_class>
{
	Q_OBJECT

	Q_PROPERTY(bool growable MEMBER growable READ is_growable)
	Q_PROPERTY(bool unemployment MEMBER unemployment READ can_have_unemployment)

public:
	static constexpr const char *class_identifier = "population_class";
	static constexpr const char *database_folder = "population_classes";

	explicit population_class(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;

	bool is_growable() const
	{
		return this->growable;
	}

	bool can_have_unemployment() const
	{
		return this->unemployment;
	}

	const std::vector<const population_class *> &get_promotion_targets() const
	{
		return this->promotion_targets;
	}

	const std::vector<const population_class *> &get_demotion_targets() const
	{
		return this->demotion_targets;
	}

	const population_class *get_unemployed_class() const
	{
		if (this->can_have_unemployment()) {
			return this;
		}

		for (const population_class *demotion_target : this->demotion_targets) {
			const population_class *unemployed_class = demotion_target->get_unemployed_class();

			if (unemployed_class != nullptr) {
				return unemployed_class;
			}
		}

		return nullptr;
	}

	int get_production_efficiency(const resource *resource) const
	{
		const auto find_iterator = this->production_efficiency_map.find(resource);

		if (find_iterator != this->production_efficiency_map.end()) {
			return find_iterator->second;
		}

		return 100;
	}

private:
	bool growable = false; //whether the population class can grow via population growth; negative growth can still occur even if false however
	bool unemployment = false; //whether the population class can be unemployed, or if when unemployment it must immediately demote
	std::vector<const population_class *> promotion_targets;
	std::vector<const population_class *> demotion_targets;
	resource_map<int> production_efficiency_map;
};

}
