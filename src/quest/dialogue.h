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
/**@name dialogue.h - The dialogue header file. */
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

#ifndef __DIALOGUE_H__
#define __DIALOGUE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CDialogueNode;
class CDialogueOption;
class CFaction;
class CTriggerEffect;
class CUnitType;
class LuaCallback;

class CDialogue : public DataElement, public DataType<CDialogue>
{
	DATA_TYPE(CDialogue, DataElement)
	
public:
	~CDialogue();
	
	static constexpr const char *ClassIdentifier = "dialogue";
	
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	
	void Call(const int player) const;
	
	std::vector<const CDialogueNode *> Nodes;	/// the nodes of the dialogue
	std::map<std::string, const CDialogueNode *> NodesByIdent;	/// the nodes of the dialogue which have an ident, mapped to it
	
protected:
	static void _bind_methods();
};

class CDialogueNode
{
public:
	~CDialogueNode();
	
	void ProcessConfigData(const CConfigData *config_data);
	
	void Call(const int player) const;
	void OptionEffect(const int option, const int player) const;
	
	std::string Ident;						/// identifier for the node, needed for links, but not necessary for all nodes
	int Index = -1;
	const CCharacter *Character = nullptr;	/// the speaker character
	const CUnitType *UnitType = nullptr;	/// the speaker unit type (the speaker will be a random unit of this type)
	const CFaction *Faction = nullptr;		/// the faction to which the speaker belongs
	std::string SpeakerName;				/// speaker name, replaces the name of the character or unit
	std::string Text;
	CDialogue *Dialogue = nullptr;
	LuaCallback *Conditions = nullptr;
	LuaCallback *ImmediateEffectsLua = nullptr;
	std::vector<const CTriggerEffect *> ImmediateEffects;
	std::vector<CDialogueOption *> Options;
};

class CDialogueOption
{
public:
	~CDialogueOption();
	
	void ProcessConfigData(const CConfigData *config_data);
	
	std::string Name;
	LuaCallback *EffectsLua;
	std::string Tooltip;
	CDialogue *Dialogue = nullptr;
	std::vector<const CTriggerEffect *> Effects;
	std::vector<const CDialogueNode *> Nodes;
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CallDialogue(const std::string &dialogue_ident, int player);
extern void CallDialogueNode(const std::string &dialogue_ident, int node, int player);
extern void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player);

#endif
