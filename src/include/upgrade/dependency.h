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
/**@name dependency.h - The dependencies header file. */
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

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @struct DependRule dependency.h
**
**  \#include "upgrade/dependency.h"
**
**  This structure is used define the requirements of upgrades or
**  unit-types. The structure is used to define the base (the wanted)
**  upgrade or unit-type and the requirements upgrades or unit-types.
**  The requirements could be combination of and-rules and or-rules.
**
**  This structure is very complex because nearly everything has two
**  meanings.
**
**  The depend-rule structure members:
**
**  DependRule::Next
**
**    Next rule in hash chain for the base upgrade/unit-type.
**    Next and-rule for the requirements.
**
**  DependRule::Count
**
**    If DependRule::Type is DependRuleUnitType, the counter is
**    how many units of the unit-type are required, if zero no unit
**    of this unit-type is allowed. if DependRule::Type is
**    DependRuleUpgrade, for a non-zero counter the upgrade must be
**    researched, for a zero counter the upgrade must be unresearched.
**
**  DependRule::Type
**
**    Type of the rule, DependRuleUnitType for an unit-type,
**    DependRuleUpgrade for an upgrade.
**
**  DependRule::Kind
**
**    Contains the element of rule. Depending on DependRule::Type.
**
**  DependRule::Kind::UnitType
**
**    An unit-type pointer.
**
**  DependRule::Kind::Upgrade
**
**    An upgrade pointer.
**
**  DependRule::Rule
**
**    For the base upgrade/unit-type the rules which must be meet.
**    For the requirements alternative or-rules.
**
*/

class CConfigData;
class CPlayer;
class CUnitType;
class CUnit;
class CUpgrade;
class ButtonAction;

namespace stratagus {

class age;
class character;
class faction;
class season;
class site;
class sml_data;
class sml_property;
class trigger;

/// Dependency rule
class dependency
{
public:
	static std::unique_ptr<const dependency> from_sml_scope(const sml_data &scope);

	virtual ~dependency() {}

	void ProcessConfigData(const CConfigData *config_data);
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property);
	virtual void ProcessConfigDataSection(const CConfigData *section);
	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);
	virtual bool check(const CPlayer *player, bool ignore_units = false) const = 0;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const;

	//get the dependency as a string
	virtual std::string get_string(const std::string &prefix = "") const = 0;
};

class and_dependency : public dependency
{
public:
	and_dependency() {}
	and_dependency(std::vector<std::unique_ptr<const dependency>> &&dependencies) : dependencies(std::move(dependencies)) {}

	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const override;
	
