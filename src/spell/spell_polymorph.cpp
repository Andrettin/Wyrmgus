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
/**@name spell_polymorph.cpp - The spell Polymorph. */
//
//      (c) Copyright 1998-2019 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Joris DAUPHIN and Andrettin
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

#include "stratagus.h"

#include "spell/spell_polymorph.h"

//Wyrmgus start
#include "action/action_upgradeto.h"
#include "character.h"
//Wyrmgus end
#include "civilization.h"
#include "faction.h"
#include "game/game.h"
#include "map/map.h"
//Wyrmgus start
#include "network/network.h"
//Wyrmgus end
#include "script.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end

/* virtual */ void Spell_Polymorph::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "new-form")) {
			value = LuaToString(l, -1, j + 1);
			this->NewForm = CUnitType::Get(value);
			if (!this->NewForm) {
				this->NewForm = nullptr;
				DebugPrint("unit type \"%s\" not found for polymorph spell.\n" _C_ value);
			}
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
			CCivilization *civilization = CCivilization::Get(value);
			if (civilization) {
				this->Civilization = civilization;
			}
		} else if (!strcmp(value, "faction")) {
			value = LuaToString(l, -1, j + 1);
			const CFaction *faction = CFaction::Get(value);
			if (faction != nullptr) {
				this->Faction = faction;
			}
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
	if (this->NewForm == nullptr && this->Civilization == nullptr && this->Faction == nullptr && !this->Detachment) {
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
/* virtual */ int Spell_Polymorph::Cast(CUnit &caster, const CSpell &spell, CUnit *target, const Vec2i &goalPos, int z, int modifier)
{
	if (!target) {
		return 0;
	}
	const CUnitType *unit_type = this->NewForm;
	//Wyrmgus start
	if (this->NewForm == nullptr) {
		const CUnitType *new_unit_type = nullptr;
		if (this->Civilization != nullptr && this->Faction != nullptr && this->Civilization == target->GetType()->GetCivilization()) { //get faction equivalent, if is of the same civilization
			new_unit_type = CFaction::GetFactionClassUnitType(this->Faction, target->GetType()->GetClass());
		} else if (this->Civilization != nullptr && this->Civilization != target->GetType()->GetCivilization()) {
			new_unit_type = CCivilization::GetCivilizationClassUnitType(this->Civilization, target->GetType()->GetClass());
		}
		if (this->Detachment && target->GetType()->GetCivilization() != nullptr && target->GetType()->GetFaction() != nullptr) {
			new_unit_type = CCivilization::GetCivilizationClassUnitType(target->GetType()->GetCivilization(), target->GetType()->GetClass());
		}
		if (new_unit_type != nullptr) {
			unit_type = new_unit_type;
		}
	}
	if (target->Character && target->Character->Custom && target->Character->GetCivilization() != nullptr && this->Civilization != nullptr && this->Civilization != target->Character->GetCivilization()) {
		target->Character->Civilization = this->Civilization;
		SaveHero(target->Character);
	}
	if (unit_type == nullptr) {
		return 0;
	}
//	const Vec2i pos(goalPos - type.GetHalfTileSize());
	//Wyrmgus end

	//Wyrmgus start
//	caster.GetPlayer()->Score += target->Variable[POINTS_INDEX].Value;
	//Wyrmgus end
	if (caster.IsEnemy(*target)) {
		//Wyrmgus start
		caster.GetPlayer()->Score += target->Variable[POINTS_INDEX].Value;
		//Wyrmgus end
		if (target->GetType()->BoolFlag[BUILDING_INDEX].value) {
			caster.GetPlayer()->TotalRazings++;
		} else {
			caster.GetPlayer()->TotalKills++;
		}
		//Wyrmgus start
		caster.GetPlayer()->UnitTypeKills[target->GetType()->GetIndex()]++;
		
		//distribute experience between nearby units belonging to the same player
		if (!target->GetType()->BoolFlag[BUILDING_INDEX].value) {
			caster.ChangeExperience(UseHPForXp ? target->Variable[HP_INDEX].Value : target->Variable[POINTS_INDEX].Value, ExperienceRange);
		}
		//Wyrmgus end
		caster.Variable[KILL_INDEX].Value++;
		caster.Variable[KILL_INDEX].Max++;
		caster.Variable[KILL_INDEX].Enable = 1;
	}

	// as said somewhere else -- no corpses :)
	//Wyrmgus start
//	target->Remove(nullptr);
//	Vec2i offset;
	//Wyrmgus end
	caster.Variable[MANA_INDEX].Value -= spell.ManaCost;
	//Wyrmgus start
//	Vec2i resPos;
//	FindNearestDrop(unit_type, pos, resPos, LookingW);
	TransformUnitIntoType(*target, *unit_type);
	//Wyrmgus end
	if (this->PlayerNeutral == 1) {
		//Wyrmgus start
//		MakeUnitAndPlace(resPos, unit_type, CPlayer::Players[PlayerNumNeutral]);
		target->ChangeOwner(*CPlayer::Players[PlayerNumNeutral]);
		//Wyrmgus end
	} else if (this->PlayerNeutral == 2) {
		//Wyrmgus start
//		MakeUnitAndPlace(resPos, unit_type, caster.GetPlayer());
		target->ChangeOwner(*caster.GetPlayer());
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		MakeUnitAndPlace(resPos, unit_type, target->GetPlayer());
		//Wyrmgus end
	}
	//Wyrmgus start
	if (target->Character && (this->PlayerNeutral == 1 || this->PlayerNeutral == 2)) {
		target->GetPlayer()->Heroes.erase(std::remove(target->GetPlayer()->Heroes.begin(), target->GetPlayer()->Heroes.end(), target), target->GetPlayer()->Heroes.end());
		target->Character = nullptr;
	}
//	UnitLost(*target);
//	UnitClearOrders(*target);
//	target->Release();
	if (!IsNetworkGame() && target->Character != nullptr && &caster == target) { //save persistent data
		if (target->GetPlayer()->AiEnabled == false) {
			target->Character->UnitType = unit_type;
			SaveHero(target->Character);
		}
	}
	//Wyrmgus end
	return 1;
}
