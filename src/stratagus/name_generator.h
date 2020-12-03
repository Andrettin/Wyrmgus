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
//

#pragma once

#include "unit/unit_class_container.h"
#include "util/singleton.h"

namespace wyrmgus {

enum class gender;

class name_generator final : public singleton<name_generator>
{
public:
	static constexpr size_t minimum_name_count = 10;

	static void propagate_ungendered_names(const std::map<gender, std::vector<std::string>> &source_name_map, std::map<gender, std::vector<std::string>> &target_name_map);

	static void propagate_ungendered_names(std::map<gender, std::vector<std::string>> &name_map)
	{
		name_generator::propagate_ungendered_names(name_map, name_map);
	}

	const std::vector<std::string> &get_specimen_names(const gender gender) const;
	void add_specimen_names(const std::map<gender, std::vector<std::string>> &specimen_names);

	const std::vector<std::string> &get_personal_names(const gender gender) const;
	void add_personal_names(const std::map<gender, std::vector<std::string>> &personal_names);

	const std::vector<std::string> &get_surnames() const
	{
		return this->surnames;
	}

	void add_surnames(const std::vector<std::string> &surnames);

	const std::vector<std::string> &get_unit_class_names(const unit_class *unit_class) const;

	void add_unit_class_names(const unit_class_map<std::vector<std::string>> &unit_class_names);

	const std::vector<std::string> &get_ship_names() const
	{
		return this->ship_names;
	}

	void add_ship_names(const std::vector<std::string> &ship_names);

private:
	//name generation lists containing all names (i.e. from each civilization, species and etc.)
	std::map<gender, std::vector<std::string>> specimen_names;
	std::map<gender, std::vector<std::string>> personal_names;
	std::vector<std::string> surnames;
	unit_class_map<std::vector<std::string>> unit_class_names;
	std::vector<std::string> ship_names;
};

}
