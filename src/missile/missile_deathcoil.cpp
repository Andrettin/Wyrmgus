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
/**@name missile_deathcoil.cpp - The missile DeathCoil. */
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

#include "stratagus.h"

#include "missile.h"

#include "actions.h"
#include "map/map.h"
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "util/assert_util.h"

/**
**  Death-Coil class. Damages organic units and gives to the caster.
**
**  @todo  do it configurable.
*/
void MissileDeathCoil::Action()
{
	this->Wait = this->Type->get_sleep();
	if (PointToPointMissile(*this) == false) {
		return;
	}
	if (this->NextMissileFrame(1, 0) == false) {
		return;
	}
	assert_throw(this->get_source_unit() != nullptr);
	CUnit &source = *this->get_source_unit();

	if (source.Destroyed) {
		return;
	}
	// source unit still exists
	//
	// Target unit still exists and casted on a special target
	//
	if (this->get_target_unit() && this->get_target_unit()->IsAlive()) {
		HitUnit(&source, *this->get_target_unit(), this->Damage);
		if (source.CurrentAction() != UnitAction::Die) {
			source.Variable[HP_INDEX].Value += this->Damage;
			//Wyrmgus start
//			source.Variable[HP_INDEX].Value = std::min(source.Variable[HP_INDEX].Max, source.Variable[HP_INDEX].Value);
			source.Variable[HP_INDEX].Value = std::min(source.GetModifiedVariable(HP_INDEX, VariableAttribute::Max), source.Variable[HP_INDEX].Value);
			//Wyrmgus end
		}
	}
	this->TTL = 0;
}
