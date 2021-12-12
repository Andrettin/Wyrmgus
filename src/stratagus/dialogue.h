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

#include "database/data_entry.h"
#include "database/data_type.h"

class CPlayer;
struct lua_State;

static int CclDefineDialogue(lua_State *l);

namespace wyrmgus {

class dialogue_node;
class dialogue_option;
struct context;

class dialogue final : public data_entry, public data_type<dialogue>
{
	Q_OBJECT

public:
	static bool has_sound_channel(const int channel)
	{
		return dialogue::sound_channels.contains(channel);
	}

	static void add_sound_channel(const int channel)
	{
		dialogue::sound_channels.insert(channel);
	}

	static void remove_sound_channel(const int channel)
	{
		dialogue::sound_channels.erase(channel);
	}

	static void stop_sound_channels();

private:
	static inline std::set<int> sound_channels; //channels currently playing dialogue node sounds

public:
	static constexpr const char *class_identifier = "dialogue";
	static constexpr const char *database_folder = "dialogues";

	explicit dialogue(const std::string &identifier);
	~dialogue();
	
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const dialogue_node *get_node(const std::string &identifier) const
	{
		auto find_iterator = this->nodes_by_identifier.find(identifier);
		if (find_iterator != this->nodes_by_identifier.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("Invalid dialogue node for dialogue \"" + this->get_identifier() + "\": \"" + identifier + "\".");
	}

	const dialogue_option *get_option(const std::string &identifier) const
	{
		const auto find_iterator = this->options_by_identifier.find(identifier);
		if (find_iterator != this->options_by_identifier.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("Invalid dialogue option for dialogue \"" + this->get_identifier() + "\": \"" + identifier + "\".");
	}

	void map_option(const dialogue_option *option, const std::string &identifier);

	void call(CPlayer *player, const context &ctx) const;
	void call_node(const int node_index, CPlayer *player, const context &ctx) const;
	void call_node_option_effect(const int node_index, const int option_index, CPlayer *player, const context &ctx) const;
	void call_node_option_effect(const int node_index, const int option_index, CPlayer *player) const;
	Q_INVOKABLE void call_node_option_effect_sync(const int node_index, const int option_index) const;

	void delete_lua_callbacks();
	
private:
	std::vector<std::unique_ptr<dialogue_node>> nodes;	/// The nodes of the dialogue
	std::map<std::string, const dialogue_node *> nodes_by_identifier;
	std::map<std::string, const dialogue_option *> options_by_identifier;

	friend int ::CclDefineDialogue(lua_State *l);
};

}

extern void CallDialogue(const std::string &dialogue_ident, int player);
extern void CallDialogueNode(const std::string &dialogue_ident, int node, int player);
extern void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player);
