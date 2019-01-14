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

#ifndef __DEPEND_H__
#define __DEPEND_H__

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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAge;
class CCharacter;
class CConfigData;
class CPlayer;
class CSeason;
class CTrigger;
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
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const = 0;
	virtual bool Check(const CUnit *unit, bool ignore_units = false) const;
	virtual std::string GetString(const std::string &prefix = "") const = 0; //get the dependency as a string
};

class CAndDependency : public CDependency
{
public:
	CAndDependency() {}
	CAndDependency(const std::vector<const CDependency *> &dependencies) : Dependencies(dependencies) {}

	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool Check(const CUnit *unit, bool ignore_units = false) const override;
	
	virtual std::string GetString(const std::string &prefix = "") const override
	{
		int element_count = 0;
		
		for (const CDependency *dependency : this->Dependencies) {
			if (!dependency->GetString(prefix + '\t').empty()) {
				element_count++;
			}
		}
		
		if (element_count >= 1) {
			std::string str;
			if (element_count > 1) {
				str += prefix + "AND:\n";
			}
		
			for (const CDependency *dependency : this->Dependencies) {
				str += dependency->GetString((element_count > 1) ? prefix + '\t' : prefix);
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<const CDependency *> Dependencies;	/// The dependencies of which all should be true
};

class COrDependency : public CDependency
{
public:
	COrDependency() {}
	COrDependency(const std::vector<const CDependency *> &dependencies) : Dependencies(dependencies) {}
	
	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool Check(const CUnit *unit, bool ignore_units = false) const override;
	
	virtual std::string GetString(const std::string &prefix = "") const override
	{
		int element_count = 0;
		
		for (const CDependency *dependency : this->Dependencies) {
			if (!dependency->GetString(prefix + '\t').empty()) {
				element_count++;
			}
		}
		
		if (element_count >= 1) {
			std::string str;
			if (element_count > 1) {
				str += prefix + "OR:\n";
			}
		
			for (const CDependency *dependency : this->Dependencies) {
				str += dependency->GetString((element_count > 1) ? prefix + '\t' : prefix);
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<const CDependency *> Dependencies;	/// The dependencies of which one should be true
};

class CNotDependency : public CDependency
{
public:
	CNotDependency() {}
	CNotDependency(const std::vector<const CDependency *> &dependencies) : Dependencies(dependencies) {}
	CNotDependency(const CDependency *dependency)
	{
		this->Dependencies.push_back(dependency);
	}
	
	virtual void ProcessConfigDataSection(const CConfigData *section) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool Check(const CUnit *unit, bool ignore_units = false) const override;
	
	virtual std::string GetString(const std::string &prefix = "") const override
	{
		int element_count = 0;
		
		for (const CDependency *dependency : this->Dependencies) {
			if (!dependency->GetString(prefix + '\t').empty()) {
				element_count++;
			}
		}
		
		if (element_count >= 1) {
			std::string str = prefix + "NOT:\n";
		
			for (const CDependency *dependency : this->Dependencies) {
				str += dependency->GetString(prefix + '\t');
			}

			return str;
		} else {
			return std::string();
		}
	}

private:
	std::vector<const CDependency *> Dependencies;	/// The dependencies of which none should be true
};

class CUnitTypeDependency : public CDependency
{
public:
	CUnitTypeDependency() {}
	CUnitTypeDependency(const CUnitType *unit_type, const int count) : UnitType(unit_type), Count(count) {}
	
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual std::string GetString(const std::string &prefix = "") const override;

private:
	const CUnitType *UnitType = nullptr;
	int Count = 1;		/// How many of the unit type are required
};

class CUpgradeDependency : public CDependency
{
public:
	CUpgradeDependency() {}
	CUpgradeDependency(const CUpgrade *upgrade) : Upgrade(upgrade) {}
	
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool Check(const CUnit *unit, bool ignore_units = false) const override;
	virtual std::string GetString(const std::string &prefix = "") const override;

private:
	const CUpgrade *Upgrade = nullptr;
};

class CAgeDependency : public CDependency
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual std::string GetString(const std::string &prefix = "") const override;

private:
	const CAge *Age = nullptr;
};

class CCharacterDependency : public CDependency
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool Check(const CUnit *unit, bool ignore_units = false) const override;
	virtual std::string GetString(const std::string &prefix = "") const override;

private:
	const CCharacter *Character = nullptr;
};

class CSeasonDependency : public CDependency
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual bool Check(const CUnit *unit, bool ignore_units = false) const override;
	virtual std::string GetString(const std::string &prefix = "") const override;

private:
	const CSeason *Season = nullptr;
};

class CTriggerDependency : public CDependency
{
public:
	virtual void ProcessConfigDataProperty(const std::pair<std::string, std::string> &property) override;
	virtual bool Check(const CPlayer *player, bool ignore_units = false) const override;
	virtual std::string GetString(const std::string &prefix = "") const override;

private:
	const CTrigger *Trigger = nullptr;
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
extern bool CheckDependencies(const CUnitType *target, const CPlayer *player, bool ignore_units = false, bool is_predependency = false, bool is_neutral_use = false);
extern bool CheckDependencies(const CUpgrade *target, const CPlayer *player, bool ignore_units = false, bool is_predependency = false, bool is_neutral_use = false);

template <typename T>
extern bool CheckDependencies(const T *target, const CPlayer *player, bool ignore_units = false, bool is_predependency = false, bool is_neutral_use = false)
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

#endif
