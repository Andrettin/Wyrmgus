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
/**@name unit_type_dependency.cpp - The unit type dependency source file */
//
//      (c) Copyright 2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "dependency/unit_type_dependency.h"

#include "player.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CUnitTypeDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "unit_type") {
		value = FindAndReplaceString(value, "_", "-");
		this->UnitType = CUnitType::Get(value);
		if (!this->UnitType) {
			fprintf(stderr, "Invalid unit type: \"%s\".\n", value.c_str());
		}
	} else if (key == "count") {
		this->Count = std::stoi(value);
	} else {
		fprintf(stderr, "Invalid unit type dependency property: \"%s\".\n", key.c_str());
	}
}

bool CUnitTypeDependency::Check(const CPlayer *player, const bool ignore_units) const
{
	if (ignore_units) {
		return true;
	}
	
	return player->GetUnitTypeCount(this->UnitType) >= this->Count;
}

std::string CUnitTypeDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->UnitType->GetName().utf8().get_data();
	
	if (this->Count > 1) {
		str += '(' + std::to_string(this->Count) + ')';
	}
	
	str += '\n';
	
	return str;
}
