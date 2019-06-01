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
/**@name animation.h - The animation header file. */
//
//      (c) Copyright 2005-2019 by Jimmy Salmon and Andrettin
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

#ifndef __ANIMATIONS_H__
#define __ANIMATIONS_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "upgrade/upgrade_structs.h" // MaxCost

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define ANIMATIONS_DEATHTYPES 40

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CFile;
class CUnit;
struct lua_State;

/**
**  Default names for the extra death types.
*/
extern std::string ExtraDeathTypes[ANIMATIONS_DEATHTYPES];

enum AnimationType {
	AnimationNone,
	AnimationFrame,
	AnimationExactFrame,
	AnimationWait,
	AnimationRandomWait,
	AnimationSound,
	AnimationRandomSound,
	AnimationAttack,
	AnimationRotate,
	AnimationRandomRotate,
	AnimationMove,
	AnimationUnbreakable,
	AnimationLabel,
	AnimationGoto,
	AnimationRandomGoto,
	AnimationSpawnMissile,
	AnimationIfVar,
	AnimationSetVar,
	AnimationDie
};

//Modify types
enum SetVar_ModifyTypes {
	modSet = 0,      /// Set value to this
	modAdd,          /// Addition
	modSub,          /// Subtraction
	modMul,          /// Multiplication
	modDiv,          /// Division
	modMod,          /// Modulo
	modAnd,          /// Bitwise AND
	modOr,           /// Bitwise OR
	modXor,          /// Bitwise XOR
	modNot,          /// Bitwise NOT
};

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CAnimation
{
public:
	CAnimation(AnimationType type) : Type(type) {}

	virtual ~CAnimation() {}

	virtual void Action(CUnit &unit, int &move, int scale) const = 0;
	virtual void Init(const char *s, lua_State *l = nullptr) {}

	const AnimationType Type;
	CAnimation *Next = nullptr;
};

class CAnimations : public DataElement
{
	GDCLASS(CAnimations, DataElement)
	
public:
	CAnimations()
	{
		memset(this->Death, 0, sizeof(this->Death));
		memset(this->Harvest, 0, sizeof(this->Harvest));
	}

	~CAnimations()
	{
		delete this->Attack;
		delete this->RangedAttack;
		delete this->Build;
		for (int i = 0; i < ANIMATIONS_DEATHTYPES + 1; ++i) {
			delete this->Death[i];
		}
		for (int i = 0; i < MaxCosts; ++i) {
			delete this->Harvest[i];
		}
		delete this->Move;
		delete this->Repair;
		delete this->Research;
		delete this->SpellCast;
		delete this->Start;
		delete this->Still;
		delete this->Train;
		delete this->Upgrade;
	}

	static void AddAnimationToArray(CAnimation *anim);
	static void SaveUnitAnim(CFile &file, const CUnit &unit);
	static void LoadUnitAnim(lua_State *l, CUnit &unit, int luaIndex);
	static void LoadWaitUnitAnim(lua_State *l, CUnit &unit, int luaIndex);

	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
public:
	CAnimation *Attack = nullptr;
	CAnimation *RangedAttack = nullptr;
	CAnimation *Build = nullptr;
	CAnimation *Death[ANIMATIONS_DEATHTYPES + 1];
	CAnimation *Harvest[MaxCosts];
	CAnimation *Move = nullptr;
	CAnimation *Repair = nullptr;
	CAnimation *Research = nullptr;
	CAnimation *SpellCast = nullptr;
	CAnimation *Start = nullptr;
	CAnimation *Still = nullptr;
	CAnimation *Train = nullptr;
	CAnimation *Upgrade = nullptr;

protected:
	static inline void _bind_methods() {}
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Get the animations structure by ident
extern CAnimations *AnimationsByIdent(const std::string &ident);

extern void AnimationCclRegister();

/// Handle the animation of a unit
extern int UnitShowAnimationScaled(CUnit &unit, const CAnimation *anim, int scale);
/// Handle the animation of a unit
extern int UnitShowAnimation(CUnit &unit, const CAnimation *anim);


extern int ParseAnimInt(const CUnit &unit, const char *parseint);
extern int ParseAnimFlags(const CUnit &unit, const char *parseflag);

extern void FindLabelLater(CAnimation **anim, const std::string &name);

extern void FreeAnimations();

//Wyrmgus start
extern std::map<std::string, CAnimations *> AnimationMap;
//Wyrmgus end

#endif
