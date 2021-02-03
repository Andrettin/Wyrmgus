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
//      (c) Copyright 1999-2021 by Vladi Belperchinov-Shabanski,
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
#include "item/item_class.h"
#include "resource.h"
#include "stratagus.h"
#include "unit/unit_variable.h"

struct lua_State;

static int CclDefineDependency(lua_State *l);
static int CclDefinePredependency(lua_State *l);
static int CclDefineUpgrade(lua_State *l);

namespace wyrmgus {
	class character;
	class civilization;
	class condition;
	class deity;
	class dynasty;
	class faction;
	class icon;
	class magic_domain;
	class unique_item;
	class unit_class;
	class unit_type;
	class upgrade_class;
	class upgrade_modifier;
	enum class gender;
	enum class item_class;
}

/**
**  These are the current stats of a unit. Upgraded or downgraded.
*/
class CUnitStats final
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

	wyrmgus::gender get_gender() const;

public:
	std::vector<wyrmgus::unit_variable> Variables;           /// user defined variable.
	int Costs[MaxCosts];            /// current costs of the unit
	int Storing[MaxCosts];          /// storage increasing
	int ImproveIncomes[MaxCosts];   /// Gives player an improved income
	int ResourceDemand[MaxCosts];	/// Resource demand
	std::map<int, int> UnitStock;	/// Units in stock
};

class CUpgrade final : public wyrmgus::detailed_data_entry, public wyrmgus::data_type<CUpgrade>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization READ get_civilization)
	Q_PROPERTY(wyrmgus::faction* faction MEMBER faction READ get_faction)
	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon READ get_icon)
	Q_PROPERTY(wyrmgus::upgrade_class* upgrade_class READ get_upgrade_class WRITE set_upgrade_class)
	Q_PROPERTY(QString requirements_string READ get_requirements_string_qstring)
	Q_PROPERTY(QString effects_string READ get_effects_string_qstring)
	Q_PROPERTY(bool ability MEMBER ability READ is_ability)
	Q_PROPERTY(bool weapon MEMBER weapon READ is_weapon)
	Q_PROPERTY(bool shield MEMBER shield READ is_shield)
	Q_PROPERTY(bool boots MEMBER boots READ is_boots)
	Q_PROPERTY(bool arrows MEMBER arrows READ is_arrows)
	Q_PROPERTY(bool magic_prefix MEMBER magic_prefix READ is_magic_prefix)
	Q_PROPERTY(bool magic_suffix MEMBER magic_suffix READ is_magic_suffix)
	Q_PROPERTY(int magic_level MEMBER magic_level READ get_magic_level)
	Q_PROPERTY(wyrmgus::unit_type* item MEMBER item READ get_item)

