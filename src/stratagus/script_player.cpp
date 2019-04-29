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
/**@name script_player.cpp - The player ccl functions. */
//
//      (c) Copyright 2001-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "player.h"

#include "action/actions.h"
#include "age.h"
#include "ai/ai.h"
#include "ai/ai_building_template.h"
#include "ai/force_template.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "dynasty.h"
#include "economy/currency.h"
//Wyrmgus start
#include "editor/editor.h"
//Wyrmgus end
#include "faction.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
//Wyrmgus start
#include "item/item.h"
//Wyrmgus end
#include "language/grammatical_gender.h"
#include "language/language.h"
#include "language/language_family.h"
#include "language/word.h"
#include "language/word_type.h"
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "map/site.h"
#include "player_color.h"
#include "quest/quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "script.h"
#include "species/gender.h"
#include "species/species.h"
#include "time/calendar.h"
#include "ui/button_action.h"
#include "ui/icon.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
//Wyrmgus start
#include "ui/ui.h"
#include "util.h"
//Wyrmgus end
//Wyrmgus start
#include "video/font.h"
//Wyrmgus end
#include "video/video.h"
#include "world/plane.h"
#include "world/province.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern CUnit *CclGetUnitFromRef(lua_State *l);

/**
**  Get a player pointer
**
**  @param l  Lua state.
**
**  @return   The player pointer
*/
static CPlayer *CclGetPlayer(lua_State *l)
{
	return CPlayer::Players[LuaToNumber(l, -1)];
}

/**
**  Parse the player configuration.
**
**  @param l  Lua state.
*/
static int CclPlayer(lua_State *l)
{
	int i = LuaToNumber(l, 1);

	CPlayer &player = *CPlayer::Players[i];
	player.Index = i;

	if (NumPlayers <= i) {
		NumPlayers = i + 1;
	}

	player.Load(l);
	return 0;
}

