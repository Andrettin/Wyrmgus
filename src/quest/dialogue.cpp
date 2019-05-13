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

#include "config.h"
#include "player.h"
#include "quest/dialogue_node.h"
#include "wyrmgus.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

const CDialogue *CDialogue::CampaignVictoryDialogue = nullptr;
const CDialogue *CDialogue::CampaignDefeatDialogue = nullptr;

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
**	@brief	Remove the existing dialogues
*/
void CDialogue::Clear()
{
	CDialogue::CampaignVictoryDialogue = nullptr;
	CDialogue::CampaignDefeatDialogue = nullptr;
	
	DataType<CDialogue>::Clear();
}

/**
**	@brief	Get the campaign victory dialogue
**
**	@return	The campaign victory dialogue
*/
const CDialogue *CDialogue::GetCampaignVictoryDialogue()
{
	return CDialogue::CampaignVictoryDialogue;
}

/**
**	@brief	Get the campaign defeat dialogue
**
**	@return	The campaign defeat dialogue
*/
const CDialogue *CDialogue::GetCampaignDefeatDialogue()
{
	return CDialogue::CampaignDefeatDialogue;
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
	if (section->Tag == "node") {
		CDialogueNode *previous_node = nullptr;
		if (!this->Nodes.empty()) {
			previous_node = this->Nodes.back();
		}
		
		CDialogueNode *node = new CDialogueNode(this, previous_node);
		node->Index = this->Nodes.size();
		this->Nodes.push_back(node);
		node->ProcessConfigData(section);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the dialogue
*/
void CDialogue::Initialize()
{
	for (CDialogueNode *node : this->Nodes) {
		node->Initialize();
	}
	
	this->Initialized = true;
}

void CDialogue::Call(CPlayer *player) const
{
	if (this->Nodes.empty()) {
		return;
	}
	
	if (player == CPlayer::GetThisPlayer()) {
		// for the "this" player, wait for the dialogue panel to give the go-ahead for calling the first node
		Wyrmgus::GetInstance()->emit_signal("dialogue_called", this);
	} else {
		this->Nodes.front()->Call(player);
	}
}

void CDialogue::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_campaign_victory_dialogue", "campaign_victory_dialogue"), [](CDialogue *dialogue, const bool campaign_victory_dialogue){
		CDialogue::CampaignVictoryDialogue = dialogue;
	});
	ClassDB::bind_method(D_METHOD("is_campaign_victory_dialogue"), [](const CDialogue *dialogue){
		return CDialogue::GetCampaignVictoryDialogue() == dialogue;
	});
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "campaign_victory_dialogue"), "set_campaign_victory_dialogue", "is_campaign_victory_dialogue");
	
	ClassDB::bind_method(D_METHOD("set_campaign_defeat_dialogue", "campaign_defeat_dialogue"), [](CDialogue *dialogue, const bool campaign_defeat_dialogue){
		CDialogue::CampaignDefeatDialogue = dialogue;
	});
	ClassDB::bind_method(D_METHOD("is_campaign_defeat_dialogue"), [](const CDialogue *dialogue){
		return CDialogue::GetCampaignDefeatDialogue() == dialogue;
	});
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "campaign_defeat_dialogue"), "set_campaign_defeat_dialogue", "is_campaign_defeat_dialogue");
	
	ClassDB::bind_method(D_METHOD("call_first_node"), [](const CDialogue *dialogue){
		if (!dialogue->Nodes.empty()) {
			dialogue->Nodes.front()->Call(CPlayer::GetThisPlayer());
		}
	});
	
	//signals that the dialogue node has changed; if nullptr is provided as the dialogue node, then that indicates that the dialogue has ended
	ADD_SIGNAL(MethodInfo("dialogue_node_changed", PropertyInfo(Variant::OBJECT, "dialogue_node"), PropertyInfo(Variant::OBJECT, "speaker_unit")));
}

void CallDialogue(const std::string &dialogue_ident, int player)
{
	CDialogue *dialogue = CDialogue::Get(dialogue_ident);
	if (!dialogue) {
		return;
	}
	
	dialogue->Call(CPlayer::Players[player]);
}

