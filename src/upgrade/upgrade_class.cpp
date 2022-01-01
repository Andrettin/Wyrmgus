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

#include "stratagus.h"

#include "upgrade/upgrade_class.h"

#include "script/condition/and_condition.h"
#include "unit/unit_class.h"
#include "upgrade/upgrade_category.h"
#include "util/vector_util.h"

namespace wyrmgus {

upgrade_class::upgrade_class(const std::string &identifier) : named_data_entry(identifier)
{
}

upgrade_class::~upgrade_class()
{
}

void upgrade_class::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition>();
		database::process_sml_data(this->preconditions, scope);
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition>();
		database::process_sml_data(this->conditions, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void upgrade_class::initialize()
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		this->tech_tree_parent_unit_class->add_tech_tree_child_upgrade_class(this);
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		this->tech_tree_parent_upgrade_class->add_tech_tree_child_upgrade_class(this);
	}

	data_entry::initialize();
}

void upgrade_class::check() const
{
	if (this->get_category() == nullptr) {
		throw std::runtime_error("Upgrade class \"" + this->get_identifier() + "\" has no category.");
	}

	if (this->get_age() == nullptr) {
		throw std::runtime_error("Upgrade class \"" + this->get_identifier() + "\" has no age.");
	}

	if (this->get_tech_tree_parent() == this) {
		throw std::runtime_error("Upgrade class \"" + this->get_identifier() + "\" is its own tech tree parent.");
	}

	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

const upgrade_category *upgrade_class::get_category(const upgrade_category_rank rank) const
{
	if (this->get_category() != nullptr) {
		if (this->get_category()->get_rank() == rank) {
			return this->get_category();
		}

		if (this->get_category()->get_rank() < rank) {
			return this->get_category()->get_category(rank);
		}
	}

	return nullptr;
}

void upgrade_class::set_category(upgrade_category *category)
{
	if (category == this->get_category()) {
		return;
	}

	if (this->category != nullptr) {
		this->category->remove_upgrade_class(this);
	}

	this->category = category;

	if (category != nullptr) {
		this->category->add_upgrade_class(this);
	}
}

bool upgrade_class::has_upgrade(CUpgrade *upgrade) const
{
	return vector::contains(this->upgrades, upgrade);
}

void upgrade_class::remove_upgrade(CUpgrade *upgrade)
{
	vector::remove(this->upgrades, upgrade);
}

data_entry *upgrade_class::get_tech_tree_parent() const
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		return this->tech_tree_parent_unit_class;
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		return this->tech_tree_parent_upgrade_class;
	}

	return nullptr;
}

int upgrade_class::get_tech_tree_x() const
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		return this->tech_tree_parent_unit_class->get_tech_tree_x() + this->get_tech_tree_relative_x();
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		return this->tech_tree_parent_upgrade_class->get_tech_tree_x() + this->get_tech_tree_relative_x();
	}

	return 0;
}

int upgrade_class::get_tech_tree_relative_x() const
{
	const std::vector<const unit_class *> &sibling_unit_classes = this->tech_tree_parent_unit_class ? this->tech_tree_parent_unit_class->get_tech_tree_child_unit_classes() : this->tech_tree_parent_upgrade_class->get_tech_tree_child_unit_classes();

	const std::vector<const upgrade_class *> &sibling_upgrade_classes = this->tech_tree_parent_unit_class ? this->tech_tree_parent_unit_class->get_tech_tree_child_upgrade_classes() : this->tech_tree_parent_upgrade_class->get_tech_tree_child_upgrade_classes();

	int relative_x = 0;

	for (const unit_class *unit_class : sibling_unit_classes) {
		relative_x += unit_class->get_tech_tree_width();
	}

	for (const upgrade_class *upgrade_class : sibling_upgrade_classes) {
		if (upgrade_class == this) {
			break;
		}

		relative_x += upgrade_class->get_tech_tree_width();
	}

	return relative_x;
}

int upgrade_class::get_tech_tree_y() const
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		return this->tech_tree_parent_unit_class->get_tech_tree_y() + 1;
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		return this->tech_tree_parent_upgrade_class->get_tech_tree_y() + 1;
	}

	return 0;
}

int upgrade_class::get_tech_tree_width() const
{
	int children_width = 0;

	for (const unit_class *unit_class : this->tech_tree_child_unit_classes) {
		children_width += unit_class->get_tech_tree_width();
	}

	for (const upgrade_class *upgrade_class : this->tech_tree_child_upgrade_classes) {
		children_width += upgrade_class->get_tech_tree_width();
	}

	return std::max(children_width, 1);
}

}