void CPlayer::Load(lua_State *l)
{
	const int args = lua_gettop(l);

	this->Units.resize(0);
	this->FreeWorkers.resize(0);
	//Wyrmgus start
	this->LevelUpUnits.resize(0);
	//Wyrmgus end

	// j = 0 represent player Index.
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "name")) {
			this->SetName(LuaToString(l, j + 1));
		} else if (!strcmp(value, "type")) {
			value = LuaToString(l, j + 1);
			if (!strcmp(value, "neutral")) {
				this->Type = PlayerNeutral;
			} else if (!strcmp(value, "nobody")) {
				this->Type = PlayerNobody;
			} else if (!strcmp(value, "computer")) {
				this->Type = PlayerComputer;
			} else if (!strcmp(value, "person")) {
				this->Type = PlayerPerson;
			} else if (!strcmp(value, "rescue-passive")) {
				this->Type = PlayerRescuePassive;
			} else if (!strcmp(value, "rescue-active")) {
				this->Type = PlayerRescueActive;
			} else {
				LuaError(l, "Unsupported tag: %s" _C_ value);
			}
		} else if (!strcmp(value, "race")) {
			const char *civilization_ident = LuaToString(l, j + 1);
			CCivilization *civilization = CCivilization::Get(civilization_ident);
			if (civilization) {
				this->Race = civilization->GetIndex();
			}
		//Wyrmgus start
		} else if (!strcmp(value, "faction")) {
			const std::string faction_ident = LuaToString(l, j + 1);
			const CFaction *faction = CFaction::Get(faction_ident);
			this->Faction = faction;
		} else if (!strcmp(value, "dynasty")) {
			this->Dynasty = CDynasty::Get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "age")) {
			this->Age = CAge::Get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "color")) {
			int color_id = LuaToNumber(l, j + 1);
			this->Color = PlayerColors[color_id][0];
			this->UnitColors.Colors = PlayerColorsRGB[color_id];
		//Wyrmgus end
		} else if (!strcmp(value, "ai-name")) {
			this->AiName = LuaToString(l, j + 1);
		} else if (!strcmp(value, "team")) {
			this->Team = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "enemy")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->Enemy &= ~(1 << i);
				} else {
					this->Enemy |= (1 << i);
				}
			}
		} else if (!strcmp(value, "allied")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->Allied &= ~(1 << i);
				} else {
					this->Allied |= (1 << i);
				}
			}
		} else if (!strcmp(value, "shared-vision")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->SharedVision &= ~(1 << i);
				} else {
					this->SharedVision |= (1 << i);
				}
			}
		} else if (!strcmp(value, "start")) {
			CclGetPos(l, &this->StartPos.x, &this->StartPos.y, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "start-map-layer")) {
			this->StartMapLayer = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "overlord")) {
			int overlord_id = LuaToNumber(l, j + 1);
			this->SetOverlord(CPlayer::Players[overlord_id]);
		//Wyrmgus end
		} else if (!strcmp(value, "resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->Resources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "stored-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->StoredResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "max-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->MaxResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "last-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->LastResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "incomes")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Incomes[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "revenue")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Revenue[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "prices")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Prices[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		//Wyrmgus end
		} else if (!strcmp(value, "ai-enabled")) {
			this->AiEnabled = true;
			--j;
		} else if (!strcmp(value, "ai-disabled")) {
			this->AiEnabled = false;
			--j;
		//Wyrmgus start
		} else if (!strcmp(value, "revealed")) {
			this->Revealed = true;
			--j;
		//Wyrmgus end
		} else if (!strcmp(value, "supply")) {
			this->Supply = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "demand")) {
			this->Demand = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "trade-cost")) {
			this->TradeCost = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unit-limit")) {
			this->UnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "building-limit")) {
			this->BuildingLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-unit-limit")) {
			this->TotalUnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "score")) {
			this->Score = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-units")) {
			this->TotalUnits = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-buildings")) {
			this->TotalBuildings = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-razings")) {
			this->TotalRazings = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-kills")) {
			this->TotalKills = LuaToNumber(l, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "unit-type-kills")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CUnitType *unit_type = CUnitType::Get(LuaToString(l, j + 1, k + 1));
				++k;
				if (unit_type) {
					this->UnitTypeKills[unit_type->GetIndex()] = LuaToNumber(l, j + 1, k + 1);
				}
			}
		} else if (!strcmp(value, "lost-town-hall-timer")) {
			this->LostTownHallTimer = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "hero-cooldown-timer")) {
			this->HeroCooldownTimer = LuaToNumber(l, j + 1);
		//Wyrmgus end
		} else if (!strcmp(value, "total-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of total-resources: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of total-resources: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->TotalResources[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-harvest")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of speed-resource-harvest: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesHarvest[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-return")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of speed-resource-return: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesReturn[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-build")) {
			this->SpeedBuild = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-train")) {
			this->SpeedTrain = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-upgrade")) {
			this->SpeedUpgrade = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-research")) {
			this->SpeedResearch = LuaToNumber(l, j + 1);
		//Wyrmgus start
		/*
		} else if (!strcmp(value, "color")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			const int r = LuaToNumber(l, j + 1, 1);
			const int g = LuaToNumber(l, j + 1, 2);
			const int b = LuaToNumber(l, j + 1, 3);
			this->Color = Video.MapRGB(TheScreen->format, r, g, b);
		*/
		//Wyrmgus end
		//Wyrmgus start
		} else if (!strcmp(value, "current-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				if (quest) {
					this->CurrentQuests.push_back(quest);
				}
			}
		} else if (!strcmp(value, "completed-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				if (quest) {
					this->CompletedQuests.push_back(quest);
					if (quest->Competitive) {
						quest->CurrentCompleted = true;
					}
				}
			}
		} else if (!strcmp(value, "quest-objectives")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, j + 1, k + 1);
				CPlayerQuestObjective *objective = new CPlayerQuestObjective;
				this->QuestObjectives.push_back(objective);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for quest objectives)");
				}
				const int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					value = LuaToString(l, -1, n + 1);
					++n;
					if (!strcmp(value, "quest")) {
						objective->Quest = GetQuest(LuaToString(l, -1, n + 1));
						if (!objective->Quest) {
							LuaError(l, "Quest doesn't exist.");
						}
					} else if (!strcmp(value, "objective-type")) {
						objective->ObjectiveType = GetQuestObjectiveTypeIdByName(LuaToString(l, -1, n + 1));
						if (objective->ObjectiveType == -1) {
							LuaError(l, "Objective type doesn't exist.");
						}
					} else if (!strcmp(value, "objective-string")) {
						objective->ObjectiveString = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "quantity")) {
						objective->Quantity = LuaToNumber(l, -1, n + 1);
					} else if (!strcmp(value, "counter")) {
						objective->Counter = LuaToNumber(l, -1, n + 1);
					} else if (!strcmp(value, "resource")) {
						int resource = GetResourceIdByName(LuaToString(l, -1, n + 1));
						if (resource == -1) {
							LuaError(l, "Resource doesn't exist.");
						}
						objective->Resource = resource;
					} else if (!strcmp(value, "unit-class")) {
						const UnitClass *unit_class = UnitClass::Get(LuaToString(l, -1, n + 1));
						if (unit_class == nullptr) {
							LuaError(l, "Unit class doesn't exist.");
						}
						objective->UnitClass = unit_class;
					} else if (!strcmp(value, "unit-type")) {
						CUnitType *unit_type = CUnitType::Get(LuaToString(l, -1, n + 1));
						if (!unit_type) {
							LuaError(l, "Unit type doesn't exist.");
						}
						objective->UnitTypes.push_back(unit_type);
					} else if (!strcmp(value, "upgrade")) {
						CUpgrade *upgrade = CUpgrade::Get(LuaToString(l, -1, n + 1));
						if (!upgrade) {
							LuaError(l, "Upgrade doesn't exist.");
						}
						objective->Upgrade = upgrade;
					} else if (!strcmp(value, "character")) {
						CCharacter *character = CCharacter::Get(LuaToString(l, -1, n + 1));
						if (!character) {
							LuaError(l, "Character doesn't exist.");
						}
						objective->Character = character;
					} else if (!strcmp(value, "unique")) {
						CUniqueItem *unique = GetUniqueItem(LuaToString(l, -1, n + 1));
						if (!unique) {
							LuaError(l, "Unique doesn't exist.");
						}
						objective->Unique = unique;
					} else if (!strcmp(value, "settlement")) {
						CSite *site = CSite::Get(LuaToString(l, -1, n + 1));
						if (!site) {
							LuaError(l, "Site doesn't exist.");
						}
						objective->Settlement = site;
					} else if (!strcmp(value, "faction")) {
						CFaction *faction = CFaction::Get(LuaToString(l, -1, n + 1));
						if (faction) {
							objective->Faction = faction;
						}
					} else {
						LuaError(l, "Invalid quest objective property.");
					}
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "modifiers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CUpgrade *modifier_upgrade = CUpgrade::Get(LuaToString(l, j + 1, k + 1));
				++k;
				int end_cycle = LuaToNumber(l, j + 1, k + 1);
				if (modifier_upgrade) {
					this->Modifiers.push_back(std::pair<CUpgrade *, int>(modifier_upgrade, end_cycle));
				}
			}
		//Wyrmgus end
		} else if (!strcmp(value, "autosell-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const int res = GetResourceIdByName(LuaToString(l, j + 1, k + 1));
				if (res != -1) {
					this->AutosellResources.push_back(res);
				}
			}
		} else if (!strcmp(value, "timers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != UpgradeMax) {
//				LuaError(l, "Wrong upgrade timer length: %d" _C_ subargs);
//			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				//Wyrmgus start
//				this->UpgradeTimers.Upgrades[k] = LuaToNumber(l, j + 1, k + 1);
				CUpgrade *timer_upgrade = CUpgrade::Get(LuaToString(l, j + 1, k + 1));
				++k;
				if (timer_upgrade) {
					this->UpgradeTimers.Upgrades[timer_upgrade->ID] = LuaToNumber(l, j + 1, k + 1);
				}
				//Wyrmgus end
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	// Manage max
	for (int i = 0; i < MaxCosts; ++i) {
		if (this->MaxResources[i] != -1) {
			this->SetResource(i, this->Resources[i] + this->StoredResources[i], STORE_BOTH);
		}
	}
}

/**
**  Change unit owner
**
**  @param l  Lua state.
*/
static int CclChangeUnitsOwner(lua_State *l)
{
	LuaCheckArgs(l, 4);

	Vec2i pos1;
	Vec2i pos2;
	CclGetPos(l, &pos1.x, &pos1.y, 1);
	CclGetPos(l, &pos2.x, &pos2.y, 2);
	const int oldp = LuaToNumber(l, 3);
	const int newp = LuaToNumber(l, 4);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(pos1, pos2, table, HasSamePlayerAs(*CPlayer::Players[oldp]));
	Select(pos1, pos2, table, 0, HasSamePlayerAs(*CPlayer::Players[oldp]));
	//Wyrmgus end
	for (size_t i = 0; i != table.size(); ++i) {
		table[i]->ChangeOwner(*CPlayer::Players[newp]);
	}
	return 0;
}

/**
**  Get ThisPlayer.
**
**  @param l  Lua state.
*/
static int CclGetThisPlayer(lua_State *l)
{
	LuaCheckArgs(l, 0);
	if (CPlayer::GetThisPlayer()) {
		lua_pushnumber(l, CPlayer::GetThisPlayer()->Index);
	} else {
		lua_pushnumber(l, 0);
	}
	return 1;
}

/**
**  Set ThisPlayer.
**
**  @param l  Lua state.
*/
static int CclSetThisPlayer(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int plynr = LuaToNumber(l, 1);

	CPlayer::SetThisPlayer(CPlayer::Players[plynr]);
	
	//Wyrmgus start
	UI.Load();
	//Wyrmgus end

	lua_pushnumber(l, plynr);
	return 1;
}

/**
**  Set MaxSelectable
**
**  @param l  Lua state.
*/
static int CclSetMaxSelectable(lua_State *l)
{
	LuaCheckArgs(l, 1);
	MaxSelectable = LuaToNumber(l, 1);

	lua_pushnumber(l, MaxSelectable);
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersUnitLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->UnitLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersBuildingLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->BuildingLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersTotalUnitLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer::Players[i]->TotalUnitLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Change the diplomacy from player to another player.
**
**  @param l  Lua state.
**
**  @return          FIXME: should return old state.
*/
static int CclSetDiplomacy(lua_State *l)
{
	LuaCheckArgs(l, 3);
	const int base = LuaToNumber(l, 1);
	const int plynr = LuaToNumber(l, 3);
	const char *state = LuaToString(l, 2);

	if (!strcmp(state, "allied")) {
		SendCommandDiplomacy(base, DiplomacyAllied, plynr);
	} else if (!strcmp(state, "neutral")) {
		SendCommandDiplomacy(base, DiplomacyNeutral, plynr);
	} else if (!strcmp(state, "crazy")) {
		SendCommandDiplomacy(base, DiplomacyCrazy, plynr);
	} else if (!strcmp(state, "enemy")) {
		SendCommandDiplomacy(base, DiplomacyEnemy, plynr);
	//Wyrmgus start
	} else if (!strcmp(state, "overlord")) {
		SendCommandDiplomacy(base, DiplomacyOverlord, plynr);
	} else if (!strcmp(state, "vassal")) {
		SendCommandDiplomacy(base, DiplomacyVassal, plynr);
	//Wyrmgus end
	}
	return 0;
}

/**
**  Change the diplomacy from ThisPlayer to another player.
**
**  @param l  Lua state.
*/
static int CclDiplomacy(lua_State *l)
{
	lua_pushnumber(l, CPlayer::GetThisPlayer()->Index);
	lua_insert(l, 1);
	return CclSetDiplomacy(l);
}

/**
**  Change the shared vision from player to another player.
**
**  @param l  Lua state.
**
**  @return   FIXME: should return old state.
*/
static int CclSetSharedVision(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int base = LuaToNumber(l, 1);
	const bool shared = LuaToBoolean(l, 2);
	const int plynr = LuaToNumber(l, 3);

	SendCommandSharedVision(base, shared, plynr);

	return 0;
}

/**
**  Change the shared vision from ThisPlayer to another player.
**
**  @param l  Lua state.
*/
static int CclSharedVision(lua_State *l)
{
	lua_pushnumber(l, CPlayer::GetThisPlayer()->Index);
	lua_insert(l, 1);
	return CclSetSharedVision(l);
}

//Wyrmgus start
/**
**  Define a civilization.
**
**  @param l  Lua state.
*/
static int CclDefineCivilization(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string civilization_name = LuaToString(l, 1);
	CCivilization *civilization = CCivilization::GetOrAdd(civilization_name);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Display")) {
			civilization->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			civilization->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			civilization->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			civilization->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Adjective")) {
			civilization->Adjective = LuaToString(l, -1);
		} else if (!strcmp(value, "Interface")) {
			civilization->Interface = LuaToString(l, -1);
		} else if (!strcmp(value, "Hidden")) {
			civilization->Hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Playable")) {
			civilization->Playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "VictoryBackgroundFile")) {
			civilization->VictoryBackgroundFile = LuaToString(l, -1);
		} else if (!strcmp(value, "DefeatBackgroundFile")) {
			civilization->DefeatBackgroundFile = LuaToString(l, -1);
		} else if (!strcmp(value, "Species")) {
			CSpecies *species = CSpecies::Get(LuaToString(l, -1));
			if (species != nullptr) {
				civilization->Species = species;
			}
		} else if (!strcmp(value, "ParentCivilization")) {
			civilization->ParentCivilization = CCivilization::Get(LuaToString(l, -1));
		} else if (!strcmp(value, "Language")) {
			CLanguage *language = CLanguage::Get(LuaToString(l, -1));
			if (language) {
				civilization->Language = language;
				language->UsedByCivilizationOrFaction = true;
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Calendar")) {
			CCalendar *calendar = CCalendar::GetCalendar(LuaToString(l, -1));
			civilization->Calendar = calendar;
		} else if (!strcmp(value, "Currency")) {
			Currency *currency = Currency::Get(LuaToString(l, -1));
			civilization->Currency = currency;
		} else if (!strcmp(value, "DefaultPlayerColor")) {
			CPlayerColor *player_color = CPlayerColor::Get(LuaToString(l, -1));
			if (player_color != nullptr) {
				civilization->DefaultPlayerColor = player_color;
			}
		} else if (!strcmp(value, "CivilizationUpgrade")) {
			civilization->Upgrade = LuaToString(l, -1);
		} else if (!strcmp(value, "DevelopsFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string originary_civilization_ident = LuaToString(l, -1, j + 1);
				CCivilization *originary_civilization = CCivilization::Get(originary_civilization_ident);
				if (originary_civilization) {
					civilization->DevelopsFrom.push_back(originary_civilization);
					originary_civilization->DevelopsTo.push_back(civilization);
				}
			}
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				int button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != -1) {
					++j;
					civilization->ButtonIcons[button_action].Name = LuaToString(l, -1, j + 1);
					civilization->ButtonIcons[button_action].Icon = nullptr;
					civilization->ButtonIcons[button_action].Load();
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForceTypeWeights")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			civilization->ForceTypeWeights.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int force_type = GetForceTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				civilization->ForceTypeWeights[force_type] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ForceTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CForceTemplate *force = new CForceTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "force-type")) {
						force->ForceType = GetForceTypeIdByName(LuaToString(l, -1, k + 1));
						if (force->ForceType == -1) {
							LuaError(l, "Force type doesn't exist.");
						}
						civilization->ForceTemplates[force->ForceType].push_back(force);
					} else if (!strcmp(value, "priority")) {
						force->Priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "weight")) {
						force->Weight = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "unit-class")) {
						const UnitClass *unit_class = UnitClass::Get(LuaToString(l, -1, k + 1));
						++k;
						int unit_quantity = LuaToNumber(l, -1, k + 1);
						force->Units.push_back(std::pair<const UnitClass *, int>(unit_class, unit_quantity));
					} else {
						printf("\n%s\n", civilization->GetIdent().utf8().get_data());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = civilization->ForceTemplates.begin(); iterator != civilization->ForceTemplates.end(); ++iterator) {
				std::sort(iterator->second.begin(), iterator->second.end(), [](CForceTemplate *a, CForceTemplate *b) {
					return a->Priority > b->Priority;
				});
			}
		} else if (!strcmp(value, "AiBuildingTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CAiBuildingTemplate *building_template = new CAiBuildingTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "unit-class")) {
						const UnitClass *unit_class = UnitClass::Get(LuaToString(l, -1, k + 1));
						building_template->UnitClass = unit_class;
						civilization->AiBuildingTemplates.push_back(building_template);
					} else if (!strcmp(value, "priority")) {
						building_template->Priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "per-settlement")) {
						building_template->PerSettlement = LuaToBoolean(l, -1, k + 1);
					} else {
						printf("\n%s\n", civilization->GetIdent().utf8().get_data());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			std::sort(civilization->AiBuildingTemplates.begin(), civilization->AiBuildingTemplates.end(), [](CAiBuildingTemplate *a, CAiBuildingTemplate *b) {
				return a->Priority > b->Priority;
			});
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			civilization->UIFillers.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CFiller filler = CFiller();
				std::string filler_file = LuaToString(l, -1, j + 1);
				if (filler_file.empty()) {
					LuaError(l, "Filler graphic file is empty.");
				}				
				filler.G = CGraphic::New(filler_file);
				++j;
				filler.X = LuaToNumber(l, -1, j + 1);
				++j;
				filler.Y = LuaToNumber(l, -1, j + 1);
				civilization->UIFillers.push_back(filler);
			}
		} else if (!strcmp(value, "UnitSounds")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, -1, k + 1);
				++k;

				if (!strcmp(value, "selected")) {
					civilization->UnitSounds.Selected.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "acknowledge")) {
					civilization->UnitSounds.Acknowledgement.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "attack")) {
					civilization->UnitSounds.Attack.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "idle")) {
					civilization->UnitSounds.Idle.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "build")) {
					civilization->UnitSounds.Build.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "ready")) {
					civilization->UnitSounds.Ready.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "repair")) {
					civilization->UnitSounds.Repair.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "harvest")) {
					const std::string name = LuaToString(l, -1, k + 1);
					++k;
					const int resId = GetResourceIdByName(l, name.c_str());
					civilization->UnitSounds.Harvest[resId].Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "help")) {
					civilization->UnitSounds.Help.Name = LuaToString(l, -1, k + 1);
				} else if (!strcmp(value, "help-town")) {
					civilization->UnitSounds.HelpTown.Name = LuaToString(l, -1, k + 1);
				} else {
					LuaError(l, "Unsupported sound tag: %s" _C_ value);
				}
			}
		} else if (!strcmp(value, "PersonalNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				const CGender *gender = CGender::Get(LuaToString(l, -1, j + 1), false);
				if (gender != nullptr) {
					++j;
				}
				
				civilization->PersonalNames[gender].push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "UnitClassNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string class_name = LuaToString(l, -1, j + 1);
				if (class_name.empty()) {
					LuaError(l, "Class is given as a blank string.");
				}
				const UnitClass *unit_class = UnitClass::Get(class_name);
				++j;
				
				civilization->UnitClassNames[unit_class].push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "FamilyNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->FamilyNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ProvinceNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ShipNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->ShipNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "MinisterTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int title = GetCharacterTitleIdByName(LuaToString(l, -1, k + 1));
				++k;
				const CGender *gender = CGender::Get(LuaToString(l, -1, k + 1), false);
				++k;
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				int faction_tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				civilization->MinisterTitles[title][gender][government_type][faction_tier] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "HistoricalUpgrades")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;

				std::string technology_ident = LuaToString(l, -1, j + 1);
				++j;
				
				bool has_upgrade = LuaToBoolean(l, -1, j + 1);

				civilization->HistoricalUpgrades[technology_ident][date] = has_upgrade;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (civilization->ParentCivilization) {
		const CCivilization *parent_civilization = civilization->ParentCivilization;
		int parent_civilization_id = parent_civilization->GetIndex();
		
		if (civilization->Interface.empty()) {
			civilization->Interface = parent_civilization->Interface;
		}

		if (civilization->Upgrade.empty() && !parent_civilization->Upgrade.empty()) { //if the civilization has no civilization upgrade, inherit that of its parent civilization
			civilization->Upgrade = parent_civilization->Upgrade;
		}
		
		//inherit button icons from the parent civilization, for button actions which none are specified
		for (std::map<int, IconConfig>::const_iterator iterator = parent_civilization->ButtonIcons.begin(); iterator != parent_civilization->ButtonIcons.end(); ++iterator) {
			if (civilization->ButtonIcons.find(iterator->first) == civilization->ButtonIcons.end()) {
				civilization->ButtonIcons[iterator->first] = iterator->second;
			}
		}
		
		//inherit historical upgrades from the parent civilization, if no historical data is given for that upgrade for this civilization
		for (std::map<std::string, std::map<CDate, bool>>::const_iterator iterator = parent_civilization->HistoricalUpgrades.begin(); iterator != parent_civilization->HistoricalUpgrades.end(); ++iterator) {
			if (civilization->HistoricalUpgrades.find(iterator->first) == civilization->HistoricalUpgrades.end()) {
				civilization->HistoricalUpgrades[iterator->first] = iterator->second;
			}
		}
		
		//unit sounds
		if (civilization->UnitSounds.Selected.Name.empty()) {
			civilization->UnitSounds.Selected = parent_civilization->UnitSounds.Selected;
		}
		if (civilization->UnitSounds.Acknowledgement.Name.empty()) {
			civilization->UnitSounds.Acknowledgement = parent_civilization->UnitSounds.Acknowledgement;
		}
		if (civilization->UnitSounds.Attack.Name.empty()) {
			civilization->UnitSounds.Attack = parent_civilization->UnitSounds.Attack;
		}
		if (civilization->UnitSounds.Idle.Name.empty()) {
			civilization->UnitSounds.Idle = parent_civilization->UnitSounds.Idle;
		}
		if (civilization->UnitSounds.Hit.Name.empty()) {
			civilization->UnitSounds.Hit = parent_civilization->UnitSounds.Hit;
		}
		if (civilization->UnitSounds.Miss.Name.empty()) {
			civilization->UnitSounds.Miss = parent_civilization->UnitSounds.Miss;
		}
		if (civilization->UnitSounds.FireMissile.Name.empty()) {
			civilization->UnitSounds.FireMissile = parent_civilization->UnitSounds.FireMissile;
		}
		if (civilization->UnitSounds.Step.Name.empty()) {
			civilization->UnitSounds.Step = parent_civilization->UnitSounds.Step;
		}
		if (civilization->UnitSounds.StepDirt.Name.empty()) {
			civilization->UnitSounds.StepDirt = parent_civilization->UnitSounds.StepDirt;
		}
		if (civilization->UnitSounds.StepGrass.Name.empty()) {
			civilization->UnitSounds.StepGrass = parent_civilization->UnitSounds.StepGrass;
		}
		if (civilization->UnitSounds.StepGravel.Name.empty()) {
			civilization->UnitSounds.StepGravel = parent_civilization->UnitSounds.StepGravel;
		}
		if (civilization->UnitSounds.StepMud.Name.empty()) {
			civilization->UnitSounds.StepMud = parent_civilization->UnitSounds.StepMud;
		}
		if (civilization->UnitSounds.StepStone.Name.empty()) {
			civilization->UnitSounds.StepStone = parent_civilization->UnitSounds.StepStone;
		}
		if (civilization->UnitSounds.Used.Name.empty()) {
			civilization->UnitSounds.Used = parent_civilization->UnitSounds.Used;
		}
		if (civilization->UnitSounds.Build.Name.empty()) {
			civilization->UnitSounds.Build = parent_civilization->UnitSounds.Build;
		}
		if (civilization->UnitSounds.Ready.Name.empty()) {
			civilization->UnitSounds.Ready = parent_civilization->UnitSounds.Ready;
		}
		if (civilization->UnitSounds.Repair.Name.empty()) {
			civilization->UnitSounds.Repair = parent_civilization->UnitSounds.Repair;
		}
		for (unsigned int j = 0; j < MaxCosts; ++j) {
			if (civilization->UnitSounds.Harvest[j].Name.empty()) {
				civilization->UnitSounds.Harvest[j] = parent_civilization->UnitSounds.Harvest[j];
			}
		}
		if (civilization->UnitSounds.Help.Name.empty()) {
			civilization->UnitSounds.Help = parent_civilization->UnitSounds.Help;
		}
		if (civilization->UnitSounds.HelpTown.Name.empty()) {
			civilization->UnitSounds.HelpTown = parent_civilization->UnitSounds.HelpTown;
		}
	}
	
	if (civilization->ButtonIcons.find(ButtonMove) != civilization->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 1,\n";
		button_definition += "\tAction = \"move\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"m\",\n";
		button_definition += "\tHint = _(\"~!Move\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(civilization->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (civilization->ButtonIcons.find(ButtonStop) != civilization->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 2,\n";
		button_definition += "\tAction = \"stop\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"s\",\n";
		button_definition += "\tHint = _(\"~!Stop\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(civilization->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (civilization->ButtonIcons.find(ButtonAttack) != civilization->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 3,\n";
		button_definition += "\tAction = \"attack\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"a\",\n";
		button_definition += "\tHint = _(\"~!Attack\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(civilization->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (civilization->ButtonIcons.find(ButtonPatrol) != civilization->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 4,\n";
		button_definition += "\tAction = \"patrol\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"p\",\n";
		button_definition += "\tHint = _(\"~!Patrol\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(civilization->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (civilization->ButtonIcons.find(ButtonStandGround) != civilization->ButtonIcons.end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 5,\n";
		button_definition += "\tAction = \"stand-ground\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"t\",\n";
		button_definition += "\tHint = _(\"S~!tand Ground\"),\n";
		button_definition += "\tForUnit = {\"" + std::string(civilization->GetIdent().utf8().get_data()) + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	civilization->Initialized = true;
	
	return 0;
}

/**
**  Define a word for a particular language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageWord(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	CWord *word = new CWord;
	word->Name = LuaToString(l, 1);
	
	CWord *replaces = nullptr;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			CLanguage *language = CLanguage::Get(LuaToString(l, -1));
			
			if (language) {
				word->Language = language;
				
				word->Language->Words.push_back(word);
				for (CLanguage *dialect : word->Language->Dialects) {
					dialect->Words.push_back(word); //copy the word over for dialects
				}
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				word->Meanings.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "Type")) {
			std::string word_type_name = LuaToString(l, -1);
			const CWordType *word_type = CWordType::Get(word_type_name);
			if (word_type != nullptr) {
				word->Type = word_type;
			} else {
				LuaError(l, "Word type \"%s\" doesn't exist." _C_ word_type_name.c_str());
			}
		} else if (!strcmp(value, "DerivesFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			CLanguage *derives_from_language = CLanguage::Get(LuaToString(l, -1, j + 1));
			++j;
			const CWordType *derives_from_word_type = CWordType::Get(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<String> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			if (derives_from_language && derives_from_word_type != nullptr) {
				String derives_from_word = LuaToString(l, -1, j + 1);
				CWord *derives_from = derives_from_language->GetWord(derives_from_word, derives_from_word_type, word_meanings);
				
				if (derives_from != nullptr) {
					word->DerivesFrom = derives_from;
					word->DerivesFrom->DerivesTo.push_back(word);
				} else {
					fprintf(stderr, "Word \"%s\" is set to derive from \"%s\" (%s, %s), but the latter doesn't exist.\n", word->GetIdent().utf8().get_data(), derives_from_word.utf8().get_data(), derives_from_language->GetIdent().utf8().get_data(), derives_from_word_type->GetIdent().utf8().get_data());
				}
			} else {
				fprintf(stderr, "Word \"%s\"'s derives from is incorrectly set, as either the language or the word type set for the original word given is incorrect.\n", word->GetIdent().utf8().get_data());
			}
		} else if (!strcmp(value, "Replaces")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			CLanguage *replaces_language = CLanguage::Get(LuaToString(l, -1, j + 1));
			++j;
			const CWordType *replaces_word_type = CWordType::Get(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<String> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			if (replaces_language && replaces_word_type != nullptr) {
				String replaces_word = LuaToString(l, -1, j + 1);
				replaces = replaces_language->GetWord(replaces_word, replaces_word_type, word_meanings);
				
				if (replaces == nullptr) {
					LuaError(l, "Word \"%s\" is set to replace \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->GetIdent().utf8().get_data() _C_ replaces_word.utf8().get_data() _C_ replaces_language->GetIdent().utf8().get_data() _C_ replaces_word_type->GetIdent().utf8().get_data());
				}
			} else {
				LuaError(l, "Word \"%s\"'s replace is incorrectly set, as either the language or the word type set for the original word given is incorrect" _C_ word->GetIdent().utf8().get_data());
			}
		} else if (!strcmp(value, "CompoundElements")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string affix_type_name = LuaToString(l, -1, j + 1);
				int affix_type = GetAffixTypeIdByName(affix_type_name);
				if (affix_type == -1) {
					LuaError(l, "Affix type \"%s\" doesn't exist." _C_ affix_type_name.c_str());
				}
				++j;
				
				CLanguage *affix_language = CLanguage::Get(LuaToString(l, -1, j + 1)); // should be the same language as that of the word, but needs to be specified since the word's language may not have been set yet
				++j;
				const CWordType *affix_word_type = CWordType::Get(LuaToString(l, -1, j + 1));
				++j;
				
				std::vector<String> word_meanings;
				lua_rawgeti(l, -1, j + 1);
				if (lua_istable(l, -1)) {
					const int subargs = lua_rawlen(l, -1);
					for (int k = 0; k < subargs; ++k) {
						word_meanings.push_back(LuaToString(l, -1, k + 1));
					}
					
					++j;
				}
				lua_pop(l, 1);

				if (affix_language && affix_word_type != nullptr) {
					String affix_word = LuaToString(l, -1, j + 1);
					word->CompoundElements[affix_type] = affix_language->GetWord(affix_word, affix_word_type, word_meanings);
					
					if (word->CompoundElements[affix_type] != nullptr) {
						word->CompoundElements[affix_type]->CompoundElementOf[affix_type].push_back(word);
					} else {
						LuaError(l, "Word \"%s\" is set to be a compound formed by \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->GetIdent().utf8().get_data() _C_ affix_word.utf8().get_data() _C_ affix_language->GetIdent().utf8().get_data() _C_ affix_word_type->GetIdent().utf8().get_data());
					}
				} else {
					LuaError(l, "Word \"%s\"'s compound elements are incorrectly set, as either the language or the word type set for one of the element words given is incorrect" _C_ word->GetIdent().utf8().get_data());
				}
			}
		} else if (!strcmp(value, "Gender")) {
			std::string grammatical_gender_name = LuaToString(l, -1);
			const CGrammaticalGender *grammatical_gender = CGrammaticalGender::Get(grammatical_gender_name);
			if (grammatical_gender != nullptr) {
				word->Gender = grammatical_gender;
			} else {
				LuaError(l, "Grammatical gender \"%s\" doesn't exist." _C_ grammatical_gender_name.c_str());
			}
		} else if (!strcmp(value, "GrammaticalNumber")) {
			std::string grammatical_number_name = LuaToString(l, -1);
			int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
			if (grammatical_number != -1) {
				word->GrammaticalNumber = grammatical_number;
			} else {
				LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
			}
		} else if (!strcmp(value, "Archaic")) {
			word->Archaic = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "NumberCaseInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_number_name = LuaToString(l, -1, j + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++j;
				
				std::string grammatical_case_name = LuaToString(l, -1, j + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++j;

				word->NumberCaseInflections[std::tuple<int, int>(grammatical_number, grammatical_case)] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "NumberPersonTenseMoodInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_number_name = LuaToString(l, -1, j + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++j;
				
				std::string grammatical_person_name = LuaToString(l, -1, j + 1);
				int grammatical_person = GetGrammaticalPersonIdByName(grammatical_person_name);
				if (grammatical_person == -1) {
					LuaError(l, "Grammatical person \"%s\" doesn't exist." _C_ grammatical_person_name.c_str());
				}
				++j;
				
				std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
				int grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
				if (grammatical_tense == -1) {
					LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
				}
				++j;
				
				std::string grammatical_mood_name = LuaToString(l, -1, j + 1);
				int grammatical_mood = GetGrammaticalMoodIdByName(grammatical_mood_name);
				if (grammatical_mood == -1) {
					LuaError(l, "Grammatical mood \"%s\" doesn't exist." _C_ grammatical_mood_name.c_str());
				}
				++j;

				word->NumberPersonTenseMoodInflections[std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood)] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ComparisonDegreeCaseInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string comparison_degree_name = LuaToString(l, -1, j + 1);
				int comparison_degree = GetComparisonDegreeIdByName(comparison_degree_name);
				if (comparison_degree == -1) {
					LuaError(l, "Comparison degree \"%s\" doesn't exist." _C_ comparison_degree_name.c_str());
				}
				++j;
				
				int grammatical_case = GrammaticalCaseNoCase;
				if (GetGrammaticalCaseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_case_name = LuaToString(l, -1, j + 1);
					grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
					if (grammatical_case == -1) {
						LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
					}
					++j;
				}
				
				word->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "Participles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
				int grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
				if (grammatical_tense == -1) {
					LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
				}
				++j;
				
				word->Participles[grammatical_tense] = LuaToString(l, -1, j + 1);
			}
		//noun-specific variables
		} else if (!strcmp(value, "Uncountable")) {
			word->Uncountable = LuaToBoolean(l, -1);
		//pronoun and article-specific variables
		} else if (!strcmp(value, "Nominative")) {
			word->Nominative = LuaToString(l, -1);
		} else if (!strcmp(value, "Accusative")) {
			word->Accusative = LuaToString(l, -1);
		} else if (!strcmp(value, "Dative")) {
			word->Dative = LuaToString(l, -1);
		} else if (!strcmp(value, "Genitive")) {
			word->Genitive = LuaToString(l, -1);
		//article-specific variables
		} else if (!strcmp(value, "ArticleType")) {
			std::string article_type_name = LuaToString(l, -1);
			int article_type = GetArticleTypeIdByName(article_type_name);
			if (article_type != -1) {
				word->ArticleType = article_type;
			} else {
				LuaError(l, "Article type \"%s\" doesn't exist." _C_ article_type_name.c_str());
			}
		//numeral-specific variables
		} else if (!strcmp(value, "Number")) {
			word->Number = LuaToNumber(l, -1);
		//type name variables
		} else if (!strcmp(value, "Mod")) {
			word->Mod = LuaToString(l, -1);
		} else if (!strcmp(value, "MapWord")) { //to keep backwards compatibility
			word->Mod = CMap::Map.Info.Filename.c_str();
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (replaces != nullptr) {
		word->Language->RemoveWord(replaces);
	}
	
	word->Initialize();
	
	return 0;
}

/**
**  Get a civilization's data.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string civilization_name = LuaToString(l, 1);
	CCivilization *civilization = CCivilization::Get(civilization_name);
	if (!civilization) {
		return 0;
	}
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Display")) {
		lua_pushstring(l, civilization->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, civilization->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, civilization->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, civilization->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Adjective")) {
		if (!civilization->Adjective.empty()) {
			lua_pushstring(l, civilization->Adjective.c_str());
		} else {
			lua_pushstring(l, civilization->GetName().utf8().get_data());
		}
		return 1;
	} else if (!strcmp(data, "Interface")) {
		lua_pushstring(l, civilization->GetInterface().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, civilization->IsPlayable());
		return 1;
	} else if (!strcmp(data, "Species")) {
		if (civilization->GetSpecies() != nullptr) {
			lua_pushstring(l, civilization->GetSpecies()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "ParentCivilization")) {
		if (civilization->ParentCivilization != nullptr) {
			lua_pushstring(l, civilization->ParentCivilization->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Language")) {
		CLanguage *language = civilization->GetLanguage();
		if (language != nullptr) {
			lua_pushstring(l, language->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultPlayerColor")) {
		if (civilization->DefaultPlayerColor) {
			lua_pushstring(l, civilization->DefaultPlayerColor->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "CivilizationUpgrade")) {
		if (civilization->GetUpgrade() != nullptr) {
			lua_pushstring(l, civilization->GetUpgrade()->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DevelopsFrom")) {
		lua_createtable(l, civilization->DevelopsFrom.size(), 0);
		for (size_t i = 1; i <= civilization->DevelopsFrom.size(); ++i) {
			lua_pushstring(l, civilization->DevelopsFrom[i-1]->GetIdent().utf8().get_data());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "DevelopsTo")) {
		lua_createtable(l, civilization->DevelopsTo.size(), 0);
		for (size_t i = 1; i <= civilization->DevelopsTo.size(); ++i) {
			lua_pushstring(l, civilization->DevelopsTo[i-1]->GetIdent().utf8().get_data());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Factions")) {
		bool is_mod = false;
		if (lua_gettop(l) >= 3) {
			is_mod = true;
		}
		
		std::string mod_file;

		if (is_mod) {
			mod_file = LuaToString(l, 3);
		}
		
		std::vector<std::string> factions;
		for (const CFaction *faction : CFaction::GetAll()) {
			if (faction->Civilization != civilization) {
				continue;
			}
			
			if (!is_mod || faction->Mod == mod_file) {
				factions.push_back(faction->GetIdent().utf8().get_data());
			}
		}
		
		lua_createtable(l, factions.size(), 0);
		for (size_t i = 1; i <= factions.size(); ++i) {
			lua_pushstring(l, factions[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Quests")) {
		lua_createtable(l, civilization->Quests.size(), 0);
		for (size_t i = 1; i <= civilization->Quests.size(); ++i) {
			lua_pushstring(l, civilization->Quests[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "ShipNames")) {
		lua_createtable(l, civilization->ShipNames.size(), 0);
		for (size_t i = 1; i <= civilization->ShipNames.size(); ++i) {
			lua_pushstring(l, civilization->ShipNames[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get a civilization's unit type/upgrade of a certain class.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationClassUnitType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string class_name = LuaToString(l, 1);
	const UnitClass *unit_class = UnitClass::Get(class_name);
	CCivilization *civilization = CCivilization::Get(LuaToString(l, 2));
	std::string unit_type_ident;
	if (civilization && unit_class != nullptr) {
		int unit_type_id = CCivilization::GetCivilizationClassUnitType(civilization, unit_class);
		if (unit_type_id != -1) {
			unit_type_ident = CUnitType::Get(unit_type_id)->Ident;
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		const int class_id = GetUpgradeClassIndexByName(class_name);
		if (civilization && class_id != -1) {
			int upgrade_id = CCivilization::GetCivilizationClassUpgrade(civilization, class_id);
			if (upgrade_id != -1) {
				unit_type_ident = AllUpgrades[upgrade_id]->Ident;
			}
		}
	}
	
	if (!unit_type_ident.empty()) {
		lua_pushstring(l, unit_type_ident.c_str());
	} else {
		lua_pushnil(l);
	}

	return 1;
}


/**
**  Get a faction's unit type/upgrade of a certain class.
**
**  @param l  Lua state.
*/
static int CclGetFactionClassUnitType(lua_State *l)
{
	std::string class_name = LuaToString(l, 1);
	const UnitClass *unit_class = UnitClass::Get(class_name);
	CFaction *faction = nullptr;
	const int nargs = lua_gettop(l);
	if (nargs == 2) {
		faction = CFaction::Get(LuaToString(l, 2));
	} else if (nargs == 3) {
		//the civilization was the second argument, but it isn't needed anymore
		faction = CFaction::Get(LuaToString(l, 3));
	}
	std::string unit_type_ident;
	if (unit_class != nullptr) {
		int unit_type_id = CFaction::GetFactionClassUnitType(faction, unit_class);
		if (unit_type_id != -1) {
			unit_type_ident = CUnitType::Get(unit_type_id)->Ident;
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		const int class_id = GetUpgradeClassIndexByName(class_name);
		if (class_id != -1) {
			int upgrade_id = CFaction::GetFactionClassUpgrade(faction, class_id);
			if (upgrade_id != -1) {
				unit_type_ident = AllUpgrades[upgrade_id]->Ident;
			}
		}
	}
	
	if (!unit_type_ident.empty()) {
		lua_pushstring(l, unit_type_ident.c_str());
	} else {
		lua_pushnil(l);
	}

	return 1;
}

/**
**  Define a faction.
**
**  @param l  Lua state.
*/
static int CclDefineFaction(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string faction_name = LuaToString(l, 1);
	std::string parent_faction;
	
	CFaction *faction = CFaction::GetOrAdd(faction_name);
	if (faction->GetIndex() < ((int) CFaction::GetAll().size() - 1)) { // redefinition
		if (faction->ParentFaction != nullptr) {
			parent_faction = faction->ParentFaction->GetIdent().utf8().get_data();
		}
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			CCivilization *civilization = CCivilization::Get(LuaToString(l, -1));
			if (civilization) {
				faction->Civilization = civilization;
			}
		} else if (!strcmp(value, "Name")) {
			faction->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			faction->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			faction->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			faction->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Adjective")) {
			faction->Adjective = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string faction_type_name = LuaToString(l, -1);
			int faction_type = GetFactionTypeIdByName(faction_type_name);
			if (faction_type != -1) {
				faction->Type = faction_type;
			} else {
				LuaError(l, "Faction type \"%s\" doesn't exist." _C_ faction_type_name.c_str());
			}
		} else if (!strcmp(value, "PrimaryColors")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			faction->PrimaryColors.clear(); //remove previously defined colors
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string color_name = LuaToString(l, -1, k + 1);
				CPlayerColor *player_color = CPlayerColor::Get(color_name);
				if (player_color != nullptr) {
					faction->PrimaryColors.push_back(player_color);
				}
			}
		} else if (!strcmp(value, "SecondaryColor")) {
			CPlayerColor *player_color = CPlayerColor::Get(LuaToString(l, -1));
			if (player_color != nullptr) {
				faction->SecondaryColor = player_color;
			}
		} else if (!strcmp(value, "DefaultTier")) {
			std::string faction_tier_name = LuaToString(l, -1);
			int faction_tier = GetFactionTierIdByName(faction_tier_name);
			if (faction_tier != -1) {
				faction->DefaultTier = faction_tier;
			} else {
				LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
			}
		} else if (!strcmp(value, "DefaultGovernmentType")) {
			std::string government_type_name = LuaToString(l, -1);
			int government_type = GetGovernmentTypeIdByName(government_type_name);
			if (government_type != -1) {
				faction->DefaultGovernmentType = government_type;
			} else {
				LuaError(l, "Government type \"%s\" doesn't exist." _C_ government_type_name.c_str());
			}
		} else if (!strcmp(value, "DefaultAI")) {
			faction->DefaultAI = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentFaction")) {
			parent_faction = LuaToString(l, -1);
		} else if (!strcmp(value, "Playable")) {
			faction->Playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "DefiniteArticle")) {
			faction->DefiniteArticle = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			faction->Icon = CIcon::Get(LuaToString(l, -1));
		} else if (!strcmp(value, "Currency")) {
			Currency *currency = Currency::Get(LuaToString(l, -1));
			faction->Currency = currency;
		} else if (!strcmp(value, "DevelopsFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				CFaction *second_faction = CFaction::Get(LuaToString(l, -1, k + 1));
				if (!second_faction) {
					LuaError(l, "Faction doesn't exist.");
				}
				faction->DevelopsFrom.push_back(second_faction);
				second_faction->DevelopsTo.push_back(faction);
			}
		} else if (!strcmp(value, "DevelopsTo")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				CFaction *second_faction = CFaction::Get(LuaToString(l, -1, k + 1));
				if (!second_faction) {
					LuaError(l, "Faction doesn't exist.");
				}
				faction->DevelopsTo.push_back(second_faction);
				second_faction->DevelopsFrom.push_back(faction);
			}
		} else if (!strcmp(value, "Titles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				int faction_tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				faction->Titles[government_type][faction_tier] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "MinisterTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int title = GetCharacterTitleIdByName(LuaToString(l, -1, k + 1));
				++k;
				const CGender *gender = CGender::Get(LuaToString(l, -1, k + 1), false);
				++k;
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				int faction_tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				faction->MinisterTitles[title][gender][government_type][faction_tier] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "FactionUpgrade")) {
			faction->FactionUpgrade = LuaToString(l, -1);
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				int button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != -1) {
					++j;
					faction->ButtonIcons[button_action].Name = LuaToString(l, -1, j + 1);
					faction->ButtonIcons[button_action].Icon = nullptr;
					faction->ButtonIcons[button_action].Load();
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForceTypeWeights")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			faction->ForceTypeWeights.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int force_type = GetForceTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				faction->ForceTypeWeights[force_type] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ForceTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CForceTemplate *force = new CForceTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "force-type")) {
						force->ForceType = GetForceTypeIdByName(LuaToString(l, -1, k + 1));
						if (force->ForceType == -1) {
							LuaError(l, "Force type doesn't exist.");
						}
						faction->ForceTemplates[force->ForceType].push_back(force);
					} else if (!strcmp(value, "priority")) {
						force->Priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "weight")) {
						force->Weight = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "unit-class")) {
						const UnitClass *unit_class = UnitClass::Get(LuaToString(l, -1, k + 1));
						++k;
						int unit_quantity = LuaToNumber(l, -1, k + 1);
						force->Units.push_back(std::pair<const UnitClass *, int>(unit_class, unit_quantity));
					} else {
						printf("\n%s\n", faction->GetIdent().utf8().get_data());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			for (std::map<int, std::vector<CForceTemplate *>>::iterator iterator = faction->ForceTemplates.begin(); iterator != faction->ForceTemplates.end(); ++iterator) {
				std::sort(iterator->second.begin(), iterator->second.end(), [](CForceTemplate *a, CForceTemplate *b) {
					return a->Priority > b->Priority;
				});
			}
		} else if (!strcmp(value, "AiBuildingTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CAiBuildingTemplate *building_template = new CAiBuildingTemplate;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "unit-class")) {
						const UnitClass *unit_class = UnitClass::Get(LuaToString(l, -1, k + 1));
						building_template->UnitClass = unit_class;
						faction->AiBuildingTemplates.push_back(building_template);
					} else if (!strcmp(value, "priority")) {
						building_template->Priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "per-settlement")) {
						building_template->PerSettlement = LuaToBoolean(l, -1, k + 1);
					} else {
						printf("\n%s\n", faction->GetIdent().utf8().get_data());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
			std::sort(faction->AiBuildingTemplates.begin(), faction->AiBuildingTemplates.end(), [](CAiBuildingTemplate *a, CAiBuildingTemplate *b) {
				return a->Priority > b->Priority;
			});
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			faction->UIFillers.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CFiller filler = CFiller();
				std::string filler_file = LuaToString(l, -1, j + 1);
				if (filler_file.empty()) {
					LuaError(l, "Filler graphic file is empty.");
				}
				filler.G = CGraphic::New(filler_file);
				++j;
				filler.X = LuaToNumber(l, -1, j + 1);
				++j;
				filler.Y = LuaToNumber(l, -1, j + 1);
				faction->UIFillers.push_back(filler);
			}
		} else if (!strcmp(value, "Conditions")) {
			faction->Conditions = new LuaCallback(l, -1);
		} else if (!strcmp(value, "ProvinceNames")) {
			faction->ProvinceNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ShipNames")) {
			faction->ShipNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->ShipNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "HistoricalUpgrades")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;

				std::string technology_ident = LuaToString(l, -1, j + 1);
				++j;
				
				bool has_upgrade = LuaToBoolean(l, -1, j + 1);

				faction->HistoricalUpgrades[technology_ident][date] = has_upgrade;
			}
		} else if (!strcmp(value, "HistoricalTiers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string faction_tier_name = LuaToString(l, -1, j + 1);
				int faction_tier = GetFactionTierIdByName(faction_tier_name);
				if (faction_tier == -1) {
					LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
				}
				faction->HistoricalTiers[year] = faction_tier;
			}
		} else if (!strcmp(value, "HistoricalGovernmentTypes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string government_type_name = LuaToString(l, -1, j + 1);
				int government_type = GetGovernmentTypeIdByName(government_type_name);
				if (government_type == -1) {
					LuaError(l, "Government type \"%s\" doesn't exist." _C_ government_type_name.c_str());
				}
				faction->HistoricalGovernmentTypes[year] = government_type;
			}
		} else if (!strcmp(value, "HistoricalDiplomacyStates")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				
				std::string diplomacy_state_faction_ident = LuaToString(l, -1, j + 1);
				CFaction *diplomacy_state_faction = CFaction::Get(diplomacy_state_faction_ident);
				if (diplomacy_state_faction == nullptr) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ diplomacy_state_faction_ident.c_str());
				}
				++j;

				std::string diplomacy_state_name = LuaToString(l, -1, j + 1);
				int diplomacy_state = GetDiplomacyStateIdByName(diplomacy_state_name);
				if (diplomacy_state == -1) {
					LuaError(l, "Diplomacy state \"%s\" doesn't exist." _C_ diplomacy_state_name.c_str());
				}
				faction->HistoricalDiplomacyStates[std::pair<CDate, CFaction *>(date, diplomacy_state_faction)] = diplomacy_state;
			}
		} else if (!strcmp(value, "HistoricalResources")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				
				std::string resource_ident = LuaToString(l, -1, j + 1);
				int resource = GetResourceIdByName(l, resource_ident.c_str());
				if (resource == -1) {
					LuaError(l, "Resource \"%s\" doesn't exist." _C_ resource_ident.c_str());
				}
				++j;

				faction->HistoricalResources[std::pair<CDate, int>(date, resource)] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "HistoricalCapitals")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				
				std::string site_ident = LuaToString(l, -1, j + 1);

				faction->HistoricalCapitals.push_back(std::pair<CDate, std::string>(date, site_ident));
			}
		} else if (!strcmp(value, "Mod")) {
			faction->Mod = LuaToString(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (faction->Type == FactionTypeTribe) {
		faction->DefiniteArticle = true;
	}
	
	if (!parent_faction.empty()) { //process this here
		faction->ParentFaction = CFaction::Get(parent_faction);
		
		if (faction->ParentFaction == nullptr) { //if a parent faction was set but wasn't found, give an error
			LuaError(l, "Faction %s doesn't exist" _C_ parent_faction.c_str());
		}
		
		if (faction->ParentFaction != nullptr && faction->FactionUpgrade.empty()) { //if the faction has no faction upgrade, inherit that of its parent faction
			faction->FactionUpgrade = faction->ParentFaction->FactionUpgrade;
		}
		
		if (faction->ParentFaction != nullptr) { //inherit button icons from parent civilization, for button actions which none are specified
			for (std::map<int, IconConfig>::const_iterator iterator = faction->ParentFaction->ButtonIcons.begin(); iterator != faction->ParentFaction->ButtonIcons.end(); ++iterator) {
				if (faction->ButtonIcons.find(iterator->first) == faction->ButtonIcons.end()) {
					faction->ButtonIcons[iterator->first] = iterator->second;
				}
			}
			
			for (std::map<std::string, std::map<CDate, bool>>::const_iterator iterator = faction->ParentFaction->HistoricalUpgrades.begin(); iterator != faction->ParentFaction->HistoricalUpgrades.end(); ++iterator) {
				if (faction->HistoricalUpgrades.find(iterator->first) == faction->HistoricalUpgrades.end()) {
					faction->HistoricalUpgrades[iterator->first] = iterator->second;
				}
			}
		}
	} else if (parent_faction.empty()) {
		faction->ParentFaction = nullptr; // to allow redefinitions to remove the parent faction setting
	}
	
	faction->Initialized = true;
	
	return 0;
}

/**
**  Define a dynasty.
**
**  @param l  Lua state.
*/
static int CclDefineDynasty(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string dynasty_ident = LuaToString(l, 1);
	
	CDynasty *dynasty = CDynasty::GetOrAdd(dynasty_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			CCivilization *civilization = CCivilization::Get(LuaToString(l, -1));
			if (civilization != nullptr) {
				dynasty->Civilization = civilization;
			}
		} else if (!strcmp(value, "Factions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				CFaction *faction = CFaction::Get(LuaToString(l, -1, k + 1));
				if (!faction) {
					LuaError(l, "Faction doesn't exist.");
				}
				dynasty->Factions.push_back(faction);
				faction->Dynasties.push_back(dynasty);
			}
		} else if (!strcmp(value, "DynastyUpgrade")) {
			dynasty->DynastyUpgrade = CUpgrade::Get(LuaToString(l, -1));
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a deity.
**
**  @param l  Lua state.
*/
static int CclDefineDeity(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string deity_ident = LuaToString(l, 1);
	CDeity *deity = CDeity::GetOrAdd(deity_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			deity->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Pantheon")) {
			deity->Pantheon = CPantheon::Get(LuaToString(l, -1));
		} else if (!strcmp(value, "Gender")) {
			deity->Gender = CGender::Get(LuaToString(l, -1));
		} else if (!strcmp(value, "Major")) {
			deity->Major = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Description")) {
			deity->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			deity->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			deity->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "HomePlane")) {
			CPlane *plane = CPlane::Get(LuaToString(l, -1));
			if (!plane) {
				LuaError(l, "Plane doesn't exist.");
			}
			deity->HomePlane = plane;
		} else if (!strcmp(value, "DeityUpgrade")) {
			CUpgrade *upgrade = CUpgrade::Get(LuaToString(l, -1));
			if (!upgrade) {
				LuaError(l, "Upgrade doesn't exist.");
			}
			deity->DeityUpgrade = upgrade;
			CDeity::DeitiesByUpgrade[upgrade] = deity;
		} else if (!strcmp(value, "CharacterUpgrade")) {
			CUpgrade *upgrade = CUpgrade::Get(LuaToString(l, -1));
			if (!upgrade) {
				LuaError(l, "Upgrade doesn't exist.");
			}
			deity->CharacterUpgrade = upgrade;
		} else if (!strcmp(value, "Icon")) {
			deity->Icon.Name = LuaToString(l, -1);
			deity->Icon.Icon = nullptr;
			deity->Icon.Load();
			deity->Icon.Icon->Load();
		} else if (!strcmp(value, "Civilizations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CCivilization *civilization = CCivilization::Get(LuaToString(l, -1, j + 1));
				if (civilization) {
					deity->Civilizations.push_back(civilization);
					civilization->Deities.push_back(deity);
				}
			}
		} else if (!strcmp(value, "Religions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CReligion *religion = CReligion::Get(LuaToString(l, -1, j + 1));
				if (religion) {
					deity->Religions.push_back(religion);
				}
			}
		} else if (!strcmp(value, "Domains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDeityDomain *deity_domain = CDeityDomain::Get(LuaToString(l, -1, j + 1));
				if (deity_domain) {
					deity->Domains.push_back(deity_domain);
				}
			}
		} else if (!strcmp(value, "HolyOrders")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CFaction *holy_order = CFaction::Get(LuaToString(l, -1, j + 1));
				if (!holy_order) {
					LuaError(l, "Holy order doesn't exist.");
				}

				deity->HolyOrders.push_back(holy_order);
				holy_order->HolyOrderDeity = deity;
			}
		} else if (!strcmp(value, "Abilities")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUpgrade *ability = CUpgrade::Get(LuaToString(l, -1, j + 1));
				if (!ability || !ability->Ability) {
					LuaError(l, "Ability doesn't exist.");
				}

				if (std::find(deity->Abilities.begin(), deity->Abilities.end(), ability) == deity->Abilities.end()) {
					deity->Abilities.push_back(ability);
				}
			}
		} else if (!strcmp(value, "Feasts")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string feast = LuaToString(l, -1, j + 1);

				deity->Feasts.push_back(feast);
			}
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const CCivilization *civilization = CCivilization::Get(LuaToString(l, -1, j + 1));
				++j;
				if (!civilization) {
					continue;
				}

				std::string cultural_name = LuaToString(l, -1, j + 1);
				deity->CulturalNames[civilization] = String(cultural_name.c_str());
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (deity->IsMajor() && deity->GetDomains().size() > MAJOR_DEITY_DOMAIN_MAX) {
		deity->Domains.resize(MAJOR_DEITY_DOMAIN_MAX);
	} else if (!deity->IsMajor() && deity->GetDomains().size() > MINOR_DEITY_DOMAIN_MAX) {
		deity->Domains.resize(MINOR_DEITY_DOMAIN_MAX);
	}
	
	for (CDeityDomain *domain : deity->GetDomains()) {
		for (CUpgrade *ability : domain->Abilities) {
			if (std::find(deity->Abilities.begin(), deity->Abilities.end(), ability) == deity->Abilities.end()) {
				deity->Abilities.push_back(ability);
			}
		}
	}
	
	return 0;
}

