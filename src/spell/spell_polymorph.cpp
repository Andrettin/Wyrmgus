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
//      (c) Copyright 1998-2012 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, and Joris DAUPHIN
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

//@{

#include "stratagus.h"

#include "spell/spell_polymorph.h"

//Wyrmgus start
#include "action/action_upgradeto.h"
#include "character.h"
//Wyrmgus end
#include "game.h"
#include "map.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "script.h"
#include "unit.h"
//Wyrmgus start
#include "unit_find.h"
//Wyrmgus end

/* virtual */ void Spell_Polymorph::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "new-form")) {
			value = LuaToString(l, -1, j + 1);
			this->NewForm = UnitTypeByIdent(value);
			if (!this->NewForm) {
				this->NewForm = 0;
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
			this->Civilization = PlayerRaces.GetRaceIndexByName(value);
			if (this->Civilization == -1) {
				fprintf(stderr, "Civilization %s doesn't exist.\n", value);
			}
		} else if (!strcmp(value, "faction")) {
			value = LuaToString(l, -1, j + 1);
			this->Faction = PlayerRaces.GetFactionIndexByName(this->Civilization, value);
			if (this->Faction == -1) {
				fprintf(stderr, "Faction %s doesn't exist.\n", value);
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
//	if (this->NewForm == NULL) {
	if (this->NewForm == NULL && this->Civilization == -1 && this->Faction == -1 && !this->Detachment) {
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
/* virtual */ int Spell_Polymorph::Cast(CUnit &caster, const SpellType &spell, CUnit *target, const Vec2i &goalPos)
{
	if (!target) {
		return 0;
	}
	CUnitType *type = this->NewForm;
	//Wyrmgus start
	if (this->NewForm == NULL) {
		int new_unit_type = -1;
		if (this->Civilization != -1 && this->Faction != -1 && PlayerRaces.Name[this->Civilization] == target->Type->Civilization) { //get faction equivalent, if is of the same civilization
			new_unit_type = PlayerRaces.GetFactionClassUnitType(this->Civilization, this->Faction, GetUnitTypeClassIndexByName(target->Type->Class));
		}
		if (this->Detachment && !target->Type->Civilization.empty() && !target->Type->Faction.empty()) {
			new_unit_type = PlayerRaces.GetCivilizationClassUnitType(PlayerRaces.GetRaceIndexByName(target->Type->Civilization.c_str()), GetUnitTypeClassIndexByName(target->Type->Class));
		}
		if (new_unit_type != -1) {
			type = UnitTypes[new_unit_type];
		}
	}
	if (type == NULL) {
		return 0;
	}
//	const Vec2i pos(goalPos - type.GetHalfTileSize());
	//Wyrmgus end

	//Wyrmgus start
//	caster.Player->Score += target->Variable[POINTS_INDEX].Value;
	//Wyrmgus end
	if (caster.IsEnemy(*target)) {
		//Wyrmgus start
		caster.Player->Score += target->Variable[POINTS_INDEX].Value;
		//Wyrmgus end
		if (target->Type->Building) {
			caster.Player->TotalRazings++;
		} else {
			caster.Player->TotalKills++;
		}
		//Wyrmgus start
		caster.Player->UnitTypeKills[target->Type->Slot]++;
		/*
		if (UseHPForXp) {
			caster.Variable[XP_INDEX].Max += target->Variable[HP_INDEX].Value;
		} else {
			caster.Variable[XP_INDEX].Max += target->Variable[POINTS_INDEX].Value;
		}
		caster.Variable[XP_INDEX].Value = caster.Variable[XP_INDEX].Max;
		*/
		
		//distribute experience between nearby units belonging to the same player
		if (!target->Type->BoolFlag[BUILDING_INDEX].value) {
			std::vector<CUnit *> table;
			SelectAroundUnit(caster, 6, table, MakeAndPredicate(HasSamePlayerAs(*caster.Player), IsNotBuildingType()));

			if (UseHPForXp) {
				caster.Variable[XP_INDEX].Max += target->Variable[HP_INDEX].Value / (table.size() + 1);
			} else {
				caster.Variable[XP_INDEX].Max += target->Variable[POINTS_INDEX].Value / (table.size() + 1);
			}
			caster.Variable[XP_INDEX].Value = caster.Variable[XP_INDEX].Max;
			caster.XPChanged();

			for (size_t i = 0; i != table.size(); ++i) {
				if (UseHPForXp) {
					table[i]->Variable[XP_INDEX].Max += target->Variable[HP_INDEX].Value / (table.size() + 1);
				} else {
					table[i]->Variable[XP_INDEX].Max += target->Variable[POINTS_INDEX].Value / (table.size() + 1);
				}
				table[i]->Variable[XP_INDEX].Value = table[i]->Variable[XP_INDEX].Max;
				table[i]->XPChanged();
			}
		}
		//Wyrmgus end
		caster.Variable[KILL_INDEX].Value++;
		caster.Variable[KILL_INDEX].Max++;
		caster.Variable[KILL_INDEX].Enable = 1;
	}

	// as said somewhere else -- no corpses :)
	//Wyrmgus start
//	target->Remove(NULL);
//	Vec2i offset;
	//Wyrmgus end
	caster.Variable[MANA_INDEX].Value -= spell.ManaCost;
	//Wyrmgus start
//	Vec2i resPos;
//	FindNearestDrop(type, pos, resPos, LookingW);
	TransformUnitIntoType(*target, *type);
	//Wyrmgus end
	if (this->PlayerNeutral == 1) {
		//Wyrmgus start
//		MakeUnitAndPlace(resPos, type, Players + PlayerNumNeutral);
		target->ChangeOwner(Players[PlayerNumNeutral]);
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
	if (target->Character && (this->PlayerNeutral == 1 || this->PlayerNeutral == 2)) {
		target->Player->Heroes.erase(std::remove(target->Player->Heroes.begin(), target->Player->Heroes.end(), target->Character->GetFullName()), target->Player->Heroes.end());
		target->Character = NULL;
		target->Player->UnitTypesNonHeroCount[target->Type->Slot]++;
	}
//	UnitLost(*target);
//	UnitClearOrders(*target);
//	target->Release();
	if (!IsNetworkGame() && target->Character != NULL && target->Character->Persistent && target->Player->AiEnabled == false && &caster == target) { //save persistent data
		target->Character->Type = const_cast<CUnitType *>(&(*type));
		SaveHero(target->Character);
	}
	//Wyrmgus end
	return 1;
}

//@}
