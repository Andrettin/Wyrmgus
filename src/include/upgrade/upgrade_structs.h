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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"
#include "data_type.h"
#include "item_class.h"
#include "resource.h"
#include "stratagus.h"
#include "unit/unit_variable.h"

class CSchoolOfMagic;
struct lua_State;

int CclDefineDependency(lua_State *l);
int CclDefinePredependency(lua_State *l);

namespace wyrmgus {
	class character;
	class civilization;
	class condition;
	class deity;
	class deity_domain;
	class dynasty;
	class icon;
	class unique_item;
	class unit_type;
	class upgrade_class;
	class upgrade_modifier;
	enum class item_class;
}

/**
**  These are the current stats of a unit. Upgraded or downgraded.
*/
class CUnitStats
{
public:
	CUnitStats()
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
	int GetUnitStock(const wyrmgus::unit_type *unit_type) const;
	void SetUnitStock(const wyrmgus::unit_type *unit_type, int quantity);
	void ChangeUnitStock(const wyrmgus::unit_type *unit_type, int quantity);
public:
	std::vector<wyrmgus::unit_variable> Variables;           /// user defined variable.
	int Costs[MaxCosts];            /// current costs of the unit
	int Storing[MaxCosts];          /// storage increasing
	int ImproveIncomes[MaxCosts];   /// Gives player an improved income
	int ResourceDemand[MaxCosts];	/// Resource demand
	std::map<int, int> UnitStock;	/// Units in stock
};

class CUpgrade final : public wyrmgus::detailed_data_entry, public wyrmgus::data_type<CUpgrade>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization READ get_civilization)
	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon READ get_icon)
	Q_PROPERTY(wyrmgus::upgrade_class* upgrade_class READ get_upgrade_class WRITE set_upgrade_class)
	Q_PROPERTY(QString requirements_string READ get_requirements_string_qstring)
	Q_PROPERTY(QString effects_string READ get_effects_string_qstring)
	Q_PROPERTY(bool ability MEMBER ability READ is_ability)
	Q_PROPERTY(bool weapon MEMBER weapon READ is_weapon)
	Q_PROPERTY(bool shield MEMBER shield READ is_shield)
	Q_PROPERTY(bool boots MEMBER boots READ is_boots)
	Q_PROPERTY(bool arrows MEMBER arrows READ is_arrows)

