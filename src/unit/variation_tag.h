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

class variation_tag final : public named_data_entry, public data_type<variation_tag>
{
	Q_OBJECT

	Q_PROPERTY(bool hair_color MEMBER hair_color READ is_hair_color)

public:
	static constexpr const char *class_identifier = "variation_tag";
	static constexpr const char property_class_identifier[] = "wyrmgus::variation_tag*";
	static constexpr const char *database_folder = "variation_tags";

	explicit variation_tag(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	bool is_hair_color() const
	{
		return this->hair_color;
	}

private:
	bool hair_color = false; //whether this is a hair color variation tag
};

}
