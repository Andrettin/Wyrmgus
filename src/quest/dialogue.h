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

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CDialogueNode;
class LuaCallback;

class CDialogue
{
public:
	static CDialogue *GetDialogue(const std::string &ident, const bool should_find = true);
	static CDialogue *GetOrAddDialogue(const std::string &ident);
	static void ClearDialogues();
	
	static std::vector<CDialogue *> Dialogues;
	static std::map<std::string, CDialogue *> DialoguesByIdent;
	
	~CDialogue();
	
	void Call(const int player) const;
	
	std::string Ident;				/// Ident of the dialogue
	std::vector<CDialogueNode *> Nodes;	/// The nodes of the dialogue
};

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
	CDialogue *Dialogue = nullptr;
	LuaCallback *Conditions = nullptr;
	LuaCallback *ImmediateEffects = nullptr;
	std::vector<std::string> Options;
	std::vector<LuaCallback *> OptionEffects;
	std::vector<std::string> OptionTooltips;
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CallDialogue(const std::string &dialogue_ident, int player);
extern void CallDialogueNode(const std::string &dialogue_ident, int node, int player);
extern void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player);

#endif
