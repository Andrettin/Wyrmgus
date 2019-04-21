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
/**@name dependency.h - The dependency header file. */
//
//      (c) Copyright 2000-2019 by Vladi Belperchinov-Shabanski and Andrettin
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

#ifndef __DEPENDENCY_H__
#define __DEPENDENCY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CPlayer;
class CUnitType;
class CUnit;
class CUpgrade;
class ButtonAction;

/// Dependency rule
class CDependency
{
public:
	void ProcessConfigData(const CConfigData *config_data);
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property);
	virtual void ProcessConfigDataSection(const CConfigData *section);
	virtual bool Check(const CPlayer *player, const bool ignore_units = false) const = 0;
	virtual bool Check(const CUnit *unit, const bool ignore_units = false) const;
	virtual std::string GetString(const std::string &prefix = "") const = 0; //get the dependency as a string
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Register CCL features for dependencies
extern void DependenciesCclRegister();

/// Print all unit dependencies into string
extern std::string PrintDependencies(const CPlayer &player, const ButtonAction &button);
extern void AddDependency(const int rule_type, const std::string &target, const int required_rule_type, const std::string &required, const int count, const int or_flag, const bool is_predependency);

/// Check dependencies for player
extern bool CheckDependencies(const CUnitType *target, const CPlayer *player, const bool ignore_units = false, const bool is_predependency = false, const bool is_neutral_use = false);
extern bool CheckDependencies(const CUpgrade *target, const CPlayer *player, const bool ignore_units = false, const bool is_predependency = false, const bool is_neutral_use = false);

template <typename T>
extern bool CheckDependencies(const T *target, const CPlayer *player, const bool ignore_units = false, const bool is_predependency = false, const bool is_neutral_use = false)
{
	if (!is_predependency && !CheckDependencies(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(player, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(player, ignore_units);
	}
}

/// Check dependencies for unit
extern bool CheckDependencies(const CUnitType *target, const CUnit *unit, const bool ignore_units = false, const bool is_predependency = false);
extern bool CheckDependencies(const CUpgrade *target, const CUnit *unit, const bool ignore_units = false, const bool is_predependency = false);

template <typename T>
extern bool CheckDependencies(const T *target, const CUnit *unit, const bool ignore_units = false, const bool is_predependency = false)
{
	if (!is_predependency && !CheckDependencies(target, unit, ignore_units, true)) {
		return false;
	}
	
	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(unit, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(unit, ignore_units);
	}
}

#endif