public:
	static constexpr const char *class_identifier = "upgrade";
	static constexpr const char *database_folder = "upgrades";

	static CUpgrade *add(const std::string &identifier, const wyrmgus::module *module)
	{
		CUpgrade *upgrade = data_type::add(identifier, module);
		upgrade->ID = CUpgrade::get_all().size() - 1;
		return upgrade;
	}

	CUpgrade(const std::string &identifier);
	~CUpgrade();

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void process_sml_property(const wyrmgus::sml_property &property) override;
	virtual void process_sml_scope(const wyrmgus::sml_data &scope) override;
	virtual void initialize() override;

	int get_index() const
	{
		return this->ID;
	}

	void set_parent(const CUpgrade *parent_upgrade);

	wyrmgus::icon *get_icon() const
	{
		return this->icon;
	}

	wyrmgus::upgrade_class *get_upgrade_class() const
	{
		return this->upgrade_class;
	}

	void set_upgrade_class(wyrmgus::upgrade_class *upgrade_class);

	wyrmgus::civilization *get_civilization() const
	{
		return this->civilization;
	}

	int get_faction() const
	{
		return this->faction;
	}

	const std::string &get_effects_string() const
	{
		return this->effects_string;
	}

	QString get_effects_string_qstring() const
	{
		return QString::fromStdString(this->get_effects_string());
	}

	Q_INVOKABLE void set_effects_string(const std::string &effects_string)
	{
		this->effects_string = effects_string;
	}

	const std::string &get_requirements_string() const
	{
		return this->requirements_string;
	}

	QString get_requirements_string_qstring() const
	{
		return QString::fromStdString(this->get_requirements_string());
	}

	Q_INVOKABLE void set_requirements_string(const std::string &requirements_string)
	{
		this->requirements_string = requirements_string;
	}

	const std::string &get_button_key() const
	{
		return this->button_key;
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

	const std::vector<std::unique_ptr<wyrmgus::upgrade_modifier>> &get_modifiers() const
	{
		return this->modifiers;
	}

	void add_modifier(std::unique_ptr<wyrmgus::upgrade_modifier> &&modifier);

	const std::unique_ptr<wyrmgus::condition> &get_preconditions() const
	{
		return this->preconditions;
	}

	const std::unique_ptr<wyrmgus::condition> &get_conditions() const
	{
		return this->conditions;
	}

	const wyrmgus::dynasty *get_dynasty() const
	{
		return this->dynasty;
	}

	void set_dynasty(const wyrmgus::dynasty *dynasty)
	{
		this->dynasty = dynasty;
	}

	const wyrmgus::deity *get_deity() const
	{
		return this->deity;
	}

	void set_deity(const wyrmgus::deity *deity)
	{
		this->deity = deity;
	}

private:
	wyrmgus::upgrade_class *upgrade_class = nullptr; //upgrade class (e.g. siege weapon projectile I)
	wyrmgus::civilization *civilization = nullptr; //which civilization this upgrade belongs to, if any
	int faction = -1;				/// which faction this upgrade belongs to, if any
	std::string effects_string; //effects string of the upgrade
	std::string requirements_string; //requirements string of the upgrade
	wyrmgus::icon *icon = nullptr; //icon to display to the user
	std::string button_key;
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
	bool ItemPrefix[static_cast<int>(wyrmgus::item_class::count)];
	bool ItemSuffix[static_cast<int>(wyrmgus::item_class::count)];
	bool IncompatibleAffixes[UpgradeMax];
	std::set<wyrmgus::item_class> WeaponClasses; //if isn't empty, one of these weapon classes will need to be equipped for the upgrade to be applied
	//Wyrmgus start
	std::vector<std::string> Epithets;	/// epithets when a character has a certain trait
	wyrmgus::unit_type *Item = nullptr;
	//Wyrmgus end
	int   ID = 0;						/// numerical id
	int   Costs[MaxCosts];				/// costs for the upgrade
	int   ScaledCosts[MaxCosts];		/// scaled costs for the upgrade
	//Wyrmgus start
	int GrandStrategyProductionEfficiencyModifier[MaxCosts];	/// Production modifier for a particular resource for grand strategy mode
	int MaxLimit = 1;					/// Maximum amount of times this upgrade can be acquired as an individual upgrade
	int MagicLevel = 0;					/// Magic level of an affix
	wyrmgus::item_class Work;			/// Form in which was inscribed (i.e. scroll or book), if is a literary work
	int Year = 0;						/// Year of publication, if is a literary work
	wyrmgus::character *Author = nullptr;		/// Author of this literary work (if it is one)
private:
	std::vector<std::unique_ptr<wyrmgus::upgrade_modifier>> modifiers; //upgrade modifiers for this upgrade
public:
	std::vector<wyrmgus::unique_item *> UniqueItems;	/// Unique items who form a part of this set upgrade
	std::vector<wyrmgus::unit_type *> ScaledCostUnits;	/// Units for which the upgrade's costs are scaled
	std::vector<wyrmgus::deity_domain *> DeityDomains;	/// Deity domains to which this ability belongs
	std::vector<CSchoolOfMagic *> SchoolsOfMagic;	/// Schools of magic to which this ability belongs
	std::vector<wyrmgus::character *> Characters;	/// Characters who appear in this literary work (if it is one)
	//Wyrmgus end
	// TODO: not used by buttons
private:
	std::unique_ptr<wyrmgus::condition> preconditions;
	std::unique_ptr<wyrmgus::condition> conditions;
	const wyrmgus::dynasty *dynasty = nullptr; //the dynasty to which the upgrade pertains, if this is a dynasty upgrade
	const wyrmgus::deity *deity = nullptr; //the deity to which the upgrade pertains, if this is a deity upgrade

	friend int CclDefineUpgrade(lua_State *l);
	friend int CclDefineDependency(lua_State *l);
	friend int CclDefinePredependency(lua_State *l);
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
