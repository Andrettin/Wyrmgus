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
/**@name upgrade_dependency.cpp - The upgrade dependency source file */
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

#include "dependency/upgrade_dependency.h"

#include "faction.h"
#include "player.h"
#include "unit/unit.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CUpgradeDependency::ProcessConfigDataProperty(const std::pair<String, String> &property)
{
	const String &key = property.first;
	String value = property.second;
	if (key == "upgrade") {
		value = value.replace("_", "-");
		this->Upgrade = CUpgrade::Get(value.utf8().get_data());
		if (!this->Upgrade) {
			print_error("Invalid upgrade: \"" + value + "\".");
		}
	} else if (key == "upgrade_class") {
		this->UpgradeClass = UpgradeClass::Get(value);
	} else {
		print_error("Invalid upgrade dependency property: \"" + key + "\".");
	}
}

void CUpgradeDependency::Initialize()
{
	if (this->Upgrade == nullptr && this->UpgradeClass == nullptr) {
		print_error("Upgrade dependency has neither a unit type nor a unit class.");
	}
}

bool CUpgradeDependency::CheckInternal(const CPlayer *player, const bool ignore_units) const
{
	const CUpgrade *upgrade = this->Upgrade;
	
	if (upgrade == nullptr) {
		upgrade = CFaction::GetFactionClassUpgrade(player->GetFaction(), this->UpgradeClass);
	}
	
	if (upgrade == nullptr) {
		return false;
	}
	
	return UpgradeIdAllowed(*player, upgrade->GetIndex()) == 'R';
}

bool CUpgradeDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	return this->CheckInternal(unit->GetPlayer(), ignore_units) || unit->GetIndividualUpgrade(this->Upgrade);
}

std::string CUpgradeDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix;
	
	if (this->Upgrade != nullptr) {
		str += this->Upgrade->GetName().utf8().get_data();
	} else if (this->UpgradeClass != nullptr) {
		str += this->UpgradeClass->GetName().utf8().get_data();
	}

	str += '\n';
	return str;
}
