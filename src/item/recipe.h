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

#include "database/data_entry.h"
#include "database/data_type.h"

namespace wyrmgus {

class unit_type;

class recipe final : public data_entry, public data_type<recipe>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::unit_type* result_unit_type MEMBER result_unit_type)

public:
	static constexpr const char *class_identifier = "recipe";
	static constexpr const char property_class_identifier[] = "wyrmgus::recipe*";
	static constexpr const char *database_folder = "recipes";

	explicit recipe(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	virtual void check() const override
	{
		if (this->get_result_unit_type() == nullptr) {
			throw std::runtime_error("Recipe \"" + this->get_identifier() + "\" has no result unit type.");
		}

		if (this->get_ingredients().empty()) {
			throw std::runtime_error("Recipe \"" + this->get_identifier() + "\" has no ingredients.");
		}
	}

	const unit_type *get_result_unit_type() const
	{
		return this->result_unit_type;
	}

	const std::vector<const unit_type *> &get_ingredients() const
	{
		return this->ingredients;
	}

private:
	unit_type *result_unit_type = nullptr;
	std::vector<const unit_type *> ingredients;
};

}
