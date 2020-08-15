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
//      (c) Copyright 2015-2020 by Andrettin
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

class CUpgrade;
struct lua_State;

int CclDefineUniqueItem(lua_State *l);

namespace wyrmgus {

class icon;
class spell;
class unit_type;

class unique_item final : public detailed_data_entry, public data_type<unique_item>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "unique_item";
	static constexpr const char *database_folder = "unique_items";

	explicit unique_item(const std::string &identifier) : detailed_data_entry(identifier)
	{
	}

	bool CanDrop() const;				/// Check whether this unique item can drop
	int GetMagicLevel() const;			/// Get this unique item's magic level
	wyrmgus::icon *get_icon() const;

	int ResourcesHeld = 0;
private:
	wyrmgus::icon *icon = nullptr; //the unique item's icon (if it differs from that of its type)
public:
	unit_type *Type = nullptr;			/// Item type of the item
	CUpgrade *Prefix = nullptr;
	CUpgrade *Suffix = nullptr;
	CUpgrade *Set = nullptr;
	spell *Spell = nullptr;
	CUpgrade *Work = nullptr;
	CUpgrade *Elixir = nullptr;

	friend int ::CclDefineUniqueItem(lua_State *l);
};

}

extern std::string GetUniqueItemEffectsString(const std::string &item_ident);
