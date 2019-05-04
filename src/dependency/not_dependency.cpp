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
/**@name not_dependency.cpp - The not-dependency source file */
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

#include "dependency/not_dependency.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CNotDependency::~CNotDependency()
{
	for (const CDependency *dependency : this->Dependencies) {
		delete dependency;
	}
}

void CNotDependency::ProcessConfigDataSection(const CConfigData *section)
{
	const CDependency *dependency = CDependency::FromConfigData(section);
	this->Dependencies.push_back(dependency);
}

bool CNotDependency::CheckInternal(const CPlayer *player, const bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (dependency->Check(player, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

bool CNotDependency::Check(const CUnit *unit, const bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (dependency->Check(unit, ignore_units)) {
			return false;
		}
	}
	
	return true;
}
