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
/**@name trigger_dependency.cpp - The trigger dependency source file */
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

#include "dependency/trigger_dependency.h"

#include "player.h"
#include "trigger/trigger.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CTriggerDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "trigger") {
		this->Trigger = CTrigger::GetTrigger(value);
	} else {
		fprintf(stderr, "Invalid trigger dependency property: \"%s\".\n", key.c_str());
	}
}

bool CTriggerDependency::CheckInternal(const CPlayer *player, const bool ignore_units) const
{
	//checks whether a trigger has already fired
	
	return std::find(CTrigger::DeactivatedTriggers.begin(), CTrigger::DeactivatedTriggers.end(), this->Trigger->Ident) != CTrigger::DeactivatedTriggers.end(); //this works fine for global triggers, but for player triggers perhaps it should check only the player?
}

std::string CTriggerDependency::GetString(const std::string &prefix) const
{
	return std::string();
}
