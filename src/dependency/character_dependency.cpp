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
/**@name character_dependency.cpp - The character dependency source file */
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

#include "dependency/character_dependency.h"

#include "character.h"
#include "faction.h"
#include "player.h"
#include "unit/unit.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CCharacterDependency::ProcessConfigDataProperty(const std::pair<String, String> &property)
{
	const String &key = property.first;
	String value = property.second;
	if (key == "character") {
		this->Character = CCharacter::Get(value);
	} else if (key == "faction") {
		this->Faction = CFaction::Get(value);
	} else if (key == "enemy") {
		this->Enemy = StringToBool(value);
	} else {
		fprintf(stderr, "Invalid character dependency property: \"%s\".\n", key.utf8().get_data());
	}
}

bool CCharacterDependency::CheckInternal(const CPlayer *player, const bool ignore_units) const
{
	if (this->Faction != nullptr) {
		CPlayer *faction_player = CPlayer::GetFactionPlayer(this->Faction);
		
		if (faction_player == nullptr) { //no player belongs to that faction, so naturally the character can't be owned by it
			return false;
		}
		
		if (!faction_player->HasHero(this->Character)) {
			return false;
		}
		
		if (this->Enemy && !player->IsEnemy(*faction_player)) {
			return false;
		}
		
		return true;
	} else if (this->Enemy) {
		for (const CPlayer *p : CPlayer::Players) {
			if (p->Type == PlayerNobody || p->GetIndex() == PlayerNumNeutral) {
				continue;
			}
			
			if (p->HasHero(this->Character) && player->IsEnemy(*p)) {
				return true;
			}
		}
		
		return false;
	}
	
	return player->HasHero(this->Character);
}

bool CCharacterDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	if (this->Faction != nullptr || this->Enemy) {
		return this->CheckInternal(unit->GetPlayer(), ignore_units);
	}
	
	return unit->Character == this->Character;
}

std::string CCharacterDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Character->GetFullName().utf8().get_data() + '\n';
	return str;
}
