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

namespace wyrmgus {

class language_family final : public named_data_entry, public data_type<language_family>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::language_family* family MEMBER family READ get_family)

public:
	static constexpr const char *class_identifier = "language_family";
	static constexpr const char property_class_identifier[] = "wyrmgus::language_family*";
	static constexpr const char *database_folder = "language_families";

	explicit language_family(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	language_family *get_family() const
	{
		return this->family;
	}

private:
	language_family *family = nullptr; //the upper family to which this language family belongs
};

}
