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

#include "script/condition/condition.h"
#include "time/season.h"
#include "unit/unit.h"

namespace wyrmgus {

class season;

class season_condition final : public condition
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;

	virtual bool check(const CPlayer *player, const bool ignore_units) const override
	{
		return CMap::Map.MapLayers[player->StartMapLayer]->GetSeason() == this->Season;
	}

	virtual bool check(const CUnit *unit, const bool ignore_units) const override
	{
		return unit->MapLayer->GetSeason() == this->Season;
	}

	virtual std::string get_string(const std::string &prefix = "") const override
	{
		std::string str = prefix + this->Season->get_name() + '\n';
		return str;
	}

private:
	const season *Season = nullptr;
};

}
