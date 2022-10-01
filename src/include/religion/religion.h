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
//      (c) Copyright 2018-2022 by Andrettin
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
#include "database/detailed_data_entry.h"

namespace wyrmgus {

class magic_domain;

class religion final : public detailed_data_entry, public data_type<religion>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "religion";
	static constexpr const char property_class_identifier[] = "wyrmgus::religion*";
	static constexpr const char *database_folder = "religions";

	explicit religion(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	bool CulturalDeities = false; //whether the religion's deities (or equivalent) must belong to the civilization that has the religion; for instance: the deities under paganism must belong to the civilization of the player, but under hinduism they musn't (meaning that a Teuton player which has hinduism as a religion can select Hindu deities, but an Indian pagan cannot select Teuton pagan deities)
	std::vector<magic_domain *> Domains;
};

}
