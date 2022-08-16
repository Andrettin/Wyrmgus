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

class CPlayer;
class LuaCallback;
struct lua_State;

extern int CclDefineDialogue(lua_State *l);

namespace wyrmgus {

class dialogue;
class dialogue_node;
class gsml_data;
class gsml_property;
struct context;

template <typename scope_type>
class effect_list;

class dialogue_option final
{
public:
	static constexpr const char *default_name = "Continue";
	static constexpr const char *default_hotkey = "c";

	explicit dialogue_option(const dialogue_node *node);
	~dialogue_option();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void initialize();
	void check() const;

	const std::string &get_name() const
	{
		return this->name;
	}

	const std::string &get_hotkey() const
	{
		return this->hotkey;
	}

	dialogue *get_dialogue() const;

	const dialogue_node *get_next_node() const
	{
		return this->next_node;
	}

	bool ends_dialogue() const
	{
		return this->end_dialogue;
	}

	void do_effects(CPlayer *player, context &ctx) const;
	std::string get_tooltip(const context &ctx) const;

	void delete_lua_callbacks();

private:
	std::string name;
	std::string hotkey;
	const dialogue_node *node = nullptr;
	std::string next_node_identifier;
	const dialogue_node *next_node = nullptr;
	bool end_dialogue = false;
	std::unique_ptr<effect_list<CPlayer>> effects;
	std::unique_ptr<LuaCallback> lua_effects;
	std::string tooltip;

	friend int ::CclDefineDialogue(lua_State *l);
};

}
