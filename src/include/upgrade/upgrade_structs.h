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
//      (c) Copyright 1999-2020 by Vladi Belperchinov-Shabanski,
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

#pragma once

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "database/data_entry.h"
#include "database/data_type.h"
#include "data_type.h"
//Wyrmgus start
#include "item.h"
//Wyrmgus end
#include "resource.h"
#include "stratagus.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CDeityDomain;
class CDependency;
class CIcon;
class CSchoolOfMagic;
class CUniqueItem;
class CUnitType;
class CUpgradeModifier;
class CVariable;
struct lua_State;

namespace stratagus {
	class civilization;
}

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

class CUpgrade final : public stratagus::data_entry, public stratagus::data_type<CUpgrade>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring WRITE set_name_qstring)
	Q_PROPERTY(stratagus::civilization*civilization MEMBER civilization READ get_civilization)
	Q_PROPERTY(CIcon* icon MEMBER icon READ get_icon)
	Q_PROPERTY(QString description READ get_description_qstring WRITE set_description_qstring)
	Q_PROPERTY(QString quote READ get_quote_qstring WRITE set_quote_qstring)
	Q_PROPERTY(QString background READ get_background_qstring WRITE set_background_qstring)
	Q_PROPERTY(bool ability MEMBER ability READ is_ability)
	Q_PROPERTY(bool weapon MEMBER weapon READ is_weapon)
	Q_PROPERTY(bool shield MEMBER shield READ is_shield)
	Q_PROPERTY(bool boots MEMBER boots READ is_boots)
	Q_PROPERTY(bool arrows MEMBER arrows READ is_arrows)

public:
	static constexpr const char *class_identifier = "upgrade";
	static constexpr const char *database_folder = "upgrades";

	static CUpgrade *add(const std::string &identifier, const stratagus::module *module)
	{
		CUpgrade *upgrade = data_type::add(identifier, module);
		upgrade->ID = CUpgrade::get_all().size() - 1;
		return upgrade;
	}

	CUpgrade(const std::string &identifier);
	~CUpgrade();

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void process_sml_property(const stratagus::sml_property &property) override;
	virtual void process_sml_scope(const stratagus::sml_data &scope) override;
	virtual void initialize() override;

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	void set_name_qstring(const QString &name)
	{
		this->name = name.toStdString();
	}

	CIcon *get_icon() const
	{
		return this->icon;
	}

	int get_class() const
	{
		return this->upgrade_class;
	}

	stratagus::civilization *get_civilization() const
	{
		return this->civilization;
	}

	int get_faction() const
	{
		return this->faction;
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	void set_description_qstring(const QString &description)
	{
		this->description = description.toStdString();
	}

	const std::string &get_quote() const
	{
		return this->quote;
	}

	QString get_quote_qstring() const
	{
		return QString::fromStdString(this->get_quote());
	}

	void set_quote_qstring(const QString &quote)
	{
		this->quote = quote.toStdString();
	}

	const std::string &get_background() const
	{
		return this->background;
	}

	QString get_background_qstring() const
	{
		return QString::fromStdString(this->get_background());
	}

	void set_background_qstring(const QString &background)
	{
		this->background = background.toStdString();
	}

	bool is_ability() const
	{
		return this->ability;
	}

	bool is_weapon() const
	{
		return this->weapon;
	}

	bool is_shield() const
	{
		return this->shield;
	}

	bool is_boots() const
	{
		return this->boots;
	}

	bool is_arrows() const
	{
		return this->arrows;
	}

private:
	std::string name;
	int upgrade_class = -1;			/// upgrade class (e.g. siege weapon projectile I)
	stratagus::civilization *civilization = nullptr; //which civilization this upgrade belongs to, if any
	int faction = -1;				/// which faction this upgrade belongs to, if any
	std::string description;		/// description of the upgrade
	std::string quote;				/// quote of the upgrade
	std::string background;			/// encyclopedia entry for the upgrade
public:
	std::string EffectsString;		/// effects string of the upgrade
	std::string RequirementsString;	/// requirements string of the upgrade
private:
	CIcon *icon = nullptr;			/// icon to display to the user
	bool ability = false;
	bool weapon = false;
	bool shield = false;
	bool boots = false;
	bool arrows = false;
public:
	bool MagicPrefix = false;
	bool MagicSuffix = false;
	bool RunicAffix = false;
	bool UniqueOnly = false;		/// whether (if this is a literary work) this should appear only on unique items (used, for instance, if a book has no copies of its text)
	bool ItemPrefix[MaxItemClasses];
	bool ItemSuffix[MaxItemClasses];
	bool IncompatibleAffixes[UpgradeMax];
	std::vector<int> WeaponClasses;		/// if isn't empty, one of these weapon classes will need to be equipped for the upgrade to be applied
	//Wyrmgus start
	std::vector<std::string> Epithets;	/// epithets when a character has a certain trait
	CUnitType *Item = nullptr;
	//Wyrmgus end
	int   ID = 0;						/// numerical id
	int   Costs[MaxCosts];				/// costs for the upgrade
	int   ScaledCosts[MaxCosts];		/// scaled costs for the upgrade
	//Wyrmgus start
	int GrandStrategyProductionEfficiencyModifier[MaxCosts];	/// Production modifier for a particular resource for grand strategy mode
	int MaxLimit = 1;					/// Maximum amount of times this upgrade can be acquired as an individual upgrade
	int MagicLevel = 0;					/// Magic level of an affix
	int Work = -1;						/// Form in which was inscribed (i.e. scroll or book), if is a literary work
	int Year = 0;						/// Year of publication, if is a literary work
	CCharacter *Author = nullptr;		/// Author of this literary work (if it is one)
	std::vector<CUpgradeModifier *> UpgradeModifiers;	/// Upgrade modifiers for this upgrade
	std::vector<CUniqueItem *> UniqueItems;	/// Unique items who form a part of this set upgrade
	std::vector<CUnitType *> ScaledCostUnits;	/// Units for which the upgrade's costs are scaled
	std::vector<CDeityDomain *> DeityDomains;	/// Deity domains to which this ability belongs
	std::vector<CSchoolOfMagic *> SchoolsOfMagic;	/// Schools of magic to which this ability belongs
	std::vector<CCharacter *> Characters;	/// Characters who appear in this literary work (if it is one)
	//Wyrmgus end
	// TODO: not used by buttons
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;

	friend int CclDefineUpgrade(lua_State *l);
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
