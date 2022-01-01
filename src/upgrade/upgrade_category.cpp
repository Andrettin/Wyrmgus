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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "upgrade/upgrade_category.h"

#include "age.h"
#include "upgrade/upgrade_category_rank.h"
#include "upgrade/upgrade_class.h"
#include "util/vector_util.h"

namespace wyrmgus {

upgrade_category::upgrade_category(const std::string &identifier)
	: named_data_entry(identifier), rank(upgrade_category_rank::none)
{
}

void upgrade_category::initialize()
{
	for (upgrade_category *subcategory : this->subcategories) {
		if (!subcategory->is_initialized()) {
			subcategory->initialize();
		}

		if (subcategory->get_start_age() == nullptr) {
			continue;
		}

		if (this->start_age == nullptr || subcategory->get_start_age()->get_priority() < this->start_age->get_priority()) {
			this->start_age = subcategory->get_start_age();
		}
	}

	for (const upgrade_class *upgrade_class : this->upgrade_classes) {
		if (upgrade_class->get_age() == nullptr) {
			continue;
		}

		if (this->start_age == nullptr || upgrade_class->get_age()->get_priority() < this->start_age->get_priority()) {
			this->start_age = upgrade_class->get_age();
		}
	}

	data_entry::initialize();
}

void upgrade_category::check() const
{
	if (this->get_rank() == upgrade_category_rank::none) {
		throw std::runtime_error("Upgrade category \"" + this->get_identifier() + "\" has no rank.");
	}

	if (this->get_category() != nullptr && this->get_rank() >= this->get_category()->get_rank()) {
		throw std::runtime_error("The rank of upgrade category \"" + this->get_identifier() + "\" is greater than or equal to that of its upper category.");
	}

	if (this->get_start_age() == nullptr) {
		throw std::runtime_error("Upgrade category \"" + this->get_identifier() + "\" has no start age.");
	}
}

void upgrade_category::set_category(upgrade_category *category)
{
	if (category == this->get_category()) {
		return;
	}

	if (this->category != nullptr) {
		this->category->remove_subcategory(this);
	}

	this->category = category;

	if (category != nullptr) {
		this->category->add_subcategory(this);
	}
}

void upgrade_category::remove_subcategory(upgrade_category *subcategory)
{
	vector::remove(this->subcategories, subcategory);
}

void upgrade_category::remove_upgrade_class(const upgrade_class *upgrade_class)
{
	vector::remove(this->upgrade_classes, upgrade_class);
}

}
