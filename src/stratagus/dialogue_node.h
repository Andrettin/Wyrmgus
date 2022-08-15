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
class CUnit;
class LuaCallback;
struct lua_State;

extern int CclDefineDialogue(lua_State *l);

namespace wyrmgus {

class character;
class dialogue;
class dialogue_option;
class faction;
class gsml_data;
class gsml_property;
class icon;
class player_color;
class sound;
class unit_type;
struct context;

template <typename scope_type>
class and_condition;

class dialogue_node final
{
public:
	explicit dialogue_node(wyrmgus::dialogue *dialogue, const int index);
	~dialogue_node();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void initialize();
	void check() const;

	wyrmgus::dialogue *get_dialogue() const
	{
		return this->dialogue;
	}

	int get_index() const
	{
		return this->index;
	}

	void add_option(std::unique_ptr<dialogue_option> &&option)
	{
		this->option_pointers.push_back(option.get());
		this->options.push_back(std::move(option));
	}

	void call(CPlayer *player, const context &ctx) const;
	void option_effect(const int option_index, CPlayer *player, const context &ctx) const;

	const CUnit *get_speaker_unit() const;
	std::string get_title_string(const CUnit *speaker_unit) const;
	const wyrmgus::icon *get_icon(const CUnit *speaker_unit) const;
	const wyrmgus::player_color *get_player_color(const CUnit *speaker_unit) const;
	std::string get_text(const context &ctx) const;

	void delete_lua_callbacks();

private:
	wyrmgus::dialogue *dialogue = nullptr;
	int index = -1;
	std::string title;
	const wyrmgus::icon *icon = nullptr;
	const wyrmgus::player_color *player_color = nullptr;
	const character *speaker = nullptr;
	const unit_type *speaker_unit_type = nullptr;
	const faction *speaker_faction = nullptr; //faction of the player to whom the speaker belongs
	size_t speaker_index = 0;
	std::string text;
	const wyrmgus::sound *sound = nullptr;
	std::unique_ptr<const and_condition<CPlayer>> conditions;
public:
	std::unique_ptr<LuaCallback> Conditions;
	std::unique_ptr<LuaCallback> ImmediateEffects;
private:
	std::vector<std::unique_ptr<dialogue_option>> options;
	std::vector<const dialogue_option *> option_pointers;

	friend int ::CclDefineDialogue(lua_State *l);
};

}
