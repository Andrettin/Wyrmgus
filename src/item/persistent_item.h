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
//      (c) Copyright 2015-2022 by Andrettin
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

class CConfigData;
class CUnit;
class CUpgrade;
struct lua_State;

static int CclDefineCharacter(lua_State *l);
static int CclDefineCustomHero(lua_State *l);

namespace wyrmgus {

class character;
class gsml_data;
class gsml_property;
class spell;
class unique_item;
class unit_type;
enum class item_class;
enum class item_slot;

class persistent_item final
{
public:
	explicit persistent_item(const wyrmgus::unit_type *unit_type, character *owner)
		: unit_type(unit_type), owner(owner)
	{
		if (unit_type == nullptr) {
			throw std::runtime_error("Cannot create a persistent item with a null type.");
		}

		if (owner == nullptr) {
			throw std::runtime_error("Cannot create a persistent item with a null owner.");
		}
	}

	explicit persistent_item(const CUnit *item_unit, character *owner);

	explicit persistent_item(character *owner) : owner(owner)
	{
		if (owner == nullptr) {
			throw std::runtime_error("Cannot create a persistent item with a null owner.");
		}
	}

	std::unique_ptr<persistent_item> duplicate() const
	{
		return std::make_unique<persistent_item>(*this);
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void ProcessConfigData(const CConfigData *config_data);
	void initialize();

	const wyrmgus::unit_type *get_unit_type() const
	{
		return this->unit_type;
	}

	item_class get_item_class() const;
	item_slot get_item_slot() const;

	const unique_item *get_unique() const
	{
		return this->unique;
	}

	const std::string &get_name() const
	{
		return this->name;
	}

	bool is_bound() const
	{
		return this->bound;
	}

	bool is_identified() const
	{
		return this->identified;
	}

	void set_identified(const bool identified)
	{
		this->identified = identified;
	}

	character *get_owner() const
	{
		return this->owner;
	}

	bool is_equipped() const
	{
		return this->equipped;
	}

private:
	const wyrmgus::unit_type *unit_type = nullptr; //the item type of the item
	const unique_item *unique = nullptr;
	std::string name;
	bool bound = false; //whether the item is bound to its owner and can't be dropped
	bool identified = true; //whether the item has been identified
public:
	const CUpgrade *Prefix = nullptr;
	const CUpgrade *Suffix = nullptr;
	const spell *Spell = nullptr;
	const CUpgrade *Work = nullptr;
	const CUpgrade *Elixir = nullptr;
private:
	character *owner = nullptr;
	bool equipped = false; //used for initialization only

	friend int ::CclDefineCharacter(lua_State *l);
	friend int ::CclDefineCustomHero(lua_State *l);
};

}

extern void ItemCclRegister();
