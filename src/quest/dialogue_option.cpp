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
/**@name dialogue_option.cpp - The dialogue option source file. */
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

#include "quest/dialogue_option.h"

#include "config.h"
#include "luacallback.h"
#include "player.h"
#include "quest/dialogue.h"
#include "quest/dialogue_node.h"
#include "trigger/trigger_effect.h"
#include "unit/unit.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CDialogueOption::~CDialogueOption()
{
	if (this->EffectsLua) {
		delete this->EffectsLua;
	}
	
	for (const CDialogueNode *node : this->ChildNodes) {
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
	config_data->ProcessPropertiesForObject(*this);
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "node") {
			CDialogueNode *previous_node = nullptr;
			if (!this->ChildNodes.empty()) {
				previous_node = this->ChildNodes.back();
			}
			
			CDialogueNode *node = new CDialogueNode(this->Dialogue, previous_node);
			this->ChildNodes.push_back(node);
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

/**
**	@brief	Initialize the dialogue option
*/
void CDialogueOption::Initialize()
{
	for (CDialogueNode *node : this->ChildNodes) {
		node->Initialize();
	}
}

void CDialogueOption::DoEffect(CPlayer *player) const
{
	if (this->EffectsLua) {
		this->EffectsLua->pushPreamble();
		this->EffectsLua->run();
	}
	
	for (const CTriggerEffect *effect : this->Effects) {
		effect->Do(player);
	}

	if (!this->ChildNodes.empty()) {
		this->ChildNodes.back()->Call(player);
	} else if (this->ParentNode->GetNextNode() != nullptr) {
		this->ParentNode->GetNextNode()->Call(player);
	} else {
		//no further dialogue node, end the dialogue
		if (player == CPlayer::GetThisPlayer()) {
			this->Dialogue->emit_signal("dialogue_node_changed", static_cast<CDialogueNode *>(nullptr), static_cast<CUnit *>(nullptr));
		}
	}
}

void CDialogueOption::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_name", "name"), [](CDialogueOption *option, const String &name){ option->Name = name; });
	ClassDB::bind_method(D_METHOD("get_name"), [](const CDialogueOption *option){ return option->Name; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");

	ClassDB::bind_method(D_METHOD("set_tooltip", "tooltip"), [](CDialogueOption *option, const String &tooltip){ option->Tooltip = tooltip; });
	ClassDB::bind_method(D_METHOD("get_tooltip"), [](const CDialogueOption *option){ return option->Tooltip; });
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "tooltip"), "set_tooltip", "get_tooltip");
	
	ClassDB::bind_method(D_METHOD("do_effect"), [](const CDialogueOption *option){ return option->DoEffect(CPlayer::GetThisPlayer()); });
}
