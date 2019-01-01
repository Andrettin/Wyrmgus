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
/**@name upgrade_structs.h - The upgrade/allow header file. */
//
//      (c) Copyright 1999-2019 by Vladi Belperchinov-Shabanski,
//		Jimmy Salmon and Andrettin
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

#ifndef __UPGRADE_STRUCTS_H__
#define __UPGRADE_STRUCTS_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

//Wyrmgus start
#include "item.h"
//Wyrmgus end
#include "resource.h"


#include <vector>
/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CConfigData;
class CDeityDomain;
class CIcon;
class CSchoolOfMagic;
class CUniqueItem;
class CUnitType;
class CUpgradeModifier;
class CVariable;
struct lua_State;

/**
**  These are the current stats of a unit. Upgraded or downgraded.
*/
class CUnitStats
{
public:
	CUnitStats() : Variables(nullptr)
	{
		memset(Costs, 0, sizeof(Costs));
		memset(Storing, 0, sizeof(Storing));
		memset(ImproveIncomes, 0, sizeof(ImproveIncomes));
		memset(ResourceDemand, 0, sizeof(ResourceDemand));
	}
	~CUnitStats();

	const CUnitStats &operator = (const CUnitStats &rhs);

	bool operator == (const CUnitStats &rhs) const;
	bool operator != (const CUnitStats &rhs) const;
	
 	int GetPrice() const;
	int GetUnitStock(CUnitType *unit_type) const;
	void SetUnitStock(CUnitType *unit_type, int quantity);
	void ChangeUnitStock(CUnitType *unit_type, int quantity);
public:
	CVariable *Variables;           /// user defined variable.
	int Costs[MaxCosts];            /// current costs of the unit
	int Storing[MaxCosts];          /// storage increasing
	int ImproveIncomes[MaxCosts];   /// Gives player an improved income
	int ResourceDemand[MaxCosts];	/// Resource demand
	std::map<CUnitType *, int> UnitStock;	/// Units in stock
};

/**
**  The main useable upgrades.
*/
class CUpgrade
{
public:
	CUpgrade(const std::string &ident);
	~CUpgrade();

	static CUpgrade *New(const std::string &ident);
	static CUpgrade *Get(const std::string &ident);

	void ProcessConfigData(const CConfigData *config_data);
	
	void SetIcon(CIcon *icon);

	std::string Ident;                /// identifier
	std::string Name;                 /// upgrade label
	//Wyrmgus start
	int Class;						/// upgrade class (i.e. siege weapon projectile I)
	int Civilization;				/// which civilization this upgrade belongs to, if any
	int Faction;					/// which faction this upgrade belongs to, if any
	std::string Description;		/// Description of the upgrade
	std::string Quote;				/// Quote of the upgrade
	std::string Background;			/// Encyclopedia entry for the upgrade
	std::string EffectsString;		/// Effects string of the upgrade
	std::string RequirementsString;	/// Requirements string of the upgrade
	bool Ability;
	bool Weapon;
	bool Shield;
	bool Boots;
	bool Arrows;
	bool MagicPrefix;
	bool MagicSuffix;
	bool RunicAffix;
	bool UniqueOnly;						/// Whether (if this is a literary work) this should appear only on unique items (used, for instance, if a book has no copies of its text)
	bool ItemPrefix[MaxItemClasses];
	bool ItemSuffix[MaxItemClasses];
	bool IncompatibleAffixes[UpgradeMax];
	std::vector<int> WeaponClasses;		/// If isn't empty, one of these weapon classes will need to be equipped for the upgrade to be applied
	std::vector<std::string> Epithets;	/// Epithets when a character has a certain trait
	CUnitType *Item;
	//Wyrmgus end
	int   ID;                         /// numerical id
	int   Costs[MaxCosts];            /// costs for the upgrade
	int   ScaledCosts[MaxCosts];      /// scaled costs for the upgrade
	//Wyrmgus start
	int GrandStrategyProductionEfficiencyModifier[MaxCosts];	/// Production modifier for a particular resource for grand strategy mode
	int MaxLimit;					/// Maximum amount of times this upgrade can be acquired as an individual upgrade
	int MagicLevel;					/// Magic level of an affix
	int Work;						/// Form in which was inscribed (i.e. scroll or book), if is a literary work
	int Year;						/// Year of publication, if is a literary work
	CCharacter *Author;				/// Author of this literary work (if it is one)
	std::vector<CUpgradeModifier *> UpgradeModifiers;	/// Upgrade modifiers for this upgrade
	std::vector<CUniqueItem *> UniqueItems;	/// Unique items who form a part of this set upgrade
	std::vector<CUnitType *> ScaledCostUnits;	/// Units for which the upgrade's costs are scaled
	std::vector<CDeityDomain *> DeityDomains;	/// Deity domains to which this ability belongs
	std::vector<CSchoolOfMagic *> SchoolsOfMagic;	/// Schools of magic to which this ability belongs
	std::vector<CCharacter *> Characters;	/// Characters who appear in this literary work (if it is one)
	//Wyrmgus end
	// TODO: not used by buttons
	CIcon *Icon;                      /// icon to display to the user
};

/**
**  Allow what a player can do. Every #CPlayer has an own allow struct.
**
**  This could allow/disallow units, actions or upgrades.
**
**  Values are:
**    @li `A' -- allowed,
**    @li `F' -- forbidden,
**    @li `R' -- acquired, perhaps other values
**    @li `Q' -- acquired but forbidden (does it make sense?:))
**    @li `E' -- enabled, allowed by level but currently forbidden
**    @li `X' -- fixed, acquired can't be disabled
*/
class CAllow
{
public:
	CAllow() { this->Clear(); }

	void Clear()
	{
		memset(Units, 0, sizeof(Units));
		memset(Upgrades, 0, sizeof(Upgrades));
	}

	int  Units[UnitTypeMax];        /// maximum amount of units allowed
	char Upgrades[UpgradeMax];      /// upgrades allowed/disallowed
};

/**
**  Upgrade timer used in the player structure.
**  Every player has an own UpgradeTimers struct.
*/
class CUpgradeTimers
{
public:
	CUpgradeTimers() { this->Clear(); }

	void Clear()
	{
		memset(Upgrades, 0, sizeof(Upgrades));
	}

	/**
	**  all 0 at the beginning, all upgrade actions do increment values in
	**  this struct.
	*/
	int Upgrades[UpgradeMax];       /// counter for each upgrade
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern std::vector<CUpgrade *> AllUpgrades;  /// the main user usable upgrades

#endif
