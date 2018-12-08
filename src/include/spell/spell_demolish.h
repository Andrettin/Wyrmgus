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
//
//      (c) Copyright 1999-2012 by Vladi Belperchinov-Shabanski,
//                                 Joris DAUPHIN, and Jimmy Salmon
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

#ifndef SPELL_DEMOLISH_H
#define SPELL_DEMOLISH_H

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "spells.h"

class Spell_Demolish : public SpellActionType
{
public:
	//Wyrmgus start
//	Spell_Demolish() : Damage(0), Range(0) {};
	Spell_Demolish() : Damage(0), BasicDamage(0), PiercingDamage(0), FireDamage(0), ColdDamage(0), ArcaneDamage(0), LightningDamage(0),
	AirDamage(0), EarthDamage(0), WaterDamage(0), Range(0),
	HackDamage(true), PierceDamage(true), BluntDamage(true), DamageSelf(true), DamageFriendly(true), DamageTerrain(true) {};
	//Wyrmgus end
	virtual int Cast(CUnit &caster, const CSpell &spell,
					 CUnit *target, const Vec2i &goalPos, int z, int modifier);
	virtual void Parse(lua_State *l, int startIndex, int endIndex);

private:
	int Damage; /// Damage for every unit in range.
	int Range;  /// Range of the explosion.
	//Wyrmgus start
	int BasicDamage; /// Basic damage for every unit in range.
	int PiercingDamage; /// Piercing damage for every unit in range.
	int FireDamage;
	int ColdDamage;
	int ArcaneDamage;
	int LightningDamage;
	int AirDamage;
	int EarthDamage;
	int WaterDamage;
	bool HackDamage;
	bool PierceDamage;
	bool BluntDamage;
	bool DamageSelf;   /// If true, damages self when casting spell
	bool DamageFriendly;   /// If true, damages friendly units when casting spell
	bool DamageTerrain;   /// If true, damages terrain when casting spell
	//Wyrmgus end
};


//@}

#endif // SPELL_DEMOLISH_H
