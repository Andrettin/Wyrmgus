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
/**@name unit_type_dependency.h - The unit type dependency header file. */
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

#ifndef __UNIT_TYPE_DEPENDENCY_H__
#define __UNIT_TYPE_DEPENDENCY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "dependency/dependency.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CPlayer;
class CUnitType;
class UnitClass;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CUnitTypeDependency : public CDependency
{
public:
	CUnitTypeDependency() {}
	CUnitTypeDependency(const CUnitType *unit_type, const int count) : UnitType(unit_type), Count(count) {}
	
	virtual void ProcessConfigDataProperty(const std::pair<String, String> &property) override;
	virtual void Initialize() override;
private:
	virtual bool CheckInternal(const CPlayer *player, const bool ignore_units = false) const override;
public:
	virtual std::string GetString(const std::string &prefix = "") const override;

private:
	const CUnitType *UnitType = nullptr;
	const UnitClass *UnitClass = nullptr;
	int Count = 1;		/// How many of the unit type are required
};

#endif
