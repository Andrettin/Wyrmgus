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
/**@name quest.cpp - The quests. */
//
//      (c) Copyright 2015 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "quest.h"

#include "luacallback.h"
#include "script.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CQuest *> Quests;
CQuest *CurrentQuest = NULL;
std::vector<CDialogue *> Dialogues;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CleanQuests()
{
	for (size_t i = 0; i < Quests.size(); ++i) {
		delete Quests[i];
	}
	Quests.clear();
}

void CleanDialogues()
{
	for (size_t i = 0; i < Dialogues.size(); ++i) {
		delete Dialogues[i];
	}
	Dialogues.clear();
}

CQuest *GetQuest(std::string quest_name)
{
	for (size_t i = 0; i < Quests.size(); ++i) {
		if (quest_name == Quests[i]->Name) {
			return Quests[i];
		}
	}
	return NULL;
}

CDialogue *GetDialogue(std::string dialogue_ident)
{
	for (size_t i = 0; i < Dialogues.size(); ++i) {
		if (dialogue_ident == Dialogues[i]->Ident) {
			return Dialogues[i];
		}
	}
	return NULL;
}

CDialogue::~CDialogue()
{
	for (size_t i = 0; i < this->Nodes.size(); ++i) {
		delete this->Nodes[i];
	}
}

void CDialogue::Call(int player)
{
	if (this->Nodes.size() == 0) {
		return;
	}
	
	this->Nodes[0]->Call(player);
}

CDialogueNode::~CDialogueNode()
{
	delete Conditions;
	
	for (size_t i = 0; i < this->OptionEffects.size(); ++i) {
		delete this->OptionEffects[i];
	}
}

void CDialogueNode::Call(int player)
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
	
	std::string lua_command = "Event(";
	
	if (this->SpeakerType == "character") {
		lua_command += "FindHero(\"" + this->Speaker + "\"), ";
	} else if (this->SpeakerType == "unit") {
		lua_command += "FindUnit(\"" + this->Speaker + "\"), ";
	} else {
		lua_command += "\"" + this->Speaker + "\", ";
	}
	
	lua_command += "\"" + this->Text + "\", ";
	lua_command += std::to_string((long long) player) + ", ";
	
	lua_command += "{\"";
	if ((this->ID + 1) < (int) this->Dialogue->Nodes.size()) {
		lua_command += "~!Continue";
	} else {
		lua_command += "~!OK";
	}
	lua_command += "\"}, ";
	
	lua_command += "{function(s) ";
	lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->Ident + "\", " + std::to_string((long long) this->ID) + ", " + std::to_string((long long) 0) + ", " + std::to_string((long long) player) + ");";
	lua_command += " end}";
	
	lua_command += ")";
	
	CclCommand(lua_command);
}

void CDialogueNode::OptionEffect(int option, int player)
{
	if ((int) this->OptionEffects.size() > option && this->OptionEffects[option]) {
		this->OptionEffects[option]->pushPreamble();
		this->OptionEffects[option]->run();
	}
	if ((this->ID + 1) < (int) this->Dialogue->Nodes.size()) {
		this->Dialogue->Nodes[this->ID + 1]->Call(player);
	}
}

void SetCurrentQuest(std::string quest_name)
{
	if (quest_name.empty()) {
		CurrentQuest = NULL;
	} else {
		CurrentQuest = GetQuest(quest_name);
	}
}

std::string GetCurrentQuest()
{
	if (!CurrentQuest) {
		return "";
	} else {
		return CurrentQuest->Name;
	}
}

void CallDialogue(std::string dialogue_ident, int player)
{
	CDialogue *dialogue = GetDialogue(dialogue_ident);
	if (!dialogue) {
		return;
	}
	
	dialogue->Call(player);
}

void CallDialogueNode(std::string dialogue_ident, int node, int player)
{
	CDialogue *dialogue = GetDialogue(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	dialogue->Nodes[node]->Call(player);
}

void CallDialogueNodeOptionEffect(std::string dialogue_ident, int node, int option, int player)
{
	CDialogue *dialogue = GetDialogue(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	dialogue->Nodes[node]->OptionEffect(option, player);
}

//@}
