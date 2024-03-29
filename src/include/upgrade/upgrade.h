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
/**@name upgrade.h - The upgrade header file. */
//
//      (c) Copyright 1999-2022 by Vladi Belperchinov-Shabanski, Jimmy Salmon
//      and Andrettin
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

#pragma once

class CFile;
class CPlayer;
class CUpgrade;
class CUnit;

namespace wyrmgus {
	class unit_type;
	class upgrade_modifier;
}

/// init upgrade/allow structures
extern void InitUpgrades();
/// save the upgrades
extern void SaveUpgrades(CFile &file);
/// cleanup upgrade module
extern void CleanUpgradeModifiers();

/// Register CCL features for upgrades
extern void UpgradesCclRegister();

/*----------------------------------------------------------------------------
--  General/Map functions
----------------------------------------------------------------------------*/

// AllowStruct and UpgradeTimers will be static in the player so will be
// load/saved with the player struct

extern int UnitTypeIdByIdent(const std::string &sid);
extern int UpgradeIdByIdent(const std::string &sid);

/*----------------------------------------------------------------------------
--  Upgrades
----------------------------------------------------------------------------*/

/// Apply researched upgrades when map is loading
extern void ApplyUpgrades();

extern void ConvertUnitTypeTo(CPlayer &player, const unit_type &src, const unit_type &dst);

extern void ApplyIndividualUpgradeModifier(CUnit &unit, const wyrmgus::upgrade_modifier *um); /// Apply upgrade modifier of an individual upgrade
//Wyrmgus start
extern void RemoveIndividualUpgradeModifier(CUnit &unit, const wyrmgus::upgrade_modifier *um);
extern void AbilityAcquire(CUnit &unit, const CUpgrade *upgrade, bool save = true);
extern void AbilityLost(CUnit &unit, CUpgrade *upgrade, bool lose_all = false);
//Wyrmgus end
extern void IndividualUpgradeAcquire(CUnit &unit, const CUpgrade *upgrade); /// Make a unit acquire in individual upgrade
extern void IndividualUpgradeLost(CUnit &unit, const CUpgrade *upgrade, bool lose_all = false); /// Make a unit lose in individual upgrade

/*----------------------------------------------------------------------------
--  Allow(s)
----------------------------------------------------------------------------*/

// all the following functions are just map handlers, no specific notes
// id -- unit type id, af -- `A'llow/`F'orbid

extern int UnitIdAllowed(const CPlayer &player, int id);
//Wyrmgus start
extern void AllowUnitId(CPlayer &player, int id, int units);
//Wyrmgus end
extern void AllowUpgradeId(CPlayer &player, int id, char af);

extern char UpgradeIdAllowed(const CPlayer &player, int id);
extern char UpgradeIdentAllowed(const CPlayer &player, const std::string &ident);

//Wyrmgus start
extern std::string GetUpgradeEffectsString(const CUpgrade *upgrade, const bool multiline = false, const size_t indent = 0);
extern bool IsPercentageVariable(const int var);
extern bool IsBonusVariable(const int var);
extern bool IsBooleanVariable(const int var);
extern bool IsKnowledgeVariable(const int var);
extern bool is_potentially_negative_variable(const int var);
extern std::string GetVariableDisplayName(int var, bool increase = false);
//Wyrmgus end
