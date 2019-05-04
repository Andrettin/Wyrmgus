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
/**@name site_dependency.cpp - The site dependency source file */
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

#include "dependency/site_dependency.h"

#include "faction.h"
#include "map/site.h"
#include "player.h"
#include "unit/unit.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CSiteDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "site") {
		this->Site = CSite::Get(value);
	} else if (key == "faction") {
		this->Faction = CFaction::Get(value);
	} else if (key == "enemy") {
		this->Enemy = StringToBool(value);
	} else {
		fprintf(stderr, "Invalid site dependency property: \"%s\".\n", key.c_str());
	}
}

bool CSiteDependency::CheckInternal(const CPlayer *player, const bool ignore_units) const
{
	if (this->Faction != nullptr) {
		CPlayer *faction_player = GetFactionPlayer(this->Faction);
		
		if (faction_player == nullptr) { //no player belongs to that faction, so naturally the site can't be owned by it
			return false;
		}
		
		if (!faction_player->HasSettlement(this->Site)) {
			return false;
		}
		
		if (this->Enemy && !player->IsEnemy(*faction_player)) {
			return false;
		}
		
		return true;
	} else if (this->Enemy) {
		for (const CPlayer *p : CPlayer::Players) {
			if (p->Type == PlayerNobody || p->Index == PlayerNumNeutral) {
				continue;
			}
			
			if (p->HasSettlement(this->Site) && player->IsEnemy(*p)) {
				return true;
			}
		}
		
		return false;
	}
	
	return player->HasSettlement(this->Site);
}

bool CSiteDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	if (this->Faction != nullptr || this->Enemy) {
		return this->CheckInternal(unit->Player, ignore_units);
	}
	
	return unit->Settlement == this->Site;
}

std::string CSiteDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Site->GetName().utf8().get_data() + '\n';
	return str;
}
