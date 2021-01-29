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
//      (c) Copyright 2005-2021 by Jimmy Salmon and Andrettin
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

#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "data_type.h"
#include "upgrade/upgrade_structs.h" // MaxCost

constexpr int ANIMATIONS_DEATHTYPES = 40;

class CFile;
class CUnit;
struct lua_State;

namespace wyrmgus {
	class animation_set;
}

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
	AnimationSpawnUnit,
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

	explicit CAnimation(AnimationType type) : Type(type) {}

	virtual ~CAnimation() {}

	virtual void Action(CUnit &unit, int &move, int scale) const = 0;

	virtual void Init(const char *s, lua_State *l = nullptr)
	{
		Q_UNUSED(s)
		Q_UNUSED(l)
	}

	CAnimation *get_next() const
	{
		return this->next_ptr;
	}

	void set_next(std::unique_ptr<CAnimation> &&animation)
	{
		this->next = std::move(animation);
		this->set_next(this->next.get());
	}

	void set_next(CAnimation *animation)
	{
		if (animation == nullptr) {
			throw std::runtime_error("Tried to set a null animation pointer as the next animation of another animation.");
		}

		this->next_ptr = animation;
	}

	const AnimationType Type;
private:
	std::unique_ptr<CAnimation> next;
	CAnimation *next_ptr = nullptr; //non-owning next pointer, needed to circle back to the beginning
};

namespace wyrmgus {

class animation_set final : public data_entry, public data_type<animation_set>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "animation_set";
	static constexpr const char *database_folder = "animation_sets";

	static void clear()
	{
		CAnimation::animation_list.clear();
		data_type::clear();
	}

	explicit animation_set(const std::string &identifier) : data_entry(identifier)
	{
	}

	~animation_set()
	{
	}

	static void AddAnimationToArray(CAnimation *anim);
	static void SaveUnitAnim(CFile &file, const CUnit &unit);
	static void LoadUnitAnim(lua_State *l, CUnit &unit, int luaIndex);
	static void LoadWaitUnitAnim(lua_State *l, CUnit &unit, int luaIndex);

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	
public:
	std::unique_ptr<CAnimation> Attack;
	std::unique_ptr<CAnimation> RangedAttack;
	std::unique_ptr<CAnimation> Build;
	std::unique_ptr<CAnimation> Death[ANIMATIONS_DEATHTYPES + 1];
	std::unique_ptr<CAnimation> Harvest[MaxCosts];
	std::unique_ptr<CAnimation> Move;
	std::unique_ptr<CAnimation> Repair;
	std::unique_ptr<CAnimation> Research;
	std::unique_ptr<CAnimation> SpellCast;
	std::unique_ptr<CAnimation> Start;
	std::unique_ptr<CAnimation> Still;
	std::unique_ptr<CAnimation> Train;
	std::unique_ptr<CAnimation> Upgrade;
};

}

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void AnimationCclRegister();

/// Handle the animation of a unit
extern int UnitShowAnimationScaled(CUnit &unit, const CAnimation *anim, int scale);
/// Handle the animation of a unit
extern int UnitShowAnimation(CUnit &unit, const CAnimation *anim);

extern int ParseAnimInt(const CUnit &unit, const std::string &parseint);
extern int ParseAnimFlags(const CUnit &unit, const char *parseflag);

extern void FindLabelLater(CAnimation **anim, const std::string &name);
