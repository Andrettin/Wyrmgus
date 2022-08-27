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
//      (c) Copyright 2001-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "player/player.h"

#include "actions.h"
#include "age.h"
#include "ai.h"
#include "ai/ai_force_template.h"
#include "ai/ai_force_type.h"
#include "character.h"
#include "character_title.h"
#include "commands.h"
#include "currency.h"
#include "economy/resource_storage_type.h"
#include "editor.h"
#include "game/game.h"
#include "gender.h"
#include "grand_strategy.h"
#include "language/grammatical_gender.h"
#include "language/language.h"
#include "language/language_family.h"
#include "language/word.h"
#include "language/word_type.h"
#include "luacallback.h"
#include "magic_domain.h"
#include "map/map.h"
#include "map/site.h"
#include "map/world.h"
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "player/diplomacy_state.h"
#include "player/dynasty.h"
#include "player/faction.h"
#include "player/faction_type.h"
#include "player/government_type.h"
#include "player/player_color.h"
#include "player/player_flag.h"
#include "player/player_type.h"
#include "player/vassalage_type.h"
#include "quest/player_quest_objective.h"
#include "quest/quest.h"
#include "religion/deity.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "script.h"
#include "species/species.h"
#include "spell/spell.h"
#include "time/calendar.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/interface_style.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "util/assert_util.h"
#include "util/string_util.h"
#include "util/util.h"
#include "util/vector_util.h"
#include "video/font.h"
#include "video/video.h"

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
	return CPlayer::Players[LuaToNumber(l, -1)].get();
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

	if (NumPlayers <= i) {
		NumPlayers = i + 1;
	}

	player.Load(l);
	return 0;
}

