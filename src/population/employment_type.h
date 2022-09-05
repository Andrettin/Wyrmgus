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
#include "util/fractional_int.h"

namespace wyrmgus {

class population_class;
class resource;

class employment_type final : public named_data_entry, public data_type<employment_type>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::resource* input_resource MEMBER input_resource)
	Q_PROPERTY(wyrmgus::resource* output_resource MEMBER output_resource)
	Q_PROPERTY(wyrmgus::centesimal_int output_multiplier MEMBER output_multiplier)

public:
	static constexpr const char *class_identifier = "employment_type";
	static constexpr const char *database_folder = "employment_types";

	explicit employment_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const resource *get_input_resource() const
	{
		return this->input_resource;
	}

	const resource *get_output_resource() const
	{
		return this->output_resource;
	}

	const centesimal_int &get_output_multiplier() const
	{
		return this->output_multiplier;
	}

	const std::vector<const population_class *> &get_employees() const
	{
		return this->employees;
	}

	bool can_employ(const population_class *population_class) const;

private:
	resource *input_resource = nullptr;
	resource *output_resource = nullptr;
	centesimal_int output_multiplier; //output per individual people employed
	std::vector<const population_class *> employees;
};

}
