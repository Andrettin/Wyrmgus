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

#include "language/name_variant.h"

namespace archimedes {
	enum class gender;
}

namespace wyrmgus {

class name_generator;

class gendered_name_generator final
{
public:
	gendered_name_generator();
	~gendered_name_generator();

	const name_generator *get_name_generator(const gender gender) const
	{
		const auto find_iterator = this->name_generators.find(gender);
		if (find_iterator != this->name_generators.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	void add_name(const gender gender, const name_variant &name);
	void add_names(const gender gender, const std::vector<name_variant> &names);
	void add_names(const gender gender, const std::vector<std::string> &names);
	void add_names_from(const std::unique_ptr<gendered_name_generator> &source_name_generator);

	void propagate_ungendered_names_from(const gendered_name_generator *source_name_generator);

	void propagate_ungendered_names_from(const std::unique_ptr<gendered_name_generator> &source_name_generator)
	{
		this->propagate_ungendered_names_from(source_name_generator.get());
	}

	void propagate_ungendered_names()
	{
		this->propagate_ungendered_names_from(this);
	}

private:
	std::map<gender, std::unique_ptr<name_generator>> name_generators;
};

}