void CPlayer::Load(lua_State *l)
{
	const int args = lua_gettop(l);

	this->Units.clear();
	this->FreeWorkers.clear();
	//Wyrmgus start
	this->LevelUpUnits.clear();
	//Wyrmgus end
	this->set_alive(false);

	// j = 0 represent player Index.
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "name")) {
			this->set_name(LuaToString(l, j + 1));
		} else if (!strcmp(value, "type")) {
			value = LuaToString(l, j + 1);
			this->set_type(string_to_player_type(value));
		} else if (!strcmp(value, "civilization")) {
			const char *civilization_ident = LuaToString(l, j + 1);
			civilization *civilization = civilization::get(civilization_ident);
			this->Race = civilization->ID;
		} else if (!strcmp(value, "faction")) {
			this->faction = faction::get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "faction-tier")) {
			this->faction_tier = string_to_faction_tier(LuaToString(l, j + 1));
		} else if (!strcmp(value, "government-type")) {
			this->government_type = string_to_government_type(LuaToString(l, j + 1));
		} else if (!strcmp(value, "dynasty")) {
			this->dynasty = dynasty::get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "age")) {
			this->age = age::get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "player-color")) {
			this->player_color = player_color::get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "ai-name")) {
			this->AiName = LuaToString(l, j + 1);
		} else if (!strcmp(value, "team")) {
			this->Team = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "enemy")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->enemies.erase(i);
				} else {
					this->enemies.insert(i);
				}
			}
		} else if (!strcmp(value, "allied")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->allies.erase(i);
				} else {
					this->allies.insert(i);
				}
			}
		} else if (!strcmp(value, "shared-vision")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->shared_vision.erase(i);
				} else {
					this->shared_vision.insert(i);
				}
			}
		} else if (!strcmp(value, "mutual-shared-vision")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->mutual_shared_vision.erase(i);
				} else {
					this->mutual_shared_vision.insert(i);
				}
			}
		} else if (!strcmp(value, "start")) {
			CclGetPos(l, &this->StartPos.x, &this->StartPos.y, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "start-map-layer")) {
			this->StartMapLayer = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "overlord")) {
			const int overlord_id = LuaToNumber(l, j + 1);
			++j;
			const wyrmgus::vassalage_type vassalage_type = string_to_vassalage_type(LuaToString(l, j + 1));
			this->set_overlord(CPlayer::Players[overlord_id].get(), vassalage_type);
		//Wyrmgus end
		} else if (!strcmp(value, "resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const resource *res = resource::get(value);
				this->set_resource(res, LuaToNumber(l, j + 1, k + 1));
			}
		} else if (!strcmp(value, "stored-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const resource *res = resource::get(value);
				this->set_stored_resource(res, LuaToNumber(l, j + 1, k + 1));
			}
		} else if (!strcmp(value, "max-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const resource *res = resource::get(value);
				this->max_resources[res] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "incomes")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const resource *res = resource::get(value);
				this->set_income(res, LuaToNumber(l, j + 1, k + 1));
			}
		} else if (!strcmp(value, "income-modifiers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const resource *res = resource::get(value);
				this->set_income_modifier(res, LuaToNumber(l, j + 1, k + 1));
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

				const resource *res = resource::get(value);
				this->set_price(res, LuaToNumber(l, j + 1, k + 1));
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
			this->set_revealed(true);
			--j;
		//Wyrmgus end
		} else if (!strcmp(value, "supply")) {
			this->supply = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "demand")) {
			this->demand = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "trade-cost")) {
			this->trade_cost = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unit-limit")) {
			this->UnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "building-limit")) {
			this->BuildingLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-unit-limit")) {
			this->TotalUnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "score")) {
			this->score = LuaToNumber(l, j + 1);
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
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, j + 1, k + 1));
				++k;
				this->UnitTypeKills[unit_type->Slot] = LuaToNumber(l, j + 1, k + 1);
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
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const resource *res = resource::get(value);
				this->set_resource_total(res, LuaToNumber(l, j + 1, k + 1));
			}
		} else if (!strcmp(value, "speed-resource-harvest")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const resource *res = resource::get(value);
				this->set_resource_harvest_speed(res, LuaToNumber(l, j + 1, k + 1));
			}
		} else if (!strcmp(value, "speed-resource-return")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const resource *res = resource::get(value);
				this->set_resource_return_speed(res, LuaToNumber(l, j + 1, k + 1));
			}
		} else if (!strcmp(value, "speed-build")) {
			this->SpeedBuild = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-train")) {
			this->SpeedTrain = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-upgrade")) {
			this->SpeedUpgrade = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-research")) {
			this->SpeedResearch = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "infantry-cost-modifier")) {
			this->infantry_cost_modifier = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "cavalry-cost-modifier")) {
			this->cavalry_cost_modifier = LuaToNumber(l, j + 1);
		//Wyrmgus start
		/*
		} else if (!strcmp(value, "color")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			const int r = LuaToNumber(l, j + 1, 1);
			const int g = LuaToNumber(l, j + 1, 2);
			const int b = LuaToNumber(l, j + 1, 3);
			this->Color = CVideo::MapRGB(r, g, b);
		*/
		//Wyrmgus end
		//Wyrmgus start
		} else if (!strcmp(value, "current-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				wyrmgus::quest *quest = wyrmgus::quest::get(LuaToString(l, j + 1, k + 1));
				this->current_quests.push_back(quest);
			}
		} else if (!strcmp(value, "completed-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				wyrmgus::quest *quest = wyrmgus::quest::get(LuaToString(l, j + 1, k + 1));
				this->completed_quests.push_back(quest);
				if (quest->is_competitive()) {
					quest->CurrentCompleted = true;
				}
			}
		} else if (!strcmp(value, "quest-objectives")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, j + 1, k + 1);
				const wyrmgus::quest *quest = nullptr;
				wyrmgus::player_quest_objective *objective = nullptr;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for quest objectives)");
				}
				const int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					value = LuaToString(l, -1, n + 1);
					++n;
					if (!strcmp(value, "quest")) {
						quest = wyrmgus::quest::get(LuaToString(l, -1, n + 1));
					} else if (!strcmp(value, "objective-index")) {
						const int objective_index = LuaToNumber(l, -1, n + 1);
						auto objective_unique_ptr = std::make_unique<wyrmgus::player_quest_objective>(quest->get_objectives()[objective_index].get(), this);
						objective = objective_unique_ptr.get();
						this->quest_objectives.push_back(std::move(objective_unique_ptr));
					} else if (!strcmp(value, "counter")) {
						objective->set_counter(LuaToNumber(l, -1, n + 1));
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
				const CUpgrade *modifier_upgrade = CUpgrade::get(LuaToString(l, j + 1, k + 1));
				++k;
				const int end_cycle = LuaToNumber(l, j + 1, k + 1);
				this->modifier_last_cycles[modifier_upgrade] = end_cycle;
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
		} else if (!strcmp(value, "flags")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const player_flag *flag = player_flag::try_get(LuaToString(l, j + 1, k + 1));
				if (flag != nullptr) {
					this->set_flag(flag);
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
				CUpgrade *timer_upgrade = CUpgrade::get(LuaToString(l, j + 1, k + 1));
				++k;
				this->UpgradeTimers.Upgrades[timer_upgrade->ID] = LuaToNumber(l, j + 1, k + 1);
				//Wyrmgus end
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	// Manage max
	for (const resource *resource : resource::get_all()) {
		if (this->get_max_resource(resource) != -1) {
			this->set_resource(resource, this->get_resource(resource) + this->get_stored_resource(resource), resource_storage_type::both);
		}
	}

	emit diplomatic_stances_changed();
	emit shared_vision_changed();
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
		lua_pushnumber(l, CPlayer::GetThisPlayer()->get_index());
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

	CPlayer::SetThisPlayer(CPlayer::Players[plynr].get());
	
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

	SendCommandDiplomacy(base, string_to_diplomacy_state(state), plynr);
	return 0;
}

/**
**  Change the diplomacy from ThisPlayer to another player.
**
**  @param l  Lua state.
*/
static int CclDiplomacy(lua_State *l)
{
	lua_pushnumber(l, CPlayer::GetThisPlayer()->get_index());
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
	lua_pushnumber(l, CPlayer::GetThisPlayer()->get_index());
	lua_insert(l, 1);
	return CclSetSharedVision(l);
}

//Wyrmgus start
/**
**  Define a civilization.
**
**  @param l  Lua state.
*/
int CclDefineCivilization(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string civilization_name = LuaToString(l, 1);
	wyrmgus::civilization *civilization = wyrmgus::civilization::get_or_add(civilization_name, nullptr);
	int civilization_id = civilization->ID;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Display")) {
			civilization->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			civilization->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			civilization->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			civilization->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Adjective")) {
			civilization->adjective = LuaToString(l, -1);
		} else if (!strcmp(value, "Interface")) {
			civilization->interface_style = interface_style::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Visible")) {
			civilization->visible = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Playable")) {
			civilization->playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Species")) {
			civilization->set_species(wyrmgus::species::get(LuaToString(l, -1)));
		} else if (!strcmp(value, "Group")) {
			civilization->set_group(wyrmgus::civilization_group::get(LuaToString(l, -1)));
		} else if (!strcmp(value, "ParentCivilization")) {
			civilization->parent_civilization = wyrmgus::civilization::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Language")) {
			wyrmgus::language *language = wyrmgus::language::get(LuaToString(l, -1));
			civilization->language = language;
			language->used_by_civilization_or_faction = true;
		} else if (!strcmp(value, "Calendar")) {
			wyrmgus::calendar *calendar = wyrmgus::calendar::get(LuaToString(l, -1));
			civilization->calendar = calendar;
		} else if (!strcmp(value, "Currency")) {
			CCurrency *currency = CCurrency::GetCurrency(LuaToString(l, -1));
			civilization->Currency = currency;
		} else if (!strcmp(value, "CivilizationUpgrade")) {
			civilization->upgrade = CUpgrade::get(LuaToString(l, -1));
		} else if (!strcmp(value, "DevelopsFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string originary_civilization_name = LuaToString(l, -1, j + 1);
				wyrmgus::civilization *originary_civilization = wyrmgus::civilization::get(originary_civilization_name);
				civilization->develops_from.push_back(originary_civilization);
				originary_civilization->develops_to.push_back(civilization);
			}
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				const ButtonCmd button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != ButtonCmd::None) {
					++j;
					PlayerRaces.ButtonIcons[civilization_id][button_action].Name = LuaToString(l, -1, j + 1);
					PlayerRaces.ButtonIcons[civilization_id][button_action].Icon = nullptr;
					PlayerRaces.ButtonIcons[civilization_id][button_action].Load();
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForceTypeWeights")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			civilization->ai_force_type_weights.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const wyrmgus::ai_force_type force_type = wyrmgus::string_to_ai_force_type(LuaToString(l, -1, j + 1));
				++j;
				civilization->ai_force_type_weights[force_type] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ForceTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				auto force = std::make_unique<wyrmgus::ai_force_template>();
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "force-type")) {
						force->force_type = wyrmgus::string_to_ai_force_type(LuaToString(l, -1, k + 1));
					} else if (!strcmp(value, "priority")) {
						force->priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "weight")) {
						force->weight = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "unit-class")) {
						const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::get(LuaToString(l, -1, k + 1));
						++k;
						int unit_quantity = LuaToNumber(l, -1, k + 1);
						force->add_unit(unit_class, unit_quantity);
					} else {
						printf("\n%s\n", civilization->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
				civilization->ai_force_templates[force->force_type].push_back(std::move(force));
			}
		} else if (!strcmp(value, "AiBuildingTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				auto building_template = std::make_unique<CAiBuildingTemplate>();
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "unit-class")) {
						const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::get(LuaToString(l, -1, k + 1));
						building_template->set_unit_class(unit_class);
					} else if (!strcmp(value, "priority")) {
						building_template->set_priority(LuaToNumber(l, -1, k + 1));
					} else if (!strcmp(value, "per-settlement")) {
						building_template->set_per_settlement(LuaToBoolean(l, -1, k + 1));
					} else {
						printf("\n%s\n", civilization->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				civilization->AiBuildingTemplates.push_back(std::move(building_template));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			civilization->ui_fillers.clear();
			
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
				civilization->ui_fillers.push_back(std::move(filler));
			}
		} else if (!strcmp(value, "PersonalNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				wyrmgus::gender gender = wyrmgus::gender::none;
				gender = wyrmgus::try_string_to_gender(LuaToString(l, -1, j + 1));
				if (gender != wyrmgus::gender::none) {
					++j;
				}
				
				civilization->add_personal_name(gender, LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "UnitClassNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string class_name = LuaToString(l, -1, j + 1);
				if (class_name.empty()) {
					LuaError(l, "Class is given as a blank string.");
				}
				const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::get(class_name);
				++j;
				
				civilization->add_unit_class_name(unit_class, LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ProvinceNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ShipNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->add_ship_name(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "MinisterTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				const wyrmgus::character_title title = string_to_character_title(LuaToString(l, -1, k + 1));
				++k;
				const wyrmgus::gender gender = wyrmgus::string_to_gender(LuaToString(l, -1, k + 1));
				++k;
				const wyrmgus::government_type government_type = wyrmgus::string_to_government_type(LuaToString(l, -1, k + 1));
				++k;
				const wyrmgus::faction_tier tier = wyrmgus::string_to_faction_tier(LuaToString(l, -1, k + 1));
				++k;
				civilization->character_title_names[title][wyrmgus::faction_type::none][government_type][tier][gender] = LuaToString(l, -1, k + 1);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

/**
**  Define a word for a particular language.
**
**  @param l  Lua state.
*/
int CclDefineLanguageWord(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	static unsigned definition_count = 0;

	const std::string word_name = LuaToString(l, 1);
	wyrmgus::word *word = wyrmgus::word::add(wyrmgus::string::lowered(word_name) + "_" + std::to_string(definition_count), nullptr);
	word->set_name(word_name);
	++definition_count;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			wyrmgus::language *language = wyrmgus::language::get(LuaToString(l, -1));
			word->set_language(language);
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				word->meanings.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "Type")) {
			std::string word_type_name = LuaToString(l, -1);
			const wyrmgus::word_type word_type = wyrmgus::string_to_word_type(word_type_name);
			word->type = word_type;
		} else if (!strcmp(value, "DerivesFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			wyrmgus::language *derives_from_language = wyrmgus::language::get(LuaToString(l, -1, j + 1));
			++j;
			const wyrmgus::word_type derives_from_word_type = wyrmgus::string_to_word_type(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<std::string> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			if (derives_from_language != nullptr) {
				const std::string derives_from_word = LuaToString(l, -1, j + 1);
				wyrmgus::word *etymon = derives_from_language->GetWord(derives_from_word, derives_from_word_type, word_meanings);
				
				if (etymon != nullptr) {
					word->set_etymon(etymon);
				} else {
					LuaError(l, "Word \"%s\" is set to derive from \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->get_identifier().c_str() _C_ derives_from_word.c_str() _C_ derives_from_language->get_identifier().c_str() _C_ wyrmgus::word_type_to_string(derives_from_word_type).c_str());
				}
			} else {
				LuaError(l, "Word \"%s\"'s derives from is incorrectly set, as either the language or the word type set for the original word given is incorrect" _C_ word->get_identifier().c_str());
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
				
				wyrmgus::language *affix_language = wyrmgus::language::get(LuaToString(l, -1, j + 1)); // should be the same language as that of the word, but needs to be specified since the word's language may not have been set yet
				++j;
				const wyrmgus::word_type affix_word_type = wyrmgus::string_to_word_type(LuaToString(l, -1, j + 1));
				++j;
				
				std::vector<std::string> word_meanings;
				lua_rawgeti(l, -1, j + 1);
				if (lua_istable(l, -1)) {
					const int subsubargs = lua_rawlen(l, -1);
					for (int k = 0; k < subsubargs; ++k) {
						word_meanings.push_back(LuaToString(l, -1, k + 1));
					}
					
					++j;
				}
				lua_pop(l, 1);

				if (affix_language && affix_word_type != wyrmgus::word_type::none) {
					std::string affix_word = LuaToString(l, -1, j + 1);
					wyrmgus::word *other_word = affix_language->GetWord(affix_word, affix_word_type, word_meanings);
					
					if (other_word != nullptr) {
						word->add_compound_element(other_word);
					} else {
						LuaError(l, "Word \"%s\" is set to be a compound formed by \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->get_identifier().c_str() _C_ affix_word.c_str() _C_ affix_language->get_identifier().c_str() _C_ wyrmgus::word_type_to_string(affix_word_type).c_str());
					}
				} else {
					LuaError(l, "Word \"%s\"'s compound elements are incorrectly set, as either the language or the word type set for one of the element words given is incorrect" _C_ word->get_identifier().c_str());
				}
			}
		} else if (!strcmp(value, "Gender")) {
			std::string grammatical_gender_name = LuaToString(l, -1);
			const wyrmgus::grammatical_gender grammatical_gender = wyrmgus::string_to_grammatical_gender(grammatical_gender_name);
			if (grammatical_gender != wyrmgus::grammatical_gender::none) {
				word->gender = grammatical_gender;
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
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (word->get_type() == wyrmgus::word_type::none) {
		LuaError(l, "Word \"%s\" has no type" _C_ word->get_identifier().c_str());
	}
	
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
	wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
	if (!civilization) {
		return 0;
	}
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Display")) {
		lua_pushstring(l, civilization->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, civilization->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, civilization->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, civilization->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Adjective")) {
		if (!civilization->get_adjective().empty()) {
			lua_pushstring(l, civilization->get_adjective().c_str());
		} else {
			lua_pushstring(l, civilization->get_name().c_str());
		}
		return 1;
	} else if (!strcmp(data, "Interface")) {
		if (civilization->get_interface_style() != nullptr) {
			lua_pushstring(l, civilization->get_interface_style()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, civilization->is_playable());
		return 1;
	} else if (!strcmp(data, "Species")) {
		if (civilization->get_species() != nullptr) {
			lua_pushstring(l, civilization->get_species()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "ParentCivilization")) {
		if (civilization->get_parent_civilization() != nullptr) {
			lua_pushstring(l, civilization->get_parent_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Language")) {
		const wyrmgus::language *language = civilization->get_language();
		if (language != nullptr) {
			lua_pushstring(l, language->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultColor")) {
		if (civilization->get_default_color() != nullptr) {
			lua_pushstring(l, civilization->get_default_color()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "CivilizationUpgrade")) {
		if (civilization->get_upgrade() != nullptr) {
			lua_pushstring(l, civilization->get_upgrade()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DevelopsFrom")) {
		lua_createtable(l, civilization->get_develops_from().size(), 0);
		for (size_t i = 1; i <= civilization->get_develops_from().size(); ++i)
		{
			lua_pushstring(l, civilization->get_develops_from()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "DevelopsTo")) {
		lua_createtable(l, civilization->get_develops_to().size(), 0);
		for (size_t i = 1; i <= civilization->get_develops_to().size(); ++i)
		{
			lua_pushstring(l, civilization->get_develops_to()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Quests")) {
		lua_createtable(l, civilization->Quests.size(), 0);
		for (size_t i = 1; i <= civilization->Quests.size(); ++i)
		{
			lua_pushstring(l, civilization->Quests[i-1]->get_identifier().c_str());
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
	const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::try_get(class_name);
	wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, 2));
	std::string unit_type_ident;
	if (civilization && unit_class != nullptr) {
		const wyrmgus::unit_type *unit_type = civilization->get_class_unit_type(unit_class);
		if (unit_type != nullptr) {
			unit_type_ident = unit_type->get_identifier();
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		const wyrmgus::upgrade_class *upgrade_class = wyrmgus::upgrade_class::try_get(class_name);
		if (civilization && upgrade_class != nullptr) {
			const CUpgrade *upgrade = civilization->get_class_upgrade(upgrade_class);
			if (upgrade != nullptr) {
				unit_type_ident = upgrade->get_identifier();
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
	const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::try_get(class_name);
	wyrmgus::faction *faction = nullptr;
	const int nargs = lua_gettop(l);
	if (nargs == 2) {
		faction = wyrmgus::faction::get(LuaToString(l, 2));
	} else if (nargs == 3) {
		//the civilization was the second argument, but it isn't needed anymore
		faction = wyrmgus::faction::get(LuaToString(l, 3));
	}
	std::string unit_type_ident;
	if (unit_class != nullptr) {
		const wyrmgus::unit_type *unit_type = faction->get_class_unit_type(unit_class);
		if (unit_type != nullptr) {
			unit_type_ident = unit_type->get_identifier();
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		const wyrmgus::upgrade_class *upgrade_class = wyrmgus::upgrade_class::try_get(class_name);
		if (upgrade_class != nullptr) {
			const CUpgrade *upgrade = faction->get_class_upgrade(upgrade_class);
			if (upgrade != nullptr) {
				unit_type_ident = upgrade->get_identifier();
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
int CclDefineFaction(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string faction_name = LuaToString(l, 1);
	
	wyrmgus::faction *faction = wyrmgus::faction::get_or_add(faction_name, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1));
			faction->civilization = civilization;
		} else if (!strcmp(value, "Name")) {
			faction->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			faction->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			faction->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			faction->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Adjective")) {
			faction->adjective = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			const std::string faction_type_name = LuaToString(l, -1);
			faction->type = wyrmgus::string_to_faction_type(faction_type_name);
		} else if (!strcmp(value, "Color")) {
			faction->color = wyrmgus::player_color::get(LuaToString(l, -1));
		} else if (!strcmp(value, "DefaultTier")) {
			std::string faction_tier_name = LuaToString(l, -1);
			const wyrmgus::faction_tier tier = wyrmgus::string_to_faction_tier(faction_tier_name);
			faction->default_tier = tier;
		} else if (!strcmp(value, "DefaultGovernmentType")) {
			std::string government_type_name = LuaToString(l, -1);
			const wyrmgus::government_type government_type = wyrmgus::string_to_government_type(government_type_name);
			faction->default_government_type = government_type;
		} else if (!strcmp(value, "DefaultAI")) {
			faction->default_ai = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentFaction")) {
			faction->parent_faction = wyrmgus::faction::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Playable")) {
			faction->playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "DefiniteArticle")) {
			faction->definite_article = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			faction->icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Currency")) {
			CCurrency *currency = CCurrency::GetCurrency(LuaToString(l, -1));
			faction->Currency = currency;
		} else if (!strcmp(value, "Titles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				const wyrmgus::government_type government_type = wyrmgus::string_to_government_type(LuaToString(l, -1, k + 1));
				++k;
				const wyrmgus::faction_tier tier = wyrmgus::string_to_faction_tier(LuaToString(l, -1, k + 1));
				++k;
				faction->title_names[government_type][tier] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "MinisterTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				const wyrmgus::character_title title = string_to_character_title(LuaToString(l, -1, k + 1));
				++k;
				const wyrmgus::gender gender = wyrmgus::string_to_gender(LuaToString(l, -1, k + 1));
				++k;
				const wyrmgus::government_type government_type = wyrmgus::string_to_government_type(LuaToString(l, -1, k + 1));
				++k;
				const wyrmgus::faction_tier tier = wyrmgus::string_to_faction_tier(LuaToString(l, -1, k + 1));
				++k;
				faction->character_title_names[title][government_type][tier][gender] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "FactionUpgrade")) {
			faction->upgrade = CUpgrade::get(LuaToString(l, -1));
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				const ButtonCmd button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != ButtonCmd::None) {
					++j;
					faction->ButtonIcons[button_action] = icon::get(LuaToString(l, -1, j + 1));
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "ForceTypeWeights")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			faction->ai_force_type_weights.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const wyrmgus::ai_force_type force_type = wyrmgus::string_to_ai_force_type(LuaToString(l, -1, j + 1));
				++j;
				faction->ai_force_type_weights[force_type] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ForceTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				auto force = std::make_unique<wyrmgus::ai_force_template>();
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "force-type")) {
						force->force_type = wyrmgus::string_to_ai_force_type(LuaToString(l, -1, k + 1));
					} else if (!strcmp(value, "priority")) {
						force->priority = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "weight")) {
						force->weight = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "unit-class")) {
						const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::get(LuaToString(l, -1, k + 1));
						++k;
						const int unit_quantity = LuaToNumber(l, -1, k + 1);
						force->add_unit(unit_class, unit_quantity);
					} else {
						printf("\n%s\n", faction->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				faction->ai_force_templates[force->force_type].push_back(std::move(force));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "AiBuildingTemplates")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				auto building_template = std::make_unique<CAiBuildingTemplate>();
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for force templates)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "unit-class")) {
						const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::get(LuaToString(l, -1, k + 1));
						building_template->set_unit_class(unit_class);
					} else if (!strcmp(value, "priority")) {
						building_template->set_priority(LuaToNumber(l, -1, k + 1));
					} else if (!strcmp(value, "per-settlement")) {
						building_template->set_per_settlement(LuaToBoolean(l, -1, k + 1));
					} else {
						printf("\n%s\n", faction->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				faction->AiBuildingTemplates.push_back(std::move(building_template));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			faction->ui_fillers.clear();
			
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
				faction->ui_fillers.push_back(std::move(filler));
			}
		} else if (!strcmp(value, "ProvinceNames")) {
			faction->ProvinceNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
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
				const wyrmgus::faction_tier tier = wyrmgus::string_to_faction_tier(faction_tier_name);
				if (tier == wyrmgus::faction_tier::none) {
					LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
				}
				faction->HistoricalTiers[year] = tier;
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
				const wyrmgus::government_type government_type = wyrmgus::string_to_government_type(government_type_name);
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
				wyrmgus::faction *diplomacy_state_faction = wyrmgus::faction::get(diplomacy_state_faction_ident);
				++j;

				std::string diplomacy_state_name = LuaToString(l, -1, j + 1);
				const wyrmgus::diplomacy_state diplomacy_state = wyrmgus::string_to_diplomacy_state(diplomacy_state_name);
				faction->HistoricalDiplomacyStates[std::pair<CDate, wyrmgus::faction *>(date, diplomacy_state_faction)] = diplomacy_state;
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
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a religion.
**
**  @param l  Lua state.
*/
static int CclDefineReligion(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string religion_ident = LuaToString(l, 1);
	wyrmgus::religion *religion = wyrmgus::religion::get_or_add(religion_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			religion->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			religion->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			religion->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			religion->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "CulturalDeities")) {
			religion->CulturalDeities = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Domains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::magic_domain *domain = wyrmgus::magic_domain::get(LuaToString(l, -1, j + 1));
				religion->Domains.push_back(domain);
			}
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
int CclDefineDeity(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string deity_ident = LuaToString(l, 1);
	wyrmgus::deity *deity = wyrmgus::deity::get_or_add(deity_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			deity->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Pantheon")) {
			deity->pantheon = wyrmgus::pantheon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Gender")) {
			deity->set_gender(wyrmgus::string_to_gender(LuaToString(l, -1)));
		} else if (!strcmp(value, "Major")) {
			deity->major = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Description")) {
			deity->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			deity->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			deity->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Homeworld")) {
			wyrmgus::world *world = wyrmgus::world::get(LuaToString(l, -1));
			deity->homeworld = world;
		} else if (!strcmp(value, "DeityUpgrade")) {
			CUpgrade *upgrade = CUpgrade::get(LuaToString(l, -1));
			deity->set_upgrade(upgrade);
		} else if (!strcmp(value, "Icon")) {
			deity->set_icon(wyrmgus::icon::get(LuaToString(l, -1)));
		} else if (!strcmp(value, "Civilizations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				deity->add_civilization(civilization);
			}
		} else if (!strcmp(value, "Religions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::religion *religion = wyrmgus::religion::get(LuaToString(l, -1, j + 1));
				deity->religions.push_back(religion);
			}
		} else if (!strcmp(value, "Domains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::magic_domain *domain = wyrmgus::magic_domain::get(LuaToString(l, -1, j + 1));
				deity->domains.push_back(domain);
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
				const wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				deity->cultural_names[civilization] = cultural_name;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a language.
**
**  @param l  Lua state.
*/
int CclDefineLanguage(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	const std::string language_ident = LuaToString(l, 1);
	wyrmgus::language *language = wyrmgus::language::get_or_add(language_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			language->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Family")) {
			language->family = wyrmgus::language_family::get(LuaToString(l, -1));
		} else if (!strcmp(value, "DialectOf")) {
			wyrmgus::language *parent_language = wyrmgus::language::get(LuaToString(l, -1));
			language->DialectOf = parent_language;
			parent_language->Dialects.push_back(language);
		} else if (!strcmp(value, "NounEndings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string grammatical_number_name = LuaToString(l, -1, k + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++k;
				
				std::string grammatical_case_name = LuaToString(l, -1, k + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++k;
				
				int word_junction_type = WordJunctionTypeNoWordJunction;
				if (GetWordJunctionTypeIdByName(LuaToString(l, -1, k + 1)) != -1) {
					std::string word_junction_type_name = LuaToString(l, -1, k + 1);
					word_junction_type = GetWordJunctionTypeIdByName(word_junction_type_name);
					if (word_junction_type == -1) {
						LuaError(l, "Word junction type \"%s\" doesn't exist." _C_ word_junction_type_name.c_str());
					}
					++k;
				}

				language->NounEndings[grammatical_number][grammatical_case][word_junction_type] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "AdjectiveEndings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string article_type_name = LuaToString(l, -1, k + 1);
				int article_type = GetArticleTypeIdByName(article_type_name);
				if (article_type == -1) {
					LuaError(l, "Article type \"%s\" doesn't exist." _C_ article_type_name.c_str());
				}
				++k;

				std::string grammatical_case_name = LuaToString(l, -1, k + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++k;

				std::string grammatical_number_name = LuaToString(l, -1, k + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++k;
				
				std::string grammatical_gender_name = LuaToString(l, -1, k + 1);
				const wyrmgus::grammatical_gender grammatical_gender = string_to_grammatical_gender(grammatical_gender_name);
				++k;
				
				language->AdjectiveEndings[article_type][grammatical_case][grammatical_number][grammatical_gender] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "NameTranslations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string translation_from = LuaToString(l, -1, k + 1); //name to be translated
				++k;
				std::string translation_to = LuaToString(l, -1, k + 1); //name translation
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
	bool only_visible = false;
	if (nargs >= 1) {
		only_visible = LuaToBoolean(l, 1);
	}

	std::vector<std::string> civilization_idents;
	for (const wyrmgus::civilization *civilization : wyrmgus::civilization::get_all()) {
		if (!only_visible || civilization->is_visible()) {
			civilization_idents.push_back(civilization->get_identifier());
		}
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
	wyrmgus::civilization *civilization = nullptr;
	if (lua_gettop(l) >= 1) {
		civilization = wyrmgus::civilization::try_get(LuaToString(l, 1));
	}
	
	wyrmgus::faction_type faction_type = wyrmgus::faction_type::none;
	if (lua_gettop(l) >= 2) {
		faction_type = wyrmgus::string_to_faction_type(LuaToString(l, 2));
	}
	
	std::vector<std::string> factions;
	if (civilization != nullptr) {
		for (wyrmgus::faction *faction : wyrmgus::faction::get_all()) {
			if (faction_type != wyrmgus::faction_type::none && faction->get_type() != faction_type) {
				continue;
			}
			if (faction->get_civilization() == civilization) {
				factions.push_back(faction->get_identifier());
			}
		}
	} else {
		for (wyrmgus::faction *faction : wyrmgus::faction::get_all()) {
			if (faction_type != wyrmgus::faction_type::none && faction->get_type() != faction_type) {
				continue;
			}
			factions.push_back(faction->get_identifier());
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
	lua_createtable(l, wyrmgus::player_color::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::player_color::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::player_color::get_all()[i-1]->get_identifier().c_str());
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
	wyrmgus::faction *faction = wyrmgus::faction::get(faction_name);
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Type")) {
		lua_pushstring(l, wyrmgus::faction_type_to_string(faction->get_type()).c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (faction->get_civilization() != nullptr) {
			lua_pushstring(l, faction->get_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultAI")) {
		lua_pushstring(l, faction->get_default_ai().c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

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
		lua_pushstring(l, p->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "RaceName")) {
		lua_pushstring(l, p->get_civilization()->get_identifier().c_str());
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (p->get_faction() != nullptr) {
			lua_pushstring(l, p->get_faction()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Type")) {
		lua_pushnumber(l, static_cast<int>(p->get_type()));
		return 1;
	} else if (!strcmp(data, "Color")) {
		if (p->get_player_color() == nullptr) {
			LuaError(l, "Player %d has no color." _C_ p->get_index());
		}
		lua_pushstring(l, p->get_player_color()->get_identifier().c_str());
		return 1;
	} else if (!strcmp(data, "StartPosX")) {
		lua_pushnumber(l, p->StartPos.x);
		return 1;
	} else if (!strcmp(data, "StartPosY")) {
		lua_pushnumber(l, p->StartPos.y);
		return 1;
	} else if (!strcmp(data, "StartMapLayer")) {
		lua_pushnumber(l, p->StartMapLayer);
		return 1;
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_resource(resource) + p->get_stored_resource(resource));
		return 1;
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_stored_resource(resource));
		return 1;
	} else if (!strcmp(data, "MaxResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_max_resource(resource));
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Prices")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_resource_price(resource));
		return 1;
	} else if (!strcmp(data, "ResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_resource_demand(resource));
		return 1;
	} else if (!strcmp(data, "StoredResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_stored_resource_demand(resource));
		return 1;
	} else if (!strcmp(data, "EffectiveResourceDemand")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_effective_resource_demand(resource));
		return 1;
	} else if (!strcmp(data, "EffectiveResourceSellPrice")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_effective_resource_sell_price(resource));
		return 1;
	} else if (!strcmp(data, "EffectiveResourceBuyPrice")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_effective_resource_buy_price(resource));
		return 1;
	} else if (!strcmp(data, "TotalPriceDifferenceWith")) {
		LuaCheckArgs(l, 3);
		
		int other_player = LuaToNumber(l, 3);

		lua_pushnumber(l, p->GetTotalPriceDifferenceWith(*CPlayer::Players[other_player]));
		return 1;
	} else if (!strcmp(data, "TradePotentialWith")) {
		LuaCheckArgs(l, 3);
		
		int other_player = LuaToNumber(l, 3);

		lua_pushnumber(l, p->GetTradePotentialWith(*CPlayer::Players[other_player]));
		return 1;
	} else if (!strcmp(data, "HasHero")) {
		LuaCheckArgs(l, 3);
		
		wyrmgus::character *hero = wyrmgus::character::get(LuaToString(l, 3));

		lua_pushboolean(l, p->HasHero(hero));
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "UnitTypesCount")) {
		LuaCheckArgs(l, 3);
		wyrmgus::unit_type *type = CclGetUnitType(l);
		assert_throw(type != nullptr);
		lua_pushnumber(l, p->GetUnitTypeCount(type));
		return 1;
	} else if (!strcmp(data, "UnitTypesUnderConstructionCount")) {
		LuaCheckArgs(l, 3);
		wyrmgus::unit_type *type = CclGetUnitType(l);
		assert_throw(type != nullptr);
		lua_pushnumber(l, p->GetUnitTypeUnderConstructionCount(type));
		return 1;
	} else if (!strcmp(data, "UnitTypesAiActiveCount")) {
		LuaCheckArgs(l, 3);
		wyrmgus::unit_type *type = CclGetUnitType(l);
		assert_throw(type != nullptr);
		lua_pushnumber(l, p->GetUnitTypeAiActiveCount(type));
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Heroes")) {
		lua_createtable(l, p->Heroes.size(), 0);
		for (size_t i = 1; i <= p->Heroes.size(); ++i)
		{
			lua_pushstring(l, p->Heroes[i-1]->get_character()->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "AiEnabled")) {
		lua_pushboolean(l, p->AiEnabled);
		return 1;
	} else if (!strcmp(data, "AiName")) {
		lua_pushstring(l, p->AiName.c_str());
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
		lua_pushnumber(l, p->get_trade_cost());
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Supply")) {
		lua_pushnumber(l, p->get_supply());
		return 1;
	} else if (!strcmp(data, "Demand")) {
		lua_pushnumber(l, p->get_demand());
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
	} else if (!strcmp(data, "TotalUnits")) {
		lua_pushnumber(l, p->TotalUnits);
		return 1;
	} else if (!strcmp(data, "TotalBuildings")) {
		lua_pushnumber(l, p->TotalBuildings);
		return 1;
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_resource_total(resource));
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
		wyrmgus::unit_type *type = CclGetUnitType(l);
		assert_throw(type != nullptr);
		lua_pushnumber(l, p->UnitTypeKills[type->Slot]);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_resource_harvest_speed(resource));
		return 1;
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		lua_pushnumber(l, p->get_resource_return_speed(resource));
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
		if (!strncmp(ident, "unit", 4)) {
			int id = UnitTypeIdByIdent(ident);
			if (UnitIdAllowed(*p, id) > 0) {
				lua_pushstring(l, "A");
			} else if (UnitIdAllowed(*p, id) == 0) {
				lua_pushstring(l, "F");
			}
		} else if (!strncmp(ident, "upgrade", 7)) {
			if (UpgradeIdentAllowed(*p, ident) == 'A') {
				lua_pushstring(l, "A");
			} else if (UpgradeIdentAllowed(*p, ident) == 'R') {
				lua_pushstring(l, "R");
			} else if (UpgradeIdentAllowed(*p, ident) == 'F') {
				lua_pushstring(l, "F");
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
		}
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "IsAllied")) {
		LuaCheckArgs(l, 3);
		const int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->has_allied_stance_with(second_player));
		return 1;
	} else if (!strcmp(data, "IsEnemy")) {
		LuaCheckArgs(l, 3);
		const int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->has_enemy_stance_with(second_player));
		return 1;
	} else if (!strcmp(data, "HasSharedVisionWith")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->has_shared_vision_with(second_player));
		return 1;
	} else if (!strcmp(data, "HasContactWith")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->HasContactWith(*CPlayer::Players[second_player]));
		return 1;
	} else if (!strcmp(data, "HasQuest")) {
		LuaCheckArgs(l, 3);
		const wyrmgus::quest *quest = wyrmgus::quest::get(LuaToString(l, 3));
		lua_pushboolean(l, p->has_quest(quest));
		return 1;
	} else if (!strcmp(data, "CompletedQuest")) {
		LuaCheckArgs(l, 3);
		const wyrmgus::quest *quest = wyrmgus::quest::get(LuaToString(l, 3));
		lua_pushboolean(l, p->is_quest_completed(quest));
		return 1;
	} else if (!strcmp(data, "FactionTitle")) {
		lua_pushstring(l, p->get_faction_title_name().data());
		return 1;
	} else if (!strcmp(data, "CharacterTitle")) {
		LuaCheckArgs(l, 4);
		std::string title_type_ident = LuaToString(l, 3);
		std::string gender_ident = LuaToString(l, 4);
		const wyrmgus::character_title title_type_id = string_to_character_title(title_type_ident);
		const wyrmgus::gender gender = wyrmgus::string_to_gender(gender_ident);
		
		lua_pushstring(l, p->GetCharacterTitleName(title_type_id, gender).data());
		return 1;
	} else if (!strcmp(data, "HasSettlement")) {
		LuaCheckArgs(l, 3);
		const std::string site_identifier = LuaToString(l, 3);
		const wyrmgus::site *site = wyrmgus::site::get(site_identifier);
		lua_pushboolean(l, p->has_settlement(site));
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Currency")) {
		const CCurrency *currency = p->GetCurrency();
		if (currency) {
			lua_pushstring(l, currency->Name.c_str());
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
	if (p->get_type() == player_type::nobody && !CEditor::get()->is_running() && strcmp(data, "Type") != 0) {
		return 0;
	}
	//Wyrmgus end

	if (!strcmp(data, "Name")) {
		p->set_name(LuaToString(l, 3));
	} else if (!strcmp(data, "RaceName")) {
		if (GameRunning) {
			p->set_faction(nullptr);
		}

		const char *civilization_ident = LuaToString(l, 3);
		const wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_ident);
		p->set_civilization(civilization);
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		std::string faction_name = LuaToString(l, 3);
		if (faction_name == "random") {
			p->set_random_faction();
		} else {
			p->set_faction(faction::try_get(faction_name));
		}
	} else if (!strcmp(data, "Type")) {
		p->set_type(static_cast<player_type>(LuaToNumber(l, 3)));
	} else if (!strcmp(data, "Dynasty")) {
		const std::string dynasty_ident = LuaToString(l, 3);
		p->set_dynasty(dynasty::get(dynasty_ident));
	//Wyrmgus end
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		p->set_resource(resource, LuaToNumber(l, 4), resource_storage_type::overall);
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		p->set_resource(resource, LuaToNumber(l, 4), resource_storage_type::building);
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
	} else if (!strcmp(data, "TotalUnits")) {
		p->TotalUnits = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalBuildings")) {
		p->TotalBuildings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		p->set_resource_total(resource, LuaToNumber(l, 4));
	} else if (!strcmp(data, "TotalRazings")) {
		p->TotalRazings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalKills")) {
		p->TotalKills = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		p->set_resource_harvest_speed(resource, LuaToNumber(l, 4));
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const resource *resource = resource::get(res);
		p->set_resource_return_speed(resource, LuaToNumber(l, 4));
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

		if (!strncmp(ident, "upgrade", 7)) {
			if (acquire == "R" && UpgradeIdentAllowed(*p, ident) != 'R') {
				p->acquire_upgrade(CUpgrade::get(ident));
			} else if (acquire == "F" || acquire == "A") {
				if (UpgradeIdentAllowed(*p, ident) == 'R') {
					p->lose_upgrade(CUpgrade::get(ident));
				}
				AllowUpgradeId(*p, UpgradeIdByIdent(ident), acquire[0]);
			}
		//Wyrmgus start
		} else if (!strncmp(ident, "unit", 4)) {
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
	} else if (!strcmp(data, "AiName")) {
		p->AiName = LuaToString(l, 3);
	} else if (!strcmp(data, "Team")) {
		p->Team = LuaToNumber(l, 3);
	} else if (!strcmp(data, "AcceptQuest")) {
		wyrmgus::quest *quest = wyrmgus::quest::get(LuaToString(l, 3));
		p->accept_quest(quest);
	} else if (!strcmp(data, "CompleteQuest")) {
		wyrmgus::quest *quest = wyrmgus::quest::get(LuaToString(l, 3));
		p->complete_quest(quest);
	} else if (!strcmp(data, "FailQuest")) {
		wyrmgus::quest *quest = wyrmgus::quest::get(LuaToString(l, 3));
		p->fail_quest(quest);
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
	for (const wyrmgus::language *language : wyrmgus::language::get_all()) {
		if (!only_used || language->used_by_civilization_or_faction) {
			languages.push_back(language->get_identifier());
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
	const wyrmgus::language *language = wyrmgus::language::get(language_name);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, language->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Family")) {
		if (language->get_family() != nullptr) {
			lua_pushstring(l, language->get_family()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetReligions(lua_State *l)
{
	lua_createtable(l, wyrmgus::religion::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::religion::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::religion::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetMagicDomains(lua_State *l)
{
	lua_createtable(l, wyrmgus::magic_domain::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::magic_domain::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::magic_domain::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetDeities(lua_State *l)
{
	lua_createtable(l, wyrmgus::deity::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::deity::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::deity::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get religion data.
**
**  @param l  Lua state.
*/
static int CclGetReligionData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string religion_ident = LuaToString(l, 1);
	const wyrmgus::religion *religion = wyrmgus::religion::get(religion_ident);

	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, religion->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, religion->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, religion->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, religion->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "CulturalDeities")) {
		lua_pushboolean(l, religion->CulturalDeities);
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetMagicDomainData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}

	const std::string magic_domain_ident = LuaToString(l, 1);
	const wyrmgus::magic_domain *magic_domain = wyrmgus::magic_domain::get(magic_domain_ident);

	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, magic_domain->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Spells")) {
		lua_createtable(l, magic_domain->get_spells().size(), 0);
		for (size_t i = 1; i <= magic_domain->get_spells().size(); ++i) {
			lua_pushstring(l, magic_domain->get_spells()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetDeityData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string deity_ident = LuaToString(l, 1);
	const wyrmgus::deity *deity = wyrmgus::deity::get(deity_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, deity->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Pantheon")) {
		if (deity->get_pantheon() != nullptr) {
			lua_pushstring(l, deity->get_pantheon()->get_name().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, deity->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, deity->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, deity->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Major")) {
		lua_pushboolean(l, deity->is_major());
		return 1;
	} else if (!strcmp(data, "Homeworld")) {
		if (deity->get_homeworld() != nullptr) {
			lua_pushstring(l, deity->get_homeworld()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (deity->get_icon() != nullptr) {
			lua_pushstring(l, deity->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Character")) {
		if (deity->get_character() != nullptr) {
			lua_pushstring(l, deity->get_character()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Civilizations")) {
		lua_createtable(l, deity->get_civilizations().size(), 0);
		for (size_t i = 1; i <= deity->get_civilizations().size(); ++i)
		{
			lua_pushstring(l, deity->get_civilizations()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Religions")) {
		lua_createtable(l, deity->get_religions().size(), 0);
		for (size_t i = 1; i <= deity->get_religions().size(); ++i)
		{
			lua_pushstring(l, deity->get_religions()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Domains")) {
		lua_createtable(l, deity->get_domains().size(), 0);
		for (size_t i = 1; i <= deity->get_domains().size(); ++i)
		{
			lua_pushstring(l, deity->get_domains()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Spells")) {
		lua_createtable(l, deity->get_spells().size(), 0);
		for (size_t i = 1; i <= deity->get_spells().size(); ++i)
		{
			lua_pushstring(l, deity->get_spells()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "CulturalName")) {
		if (lua_gettop(l) < 3) {
			LuaError(l, "incorrect argument");
		}
		
		const wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, 3));
		lua_pushstring(l, deity->get_cultural_name(civilization).c_str());
		
		return 1;
	} else if (!strcmp(data, "Gender")) {
		lua_pushstring(l, wyrmgus::gender_to_string(deity->get_gender()).c_str());
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
	lua_register(Lua, "DefineReligion", CclDefineReligion);
	lua_register(Lua, "DefineDeity", CclDefineDeity);
	lua_register(Lua, "DefineLanguage", CclDefineLanguage);
	lua_register(Lua, "GetCivilizations", CclGetCivilizations);
	lua_register(Lua, "GetFactions", CclGetFactions);
	lua_register(Lua, "GetPlayerColors", CclGetPlayerColors);
	lua_register(Lua, "GetFactionData", CclGetFactionData);
	//Wyrmgus end

	// player member access functions
	lua_register(Lua, "GetPlayerData", CclGetPlayerData);
	lua_register(Lua, "SetPlayerData", CclSetPlayerData);
	lua_register(Lua, "SetAiType", CclSetAiType);
	//Wyrmgus start
	lua_register(Lua, "InitAi", CclInitAi);
	lua_register(Lua, "GetLanguages", CclGetLanguages);
	lua_register(Lua, "GetLanguageData", CclGetLanguageData);
	
	lua_register(Lua, "GetReligions", CclGetReligions);
	lua_register(Lua, "GetMagicDomains", CclGetMagicDomains);
	lua_register(Lua, "GetDeities", CclGetDeities);
	lua_register(Lua, "GetReligionData", CclGetReligionData);
	lua_register(Lua, "GetMagicDomainData", CclGetMagicDomainData);
	lua_register(Lua, "GetDeityData", CclGetDeityData);
	//Wyrmgus end
}