public:
	static constexpr const char *class_identifier = "upgrade";
	static constexpr const char *database_folder = "upgrades";

	static CUpgrade *add(const std::string &identifier, const wyrmgus::data_module *data_module)
	{
		CUpgrade *upgrade = data_type::add(identifier, data_module);
		upgrade->ID = CUpgrade::get_all().size() - 1;
		return upgrade;
	}

	static CUpgrade *add(const std::string &identifier, const wyrmgus::data_module *data_module, const wyrmgus::deity *deity)
	{
		CUpgrade *upgrade = CUpgrade::add(identifier, data_module);
		upgrade->deity = deity;
		return upgrade;
	}

	static CUpgrade *add(const std::string &identifier, const wyrmgus::data_module *data_module, const wyrmgus::dynasty *dynasty)
	{
		CUpgrade *upgrade = CUpgrade::add(identifier, data_module);
		upgrade->dynasty = dynasty;
		return upgrade;
	}

	static CUpgrade *add(const std::string &identifier, const wyrmgus::data_module *data_module, const wyrmgus::magic_domain *domain)
	{
		CUpgrade *upgrade = CUpgrade::add(identifier, data_module);
		upgrade->magic_domain = domain;
		return upgrade;
	}

	explicit CUpgrade(const std::string &identifier);
	~CUpgrade();

	virtual void process_sml_property(const wyrmgus::sml_property &property) override;
	virtual void process_sml_scope(const wyrmgus::sml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

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

	wyrmgus::faction *get_faction() const
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

	bool is_magic_prefix() const
	{
		return this->magic_prefix;
	}

	bool is_magic_suffix() const
	{
		return this->magic_suffix;
	}

	int get_magic_level() const
	{
		return this->magic_level;
	}

	const std::set<wyrmgus::item_class> &get_affixed_item_classes() const
	{
		return this->affixed_item_classes;
	}

	bool has_affixed_item_class(const wyrmgus::item_class item_class) const
	{
		return this->affixed_item_classes.contains(item_class);
	}

	wyrmgus::unit_type *get_item() const
	{
		return this->item;
	}

	const std::vector<std::unique_ptr<const wyrmgus::upgrade_modifier>> &get_modifiers() const
	{
		return this->modifiers;
	}

	void add_modifier(std::unique_ptr<const wyrmgus::upgrade_modifier> &&modifier);

	int get_scaled_cost(const wyrmgus::resource *resource) const
	{
		auto find_iterator = this->scaled_costs.find(resource);

		if (find_iterator != this->scaled_costs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	const std::vector<const wyrmgus::unit_type *> &get_scaled_cost_unit_types() const
	{
		return this->scaled_cost_unit_types;
	}

	const std::vector<const wyrmgus::unit_class *> &get_scaled_cost_unit_classes() const
	{
		return this->scaled_cost_unit_classes;
	}

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
	wyrmgus::faction *faction = nullptr; //which faction this upgrade belongs to, if any
	std::string effects_string; //effects string of the upgrade
	std::string requirements_string; //requirements string of the upgrade
	wyrmgus::icon *icon = nullptr; //icon to display to the user
	std::string button_key;
	bool ability = false;
	bool weapon = false;
	bool shield = false;
	bool boots = false;
	bool arrows = false;
	bool magic_prefix = false;
	bool magic_suffix = false;
public:
	bool RunicAffix = false;
	bool UniqueOnly = false; //whether (if this is a literary work) this should appear only on unique items (used, for instance, if a book has no copies of its text)
private:
	int magic_level = 0; //magic level of an affix
	std::set<wyrmgus::item_class> affixed_item_classes;
public:
	bool IncompatibleAffixes[UpgradeMax];
	std::set<wyrmgus::item_class> WeaponClasses; //if isn't empty, one of these weapon classes will need to be equipped for the upgrade to be applied
	//Wyrmgus start
	std::vector<std::string> Epithets;	/// epithets when a character has a certain trait
	//Wyrmgus end
private:
	wyrmgus::unit_type *item = nullptr;
public:
	int   ID = 0;						/// numerical id
	int   Costs[MaxCosts];				/// costs for the upgrade
private:
	std::map<const wyrmgus::resource *, int> scaled_costs; //scaled costs for the upgrade
public:
	//Wyrmgus start
	int GrandStrategyProductionEfficiencyModifier[MaxCosts];	/// Production modifier for a particular resource for grand strategy mode
	int MaxLimit = 1;					/// Maximum amount of times this upgrade can be acquired as an individual upgrade
	wyrmgus::item_class Work;			/// Form in which was inscribed (i.e. scroll or book), if is a literary work
	int Year = 0;						/// Year of publication, if is a literary work
	wyrmgus::character *Author = nullptr;		/// Author of this literary work (if it is one)
private:
	std::vector<std::unique_ptr<const wyrmgus::upgrade_modifier>> modifiers; //upgrade modifiers for this upgrade
public:
	std::vector<const wyrmgus::unique_item *> UniqueItems;	/// Unique items who form a part of this set upgrade
private:
	std::vector<const wyrmgus::unit_type *> scaled_cost_unit_types;	//units for which the upgrade's costs are scaled
	std::vector<const wyrmgus::unit_class *> scaled_cost_unit_classes;	//units for which the upgrade's costs are scaled
public:
	std::vector<wyrmgus::character *> Characters;	/// Characters who appear in this literary work (if it is one)
	//Wyrmgus end
	// TODO: not used by buttons
private:
	std::unique_ptr<wyrmgus::condition> preconditions;
	std::unique_ptr<wyrmgus::condition> conditions;
	const wyrmgus::dynasty *dynasty = nullptr; //the dynasty to which the upgrade pertains, if this is a dynasty upgrade
	const wyrmgus::deity *deity = nullptr; //the deity to which the upgrade pertains, if this is a deity upgrade
	const wyrmgus::magic_domain *magic_domain = nullptr; //the magic domain to which the upgrade pertains, if this is a magic domain upgrade

	friend int CclDefineUpgrade(lua_State *l);
	friend int CclDefineDependency(lua_State *l);
	friend int CclDefinePredependency(lua_State *l);
};

Q_DECLARE_METATYPE(std::vector<const CUpgrade *>)

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
