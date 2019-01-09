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
/**@name dialogue.cpp - The dialogue source file. */
//
//      (c) Copyright 2015-2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "dialogue.h"

#include "luacallback.h"
#include "script.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CDialogue *> CDialogue::Dialogues;
std::map<std::string, CDialogue *> CDialogue::DialoguesByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CDialogue *CDialogue::GetDialogue(const std::string &ident, const bool should_find)
{
	std::map<std::string, CDialogue *>::const_iterator find_iterator = CDialogue::DialoguesByIdent.find(ident);
	
	if (find_iterator != CDialogue::DialoguesByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid dialogue: \"%s\".\n", ident.c_str());
	}

	return nullptr;
}

CDialogue *CDialogue::GetOrAddDialogue(const std::string &ident)
{
	CDialogue *dialogue = CDialogue::GetDialogue(ident, false);
	
	if (!dialogue) {
		dialogue = new CDialogue;
		dialogue->Ident = ident;
		CDialogue::Dialogues.push_back(dialogue);
		CDialogue::DialoguesByIdent[ident] = dialogue;
	}
	
	return dialogue;
}

void CDialogue::ClearDialogues()
{
	for (CDialogue *dialogue : CDialogue::Dialogues) {
		delete dialogue;
	}
	
	CDialogue::Dialogues.clear();
	CDialogue::DialoguesByIdent.clear();
}

CDialogue::~CDialogue()
{
	for (CDialogueNode *node : this->Nodes) {
		delete node;
	}
}

void CDialogue::Call(const int player) const
{
	if (this->Nodes.empty()) {
		return;
	}
	
	this->Nodes[0]->Call(player);
}

CDialogueNode::~CDialogueNode()
{
	if (this->Conditions) {
		delete Conditions;
	}
	
	if (this->ImmediateEffects) {
		delete ImmediateEffects;
	}
	
	for (LuaCallback *option_effect : this->OptionEffects) {
		delete option_effect;
	}
}

void CDialogueNode::Call(const int player) const
{
	if (this->Conditions) {
		this->Conditions->pushPreamble();
		this->Conditions->run(1);
		if (this->Conditions->popBoolean() == false) {
			if ((this->ID + 1) < (int) this->Dialogue->Nodes.size()) {
				this->Dialogue->Nodes[this->ID + 1]->Call(player);
			}
			return;
		}
	}
	
	if (this->ImmediateEffects) {
		this->ImmediateEffects->pushPreamble();
		this->ImmediateEffects->run();
	}
	
	std::string lua_command = "Event(";
	
	if (this->SpeakerType == "character") {
		lua_command += "FindHero(\"" + this->Speaker;
	} else if (this->SpeakerType == "unit") {
		lua_command += "FindUnit(\"" + this->Speaker;
	} else {
		lua_command += "\"" + this->Speaker + "\", ";
	}
	
	if (this->SpeakerType == "character" || this->SpeakerType == "unit") {
		lua_command += "\"";
		if (!this->SpeakerPlayer.empty()) {
			lua_command += ", GetFactionPlayer(\"" + this->SpeakerPlayer + "\")";
		}
		lua_command += "), ";
	}
	
	lua_command += "\"" + FindAndReplaceString(FindAndReplaceString(this->Text, "\"", "\\\""), "\n", "\\n") + "\", ";
	lua_command += std::to_string((long long) player) + ", ";
	
	lua_command += "{";
	if (this->Options.size() > 0) {
		bool first = true;
		for (size_t i = 0; i < this->Options.size(); ++i) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "\"" + this->Options[i] + "\"";
		}
	} else {
		lua_command += "\"~!Continue\"";
	}
	lua_command += "}, ";
	
	lua_command += "{";
	if (this->Options.size() > 0) {
		bool first = true;
		for (size_t i = 0; i < this->Options.size(); ++i) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "function(s) ";
			lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->Ident + "\", " + std::to_string((long long) this->ID) + ", " + std::to_string((long long) i) + ", " + std::to_string((long long) player) + ");";
			lua_command += " end";
		}
	} else {
		lua_command += "function(s) ";
		lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->Ident + "\", " + std::to_string((long long) this->ID) + ", " + std::to_string((long long) 0) + ", " + std::to_string((long long) player) + ");";
		lua_command += " end";
	}
	lua_command += "}, ";

	lua_command += "nil, nil, nil, ";
	
	lua_command += "{";
	if (this->OptionTooltips.size() > 0) {
		lua_command += "OptionTooltips = {";
		bool first = true;
		for (size_t i = 0; i < this->OptionTooltips.size(); ++i) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "\"" + this->OptionTooltips[i] + "\"";
		}
		lua_command += "}";
	}
	lua_command += "}";
	
	lua_command += ")";
	
	CclCommand(lua_command);
}

void CDialogueNode::OptionEffect(const int option, const int player) const
{
	if ((int) this->OptionEffects.size() > option && this->OptionEffects[option]) {
		this->OptionEffects[option]->pushPreamble();
		this->OptionEffects[option]->run();
	}
	if ((this->ID + 1) < (int) this->Dialogue->Nodes.size()) {
		this->Dialogue->Nodes[this->ID + 1]->Call(player);
	}
}

void CallDialogue(const std::string &dialogue_ident, int player)
{
	CDialogue *dialogue = CDialogue::GetDialogue(dialogue_ident);
	if (!dialogue) {
		return;
	}
	
	dialogue->Call(player);
}

void CallDialogueNode(const std::string &dialogue_ident, int node, int player)
{
	CDialogue *dialogue = CDialogue::GetDialogue(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	dialogue->Nodes[node]->Call(player);
}

void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player)
{
	CDialogue *dialogue = CDialogue::GetDialogue(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	CclCommand("trigger_player = " + std::to_string((long long) player) + ";");
	dialogue->Nodes[node]->OptionEffect(option, player);
}
