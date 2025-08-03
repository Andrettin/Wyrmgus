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
//      (c) Copyright 2019-2022 by Andrettin
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

#include "unit/unit_class.h"

#include "language/name_generator.h"
#include "map/terrain_type.h"
#include "script/condition/and_condition.h"
#include "script/conditional_string.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_class.h"
#include "util/vector_util.h"

namespace wyrmgus {

void unit_class::propagate_unit_class_names(const unit_class_map<std::unique_ptr<name_generator>> &unit_class_name_generators, std::unique_ptr<name_generator> &ship_name_generator)
{
	for (const auto &kv_pair : unit_class_name_generators) {
		const unit_class *unit_class = kv_pair.first;

		if (unit_class->is_ship()) {
			if (ship_name_generator == nullptr) {
				ship_name_generator = std::make_unique<name_generator>();
			}

			ship_name_generator->add_names(kv_pair.second->get_names());
		}
	}
}

unit_class::unit_class(const std::string &identifier) : named_data_entry(identifier)
{
}

unit_class::~unit_class()
{
}

void unit_class::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "conditional_requirements_strings") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			auto conditional_string = std::make_unique<wyrmgus::conditional_string<CPlayer>>();
			child_scope.process(conditional_string.get());
			this->conditional_requirements_strings.push_back(std::move(conditional_string));
		});
	} else if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition<CPlayer>>();
		scope.process(this->preconditions.get());
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition<CPlayer>>();
		scope.process(this->conditions.get());
	} else if (tag == "0_ad_template_names") {
		for (const std::string &value : values) {
			this->map_to_0_ad_template_name(value);
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void unit_class::initialize()
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		this->tech_tree_parent_unit_class->add_tech_tree_child_unit_class(this);
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		this->tech_tree_parent_upgrade_class->add_tech_tree_child_unit_class(this);
	}

	data_entry::initialize();
}

void unit_class::check() const
{
	if (this->get_tech_tree_parent() == this) {
		throw std::runtime_error("Unit class \"" + this->get_identifier() + "\" is its own tech tree parent.");
	}

	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

void unit_class::set_town_hall(const bool town_hall)
{
	if (town_hall == this->is_town_hall()) {
		return;
	}

	this->town_hall = town_hall;

	if (town_hall) {
		unit_class::town_hall_classes.push_back(this);
	} else {
		vector::remove(unit_class::town_hall_classes, this);
	}
}

const std::string &unit_class::get_requirements_string(const CPlayer *player) const
{
	const read_only_context ctx = read_only_context::from_scope(player);

	for (const auto &conditional_string : this->conditional_requirements_strings) {
		if (!conditional_string->get_conditions()->check(player, ctx)) {
			continue;
		}

		return conditional_string->get_text();
	}

	return this->get_requirements_string();
}

bool unit_class::has_unit_type(unit_type *unit_type) const
{
	return vector::contains(this->unit_types, unit_type);
}

void unit_class::remove_unit_type(unit_type *unit_type)
{
	vector::remove(this->unit_types, unit_type);
}

data_entry *unit_class::get_tech_tree_parent() const
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		return this->tech_tree_parent_unit_class;
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		return this->tech_tree_parent_upgrade_class;
	}

	return nullptr;
}

int unit_class::get_tech_tree_x() const
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		return this->tech_tree_parent_unit_class->get_tech_tree_x() + this->get_tech_tree_relative_x();
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		return this->tech_tree_parent_upgrade_class->get_tech_tree_x() + this->get_tech_tree_relative_x();
	}

	return 0;
}

int unit_class::get_tech_tree_relative_x() const
{
	const std::vector<const unit_class *> &sibling_unit_classes = this->tech_tree_parent_unit_class ? this->tech_tree_parent_unit_class->get_tech_tree_child_unit_classes() : this->tech_tree_parent_upgrade_class->get_tech_tree_child_unit_classes();

	int relative_x = 0;

	for (const unit_class *unit_class : sibling_unit_classes) {
		if (unit_class == this) {
			break;
		}

		relative_x += unit_class->get_tech_tree_width();
	}

	return relative_x;
}

int unit_class::get_tech_tree_y() const
{
	if (this->tech_tree_parent_unit_class != nullptr) {
		return this->tech_tree_parent_unit_class->get_tech_tree_y() + 1;
	} else if (this->tech_tree_parent_upgrade_class != nullptr) {
		return this->tech_tree_parent_upgrade_class->get_tech_tree_y() + 1;
	}

	return 0;
}

int unit_class::get_tech_tree_width() const
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

void unit_class::map_to_0_ad_template_name(const std::string &str)
{
	if (terrain_type::try_get_by_0_ad_template_name(str) != nullptr) {
		throw std::runtime_error("0 A.D. template name \"" + str + "\" is already used by a terrain type.");
	}

	if (unit_class::try_get_by_0_ad_template_name(str) != nullptr) {
		throw std::runtime_error("0 A.D. template name \"" + str + "\" is already used by another unit class.");
	}

	if (unit_type::try_get_by_0_ad_template_name(str) != nullptr) {
		throw std::runtime_error("0 A.D. template name \"" + str + "\" is already used by a unit type.");
	}

	unit_class::unit_classes_by_0_ad_template_name[str] = this;
}

}
