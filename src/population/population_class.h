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
#include "util/fractional_int.h"

namespace wyrmgus {

class resource_icon;

class population_class final : public named_data_entry, public data_type<population_class>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::resource_icon* resource_icon MEMBER resource_icon)
	Q_PROPERTY(bool growable MEMBER growable READ is_growable)
	Q_PROPERTY(bool unemployment MEMBER unemployment READ can_have_unemployment)
	Q_PROPERTY(wyrmgus::resource* unemployed_output_resource MEMBER unemployed_output_resource)
	Q_PROPERTY(archimedes::centesimal_int unemployed_output_multiplier MEMBER unemployed_output_multiplier)

public:
	static constexpr const char *class_identifier = "population_class";
	static constexpr const char property_class_identifier[] = "wyrmgus::population_class*";
	static constexpr const char *database_folder = "population_classes";

	explicit population_class(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const wyrmgus::resource_icon *get_resource_icon() const
	{
		return this->resource_icon;
	}

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

	bool promotes_to(const population_class *other, const bool include_indirectly) const;

	int get_production_efficiency(const resource *resource) const
	{
		const auto find_iterator = this->production_efficiency_map.find(resource);

		if (find_iterator != this->production_efficiency_map.end()) {
			return find_iterator->second;
		}

		return 100;
	}

	const resource *get_unemployed_output_resource() const
	{
		return this->unemployed_output_resource;
	}

	const centesimal_int &get_unemployed_output_multiplier() const
	{
		return this->unemployed_output_multiplier;
	}

private:
	wyrmgus::resource_icon *resource_icon = nullptr;
	bool growable = false; //whether the population class can grow via population growth; negative growth can still occur even if false however
	bool unemployment = false; //whether the population class can be unemployed, or if when unemployment it must immediately demote
	std::vector<const population_class *> promotion_targets;
	std::vector<const population_class *> demotion_targets;
	resource_map<int> production_efficiency_map;
	resource *unemployed_output_resource = nullptr;
	centesimal_int unemployed_output_multiplier;
};

}