/**
**  Define a language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguage(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string language_ident = LuaToString(l, 1);
	CLanguage *language = CLanguage::GetOrAdd(language_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			language->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "NameTranslations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				String translation_from = LuaToString(l, -1, k + 1); //name to be translated
				++k;
				String translation_to = LuaToString(l, -1, k + 1); //name translation
				language->NameTranslations[translation_from].push_back(translation_to);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}
//Wyrmgus end

/**
**  Get the civilizations.
**
**  @param l  Lua state.
*/
static int CclGetCivilizations(lua_State *l)
{
	const int nargs = lua_gettop(l);
	bool ignore_hidden = false;
	if (nargs >= 1) {
		ignore_hidden = LuaToBoolean(l, 1);
	}

	std::vector<std::string> civilization_idents;
	for (const CCivilization *civilization : CCivilization::GetAll()) {
		if (ignore_hidden && civilization->IsHidden()) {
			continue;
		}
		
		civilization_idents.push_back(civilization->GetIdent().utf8().get_data());
	}

	lua_createtable(l, civilization_idents.size(), 0);
	for (unsigned int i = 1; i <= civilization_idents.size(); ++i)
	{
		lua_pushstring(l, civilization_idents[i - 1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get the factions.
**
**  @param l  Lua state.
*/
static int CclGetFactions(lua_State *l)
{
	CCivilization *civilization = nullptr;
	if (lua_gettop(l) >= 1) {
		civilization = CCivilization::Get(LuaToString(l, 1));
	}
	
	int faction_type = -1;
	if (lua_gettop(l) >= 2) {
		faction_type = GetFactionTypeIdByName(LuaToString(l, 2));
	}
	
	std::vector<std::string> factions;
	for (const CFaction *faction : CFaction::GetAll()) {
		if (faction_type != -1 && faction->Type != faction_type) {
			continue;
		}
		if (civilization == nullptr || faction->Civilization == civilization) {
			factions.push_back(faction->GetIdent().utf8().get_data());
		}
	}
		
	lua_createtable(l, factions.size(), 0);
	for (size_t i = 1; i <= factions.size(); ++i)
	{
		lua_pushstring(l, factions[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get the player colors.
**
**  @param l  Lua state.
*/
static int CclGetPlayerColors(lua_State *l)
{
	std::vector<std::string> player_colors;
	for (int i = 0; i < PlayerColorMax; ++i)
	{
		if (!PlayerColorNames[i].empty()) {
			player_colors.push_back(PlayerColorNames[i]);
		}
	}
		
	lua_createtable(l, player_colors.size(), 0);
	for (size_t i = 1; i <= player_colors.size(); ++i)
	{
		lua_pushstring(l, player_colors[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get faction data.
**
**  @param l  Lua state.
*/
static int CclGetFactionData(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string faction_name = LuaToString(l, 1);
	CFaction *faction = CFaction::Get(faction_name);
	if (faction == nullptr) {
		LuaError(l, "Faction \"%s\" doesn't exist." _C_ faction_name.c_str());
	}
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, faction->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, faction->GetDescription().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, faction->GetQuote().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, faction->GetBackground().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Adjective")) {
		if (!faction->Adjective.empty()) {
			lua_pushstring(l, faction->Adjective.c_str());
		} else {
			lua_pushstring(l, faction->GetName().utf8().get_data());
		}
		return 1;
	} else if (!strcmp(data, "Type")) {
		lua_pushstring(l, GetFactionTypeNameById(faction->Type).c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (faction->Civilization != nullptr) {
			lua_pushstring(l, faction->Civilization->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "PrimaryColor")) {
		if (faction->GetPrimaryColor() != nullptr) {
			lua_pushstring(l, faction->GetPrimaryColor()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, faction->Playable);
		return 1;
	} else if (!strcmp(data, "FactionUpgrade")) {
		lua_pushstring(l, faction->FactionUpgrade.c_str());
		return 1;
	} else if (!strcmp(data, "ParentFaction")) {
		if (faction->ParentFaction != nullptr) {
			lua_pushstring(l, faction->ParentFaction->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultAI")) {
		lua_pushstring(l, faction->DefaultAI.c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

/**
**  Define player colors
**
**  @param l  Lua state.
*/
static int CclDefinePlayerColors(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	const int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		PlayerColorNames[i / 2] = LuaToString(l, 1, i + 1);
		++i;
		lua_rawgeti(l, 1, i + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		const int numcolors = lua_rawlen(l, -1);
		if (numcolors != PlayerColorIndexCount) {
			LuaError(l, "You should use %d colors (See DefinePlayerColorIndex())" _C_ PlayerColorIndexCount);
		}
		for (int j = 0; j < numcolors; ++j) {
			lua_rawgeti(l, -1, j + 1);
			PlayerColorsRGB[i / 2][j].Parse(l);
			lua_pop(l, 1);
		}
	}

	return 0;
}

/**
**  Make new player colors
**
**  @param l  Lua state.
*/
static int CclNewPlayerColors(lua_State *l)
{
	LuaCheckArgs(l, 0);
	SetPlayersPalette();

	return 0;
}

/**
**  Define player color indexes
**
**  @param l  Lua state.
*/
static int CclDefinePlayerColorIndex(lua_State *l)
{
	LuaCheckArgs(l, 2);
	PlayerColorIndexStart = LuaToNumber(l, 1);
	PlayerColorIndexCount = LuaToNumber(l, 2);

	//Wyrmgus start
//	for (int i = 0; i < PlayerMax; ++i) {
	for (int i = 0; i < PlayerColorMax; ++i) {
	//Wyrmgus end
		PlayerColorsRGB[i].clear();
		PlayerColorsRGB[i].resize(PlayerColorIndexCount);
		PlayerColors[i].clear();
		PlayerColors[i].resize(PlayerColorIndexCount, 0);
	}
	return 0;
}

/**
**  Define conversible player colors.
**
**  @param l  Lua state.
*/
static int CclDefineConversiblePlayerColors(lua_State *l)
{
	ConversiblePlayerColors.clear();
	
	const unsigned int args = lua_gettop(l);
	for (unsigned int i = 0; i < args; ++i) {
		std::string player_color_name = LuaToString(l, i + 1);
		int player_color = GetPlayerColorIndexByName(player_color_name);
		if (player_color != -1) {
			ConversiblePlayerColors.push_back(player_color);
		} else {
			LuaError(l, "Player color \"%s\" doesn't exist." _C_ player_color_name.c_str());
		}
	}
	
	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Get player data.
**
**  @param l  Lua state.
*/
static int CclGetPlayerData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	const CPlayer *p = CclGetPlayer(l);
	lua_pop(l, 1);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, p->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Type")) {
		lua_pushnumber(l, p->Type);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		if (p->Race != -1 && p->GetFaction() != nullptr) {
			lua_pushstring(l, p->GetFaction()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Dynasty")) {
		if (p->Dynasty) {
			lua_pushstring(l, p->Dynasty->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "RaceName")) {
		lua_pushstring(l, CCivilization::Get(p->Race)->GetIdent().utf8().get_data());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Color")) {
		bool found_color = false;
		for (int i = 0; i < PlayerColorMax; ++i) {
			if (PlayerColors[i][0] == p->Color) {
				lua_pushstring(l, PlayerColorNames[i].c_str());
				found_color = true;
				break;
			}		
		}
		if (!found_color) {
			LuaError(l, "Player %d has no color." _C_ p->Index);
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "StartPosX")) {
		lua_pushnumber(l, p->StartPos.x);
		return 1;
	} else if (!strcmp(data, "StartPosY")) {
		lua_pushnumber(l, p->StartPos.y);
		return 1;
	} else if (!strcmp(data, "StartMapLayer")) {
		lua_pushnumber(l, p->StartMapLayer);
		return 1;
	} else if (!strcmp(data, "AiName")) {
		lua_pushstring(l, p->AiName.c_str());
		return 1;
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->Resources[resId] + p->StoredResources[resId]);
		return 1;
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->StoredResources[resId]);
		return 1;
	} else if (!strcmp(data, "MaxResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->MaxResources[resId]);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Prices")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetResourcePrice(resId));
		return 1;
	} else if (!strcmp(data, "ResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->ResourceDemand[resId]);
		return 1;
	} else if (!strcmp(data, "StoredResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->StoredResourceDemand[resId]);
		return 1;
	} else if (!strcmp(data, "EffectiveResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetEffectiveResourceDemand(resId));
		return 1;
	} else if (!strcmp(data, "EffectiveResourceSellPrice")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetEffectiveResourceSellPrice(resId));
		return 1;
	} else if (!strcmp(data, "EffectiveResourceBuyPrice")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->GetEffectiveResourceBuyPrice(resId));
		return 1;
	} else if (!strcmp(data, "TotalPriceDifferenceWith")) {
		LuaCheckArgs(l, 3);
		
		int other_player = LuaToNumber(l, 3);;

		lua_pushnumber(l, p->GetTotalPriceDifferenceWith(*CPlayer::Players[other_player]));
		return 1;
	} else if (!strcmp(data, "TradePotentialWith")) {
		LuaCheckArgs(l, 3);
		
		int other_player = LuaToNumber(l, 3);;

		lua_pushnumber(l, p->GetTradePotentialWith(*CPlayer::Players[other_player]));
		return 1;
	} else if (!strcmp(data, "HasHero")) {
		LuaCheckArgs(l, 3);
		
		CCharacter *hero = CCharacter::Get(LuaToString(l, 3));

		lua_pushboolean(l, p->HasHero(hero));
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "UnitTypesCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->GetUnitTypeCount(type));
		return 1;
	} else if (!strcmp(data, "UnitTypesUnderConstructionCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->GetUnitTypeUnderConstructionCount(type));
		return 1;
	} else if (!strcmp(data, "UnitTypesAiActiveCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->GetUnitTypeAiActiveCount(type));
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Heroes")) {
		lua_createtable(l, p->Heroes.size(), 0);
		for (size_t i = 1; i <= p->Heroes.size(); ++i)
		{
			lua_pushstring(l, p->Heroes[i-1]->Character->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "AiEnabled")) {
		lua_pushboolean(l, p->AiEnabled);
		return 1;
	} else if (!strcmp(data, "TotalNumUnits")) {
		lua_pushnumber(l, p->GetUnitCount());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "TotalNumUnitsConstructed")) {
		lua_pushnumber(l, p->GetUnitCount() - p->NumBuildingsUnderConstruction);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "NumBuildings")) {
		lua_pushnumber(l, p->NumBuildings);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "NumBuildingsUnderConstruction")) {
		lua_pushnumber(l, p->NumBuildingsUnderConstruction);
		return 1;
	} else if (!strcmp(data, "NumTownHalls")) {
		lua_pushnumber(l, p->NumTownHalls);
		return 1;
	} else if (!strcmp(data, "NumHeroes")) {
		lua_pushnumber(l, p->Heroes.size());
		return 1;
	} else if (!strcmp(data, "TradeCost")) {
		lua_pushnumber(l, p->TradeCost);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Supply")) {
		lua_pushnumber(l, p->Supply);
		return 1;
	} else if (!strcmp(data, "Demand")) {
		lua_pushnumber(l, p->Demand);
		return 1;
	} else if (!strcmp(data, "UnitLimit")) {
		lua_pushnumber(l, p->UnitLimit);
		return 1;
	} else if (!strcmp(data, "BuildingLimit")) {
		lua_pushnumber(l, p->BuildingLimit);
		return 1;
	} else if (!strcmp(data, "TotalUnitLimit")) {
		lua_pushnumber(l, p->TotalUnitLimit);
		return 1;
	} else if (!strcmp(data, "Score")) {
		lua_pushnumber(l, p->Score);
		return 1;
	} else if (!strcmp(data, "TotalUnits")) {
		lua_pushnumber(l, p->TotalUnits);
		return 1;
	} else if (!strcmp(data, "TotalBuildings")) {
		lua_pushnumber(l, p->TotalBuildings);
		return 1;
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->TotalResources[resId]);
		return 1;
	} else if (!strcmp(data, "TotalRazings")) {
		lua_pushnumber(l, p->TotalRazings);
		return 1;
	} else if (!strcmp(data, "TotalKills")) {
		lua_pushnumber(l, p->TotalKills);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "UnitTypeKills")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypeKills[type->GetIndex()]);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->SpeedResourcesHarvest[resId]);
		return 1;
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->SpeedResourcesReturn[resId]);
		return 1;
	} else if (!strcmp(data, "SpeedBuild")) {
		lua_pushnumber(l, p->SpeedBuild);
		return 1;
	} else if (!strcmp(data, "SpeedTrain")) {
		lua_pushnumber(l, p->SpeedTrain);
		return 1;
	} else if (!strcmp(data, "SpeedUpgrade")) {
		lua_pushnumber(l, p->SpeedUpgrade);
		return 1;
	} else if (!strcmp(data, "SpeedResearch")) {
		lua_pushnumber(l, p->SpeedResearch);
		return 1;
	} else if (!strcmp(data, "Allow")) {
		LuaCheckArgs(l, 3);
		const char *ident = LuaToString(l, 3);
		if (!strncmp(ident, "unit-", 5)) {
			int id = UnitTypeIdByIdent(ident);
			if (UnitIdAllowed(*CPlayer::Players[p->Index], id) > 0) {
				lua_pushstring(l, "A");
			} else if (UnitIdAllowed(*CPlayer::Players[p->Index], id) == 0) {
				lua_pushstring(l, "F");
			}
		} else if (!strncmp(ident, "upgrade-", 8)) {
			if (UpgradeIdentAllowed(*CPlayer::Players[p->Index], ident) == 'A') {
				lua_pushstring(l, "A");
			} else if (UpgradeIdentAllowed(*CPlayer::Players[p->Index], ident) == 'R') {
				lua_pushstring(l, "R");
			} else if (UpgradeIdentAllowed(*CPlayer::Players[p->Index], ident) == 'F') {
				lua_pushstring(l, "F");
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
		}
		return 1;
	} else if (!strcmp(data, "IsAllied")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->IsAllied(*CPlayer::Players[second_player]));
		return 1;
	} else if (!strcmp(data, "IsEnemy")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->IsEnemy(*CPlayer::Players[second_player]));
		return 1;
	} else if (!strcmp(data, "IsSharedVision")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->IsSharedVision(*CPlayer::Players[second_player]));
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "HasContactWith")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->HasContactWith(*CPlayer::Players[second_player]));
		return 1;
	} else if (!strcmp(data, "HasQuest")) {
		LuaCheckArgs(l, 3);
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (std::find(p->CurrentQuests.begin(), p->CurrentQuests.end(), quest) != p->CurrentQuests.end()) {
			lua_pushboolean(l, true);
		} else {
			lua_pushboolean(l, false);
		}
		return 1;
	} else if (!strcmp(data, "CompletedQuest")) {
		LuaCheckArgs(l, 3);
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (std::find(p->CompletedQuests.begin(), p->CompletedQuests.end(), quest) != p->CompletedQuests.end()) {
			lua_pushboolean(l, true);
		} else {
			lua_pushboolean(l, false);
		}
		return 1;
	} else if (!strcmp(data, "FactionTitle")) {
		lua_pushstring(l, p->GetFactionTitleName().c_str());
		return 1;
	} else if (!strcmp(data, "CharacterTitle")) {
		LuaCheckArgs(l, 4);
		std::string title_type_ident = LuaToString(l, 3);
		std::string gender_ident = LuaToString(l, 4);
		int title_type_id = GetCharacterTitleIdByName(title_type_ident);
		const CGender *gender = CGender::Get(gender_ident);
		
		lua_pushstring(l, p->GetCharacterTitleName(title_type_id, gender).c_str());
		return 1;
	} else if (!strcmp(data, "HasSettlement")) {
		LuaCheckArgs(l, 3);
		std::string site_ident = LuaToString(l, 3);
		CSite *site = CSite::Get(site_ident);
		lua_pushboolean(l, p->HasSettlement(site));
		return 1;
	} else if (!strcmp(data, "SettlementName")) {
		LuaCheckArgs(l, 3);
		std::string site_ident = LuaToString(l, 3);
		const CSite *site = CSite::Get(site_ident);
		if (site) {
			lua_pushstring(l, site->GetCulturalName(CCivilization::Get(p->Race)).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Currency")) {
		const Currency *currency = p->GetCurrency();
		if (currency) {
			lua_pushstring(l, currency->GetName().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Set player data.
**
**  @param l  Lua state.
*/
static int CclSetPlayerData(lua_State *l)
{
	if (lua_gettop(l) < 3) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	CPlayer *p = CclGetPlayer(l);
	lua_pop(l, 1);
	const char *data = LuaToString(l, 2);
	
	//Wyrmgus start
	//if player is unused, return
	if (p->Type == PlayerNobody && Editor.Running == EditorNotRunning) {
		return 0;
	}
	//Wyrmgus end

	if (!strcmp(data, "Name")) {
		p->SetName(LuaToString(l, 3));
	} else if (!strcmp(data, "Type")) {
		p->Type = LuaToNumber(l, 3);
	} else if (!strcmp(data, "RaceName")) {
		const char *civilization_ident = LuaToString(l, 3);
		CCivilization *civilization = CCivilization::Get(civilization_ident);
		
		if (Editor.Running == EditorNotRunning) {
			if (GameRunning) {
				p->SetFaction(nullptr);
			}

			if (civilization) {
				p->SetCivilization(civilization->GetIndex());
			}
		} else {
			p->Race = civilization->GetIndex();
		}
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		std::string faction_name = LuaToString(l, 3);
		if (faction_name == "random") {
			p->SetRandomFaction();
		} else {
			p->SetFaction(CFaction::Get(faction_name));
		}
	} else if (!strcmp(data, "Dynasty")) {
		std::string dynasty_ident = LuaToString(l, 3);
		p->SetDynasty(CDynasty::Get(dynasty_ident));
	//Wyrmgus end
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SetResource(resId, LuaToNumber(l, 4));
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SetResource(resId, LuaToNumber(l, 4), STORE_BUILDING);
		// } else if (!strcmp(data, "UnitTypesCount")) {
		// } else if (!strcmp(data, "AiEnabled")) {
		// } else if (!strcmp(data, "TotalNumUnits")) {
		// } else if (!strcmp(data, "NumBuildings")) {
		// } else if (!strcmp(data, "Supply")) {
		// } else if (!strcmp(data, "Demand")) {
	} else if (!strcmp(data, "UnitLimit")) {
		p->UnitLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "BuildingLimit")) {
		p->BuildingLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalUnitLimit")) {
		p->TotalUnitLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "Score")) {
		p->Score = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalUnits")) {
		p->TotalUnits = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalBuildings")) {
		p->TotalBuildings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->TotalResources[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "TotalRazings")) {
		p->TotalRazings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalKills")) {
		p->TotalKills = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SpeedResourcesHarvest[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SpeedResourcesReturn[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "SpeedBuild")) {
		p->SpeedBuild = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedTrain")) {
		p->SpeedTrain = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedUpgrade")) {
		p->SpeedUpgrade = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedResearch")) {
		p->SpeedResearch = LuaToNumber(l, 3);
	} else if (!strcmp(data, "Allow")) {
		LuaCheckArgs(l, 4);
		const char *ident = LuaToString(l, 3);
		const std::string acquire = LuaToString(l, 4);

		if (!strncmp(ident, "upgrade-", 8)) {
			if (acquire == "R" && UpgradeIdentAllowed(*p, ident) != 'R') {
				UpgradeAcquire(*p, CUpgrade::Get(ident));
			} else if (acquire == "F" || acquire == "A") {
				if (UpgradeIdentAllowed(*p, ident) == 'R') {
					UpgradeLost(*p, CUpgrade::Get(ident)->ID);
				}
				AllowUpgradeId(*p, UpgradeIdByIdent(ident), acquire[0]);
			}
		//Wyrmgus start
		} else if (!strncmp(ident, "unit-", 5)) {
			const int UnitMax = 65536; /// How many units supported
			int id = UnitTypeIdByIdent(ident);
			if (acquire == "A" || acquire == "R") {
				AllowUnitId(*p, id, UnitMax);
			} else if (acquire == "F") {
				AllowUnitId(*p, id, 0);
			}
		//Wyrmgus end
		} else {
			LuaError(l, " wrong ident %s\n" _C_ ident);
		}
	//Wyrmgus start
	} else if (!strcmp(data, "AiEnabled")) {
		p->AiEnabled = LuaToBoolean(l, 3);
	} else if (!strcmp(data, "Team")) {
		p->Team = LuaToNumber(l, 3);
	} else if (!strcmp(data, "AcceptQuest")) {
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (quest) {
			p->AcceptQuest(quest);
		}
	} else if (!strcmp(data, "CompleteQuest")) {
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (quest) {
			p->CompleteQuest(quest);
		}
	} else if (!strcmp(data, "FailQuest")) {
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (quest) {
			p->FailQuest(quest);
		}
	} else if (!strcmp(data, "AddModifier")) {
		LuaCheckArgs(l, 4);
		CUpgrade *modifier_upgrade = CUpgrade::Get(LuaToString(l, 3));
		int cycles = LuaToNumber(l, 4);
		if (modifier_upgrade) {
			p->AddModifier(modifier_upgrade, cycles);
		}
	//Wyrmgus end
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Set ai player algo.
**
**  @param l  Lua state.
*/
static int CclSetAiType(lua_State *l)
{
	CPlayer *p;

	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	p = CclGetPlayer(l);
	lua_pop(l, 1);

	p->AiName = LuaToString(l, 2);

	return 0;
}

//Wyrmgus start
/**
**  Init ai for player.
**
**  @param l  Lua state.
*/
static int CclInitAi(lua_State *l)
{
	CPlayer *p;

	if (lua_gettop(l) < 1) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	p = CclGetPlayer(l);
	lua_pop(l, 1);

	AiInit(*p);

	return 0;
}

static int CclGetLanguages(lua_State *l)
{
	bool only_used = false;
	if (lua_gettop(l) >= 1) {
		only_used = LuaToBoolean(l, 1);
	}
	
	std::vector<std::string> languages;
	for (const CLanguage *language : CLanguage::GetAll()) {
		if (!only_used || language->UsedByCivilizationOrFaction) {
			languages.push_back(language->Ident);
		}
	}
		
	lua_createtable(l, languages.size(), 0);
	for (size_t i = 1; i <= languages.size(); ++i)
	{
		lua_pushstring(l, languages[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get language data.
**
**  @param l  Lua state.
*/
static int CclGetLanguageData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string language_name = LuaToString(l, 1);
	const CLanguage *language = CLanguage::Get(language_name);
	if (!language) {
		LuaError(l, "Language \"%s\" doesn't exist." _C_ language_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, language->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Family")) {
		if (language->Family != nullptr) {
			lua_pushstring(l, language->Family->GetName().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Words")) {
		lua_createtable(l, language->Words.size(), 0);
		for (size_t i = 1; i <= language->Words.size(); ++i)
		{
			lua_pushstring(l, language->Words[i-1]->GetName().utf8().get_data());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get language word data.
**
**  @param l  Lua state.
*/
static int CclGetLanguageWordData(lua_State *l)
{
	if (lua_gettop(l) < 3) {
		LuaError(l, "incorrect argument");
	}
	std::string language_name = LuaToString(l, 1);
	const CLanguage *language = CLanguage::Get(language_name);
	if (!language) {
		LuaError(l, "Language \"%s\" doesn't exist." _C_ language_name.c_str());
	}
	
	String word_name = LuaToString(l, 2);
	std::vector<String> word_meanings;
	const CWord *word = language->GetWord(word_name, nullptr, word_meanings);
	if (word == nullptr) {
		LuaError(l, "Word \"%s\" doesn't exist for the \"%s\" language." _C_ word_name.utf8().get_data() _C_ language_name.c_str());
	}
	
	const char *data = LuaToString(l, 3);

	if (!strcmp(data, "Type")) {
		if (word->GetType() != nullptr) {
			lua_pushstring(l, word->GetType()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Meaning")) {
		for (size_t i = 0; i < word->GetMeanings().size(); ++i) {
			lua_pushstring(l, word->GetMeanings()[i].utf8().get_data());
			return 1;
		}
		lua_pushstring(l, "");
		return 1;
	} else if (!strcmp(data, "Gender")) {
		if (word->GetGender() != nullptr) {
			lua_pushstring(l, word->GetGender()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetDeityDomains(lua_State *l)
{
	lua_createtable(l, CDeityDomain::GetAll().size(), 0);
	for (size_t i = 1; i <= CDeityDomain::GetAll().size(); ++i)
	{
		lua_pushstring(l, CDeityDomain::GetAll()[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetDeities(lua_State *l)
{
	lua_createtable(l, CDeity::GetAll().size(), 0);
	for (size_t i = 1; i <= CDeity::GetAll().size(); ++i)
	{
		lua_pushstring(l, CDeity::GetAll()[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get deity domain data.
**
**  @param l  Lua state.
*/
static int CclGetDeityDomainData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string deity_domain_ident = LuaToString(l, 1);
	const CDeityDomain *deity_domain = CDeityDomain::Get(deity_domain_ident);
	if (!deity_domain) {
		return 0;
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, deity_domain->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, deity_domain->Abilities.size(), 0);
		for (size_t i = 1; i <= deity_domain->Abilities.size(); ++i)
		{
			lua_pushstring(l, deity_domain->Abilities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get deity data.
**
**  @param l  Lua state.
*/
static int CclGetDeityData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string deity_ident = LuaToString(l, 1);
	const CDeity *deity = CDeity::Get(deity_ident);
	if (!deity) {
		return 0;
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, deity->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Pantheon")) {
		if (deity->Pantheon) {
			lua_pushstring(l, deity->Pantheon->GetName().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, deity->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, deity->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, deity->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Major")) {
		lua_pushboolean(l, deity->IsMajor());
		return 1;
	} else if (!strcmp(data, "HomePlane")) {
		if (deity->HomePlane) {
			lua_pushstring(l, deity->HomePlane->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, deity->Icon.Name.c_str());
		return 1;
	} else if (!strcmp(data, "Civilizations")) {
		lua_createtable(l, deity->Civilizations.size(), 0);
		for (size_t i = 1; i <= deity->Civilizations.size(); ++i)
		{
			lua_pushstring(l, deity->Civilizations[i-1]->GetIdent().utf8().get_data());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Religions")) {
		lua_createtable(l, deity->Religions.size(), 0);
		for (size_t i = 1; i <= deity->Religions.size(); ++i)
		{
			lua_pushstring(l, deity->Religions[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Domains")) {
		lua_createtable(l, deity->GetDomains().size(), 0);
		for (size_t i = 1; i <= deity->GetDomains().size(); ++i)
		{
			lua_pushstring(l, deity->GetDomains()[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, deity->Abilities.size(), 0);
		for (size_t i = 1; i <= deity->Abilities.size(); ++i)
		{
			lua_pushstring(l, deity->Abilities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "CulturalName")) {
		if (lua_gettop(l) < 3) {
			LuaError(l, "incorrect argument");
		}
		
		const CCivilization *civilization = CCivilization::Get(LuaToString(l, 3));
		lua_pushstring(l, deity->GetCulturalName(civilization).utf8().get_data());
		
		return 1;
	} else if (!strcmp(data, "Gender")) {
		if (deity->GetGender() != nullptr) {
			lua_pushstring(l, deity->GetGender()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

// ----------------------------------------------------------------------------

/**
**  Register CCL features for players.
*/
void PlayerCclRegister()
{
	lua_register(Lua, "Player", CclPlayer);
	lua_register(Lua, "ChangeUnitsOwner", CclChangeUnitsOwner);
	lua_register(Lua, "GetThisPlayer", CclGetThisPlayer);
	lua_register(Lua, "SetThisPlayer", CclSetThisPlayer);

	lua_register(Lua, "SetMaxSelectable", CclSetMaxSelectable);

	lua_register(Lua, "SetAllPlayersUnitLimit", CclSetAllPlayersUnitLimit);
	lua_register(Lua, "SetAllPlayersBuildingLimit", CclSetAllPlayersBuildingLimit);
	lua_register(Lua, "SetAllPlayersTotalUnitLimit", CclSetAllPlayersTotalUnitLimit);

	lua_register(Lua, "SetDiplomacy", CclSetDiplomacy);
	lua_register(Lua, "Diplomacy", CclDiplomacy);
	lua_register(Lua, "SetSharedVision", CclSetSharedVision);
	lua_register(Lua, "SharedVision", CclSharedVision);

	//Wyrmgus start
	lua_register(Lua, "DefineCivilization", CclDefineCivilization);
	lua_register(Lua, "DefineLanguageWord", CclDefineLanguageWord);
	lua_register(Lua, "GetCivilizationData", CclGetCivilizationData);
	lua_register(Lua, "GetCivilizationClassUnitType", CclGetCivilizationClassUnitType);
	lua_register(Lua, "GetFactionClassUnitType", CclGetFactionClassUnitType);
	lua_register(Lua, "DefineFaction", CclDefineFaction);
	lua_register(Lua, "DefineDynasty", CclDefineDynasty);
	lua_register(Lua, "DefineDeity", CclDefineDeity);
	lua_register(Lua, "DefineLanguage", CclDefineLanguage);
	lua_register(Lua, "GetCivilizations", CclGetCivilizations);
	lua_register(Lua, "GetFactions", CclGetFactions);
	lua_register(Lua, "GetPlayerColors", CclGetPlayerColors);
	lua_register(Lua, "GetFactionData", CclGetFactionData);
	//Wyrmgus end
	lua_register(Lua, "DefinePlayerColors", CclDefinePlayerColors);
	lua_register(Lua, "DefinePlayerColorIndex", CclDefinePlayerColorIndex);

	lua_register(Lua, "NewColors", CclNewPlayerColors);

	lua_register(Lua, "DefineConversiblePlayerColors", CclDefineConversiblePlayerColors);
	
	// player member access functions
	lua_register(Lua, "GetPlayerData", CclGetPlayerData);
	lua_register(Lua, "SetPlayerData", CclSetPlayerData);
	lua_register(Lua, "SetAiType", CclSetAiType);
	//Wyrmgus start
	lua_register(Lua, "InitAi", CclInitAi);
	lua_register(Lua, "GetLanguages", CclGetLanguages);
	lua_register(Lua, "GetLanguageData", CclGetLanguageData);
	lua_register(Lua, "GetLanguageWordData", CclGetLanguageWordData);
	
	lua_register(Lua, "GetDeityDomains", CclGetDeityDomains);
	lua_register(Lua, "GetDeities", CclGetDeities);
	lua_register(Lua, "GetDeityDomainData", CclGetDeityDomainData);
	lua_register(Lua, "GetDeityData", CclGetDeityData);
	//Wyrmgus end
}
