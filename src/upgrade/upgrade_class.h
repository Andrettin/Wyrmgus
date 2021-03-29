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
//      (c) Copyright 2020-2021 by Andrettin
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

class CUpgrade;

namespace wyrmgus {

class age;
class condition;
class upgrade_category;
enum class upgrade_category_rank;

class upgrade_class final : public named_data_entry, public data_type<upgrade_class>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::upgrade_category* category MEMBER category)
	Q_PROPERTY(wyrmgus::age* age MEMBER age)

public:
	static constexpr const char *class_identifier = "upgrade_class";
	static constexpr const char *database_folder = "upgrade_classes";

	static upgrade_class *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		upgrade_class *upgrade_class = data_type::add(identifier, data_module);
		upgrade_class->index = upgrade_class::get_all().size() - 1;
		return upgrade_class;
	}

	explicit upgrade_class(const std::string &identifier);
	~upgrade_class();

	virtual void process_sml_scope(const sml_data &scope) override;
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

	const age *get_age() const
	{
		return this->age;
	}

	const std::unique_ptr<condition> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<condition> &get_conditions() const
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

private:
	int index = -1;
	upgrade_category *category = nullptr;
	age *age = nullptr;
	std::unique_ptr<condition> preconditions;
	std::unique_ptr<condition> conditions;
	std::vector<CUpgrade *> upgrades;
};

}

Q_DECLARE_METATYPE(std::vector<const wyrmgus::upgrade_class *>)
