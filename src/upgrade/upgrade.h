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
//      (c) Copyright 1999-2019 by Vladi Belperchinov-Shabanski, Jimmy Salmon
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
//

#ifndef __UPGRADE_H__
#define __UPGRADE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"
#include "economy/resource.h"

#include <set>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CCivilization;
class CDeityDomain;
class CDependency;
class CFaction;
class CFile;
class CPlayer;
class CSchoolOfMagic;
class CUnit;
class CUnitType;
class CUpgrade;
class CUpgradeModifier;
class ItemClass;
class ItemSlot;
class UniqueItem;
class UpgradeClass;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

/**
**  The main useable upgrades.
*/
class CUpgrade : public DetailedDataElement, public DataType<CUpgrade>
{
	GDCLASS(CUpgrade, DetailedDataElement)
	
public:
	CUpgrade(const std::string &ident = "");

	static constexpr const char *ClassIdentifier = "upgrade";
	
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	void SetParentUpgrade(const CUpgrade *parent_upgrade);
	
	const UpgradeClass *GetClass() const
	{
		return this->Class;
	}
	
	const CCivilization *GetCivilization() const
	{
		return this->Civilization;
	}
	
	const CFaction *GetFaction() const
	{
		return this->Faction;
	}
	
	bool IsAbility() const
	{
		return this->Ability;
	}
	
	const ::ItemSlot *GetItemSlot() const
	{
		return this->ItemSlot;
	}
	
	const CUnitType *GetItem() const
	{
		return this->Item;
	}
	
	int GetMagicLevel() const
	{
		return this->MagicLevel;
	}
	
	const String &GetEffectsString() const
	{
		return this->EffectsString;
	}

	const String &GetRequirementsString() const
	{
		return this->RequirementsString;
	}

	const std::vector<String> &GetEpithets() const
	{
		return this->Epithets;
	}

private:
	const CUpgrade *ParentUpgrade = nullptr;	/// the upgrade's parent upgrade (from which it inherits features)
	UpgradeClass *Class = nullptr;	/// upgrade class (i.e. siege weapon projectile I)
	CCivilization *Civilization = nullptr;	/// which civilization this upgrade belongs to, if any
	CFaction *Faction = nullptr;				/// which faction this upgrade belongs to, if any
	String EffectsString;		/// Effects string of the upgrade
	String RequirementsString;	/// Requirements string of the upgrade
	bool Ability = false;
	const ::ItemSlot *ItemSlot = nullptr;
public:
	bool MagicPrefix = false;
	bool MagicSuffix = false;
	bool RunicAffix = false;
	bool UniqueOnly = false;		/// Whether (if this is a literary work) this should appear only on unique items (used, for instance, if a book has no copies of its text)
	std::set<const ItemClass *> ItemPrefix;
	std::set<const ItemClass *> ItemSuffix;
	bool IncompatibleAffixes[UpgradeMax];
	std::vector<const ItemClass *> WeaponClasses;	/// If isn't empty, one of these weapon classes will need to be equipped for the upgrade to be applied
private:
	std::vector<String> Epithets;		/// epithets when a character has a certain trait
	const CUnitType *Item = nullptr;
public:
	int   Costs[MaxCosts];				/// costs for the upgrade
	int   ScaledCosts[MaxCosts];		/// scaled costs for the upgrade
	//Wyrmgus start
	int GrandStrategyProductionEfficiencyModifier[MaxCosts];	/// Production modifier for a particular resource for grand strategy mode
	int MaxLimit = 1;					/// Maximum amount of times this upgrade can be acquired as an individual upgrade
private:
	int MagicLevel = 0;					/// Magic level of an affix
public:
	const ItemClass *Work = nullptr;	/// Form in which was inscribed (i.e. scroll or book), if is a literary work
	int Year = 0;						/// Year of publication, if is a literary work
	CCharacter *Author = nullptr;		/// Author of this literary work (if it is one)
	std::vector<CUpgradeModifier *> UpgradeModifiers;	/// Upgrade modifiers for this upgrade
	std::vector<UniqueItem *> UniqueItems;	/// Unique items who form a part of this set upgrade
	std::vector<CUnitType *> ScaledCostUnits;	/// Units for which the upgrade's costs are scaled
	std::vector<CDeityDomain *> DeityDomains;	/// Deity domains to which this ability belongs
	std::vector<CSchoolOfMagic *> SchoolsOfMagic;	/// Schools of magic to which this ability belongs
	std::vector<CCharacter *> Characters;	/// Characters who appear in this literary work (if it is one)
	//Wyrmgus end
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;
	
	friend int CclDefineUpgrade(lua_State *l);

protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// init upgrade/allow structures
extern void InitUpgrades();
/// save the upgrades
extern void SaveUpgrades(CFile &file);
/// cleanup upgrade module
extern void CleanUpgrades();

/// Register CCL features for upgrades
extern void UpgradesCclRegister();

/*----------------------------------------------------------------------------
--  General/Map functions
----------------------------------------------------------------------------*/

// AllowStruct and UpgradeTimers will be static in the player so will be
// load/saved with the player struct

extern int UnitTypeIdByIdent(const std::string &sid);

/*----------------------------------------------------------------------------
--  Upgrades
----------------------------------------------------------------------------*/

/// Upgrade will be acquired
extern void UpgradeAcquire(CPlayer &player, const CUpgrade *upgrade);

/// Upgrade will be lost
extern void UpgradeLost(CPlayer &player, const CUpgrade *upgrade);
/// Apply researched upgrades when map is loading
extern void ApplyUpgrades();

extern void ApplyIndividualUpgradeModifier(CUnit &unit, const CUpgradeModifier *um); /// Apply upgrade modifier of an individual upgrade
//Wyrmgus start
extern void RemoveIndividualUpgradeModifier(CUnit &unit, const CUpgradeModifier *um);
extern void AbilityAcquire(CUnit &unit, const CUpgrade *upgrade, const bool save = true);
extern void AbilityLost(CUnit &unit, const CUpgrade *upgrade, const bool lose_all = false);
extern void TraitAcquire(CUnit &unit, const CUpgrade *upgrade);
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
extern std::string GetUpgradeEffectsString(const std::string &upgrade_ident);
extern bool IsPercentageVariable(int var);
extern bool IsBonusVariable(int var);
extern bool IsBooleanVariable(int var);
extern bool IsKnowledgeVariable(int var);
extern bool IsPotentiallyNegativeVariable(int var);
extern String GetVariableDisplayName(const int var_index, const bool increase = false);
//Wyrmgus end

#endif
