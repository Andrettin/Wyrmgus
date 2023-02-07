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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "unit/unit_class_container.h"

class CPlayer;

namespace archimedes {
	class name_generator;
}

namespace wyrmgus {

class unit_type;
class upgrade_class;

template <typename scope_type>
class condition;

class unit_class final : public named_data_entry, public data_type<unit_class>
{
	Q_OBJECT

	Q_PROPERTY(bool town_hall MEMBER town_hall READ is_town_hall)
	Q_PROPERTY(bool ship MEMBER ship READ is_ship)
	Q_PROPERTY(std::string requirements_string MEMBER requirements_string)
	Q_PROPERTY(wyrmgus::unit_class* tech_tree_parent_unit_class MEMBER tech_tree_parent_unit_class)
	Q_PROPERTY(wyrmgus::upgrade_class* tech_tree_parent_upgrade_class MEMBER tech_tree_parent_upgrade_class)
	Q_PROPERTY(wyrmgus::data_entry* tech_tree_parent READ get_tech_tree_parent CONSTANT)
	Q_PROPERTY(int tech_tree_x READ get_tech_tree_x CONSTANT)
	Q_PROPERTY(int tech_tree_y READ get_tech_tree_y CONSTANT)
	Q_PROPERTY(int tech_tree_width READ get_tech_tree_width CONSTANT)

public:
	static constexpr const char class_identifier[] = "unit_class";
	static constexpr const char property_class_identifier[] = "wyrmgus::unit_class*";
	static constexpr const char database_folder[] = "unit_classes";

	static unit_class *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		unit_class *unit_class = data_type::add(identifier, data_module);
		unit_class->index = unit_class::get_all().size() - 1;
		return unit_class;
	}

	static const unit_class *try_get_by_0_ad_template_name(const std::string &str)
	{
		const auto find_iterator = unit_class::unit_classes_by_0_ad_template_name.find(str);
		if (find_iterator != unit_class::unit_classes_by_0_ad_template_name.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static const std::vector<unit_class *> &get_town_hall_classes()
	{
		return unit_class::town_hall_classes;
	}

	static void clear()
	{
		data_type::clear();

		unit_class::unit_classes_by_0_ad_template_name.clear();
	}

	static void propagate_unit_class_names(const unit_class_map<std::unique_ptr<name_generator>> &unit_class_name_generators, std::unique_ptr<name_generator> &ship_name_generator);

private:
	static inline std::map<std::string, const unit_class *> unit_classes_by_0_ad_template_name;
	static inline std::vector<unit_class *> town_hall_classes;

public:
	explicit unit_class(const std::string &identifier);
	~unit_class();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	int get_index() const
	{
		return this->index;
	}

	bool is_town_hall() const
	{
		return this->town_hall;
	}

	void set_town_hall(const bool town_hall);

	bool is_ship() const
	{
		return this->ship;
	}

	const std::unique_ptr<condition<CPlayer>> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition<CPlayer>> &get_conditions() const
	{
		return this->conditions;
	}

	const std::string &get_requirements_string() const
	{
		return this->requirements_string;
	}

	const std::vector<unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}

	bool has_unit_type(unit_type *unit_type) const;

	void add_unit_type(unit_type *unit_type)
	{
		this->unit_types.push_back(unit_type);
	}

	void remove_unit_type(unit_type *unit_type);

	const unit_class *get_tech_tree_parent_unit_class() const
	{
		return this->tech_tree_parent_unit_class;
	}

	const upgrade_class *get_tech_tree_parent_upgrade_class() const
	{
		return this->tech_tree_parent_upgrade_class;
	}

	data_entry *get_tech_tree_parent() const;

	Q_INVOKABLE bool has_tech_tree_children() const
	{
		return !this->get_tech_tree_child_unit_classes().empty() || !this->get_tech_tree_child_upgrade_classes().empty();
	}

	bool is_on_tech_tree() const
	{
		//only entries which either have a tech tree parent or tech tree children are displayed on the tech tree
		return this->get_tech_tree_parent() != nullptr || this->has_tech_tree_children();;
	}

	const std::vector<const unit_class *> &get_tech_tree_child_unit_classes() const
	{
		return this->tech_tree_child_unit_classes;
	}

	void add_tech_tree_child_unit_class(const unit_class *unit_class)
	{
		this->tech_tree_child_unit_classes.push_back(unit_class);
	}

	const std::vector<const upgrade_class *> &get_tech_tree_child_upgrade_classes() const
	{
		return this->tech_tree_child_upgrade_classes;
	}

	void add_tech_tree_child_upgrade_class(const upgrade_class *upgrade_class)
	{
		this->tech_tree_child_upgrade_classes.push_back(upgrade_class);
	}

	int get_tech_tree_x() const;
	int get_tech_tree_relative_x() const;
	int get_tech_tree_y() const;
	int get_tech_tree_width() const;

	void map_to_0_ad_template_name(const std::string &str);

private:
	int index = -1;
	bool town_hall = false; //whether the building class is a settlement head building class, e.g. a town hall or fortress
	bool ship = false; //whether the unit class is a ship
	std::unique_ptr<condition<CPlayer>> preconditions;
	std::unique_ptr<condition<CPlayer>> conditions;
	std::string requirements_string;
	std::vector<unit_type *> unit_types;
	unit_class *tech_tree_parent_unit_class = nullptr;
	upgrade_class *tech_tree_parent_upgrade_class = nullptr;
	std::vector<const unit_class *> tech_tree_child_unit_classes;
	std::vector<const upgrade_class *> tech_tree_child_upgrade_classes;
};

}
