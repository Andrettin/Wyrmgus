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
//      (c) Copyright 2020 by Andrettin
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
//

#pragma once

#include "civilization_base.h"
#include "database/data_type.h"

namespace wyrmgus {

class civilization_supergroup;

class civilization_group final : public civilization_base, public data_type<civilization_group>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::civilization_supergroup* supergroup MEMBER supergroup READ get_supergroup)

public:
	static constexpr const char *class_identifier = "civilization_group";
	static constexpr const char *database_folder = "civilization_groups";

	civilization_group(const std::string &identifier) : civilization_base(identifier) {}

	virtual void initialize() override;

	civilization_supergroup *get_supergroup() const
	{
		return this->supergroup;
	}

private:
	civilization_supergroup *supergroup = nullptr;
};

}
