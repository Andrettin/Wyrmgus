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
/**@name season_dependency.cpp - The season dependency source file */
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

#include "dependency/season_dependency.h"

#include "map/map.h"
#include "map/map_layer.h"
#include "player.h"
#include "time/season.h"
#include "unit/unit.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CSeasonDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "season") {
		value = FindAndReplaceString(value, "_", "-");
		this->Season = CSeason::Get(value);
	} else {
		fprintf(stderr, "Invalid season dependency property: \"%s\".\n", key.c_str());
	}
}

bool CSeasonDependency::Check(const CPlayer *player, const bool ignore_units) const
{
	return CMap::Map.MapLayers[player->StartMapLayer]->GetSeason() == this->Season;
}

bool CSeasonDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	return unit->MapLayer->GetSeason() == this->Season;
}

std::string CSeasonDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Season->GetName().utf8().get_data() + '\n';
	return str;
}
