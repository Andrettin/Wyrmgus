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

#include "database/data_type.h"
#include "database/named_data_entry.h"

class CPlayer;
class CUpgrade;

namespace wyrmgus {

class age;
class unit_class;
class upgrade_category;
enum class upgrade_category_rank;

template <typename scope_type>
class condition;

class upgrade_class final : public named_data_entry, public data_type<upgrade_class>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::upgrade_category* category MEMBER category WRITE set_category)
	Q_PROPERTY(wyrmgus::age* age MEMBER age)
	Q_PROPERTY(wyrmgus::unit_class* tech_tree_parent_unit_class MEMBER tech_tree_parent_unit_class)
	Q_PROPERTY(wyrmgus::upgrade_class* tech_tree_parent_upgrade_class MEMBER tech_tree_parent_upgrade_class)
	Q_PROPERTY(wyrmgus::data_entry* tech_tree_parent READ get_tech_tree_parent CONSTANT)
	Q_PROPERTY(int tech_tree_x READ get_tech_tree_x CONSTANT)
	Q_PROPERTY(int tech_tree_y READ get_tech_tree_y CONSTANT)
	Q_PROPERTY(int tech_tree_width READ get_tech_tree_width CONSTANT)

public:
	static constexpr const char *class_identifier = "upgrade_class";
	static constexpr const char property_class_identifier[] = "wyrmgus::upgrade_class*";
	static constexpr const char *database_folder = "upgrade_classes";

	static upgrade_class *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		upgrade_class *upgrade_class = data_type::add(identifier, data_module);
		upgrade_class->index = upgrade_class::get_all().size() - 1;
		return upgrade_class;
	}

	explicit upgrade_class(const std::string &identifier);
	~upgrade_class();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	int get_index() const
	{
		return this->index;
	}

	const upgrade_category *get_category() const
	{
		return this->category;
	}

	const upgrade_category *get_category(const upgrade_category_rank rank) const;
	void set_category(upgrade_category *category);

	const wyrmgus::age *get_age() const
	{
		return this->age;
	}

	const std::unique_ptr<condition<CPlayer>> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition<CPlayer>> &get_conditions() const
	{
		return this->conditions;
	}

	const std::vector<CUpgrade *> &get_upgrades() const
	{
		return this->upgrades;
	}

	bool has_upgrade(CUpgrade *unit_type) const;

	void add_upgrade(CUpgrade *unit_type)
	{
		this->upgrades.push_back(unit_type);
	}

	void remove_upgrade(CUpgrade *unit_type);

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
		return this->get_tech_tree_parent() != nullptr || this->has_tech_tree_children();
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

private:
	int index = -1;
	upgrade_category *category = nullptr;
	wyrmgus::age *age = nullptr;
	std::unique_ptr<condition<CPlayer>> preconditions;
	std::unique_ptr<condition<CPlayer>> conditions;
	std::vector<CUpgrade *> upgrades;
	unit_class *tech_tree_parent_unit_class = nullptr;
	upgrade_class *tech_tree_parent_upgrade_class = nullptr;
	std::vector<const unit_class *> tech_tree_child_unit_classes;
	std::vector<const upgrade_class *> tech_tree_child_upgrade_classes;
};

}

Q_DECLARE_METATYPE(std::vector<const wyrmgus::upgrade_class *>)
