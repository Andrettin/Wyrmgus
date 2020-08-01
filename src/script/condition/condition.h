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
//      (c) Copyright 2000-2020 by Vladi Belperchinov-Shabanski and Andrettin
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

class CConfigData;
class CPlayer;
class CUnit;
class CUpgrade;

namespace stratagus {

class button;
class season;
class site;
class sml_data;
class sml_property;
class trigger;
class unit_type;

class condition
{
public:
	static std::unique_ptr<const condition> from_sml_property(const sml_property &property);
	static std::unique_ptr<const condition> from_sml_scope(const sml_data &scope);

	virtual ~condition() {}

	void ProcessConfigData(const CConfigData *config_data);
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property);
	virtual void ProcessConfigDataSection(const CConfigData *section);
	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);
	virtual bool check(const CPlayer *player, bool ignore_units = false) const = 0;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const;

	//get the condition as a string
	virtual std::string get_string(const std::string &prefix = "") const = 0;
};

}

/// Register CCL features for dependencies
extern void DependenciesCclRegister();

/// Print all unit conditions into string
extern std::string PrintConditions(const CPlayer &player, const stratagus::button &button);

/// Check conditions for player
extern bool CheckConditions(const stratagus::unit_type *target, const CPlayer *player, bool ignore_units = false, bool is_precondition = false, bool is_neutral_use = false);
extern bool CheckConditions(const CUpgrade *target, const CPlayer *player, bool ignore_units = false, bool is_precondition = false, bool is_neutral_use = false);

template <typename T>
extern bool CheckConditions(const T *target, const CPlayer *player, bool ignore_units = false, bool is_precondition = false, bool is_neutral_use = false)
{
	if (!is_precondition && !CheckConditions(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (is_precondition) {
		return target->get_preconditions() == nullptr || target->get_preconditions()->check(player, ignore_units);
	} else {
		return target->get_conditions() == nullptr || target->get_conditions()->check(player, ignore_units);
	}
}

/// Check conditions for unit
extern bool CheckConditions(const stratagus::unit_type *target, const CUnit *unit, bool ignore_units = false, bool is_precondition = false);
extern bool CheckConditions(const CUpgrade *target, const CUnit *unit, bool ignore_units = false, bool is_precondition = false);

template <typename T>
extern bool CheckConditions(const T *target, const CUnit *unit, bool ignore_units = false, bool is_precondition = false)
{
	if (!is_precondition && !CheckConditions(target, unit, ignore_units, true)) {
		return false;
	}
	
	if (is_precondition) {
		return target->get_preconditions() == nullptr || target->get_preconditions()->Check(unit, ignore_units);
	} else {
		return target->get_conditions() == nullptr || target->get_conditions()->Check(unit, ignore_units);
	}
}
