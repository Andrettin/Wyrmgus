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
/**@name and_dependency.cpp - The and-dependency source file */
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

#include "dependency/and_dependency.h"

#include "config.h"
#include "dependency/age_dependency.h"
#include "dependency/character_dependency.h"
#include "dependency/not_dependency.h"
#include "dependency/or_dependency.h"
#include "dependency/season_dependency.h"
#include "dependency/trigger_dependency.h"
#include "dependency/unit_type_dependency.h"
#include "dependency/upgrade_dependency.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CAndDependency::~CAndDependency()
{
	for (CDependency *dependency : this->Dependencies) {
		delete dependency;
	}
}

void CAndDependency::ProcessConfigDataSection(const CConfigData *section)
{
	CDependency *dependency = nullptr;
	if (section->Tag == "and") {
		dependency = new CAndDependency;
	} else if (section->Tag == "or") {
		dependency = new COrDependency;
	} else if (section->Tag == "not") {
		dependency = new CNotDependency;
	} else if (section->Tag == "unit_type") {
		dependency = new CUnitTypeDependency;
	} else if (section->Tag == "upgrade") {
		dependency = new CUpgradeDependency;
	} else if (section->Tag == "age") {
		dependency = new CAgeDependency;
	} else if (section->Tag == "character") {
		dependency = new CCharacterDependency;
	} else if (section->Tag == "season") {
		dependency = new CSeasonDependency;
	} else if (section->Tag == "trigger") {
		dependency = new CTriggerDependency;
	} else {
		fprintf(stderr, "Invalid and dependency property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	dependency->ProcessConfigData(section);
	this->Dependencies.push_back(dependency);
}

bool CAndDependency::Check(const CPlayer *player, const bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (!dependency->Check(player, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

bool CAndDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (!dependency->Check(unit, ignore_units)) {
			return false;
		}
	}
	
	return true;
}
