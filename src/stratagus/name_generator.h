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

#include "language/name_variant.h"
#include "unit/unit_class_container.h"

namespace wyrmgus {

enum class gender;

class name_generator final
{
public:
	static constexpr size_t minimum_name_count = 10;

	static void propagate_ungendered_names(const std::map<gender, std::unique_ptr<name_generator>> &source_name_map, std::map<gender, std::unique_ptr<name_generator>> &target_name_map);

	static void propagate_ungendered_names(std::map<gender, std::unique_ptr<name_generator>> &name_map)
	{
		name_generator::propagate_ungendered_names(name_map, name_map);
	}

	static void propagate_unit_class_names(const unit_class_map<std::unique_ptr<name_generator>> &unit_class_name_generators, std::unique_ptr<name_generator> &ship_name_generator);

	const std::vector<name_variant> &get_names() const
	{
		return this->names;
	}

	size_t get_name_count() const
	{
		return this->names.size();
	}

	bool is_name_valid(const std::string &name) const;

	void add_name(const name_variant &name)
	{
		this->names.push_back(name);
	}

	void add_names(const std::vector<name_variant> &names);
	void add_names(const std::vector<std::string> &names);

	std::string generate_name() const;

private:
	std::vector<name_variant> names; //name list for generation
};

}
