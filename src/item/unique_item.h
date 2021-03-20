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
//      (c) Copyright 2015-2021 by Andrettin
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

#include "database/data_type.h"
#include "database/detailed_data_entry.h"

class CUpgrade;
struct lua_State;

static int CclDefineUniqueItem(lua_State *l);

namespace wyrmgus {

class icon;
class spell;
class unit_type;

class unique_item final : public detailed_data_entry, public data_type<unique_item>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::unit_type* unit_type MEMBER unit_type)
	Q_PROPERTY(wyrmgus::icon* icon MEMBER icon)
	Q_PROPERTY(CUpgrade* prefix MEMBER prefix)
	Q_PROPERTY(CUpgrade* suffix MEMBER suffix)
	Q_PROPERTY(CUpgrade* set MEMBER set)
	Q_PROPERTY(wyrmgus::spell* spell MEMBER spell)
	Q_PROPERTY(CUpgrade* work MEMBER work)
	Q_PROPERTY(CUpgrade* elixir MEMBER elixir)
	Q_PROPERTY(int resources_held MEMBER resources_held READ get_resources_held)

public:
	static constexpr const char *class_identifier = "unique_item";
	static constexpr const char *database_folder = "unique_items";

	explicit unique_item(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	virtual void check() const override
	{
		if (this->get_unit_type() == nullptr) {
			throw std::runtime_error("Unique item \"" + this->get_identifier() + "\" has no unit type.");
		}
	}

	const wyrmgus::unit_type *get_unit_type() const
	{
		return this->unit_type;
	}

	const CUpgrade *get_prefix() const
	{
		return this->prefix;
	}

	const CUpgrade *get_suffix() const
	{
		return this->suffix;
	}

	const CUpgrade *get_set() const
	{
		return this->set;
	}

	const wyrmgus::spell *get_spell() const
	{
		return this->spell;
	}

	const CUpgrade *get_work() const
	{
		return this->work;
	}

	const CUpgrade *get_elixir() const
	{
		return this->elixir;
	}

	int get_resources_held() const
	{
		return this->resources_held;
	}

	bool can_drop() const;				/// Check whether this unique item can drop
	int get_magic_level() const;			/// Get this unique item's magic level
	const wyrmgus::icon *get_icon() const;

private:
	wyrmgus::unit_type *unit_type = nullptr; //unit type of the unique
	wyrmgus::icon *icon = nullptr; //the unique item's icon (if it differs from that of its type)
	CUpgrade *prefix = nullptr;
	CUpgrade *suffix = nullptr;
	CUpgrade *set = nullptr;
	wyrmgus::spell *spell = nullptr;
	CUpgrade *work = nullptr;
	CUpgrade *elixir = nullptr;
	int resources_held = 0;

	friend int ::CclDefineUniqueItem(lua_State *l);
};

}

extern std::string GetUniqueItemEffectsString(const std::string &item_ident);
