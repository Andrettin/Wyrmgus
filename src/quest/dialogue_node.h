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
/**@name dialogue_node.h - The dialogue node header file. */
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

#ifndef __DIALOGUE_NODE_H__
#define __DIALOGUE_NODE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#pragma warning(push, 0)
#include <core/object.h>
#include <core/ustring.h>
#pragma warning(pop)

#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CConfigData;
class CDialogue;
class CDialogueOption;
class CFaction;
class CPlayer;
class CTriggerEffect;
class CUnitType;
class LuaCallback;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CDialogueNode : public Object
{
	GDCLASS(CDialogueNode, Object)
	
public:
	CDialogueNode(CDialogue *dialogue = nullptr, CDialogueNode *previous_node = nullptr) : Dialogue(dialogue)
	{
		if (previous_node != nullptr) {
			previous_node->NextNode = this;
		}
	}

	~CDialogueNode();
	
	void ProcessConfigData(const CConfigData *config_data);
	void Initialize();
	
	void Call(CPlayer *player) const;
	
	const CDialogueNode *GetNextNode() const
	{
		return this->NextNode;
	}
	
	String Ident;							/// identifier for the node, needed for links, but not necessary for all nodes
	int Index = -1;
	const CCharacter *Character = nullptr;	/// the speaker character
	const CUnitType *UnitType = nullptr;	/// the speaker unit type (the speaker will be a random unit of this type)
	const CFaction *Faction = nullptr;		/// the faction to which the speaker belongs
	String SpeakerName;						/// speaker name, replaces the name of the character or unit
	String Text;
	CDialogue *Dialogue = nullptr;
private:
	const CDialogueNode *NextNode = nullptr;
public:
	LuaCallback *Conditions = nullptr;
	LuaCallback *ImmediateEffectsLua = nullptr;
	std::vector<const CTriggerEffect *> ImmediateEffects;
	std::vector<CDialogueOption *> Options;
	
	friend int CclDefineDialogue(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
