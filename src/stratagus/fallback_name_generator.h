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
#include "util/singleton.h"

namespace archimedes {
	enum class gender;
}

namespace wyrmgus {

class gendered_name_generator;
class name_generator;

class fallback_name_generator final : public singleton<fallback_name_generator>
{
public:
	fallback_name_generator();
	~fallback_name_generator();

	const name_generator *get_specimen_name_generator(const gender gender) const;
	void add_specimen_names(const std::unique_ptr<gendered_name_generator> &source_name_generator);

	const name_generator *get_personal_name_generator(const gender gender) const;
	void add_personal_names(const std::unique_ptr<gendered_name_generator> &source_name_generator);

	const name_generator *get_surname_generator(const gender gender) const;
	void add_surnames(const std::unique_ptr<gendered_name_generator> &source_name_generator);

	const name_generator *get_unit_class_name_generator(const unit_class *unit_class) const;
	void add_unit_class_names(const unit_class_map<std::unique_ptr<name_generator>> &unit_class_names);

	void add_ship_names(const std::vector<name_variant> &ship_names);

private:
	//name generation lists containing all names (i.e. from each civilization, species and etc.)
	std::unique_ptr<gendered_name_generator> specimen_name_generator;
	std::unique_ptr<gendered_name_generator> personal_name_generator;
	std::unique_ptr<gendered_name_generator> surname_generator;
	unit_class_map<std::unique_ptr<name_generator>> unit_class_name_generators;
	std::unique_ptr<name_generator> ship_name_generator;
};

}
