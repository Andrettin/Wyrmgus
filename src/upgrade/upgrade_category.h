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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace wyrmgus {

class age;
class upgrade_class;
enum class upgrade_category_rank;

class upgrade_category final : public named_data_entry, public data_type<upgrade_category>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::upgrade_category_rank rank MEMBER rank READ get_rank)
	Q_PROPERTY(wyrmgus::upgrade_category* category MEMBER category WRITE set_category)

public:
	static constexpr const char *class_identifier = "upgrade_category";
	static constexpr const char property_class_identifier[] = "wyrmgus::upgrade_category*";
	static constexpr const char *database_folder = "upgrade_categories";

	explicit upgrade_category(const std::string &identifier);

	virtual void initialize() override;
	virtual void check() const override;

	upgrade_category_rank get_rank() const
	{
		return this->rank;
	}

	const upgrade_category *get_category() const
	{
		return this->category;
	}

	const upgrade_category *get_category(const upgrade_category_rank rank) const
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

	void set_category(upgrade_category *category);

	const age *get_start_age() const
	{
		return this->start_age;
	}

	void add_subcategory(upgrade_category *subcategory)
	{
		this->subcategories.push_back(subcategory);
	}

	void remove_subcategory(upgrade_category *subcategory);

	void add_upgrade_class(const upgrade_class *upgrade_class)
	{
		this->upgrade_classes.push_back(upgrade_class);
	}

	void remove_upgrade_class(const upgrade_class *upgrade_class);

private:
	upgrade_category_rank rank;
	upgrade_category *category = nullptr; //the upper category to which this upgrade category belongs
	const age *start_age = nullptr; //the starting age for this category
	std::vector<upgrade_category *> subcategories;
	std::vector<const upgrade_class *> upgrade_classes; //the upgrade classes which belong to this category
};

}
