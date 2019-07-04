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
/**@name dialogue_option.h - The dialogue option header file. */
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

#ifndef __DIALOGUE_OPTION_H__
#define __DIALOGUE_OPTION_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <core/object.h>
#include <core/method_bind_free_func.gen.inc>
#include <core/ustring.h>

#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CDialogue;
class CDialogueNode;
class CPlayer;
class CTriggerEffect;
class LuaCallback;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CDialogueOption : public Object
{
	GDCLASS(CDialogueOption, Object)

public:
	~CDialogueOption();
	
	void ProcessConfigData(const CConfigData *config_data);
	void Initialize();
	void DoEffect(CPlayer *player) const;
	
	String Name;
	LuaCallback *EffectsLua = nullptr;
	String Tooltip;
	CDialogue *Dialogue = nullptr;
	const CDialogueNode *ParentNode = nullptr;
	std::vector<const CTriggerEffect *> Effects;
	std::vector<CDialogueNode *> ChildNodes;
	
	friend int CclDefineDialogue(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
