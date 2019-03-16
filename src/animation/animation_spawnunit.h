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
/**@name animation_spawnunit.h - The animation SpawnUnit headerfile. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#ifndef ANIMATION_SPAWNUNIT_H
#define ANIMATION_SPAWNUNIT_H

#include "animation/animation.h"

#include <string>

//SpawnUnit flags
enum SpawnUnit_Flags {
	SU_None = 0,           /// Clears all flags
	SU_Summoned = 1,       /// Unit is marked as "summoned"
	SU_JoinToAIForce = 2   /// Unit is included into spawner's AI force, if available
};

class CAnimation_SpawnUnit : public CAnimation
{
public:
	CAnimation_SpawnUnit() : CAnimation(AnimationSpawnUnit) {}

	virtual void Action(CUnit &unit, int &move, int scale) const;
	virtual void Init(const char *s, lua_State *l);

private:
	std::string unitTypeStr;
	std::string offXStr;
	std::string offYStr;
	std::string rangeStr;
	std::string playerStr;
	std::string flagsStr;
};

#endif
