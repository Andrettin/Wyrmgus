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
//      (c) Copyright 1999-2019 by Vladi Belperchinov-Shabanski,
//                                 Joris DAUPHIN, Jimmy Salmon and Andrettin
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

#ifndef SPELL_POLYMORPH_H
#define SPELL_POLYMORPH_H

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "spell/spells.h"

class CCivilization;
class CFaction;

class Spell_Polymorph : public SpellActionType
{
public:
	Spell_Polymorph() : SpellActionType(1) {};
	virtual void ProcessConfigData(const CConfigData *config_data) override {}
	virtual int Cast(CUnit &caster, const CSpell &spell,
					 CUnit *target, const Vec2i &goalPos, int z, int modifier);
	virtual void Parse(lua_State *l, int startIndex, int endIndex);

private:
	CUnitType *NewForm = nullptr;	/// The new form
	int PlayerNeutral = 0;			/// Convert the unit to the neutral player, or to the caster's player.
	//Wyrmgus start
	CCivilization *Civilization = nullptr;	/// For using with the Faction value.
	const CFaction *Faction = nullptr;	/// If the unit should be transformed in its faction equivalent.
	bool Detachment = false;		/// If the unit should be transformed from its faction-specific type to the generic civilization equivalent.
	//Wyrmgus end
	// TODO: temporary polymorphs would be awesome, but hard to implement
};

#endif
