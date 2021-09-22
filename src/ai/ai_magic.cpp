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
/**@name ai_magic.cpp - AI magic functions. */
//
//      (c) Copyright 2002-2005 by Lutz Sammer, Joris Dauphin
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

#include "ai_local.h"

#include "actions.h"
#include "player/player.h"
#include "spell/spell.h"
#include "unit/unit.h"
#include "unit/unit_type.h"

/**
**  Check what computer units can do with magic.
**  In fact, turn on autocast for AI.
*/
void AiCheckMagic()
{
	CPlayer &player = *AiPlayer->Player;
	const int n = player.GetUnitCount();

	for (int i = 0; i < n; ++i) {
		CUnit &unit = player.GetUnit(i);

		if (unit.Type->Spells.size() > 0) {
			// Check only idle magic units
			for (const std::unique_ptr<COrder> &order : unit.Orders) {
				if (order->Action == UnitAction::SpellCast) {
					return;
				}
			}
			for (unsigned int j = 0; j < unit.Type->Spells.size(); ++j) {
				wyrmgus::spell *spell = unit.Type->Spells[j];
				// Check if we can cast this spell. SpellIsAvailable checks for upgrades.
				if (spell->IsAvailableForUnit(unit) && spell->get_ai_cast_info() != nullptr) {
					if (AutoCastSpell(unit, *spell)) {
						break;
					}
				}
			}
		}
	}
}