	virtual std::string get_string(const std::string &prefix = "") const override
	{
		int element_count = 0;
		
		for (const auto &dependency : this->dependencies) {
			if (!dependency->get_string(prefix + '\t').empty()) {
				element_count++;
			}
		}
		
		if (element_count >= 1) {
			std::string str;
			if (element_count > 1) {
				str += prefix + "AND:\n";
			}
		
			for (const auto &dependency : this->dependencies) {
				str += dependency->get_string((element_count > 1) ? prefix + '\t' : prefix);
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<std::unique_ptr<const dependency>> dependencies; //the dependencies of which all should be true
};

class or_dependency : public dependency
{
public:
	or_dependency() {}
	or_dependency(std::vector<std::unique_ptr<const dependency>> &&dependencies) : dependencies(std::move(dependencies))
	{
	}
	
	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const override;
	
	virtual std::string get_string(const std::string &prefix = "") const override
	{
		int element_count = 0;
		
		for (const auto &dependency : this->dependencies) {
			if (!dependency->get_string(prefix + '\t').empty()) {
				element_count++;
			}
		}
		
		if (element_count >= 1) {
			std::string str;
			if (element_count > 1) {
				str += prefix + "OR:\n";
			}
		
			for (const auto &dependency : this->dependencies) {
				str += dependency->get_string((element_count > 1) ? prefix + '\t' : prefix);
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<std::unique_ptr<const dependency>> dependencies;	/// The dependencies of which one should be true
};

class not_dependency : public dependency
{
public:
	not_dependency() {}
	not_dependency(std::vector<std::unique_ptr<const dependency>> &&dependencies)
		: dependencies(std::move(dependencies))
	{
	}

	not_dependency(std::unique_ptr<const dependency> &&dependency)
	{
		this->dependencies.push_back(std::move(dependency));
	}
	
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const override;
	
	virtual std::string get_string(const std::string &prefix = "") const override
	{
		int element_count = 0;
		
		for (const auto &dependency : this->dependencies) {
			if (!dependency->get_string(prefix + '\t').empty()) {
				element_count++;
			}
		}
		
		if (element_count >= 1) {
			std::string str = prefix + "NOT:\n";
		
			for (const auto &dependency : this->dependencies) {
				str += dependency->get_string(prefix + '\t');
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<std::unique_ptr<const dependency>> dependencies;	/// The dependencies of which none should be true
};

class unit_type_dependency : public dependency
{
public:
	unit_type_dependency() {}
	unit_type_dependency(const CUnitType *unit_type, const int count) : UnitType(unit_type), Count(count) {}
	
	virtual void process_sml_property(const sml_property &property) override;
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual std::string get_string(const std::string &prefix = "") const override;

private:
	const CUnitType *UnitType = nullptr;
	int Count = 1;		/// How many of the unit type are required
};

class upgrade_dependency : public dependency
{
public:
	upgrade_dependency() {}
	upgrade_dependency(const CUpgrade *upgrade) : Upgrade(upgrade) {}
	
	virtual void process_sml_property(const sml_property &property) override;
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const override;
	virtual std::string get_string(const std::string &prefix = "") const override;

private:
	const CUpgrade *Upgrade = nullptr;
};

class age_dependency : public dependency
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual std::string get_string(const std::string &prefix = "") const override;

private:
	const age *age = nullptr;
};

class character_dependency : public dependency
{
public:
	virtual void process_sml_property(const sml_property &property) override;
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const override;
	virtual std::string get_string(const std::string &prefix = "") const override;

private:
	const character *character = nullptr;
};

class season_dependency : public dependency
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool check(const CUnit *unit, bool ignore_units = false) const override;
	virtual std::string get_string(const std::string &prefix = "") const override;

private:
	const season *Season = nullptr;
};

class settlement_dependency : public dependency
{
	virtual void process_sml_property(const sml_property &property) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual std::string get_string(const std::string &prefix = "") const override;

private:
	const site *settlement = nullptr;
	const faction *faction = nullptr;
	bool enemy = false;
};

class trigger_dependency : public dependency
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool check(const CPlayer *player, bool ignore_units = false) const override;
	virtual std::string get_string(const std::string &prefix = "") const override;

private:
	const trigger *trigger = nullptr;
};

}
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Register CCL features for dependencies
extern void DependenciesCclRegister();

/// Print all unit dependencies into string
extern std::string PrintDependencies(const CPlayer &player, const ButtonAction &button);
extern void AddDependency(const int rule_type, const std::string &target, const int required_rule_type, const std::string &required, const int count, const int or_flag, const bool is_predependency);

/// Check dependencies for player
extern bool CheckDependencies(const CUnitType *target, const CPlayer *player, bool ignore_units = false, bool is_predependency = false, bool is_neutral_use = false);
extern bool CheckDependencies(const CUpgrade *target, const CPlayer *player, bool ignore_units = false, bool is_predependency = false, bool is_neutral_use = false);

template <typename T>
extern bool CheckDependencies(const T *target, const CPlayer *player, bool ignore_units = false, bool is_predependency = false, bool is_neutral_use = false)
{
	if (!is_predependency && !CheckDependencies(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if constexpr (std::is_same_v<stratagus::age, T>) {
		if (is_predependency) {
			return !target->get_predependency() || target->get_predependency()->check(player, ignore_units);
		} else {
			return !target->get_dependency() || target->get_dependency()->check(player, ignore_units);
		}
	} else {
		if (is_predependency) {
			return !target->Predependency || target->Predependency->check(player, ignore_units);
		} else {
			return !target->Dependency || target->Dependency->check(player, ignore_units);
		}
	}
}

/// Check dependencies for unit
extern bool CheckDependencies(const CUnitType *target, const CUnit *unit, bool ignore_units = false, bool is_predependency = false);
extern bool CheckDependencies(const CUpgrade *target, const CUnit *unit, bool ignore_units = false, bool is_predependency = false);

template <typename T>
extern bool CheckDependencies(const T *target, const CUnit *unit, bool ignore_units = false, bool is_predependency = false)
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
