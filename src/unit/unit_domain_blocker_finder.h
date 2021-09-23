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
//      (c) Copyright 2021 by Andrettin
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

#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_type.h"

namespace wyrmgus {

class unit_domain_blocker_finder final
{
public:
	explicit unit_domain_blocker_finder(const unit_domain domain) : domain(domain)
	{
	}

	bool operator()(const CUnit *const unit) const
	{
		if (unit == nullptr) {
			throw std::runtime_error("unit_domain_blocker_finder error: unit is null.");
		}

		if (unit->Type == nullptr) {
			throw std::runtime_error("unit_domain_blocker_finder error: unit's type is null.");
		}

		const unit_type &type = *unit->Type;

		if (type.BoolFlag[VANISHES_INDEX].value) {
			return false;
		}

		switch (this->domain) {
			case unit_domain::none:
				return true;
			case unit_domain::land:
			case unit_domain::water:
			case unit_domain::air_low:
				switch (type.get_domain()) {
					case unit_domain::land:
					case unit_domain::water:
					case unit_domain::air_low:
						return true;
					default:
						return false;
				}
			case unit_domain::air:
			case unit_domain::space:
				switch (type.get_domain()) {
					case unit_domain::air:
					case unit_domain::space:
						return true;
					default:
						return false;
				}
			default:
				return type.get_domain() == this->domain;
		}
	}

private:
	const unit_domain domain;
};

}
