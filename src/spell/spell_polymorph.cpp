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
//      (c) Copyright 1998-2022 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Joris Dauphin and Andrettin
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

#include "spell/spell_polymorph.h"

//Wyrmgus start
#include "action/action_upgradeto.h"
#include "character.h"
//Wyrmgus end
#include "game/game.h"
#include "map/map.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "script.h"
#include "spell/spell.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "util/vector_util.h"

void Spell_Polymorph::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "new-form")) {
			value = LuaToString(l, -1, j + 1);
			this->NewForm = wyrmgus::unit_type::get(value);
			// FIXME: temp polymorphs? hard to do.
		} else if (!strcmp(value, "player-neutral")) {
			this->PlayerNeutral = 1;
			--j;
		} else if (!strcmp(value, "player-caster")) {
			this->PlayerNeutral = 2;
			--j;
		//Wyrmgus start
		} else if (!strcmp(value, "civilization")) {
			value = LuaToString(l, -1, j + 1);
			wyrmgus::civilization *civilization = wyrmgus::civilization::get(value);
			this->civilization = civilization;
		} else if (!strcmp(value, "faction")) {
			value = LuaToString(l, -1, j + 1);
			this->Faction = faction::get(value)->ID;
		} else if (!strcmp(value, "detachment")) {
			this->Detachment = true;
			--j;
		//Wyrmgus end
		} else {
			LuaError(l, "Unsupported polymorph tag: %s" _C_ value);
		}
	}
	// Now, checking value.
	//Wyrmgus start
//	if (this->NewForm == nullptr) {
	if (this->NewForm == nullptr && this->civilization == nullptr && this->Faction == -1 && !this->Detachment) {
	//Wyrmgus end
		LuaError(l, "Use a unittype for polymorph (with new-form)");
	}
}

/**
** Cast polymorph.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Spell_Polymorph::Cast(CUnit &caster, const wyrmgus::spell &spell, CUnit *target, const Vec2i &goalPos, const int z, const int modifier)
{
	Q_UNUSED(goalPos)
	Q_UNUSED(z)
	Q_UNUSED(modifier)

	if (!target) {
		return 0;
	}

	wyrmgus::unit_type *type = this->NewForm;
	//Wyrmgus start
	if (this->NewForm == nullptr) {
		wyrmgus::unit_type *new_unit_type = nullptr;
		if (this->civilization != nullptr && this->Faction != -1 && this->civilization == target->Type->get_civilization()) { //get faction equivalent, if is of the same civilization
			new_unit_type = wyrmgus::faction::get_all()[this->Faction]->get_class_unit_type(target->Type->get_unit_class());
		} else if (this->civilization != nullptr && this->civilization != target->Type->get_civilization()) {
			new_unit_type = this->civilization->get_class_unit_type(target->Type->get_unit_class());
		}
		if (this->Detachment && target->Type->get_civilization() != nullptr && target->Type->get_faction() != nullptr) {
			new_unit_type = target->Type->get_civilization()->get_class_unit_type(target->Type->get_unit_class());
		}
		if (new_unit_type != nullptr) {
			type = new_unit_type;
		}
	}

	if (game::get()->is_persistency_enabled() && target->get_character() != nullptr && target->get_character()->is_custom() && target->get_character()->get_civilization() && this->civilization != nullptr && this->civilization != target->get_character()->get_civilization() && target->Player == CPlayer::GetThisPlayer()) {
		target->get_character()->civilization = this->civilization;
		target->get_character()->save();
	}
	if (type == nullptr) {
		return 0;
	}
//	const Vec2i pos(goalPos - type.GetHalfTileSize());
	//Wyrmgus end

	if (caster.is_enemy_of(*target)) {
		HitUnit_IncreaseScoreForKill(caster, *target, false);
	}

	// as said somewhere else -- no corpses :)
	//Wyrmgus start
//	target->Remove(nullptr);
//	Vec2i offset;
	//Wyrmgus end
	caster.Variable[MANA_INDEX].Value -= spell.get_mana_cost();
	//Wyrmgus start
//	Vec2i resPos;
//	FindNearestDrop(type, pos, resPos, LookingW);
	TransformUnitIntoType(*target, *type);
	//Wyrmgus end
	if (this->PlayerNeutral == 1) {
		//Wyrmgus start
//		MakeUnitAndPlace(resPos, type, CPlayer::get_neutral_player());
		target->ChangeOwner(*CPlayer::get_neutral_player());
		//Wyrmgus end
	} else if (this->PlayerNeutral == 2) {
		//Wyrmgus start
//		MakeUnitAndPlace(resPos, type, caster.Player);
		target->ChangeOwner(*caster.Player);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		MakeUnitAndPlace(resPos, type, target->Player);
		//Wyrmgus end
	}
	//Wyrmgus start
	if (target->get_character() != nullptr && (this->PlayerNeutral == 1 || this->PlayerNeutral == 2)) {
		wyrmgus::vector::remove(target->Player->Heroes, target);
		target->set_character(nullptr);
	}
//	UnitLost(*target);
//	target->clear_orders();
//	target->Release();
	if (game::get()->is_persistency_enabled() && target->get_character() != nullptr && &caster == target) { //save persistent data
		if (target->Player == CPlayer::GetThisPlayer()) {
			target->get_character()->set_unit_type(type);
			target->get_character()->save();
		}
	}
	//Wyrmgus end
	return 1;
}
