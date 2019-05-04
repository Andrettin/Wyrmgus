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

#include "quest/dialogue.h"

#include "character.h"
#include "faction.h"
#include "luacallback.h"
#include "script.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CDialogue::~CDialogue()
{
	for (const CDialogueNode *node : this->Nodes) {
		delete node;
	}
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CDialogue::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "dialogue_node") {
		CDialogueNode *node = new CDialogueNode;
		node->Index = this->Nodes.size();
		node->Dialogue = this;
		this->Nodes.push_back(node);
		node->ProcessConfigData(section);
	} else {
		return false;
	}
	
	return true;
}

void CDialogue::Call(const int player) const
{
	if (this->Nodes.empty()) {
		return;
	}
	
	this->Nodes[0]->Call(player);
}

void CDialogue::_bind_methods()
{
}

CDialogueNode::~CDialogueNode()
{
	if (this->Conditions) {
		delete Conditions;
	}
	
	if (this->ImmediateEffectsLua) {
		delete ImmediateEffectsLua;
	}
	
	for (const CDialogueOption *option : this->Options) {
		delete option;
	}
	
	for (const CTriggerEffect *effect : this->ImmediateEffects) {
		delete effect;
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDialogueNode::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		if (property.Key == "ident") {
			this->Ident = property.Value;
			this->Dialogue->NodesByIdent[this->Ident] = this;
		} else if (property.Key == "character") {
			this->Character = CCharacter::Get(property.Value);
		} else if (property.Key == "unit_type") {
			this->UnitType = CUnitType::Get(property.Value);
		} else if (property.Key == "faction") {
			this->Faction = CFaction::Get(property.Value);
		} else if (property.Key == "speaker_name") {
			this->SpeakerName = property.Value;
		} else if (property.Key == "text") {
			this->Text = property.Value;
		} else {
			fprintf(stderr, "Invalid dialogue node property: \"%s\".\n", property.Key.c_str());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "option") {
			CDialogueOption *option = new CDialogueOption;
			option->Dialogue = this->Dialogue;
			this->Options.push_back(option);
			option->ProcessConfigData(section);
		} else if (section->Tag == "effects") {
			for (const CConfigData *subsection : section->Sections) {
				const CTriggerEffect *trigger_effect = CTriggerEffect::FromConfigData(subsection);
				
				this->ImmediateEffects.push_back(trigger_effect);
			}
		} else {
			fprintf(stderr, "Invalid dialogue node section: \"%s\".\n", section->Tag.c_str());
		}
	}
}

void CDialogueNode::Call(const int player) const
{
	if (this->Conditions) {
		this->Conditions->pushPreamble();
		this->Conditions->run(1);
		if (this->Conditions->popBoolean() == false) {
			if ((this->Index + 1) < (int) this->Dialogue->Nodes.size()) {
				this->Dialogue->Nodes[this->Index + 1]->Call(player);
			}
			return;
		}
	}
	
	if (this->ImmediateEffectsLua) {
		this->ImmediateEffectsLua->pushPreamble();
		this->ImmediateEffectsLua->run();
	}
	
	for (const CTriggerEffect *effect : this->ImmediateEffects) {
		effect->Do(CPlayer::Players[player]);
	}
	
	std::string lua_command = "Event(";
	
	if (this->Character != nullptr) {
		lua_command += "FindHero(\"" + this->Character->Ident;
	} else if (this->UnitType != nullptr) {
		lua_command += "FindUnit(\"" + this->UnitType->Ident;
	} else {
		lua_command += "\"" + this->SpeakerName + "\", ";
	}
	
	if (this->Character != nullptr || this->UnitType != nullptr) {
		lua_command += "\"";
		if (this->Faction != nullptr) {
			lua_command += ", GetFactionPlayer(\"" + this->Faction->Ident + "\")";
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
			lua_command += "\"" + this->Options[i]->Name + "\"";
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
			lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->Ident + "\", " + std::to_string((long long) this->Index) + ", " + std::to_string((long long) i) + ", " + std::to_string((long long) player) + ");";
			lua_command += " end";
		}
	} else {
		lua_command += "function(s) ";
		lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->Ident + "\", " + std::to_string((long long) this->Index) + ", " + std::to_string((long long) 0) + ", " + std::to_string((long long) player) + ");";
		lua_command += " end";
	}
	lua_command += "}, ";

	lua_command += "nil, nil, nil, ";
	
	lua_command += "{";
	if (this->Options.size() > 0) {
		lua_command += "OptionTooltips = {";
		bool first = true;
		for (size_t i = 0; i < this->Options.size(); ++i) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "\"" + this->Options[i]->Tooltip + "\"";
		}
		lua_command += "}";
	}
	lua_command += "}";
	
	lua_command += ")";
	
	CclCommand(lua_command);
}

void CDialogueNode::OptionEffect(const int option, const int player) const
{
	if ((int) this->Options.size() > option && this->Options[option]->EffectsLua) {
		this->Options[option]->EffectsLua->pushPreamble();
		this->Options[option]->EffectsLua->run();
	}
	
	for (const CTriggerEffect *effect : this->Effects) {
		effect->Do(CPlayer::Players[player]);
	}
	
	if ((this->Index + 1) < (int) this->Dialogue->Nodes.size()) {
		this->Dialogue->Nodes[this->Index + 1]->Call(player);
	}
}

CDialogueOption::~CDialogueOption()
{
	if (this->EffectsLua) {
		delete this->EffectsLua;
	}
	
	for (const CDialogueNode *node : this->Nodes) {
		delete node;
	}
	
	for (const CTriggerEffect *effect : this->Effects) {
		delete effect;
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDialogueOption::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		if (property.Key == "name") {
			this->Name = property.Value;
		} else if (property.Key == "tooltip") {
			this->Tooltip = property.Value;
		} else {
			fprintf(stderr, "Invalid dialogue option property: \"%s\".\n", property.Key.c_str());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "node") {
			CDialogueNode *node = new CDialogueNode;
			node->Dialogue = this->Dialogue;
			this->Nodes.push_back(node);
			node->ProcessConfigData(section);
		} else if (section->Tag == "effects") {
			for (const CConfigData *subsection : section->Sections) {
				const CTriggerEffect *trigger_effect = CTriggerEffect::FromConfigData(subsection);
				
				this->Effects.push_back(trigger_effect);
			}
		} else {
			fprintf(stderr, "Invalid dialogue option section: \"%s\".\n", section->Tag.c_str());
		}
	}
}

void CallDialogue(const std::string &dialogue_ident, int player)
{
	CDialogue *dialogue = CDialogue::Get(dialogue_ident);
	if (!dialogue) {
		return;
	}
	
	dialogue->Call(player);
}

void CallDialogueNode(const std::string &dialogue_ident, int node, int player)
{
	CDialogue *dialogue = CDialogue::Get(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	dialogue->Nodes[node]->Call(player);
}

void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player)
{
	CDialogue *dialogue = CDialogue::Get(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	CclCommand("trigger_player = " + std::to_string((long long) player) + ";");
	dialogue->Nodes[node]->OptionEffect(option, player);
}
