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
/**@name resource_dependency.cpp - The resource dependency source file */
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

#include "dependency/resource_dependency.h"

#include "economy/resource.h"
#include "player.h"
#include "unit/unit.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CResourceDependency::ProcessConfigDataProperty(const std::pair<String, String> &property)
{
	const String &key = property.first;
	String value = property.second;
	if (key == "resource") {
		this->Resource = CResource::Get(value);
	} else {
		fprintf(stderr, "Invalid resource dependency property: \"%s\".\n", key.utf8().get_data());
	}
}

bool CResourceDependency::CheckInternal(const CPlayer *player, const bool ignore_units) const
{
	//always return true for players, since at least for now this dependency is intended only for units
	return true;
}

bool CResourceDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	if (unit->GetCurrentResource() == this->Resource->GetIndex() || unit->GivesResource == this->Resource->GetIndex()) {
		if (unit->GetResourcesHeld() > 0) {
			return true;
		}
	}
	
	return false;
}

std::string CResourceDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Resource->GetName().utf8().get_data() + '\n';
	return str;
}
