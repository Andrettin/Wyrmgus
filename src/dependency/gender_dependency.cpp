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
/**@name gender_dependency.cpp - The gender dependency source file */
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

#include "dependency/gender_dependency.h"

#include "character.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "player.h"
#include "species/gender.h"
#include "unit/unit.h"
#include "util.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CGenderDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "gender") {
		this->Gender = CGender::Get(value);
	} else {
		fprintf(stderr, "Invalid gender dependency property: \"%s\".\n", key.c_str());
	}
}

bool CGenderDependency::CheckInternal(const CPlayer *player, const bool ignore_units) const
{
	return true;
}

bool CGenderDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	return unit->Variable[GENDER_INDEX].Value == (this->Gender->GetIndex() + 1);
}

std::string CGenderDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Gender->GetIdent().utf8().get_data() + '\n';
	return str;
}
