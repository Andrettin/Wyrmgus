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

#include "stratagus.h"

#include "gendered_name_generator.h"

#include "gender.h"
#include "name_generator.h"

namespace wyrmgus {

gendered_name_generator::gendered_name_generator()
{
}

gendered_name_generator::~gendered_name_generator()
{
}

void gendered_name_generator::add_name(const gender gender, const name_variant &name)
{
	if (this->name_generators.find(gender) == this->name_generators.end()) {
		this->name_generators[gender] = std::make_unique<name_generator>();
	}

	this->name_generators[gender]->add_name(name);
}

void gendered_name_generator::add_names(const gender gender, const std::vector<name_variant> &names)
{
	if (this->name_generators.find(gender) == this->name_generators.end()) {
		this->name_generators[gender] = std::make_unique<name_generator>();
	}

	this->name_generators[gender]->add_names(names);
}

void gendered_name_generator::add_names(const gender gender, const std::vector<std::string> &names)
{
	if (this->name_generators.find(gender) == this->name_generators.end()) {
		this->name_generators[gender] = std::make_unique<name_generator>();
	}

	this->name_generators[gender]->add_names(names);
}

void gendered_name_generator::add_names_from(const std::unique_ptr<gendered_name_generator> &source_name_generator)
{
	for (const auto &kv_pair : source_name_generator->name_generators) {
		this->add_names(kv_pair.first, kv_pair.second->get_names());
	}

	//propagate ungendered names
	this->propagate_ungendered_names_from(source_name_generator);
}

void gendered_name_generator::propagate_ungendered_names_from(const gendered_name_generator *source_name_generator)
{
	const auto find_iterator = source_name_generator->name_generators.find(gender::none);
	if (find_iterator == source_name_generator->name_generators.end()) {
		return;
	}

	this->add_names(gender::male, find_iterator->second->get_names());
	this->add_names(gender::female, find_iterator->second->get_names());
}

}
