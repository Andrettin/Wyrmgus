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
/**@name animation.h - The animations header file. */
//
//      (c) Copyright 2005-2022 by Jimmy Salmon and Andrettin
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

#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "data_type.h"
#include "economy/resource_container.h"

class CUnit;
struct lua_State;

namespace wyrmgus {
	class animation_sequence;
}

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
	AnimationSetPlayerVar,
	AnimationDie,
	AnimationLuaCallback
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

class CAnimation
{
public:
	static inline std::vector<CAnimation *> animation_list;

	explicit CAnimation(const AnimationType type) : Type(type)
	{
	}

	virtual ~CAnimation() {}

	virtual void Action(CUnit &unit, int &move, int scale) const = 0;

	virtual void Init(const char *s, animation_sequence *sequence = nullptr)
	{
		Q_UNUSED(s);
		Q_UNUSED(sequence);
	}

	const CAnimation *get_next() const
	{
		return this->next;
	}

	void set_next(const CAnimation *animation)
	{
		if (animation == nullptr) {
			throw std::runtime_error("Tried to set a null animation pointer as the next animation of another animation.");
		}

		this->next = animation;
	}

	const AnimationType Type;
private:
	const CAnimation *next = nullptr;
};

extern void AnimationCclRegister();

/// Handle the animation of a unit
extern int UnitShowAnimationScaled(CUnit &unit, const CAnimation *anim, int scale);
/// Handle the animation of a unit
extern int UnitShowAnimation(CUnit &unit, const CAnimation *anim);

extern int ParseAnimInt(const CUnit &unit, const std::string &parseint);
extern int ParseAnimFlags(const CUnit &unit, const char *parseflag);
