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

#include "database/data_entry_history.h"

namespace wyrmgus {

class faction;
class historical_location;

class historical_unit_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(bool active MEMBER active READ is_active)
	Q_PROPERTY(wyrmgus::faction* faction MEMBER faction)

public:
	historical_unit_history();
	~historical_unit_history();

	void process_sml_property(const sml_property &property) override;
	void process_sml_scope(const sml_data &scope) override;

	bool is_active() const
	{
		return this->active;
	}

	const wyrmgus::faction *get_faction() const
	{
		return this->faction;
	}

	const historical_location *get_location() const
	{
		return this->location.get();
	}

private:
	bool active = false; //whether the unit is active, i.e. should be applied to the map
	wyrmgus::faction *faction = nullptr;
	std::unique_ptr<historical_location> location;
};

}
