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

#include "database/data_entry.h"
#include "database/data_type.h"

class CDialogueNode;
class LuaCallback;

namespace stratagus {

class dialogue final : public data_entry, public data_type<dialogue>
{
public:
	static constexpr const char *class_identifier = "dialogue";
	static constexpr const char *database_folder = "dialogues";

	dialogue(const std::string &identifier) : data_entry(identifier)
	{
	}

	~dialogue();
	
	void Call(const int player) const;
	
	std::vector<CDialogueNode *> Nodes;	/// The nodes of the dialogue
};

}

class CDialogueNode
{
public:
	~CDialogueNode();
	
	void Call(const int player) const;
	void OptionEffect(const int option, const int player) const;
	
	int ID = -1;
	std::string SpeakerType;			/// "character" if the speaker is a character, "unit" if the speaker belongs to a particular unit type, and empty if the Speaker string will be used as the displayed name of the speaker itself
	std::string SpeakerPlayer;			/// name of the player to whom the speaker belongs
	std::string Speaker;
	std::string Text;
	stratagus::dialogue *Dialogue = nullptr;
	LuaCallback *Conditions = nullptr;
	LuaCallback *ImmediateEffects = nullptr;
	std::vector<std::string> Options;
	std::vector<LuaCallback *> OptionEffects;
	std::vector<std::string> OptionTooltips;
};

extern void CallDialogue(const std::string &dialogue_ident, int player);
extern void CallDialogueNode(const std::string &dialogue_ident, int node, int player);
extern void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player);
