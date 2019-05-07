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
/**@name dialogue_node.cpp - The dialogue node source file. */
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

#include "quest/dialogue_node.h"

#include "character.h"
#include "config.h"
#include "faction.h"
#include "luacallback.h"
#include "player.h"
#include "quest/dialogue.h"
#include "quest/dialogue_option.h"
#include "script.h"
#include "trigger/trigger_effect.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
	config_data->ProcessPropertiesForObject(*this);
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "option") {
			CDialogueOption *option = new CDialogueOption;
			option->Dialogue = this->Dialogue;
			option->ParentNode = this;
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

/**
**	@brief	Initialize the dialogue option
*/
void CDialogueNode::Initialize()
{
	for (CDialogueOption *option : this->Options) {
		option->Initialize();
	}
	
	//if no options have been defined for the dialogue node, create one that will simply continue to the next node
	if (this->Options.empty()) {
		CDialogueOption *option = new CDialogueOption;
		option->ParentNode = this;
		this->Options.push_back(option);
	}
}

void CDialogueNode::Call(CPlayer *player) const
{
	if (this->Conditions) {
		this->Conditions->pushPreamble();
		this->Conditions->run(1);
		if (this->Conditions->popBoolean() == false) {
			if (this->GetNextNode() != nullptr) {
				this->GetNextNode()->Call(player);
			}
			return;
		}
	}
	
	if (this->ImmediateEffectsLua) {
		this->ImmediateEffectsLua->pushPreamble();
		this->ImmediateEffectsLua->run();
	}
	
	for (const CTriggerEffect *effect : this->ImmediateEffects) {
		effect->Do(player);
	}
	
	std::string lua_command = "Event(";
	
	CPlayer *faction_player = nullptr;
	
	if (this->Faction != nullptr) {
		faction_player = GetFactionPlayer(this->Faction);
	}
	
	CUnit *speaker_unit = nullptr;
	
	if (this->Character != nullptr) {
		lua_command += "FindHero(\"" + this->Character->Ident;
		
		if (faction_player != nullptr) {
			speaker_unit = faction_player->GetHeroUnit(this->Character);
		} else {
			speaker_unit = this->Character->GetUnit();
		}
	} else if (this->UnitType != nullptr) {
		lua_command += "FindUnit(\"" + this->UnitType->Ident;
		
		if (faction_player != nullptr) {
			std::vector<CUnit *> unit_table;
			FindPlayerUnitsByType(*faction_player, *this->UnitType, unit_table);
			
			if (!unit_table.empty()) {
				speaker_unit = unit_table[SyncRand(unit_table.size())];
			}
		}
	} else {
		lua_command += "\"" + std::string(this->SpeakerName.utf8().get_data()) + "\", ";
	}
	
	if (this->Character != nullptr || this->UnitType != nullptr) {
		lua_command += "\"";
		if (this->Faction != nullptr) {
			lua_command += ", GetFactionPlayer(\"" + this->Faction->Ident + "\")";
		}
		lua_command += "), ";
	}
	
	lua_command += "\"" + FindAndReplaceString(FindAndReplaceString(this->Text.utf8().get_data(), "\"", "\\\""), "\n", "\\n") + "\", ";
	lua_command += std::to_string((long long) player->Index) + ", ";
	
	lua_command += "{";
	if (this->Options.size() > 0) {
		bool first = true;
		for (size_t i = 0; i < this->Options.size(); ++i) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "\"" + std::string(this->Options[i]->Name.utf8().get_data()) + "\"";
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
			lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->Ident + "\", " + std::to_string((long long) this->Index) + ", " + std::to_string((long long) i) + ", " + std::to_string((long long) player->Index) + ");";
			lua_command += " end";
		}
	} else {
		lua_command += "function(s) ";
		lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->Ident + "\", " + std::to_string((long long) this->Index) + ", " + std::to_string((long long) 0) + ", " + std::to_string((long long) player->Index) + ");";
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
			lua_command += "\"" + std::string(this->Options[i]->Tooltip.utf8().get_data()) + "\"";
		}
		lua_command += "}";
	}
	lua_command += "}";
	
	lua_command += ")";
	
	CclCommand(lua_command);
	
	if (player == CPlayer::GetThisPlayer()) {
		this->Dialogue->emit_signal("dialogue_node_changed", this, speaker_unit);
	}
}

void CDialogueNode::OptionEffect(const int option, CPlayer *player) const
{
	if ((int) this->Options.size() > option) {
		this->Options[option]->DoEffect(player);
	} else {
		if (this->GetNextNode() != nullptr) {
			this->GetNextNode()->Call(player);
		}
	}
}

void CDialogueNode::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_ident", "ident"), [](CDialogueNode *node, const String &ident){
		node->Ident = ident;
		node->Dialogue->NodesByIdent[node->Ident] = node;
	});
	ClassDB::bind_method(D_METHOD("get_ident"), [](const CDialogueNode *node){ return node->Ident; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ident"), "set_ident", "get_ident");
	
	ClassDB::bind_method(D_METHOD("set_speaker_name", "speaker_name"), [](CDialogueNode *node, const String &speaker_name){ node->SpeakerName = speaker_name; });
	ClassDB::bind_method(D_METHOD("get_speaker_name"), [](const CDialogueNode *node){ return node->SpeakerName; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "speaker_name"), "set_speaker_name", "get_speaker_name");

	ClassDB::bind_method(D_METHOD("set_text", "text"), [](CDialogueNode *node, const String &text){ node->Text = text; });
	ClassDB::bind_method(D_METHOD("get_text"), [](const CDialogueNode *node){ return node->Text; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");

	ClassDB::bind_method(D_METHOD("set_character", "character"), [](CDialogueNode *node, const String &character_ident){ node->Character = CCharacter::Get(character_ident); });
	ClassDB::bind_method(D_METHOD("get_character"), [](const CDialogueNode *node){ return const_cast<CCharacter *>(node->Character); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "character"), "set_character", "get_character");

	ClassDB::bind_method(D_METHOD("set_unit_type", "unit_type"), [](CDialogueNode *node, const String &unit_type_ident){ node->UnitType = CUnitType::Get(unit_type_ident); });
	ClassDB::bind_method(D_METHOD("get_unit_type"), [](const CDialogueNode *node){ return const_cast<CUnitType *>(node->UnitType); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "unit_type"), "set_unit_type", "get_unit_type");

	ClassDB::bind_method(D_METHOD("set_faction", "faction"), [](CDialogueNode *node, const String &faction_ident){ node->Faction = CFaction::Get(faction_ident); });
	ClassDB::bind_method(D_METHOD("get_faction"), [](const CDialogueNode *node){ return const_cast<CFaction *>(node->Faction); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "faction"), "set_faction", "get_faction");
	
	ClassDB::bind_method(D_METHOD("get_options"), [](const CDialogueNode *node){ return VectorToGodotArray(node->Options); });
}

void CallDialogueNode(const std::string &dialogue_ident, int node, int player)
{
	CDialogue *dialogue = CDialogue::Get(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	dialogue->Nodes[node]->Call(CPlayer::Players[player]);
}

void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player)
{
	CDialogue *dialogue = CDialogue::Get(dialogue_ident);
	if (!dialogue || node >= (int) dialogue->Nodes.size()) {
		return;
	}
	
	CclCommand("trigger_player = " + std::to_string((long long) player) + ";");
	dialogue->Nodes[node]->OptionEffect(option, CPlayer::Players[player]);
}
