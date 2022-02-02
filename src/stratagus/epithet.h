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

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace wyrmgus {

class condition;

class epithet final : public named_data_entry, public data_type<epithet>
{
	Q_OBJECT

	Q_PROPERTY(int weight MEMBER weight READ get_weight)

public:
	static constexpr const char *class_identifier = "epithet";
	static constexpr const char *database_folder = "epithets";

	explicit epithet(const std::string &identifier);
	virtual ~epithet() override;
	
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void check() const override;

	int get_weight() const
	{
		return this->weight;
	}

	void increment_weight()
	{
		++this->weight;
	}

	const std::unique_ptr<condition> &get_conditions() const
	{
		return this->conditions;
	}

private:
	int weight = 1;
	std::unique_ptr<condition> conditions;
};

}