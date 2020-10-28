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

class CPlayer;
class LuaCallback;
struct lua_State;

static int CclDefineDialogue(lua_State *l);

namespace wyrmgus {

class dialogue_node;
class effect_list;
class sml_data;
class sml_property;

class dialogue_option final
{
public:
	dialogue_option();
	~dialogue_option();

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	const std::string &get_name() const
	{
		return this->name;
	}

	void do_effects(CPlayer *player) const;
	std::string get_tooltip() const;

private:
	std::string name;
	std::unique_ptr<effect_list> effects;
	std::unique_ptr<LuaCallback> lua_effects = nullptr;
	std::string tooltip;

	friend static int ::CclDefineDialogue(lua_State *l);
};

}
