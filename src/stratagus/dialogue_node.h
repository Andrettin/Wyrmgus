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
//

#pragma once

class CPlayer;
class LuaCallback;
struct lua_State;

static int CclDefineDialogue(lua_State *l);

namespace wyrmgus {

class and_condition;
class character;
class dialogue;
class dialogue_option;
class faction;
class sml_data;
class sml_property;
class unit_type;

class dialogue_node final
{
public:
	explicit dialogue_node(wyrmgus::dialogue *dialogue);
	~dialogue_node();

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	void initialize();
	void check() const;

	wyrmgus::dialogue *get_dialogue() const
	{
		return this->dialogue;
	}

	void add_option(std::unique_ptr<dialogue_option> &&option)
	{
		this->option_pointers.push_back(option.get());
		this->options.push_back(std::move(option));
	}

	void call(CPlayer *player) const;
	void option_effect(const int option_index, CPlayer *player) const;

	void delete_lua_callbacks();

	int ID = -1;
private:
	wyrmgus::dialogue *dialogue = nullptr;
	const character *speaker = nullptr;
	const unit_type *speaker_unit_type = nullptr;
	std::string speaker_name;
	const faction *speaker_faction = nullptr; //faction of the player to whom the speaker belongs
	std::string text;
	std::unique_ptr<const and_condition> conditions;
public:
	std::unique_ptr<LuaCallback> Conditions;
	std::unique_ptr<LuaCallback> ImmediateEffects;
private:
	std::vector<std::unique_ptr<dialogue_option>> options;
	std::vector<const dialogue_option *> option_pointers;

	friend int ::CclDefineDialogue(lua_State *l);
};

}
