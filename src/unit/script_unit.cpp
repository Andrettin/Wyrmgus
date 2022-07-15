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
/**@name script_unit.cpp - The unit ccl functions. */
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

#include "unit/unit.h"

#include "actions.h"
//Wyrmgus start
#include "ai/ai_local.h"
//Wyrmgus end
#include "animation/animation_set.h"
#include "commands.h"
#include "epithet.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "item/unique_item.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "pathfinder/pathfinder.h"
#include "player/player.h"
#include "player/player_type.h"
#include "script.h"
#include "script/trigger.h"
//Wyrmgus start
#include "sound/sound.h"
//Wyrmgus end
#include "spell/spell.h"
#include "spell/status_effect.h"
//Wyrmgus start
#include "translate.h"
//Wyrmgus end
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/util.h"

/// Get resource by name
extern unsigned CclGetResourceByName(lua_State *l);

/**
**  Set training queue
**
**  @param l  Lua state.
**
**  @return  The old state of the training queue
*/
static int CclSetTrainingQueue(lua_State *l)
{
	LuaCheckArgs(l, 1);
	EnableTrainingQueue = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Set reveal attacker
**
**  @param l  Lua state.
**
**  @return   The old state of the flag
*/
static int CclSetRevealAttacker(lua_State *l)
{
	LuaCheckArgs(l, 1);
	RevealAttacker = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Set cost multiplier to RepairCost for buildings additional workers helping (0 = no additional cost)
**
**  @param l  Lua state.
*/
static int CclResourcesMultiBuildersMultiplier(lua_State *l)
{
	LuaCheckArgs(l, 1);
	ResourcesMultiBuildersMultiplier = LuaToNumber(l, 1);
	return 0;
}

/**
**  Get a unit pointer
**
**  @param l  Lua state.
**
**  @return   The unit pointer
*/
static CUnit *CclGetUnit(lua_State *l)
{
	int num = LuaToNumber(l, -1);
	if (num == -1) {
		if (!Selected.empty()) {
			return Selected[0];
		} else if (UnitUnderCursor) {
			return UnitUnderCursor;
		}
		return nullptr;
	//Wyrmgus start
	} else if (num == -2) {
		if (UnitUnderCursor) {
			return UnitUnderCursor;
		}
		return nullptr;
	//Wyrmgus end
	}
	return &wyrmgus::unit_manager::get()->GetSlotUnit(num);
}

/**
**  Get a unit pointer from ref string
**
**  @param l  Lua state.
**
**  @return   The unit pointer
*/
CUnit *CclGetUnitFromRef(lua_State *l)
{
	const char *const value = LuaToString(l, -1);
	unsigned int slot = strtol(value + 1, nullptr, 16);
	assert_throw(slot < wyrmgus::unit_manager::get()->GetUsedSlotCount());
	return &wyrmgus::unit_manager::get()->GetSlotUnit(slot);
}


bool COrder::ParseGenericData(lua_State *l, int &j, const char *value)
{
	if (!strcmp(value, "finished")) {
		this->Finished = true;
	} else if (!strcmp(value, "goal")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		this->set_goal(CclGetUnitFromRef(l));
		lua_pop(l, 1);
	} else {
		return false;
	}
	return true;
}



void PathFinderInput::Load(lua_State *l)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument in PathFinderInput::Load");
	}
	const int args = 1 + lua_rawlen(l, -1);
	for (int i = 1; i < args; ++i) {
		const char *tag = LuaToString(l, -1, i);
		++i;
		if (!strcmp(tag, "unit-size")) {
			lua_rawgeti(l, -1, i);
			CclGetPos(l, &this->unitSize.x, &this->unitSize.y);
			lua_pop(l, 1);
		} else if (!strcmp(tag, "goalpos")) {
			lua_rawgeti(l, -1, i);
			CclGetPos(l, &this->goalPos.x, &this->goalPos.y);
			lua_pop(l, 1);
		} else if (!strcmp(tag, "goal-size")) {
			lua_rawgeti(l, -1, i);
			CclGetPos(l, &this->goalSize.x, &this->goalSize.y);
			lua_pop(l, 1);
		} else if (!strcmp(tag, "minrange")) {
			this->minRange = LuaToNumber(l, -1, i);
		} else if (!strcmp(tag, "maxrange")) {
			this->maxRange = LuaToNumber(l, -1, i);
		} else if (!strcmp(tag, "invalid")) {
			this->isRecalculatePathNeeded = true;
			--i;
		} else {
			LuaError(l, "PathFinderInput::Load: Unsupported tag: %s" _C_ tag);
		}
	}
}


void PathFinderOutput::Load(lua_State *l)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument in PathFinderOutput::Load");
	}
	const int args = 1 + lua_rawlen(l, -1);
	for (int i = 1; i < args; ++i) {
		const char *tag = LuaToString(l, -1, i);
		++i;
		if (!strcmp(tag, "cycles")) {
			this->Cycles = LuaToNumber(l, -1, i);
		} else if (!strcmp(tag, "fast")) {
			this->Fast = 1;
			--i;
		} else if (!strcmp(tag, "path")) {
			lua_rawgeti(l, -1, i);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument _");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				this->Path[k] = LuaToNumber(l, -1, k + 1);
			}
			this->Length = subargs;
			lua_pop(l, 1);
		} else {
			LuaError(l, "PathFinderOutput::Load: Unsupported tag: %s" _C_ tag);
		}
	}
}

/**
**  Parse orders.
**
**  @param l     Lua state.
**  @param unit  Unit pointer which should get the orders.
*/
static void CclParseOrders(lua_State *l, CUnit &unit)
{
	unit.Orders.clear();
	const int n = lua_rawlen(l, -1);

	for (int j = 0; j < n; ++j) {
		lua_rawgeti(l, -1, j + 1);

		unit.Orders.push_back(CclParseOrder(l, unit));
		lua_pop(l, 1);
	}
}

/**
**  Parse unit
**
**  @param l  Lua state.
**
**  @todo  Verify that vision table is always correct (transporter)
**  @todo (PlaceUnit() and host-info).
*/
int CclUnit(lua_State *l)
{
	const int slot = LuaToNumber(l, 1);

	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	CUnit *unit = nullptr;
	const wyrmgus::unit_type *type = nullptr;
	wyrmgus::unit_type *seentype = nullptr;
	CPlayer *player = nullptr;

	// Parse the list:
	const int args = lua_rawlen(l, 2);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, 2, j + 1);
		++j;

		if (!strcmp(value, "type")) {
			type = wyrmgus::unit_type::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "seen-type")) {
			seentype = wyrmgus::unit_type::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "player")) {
			player = CPlayer::Players[LuaToNumber(l, 2, j + 1)].get();

			// During a unit's death animation (when action is "die" but the
			// unit still has its original type, i.e. it's still not a corpse)
			// the unit is already removed from map and from player's
			// unit list (=the unit went through LetUnitDie() which
			// calls RemoveUnit() and UnitLost()).  Such a unit should not
			// be put on player's unit list!  However, this state is not
			// easily detected from this place.  It seems that it is
			// characterized by
			// unit->CurrentAction()==UnitAction::Die so we have to wait
			// until we parsed at least Unit::Orders[].
			assert_throw(type != nullptr);
			unit = &wyrmgus::unit_manager::get()->GetSlotUnit(slot);
			unit->Init(*type, true);
			unit->Seen.Type = seentype;
			unit->Active = 0;
			unit->Removed = 0;
			assert_throw(UnitNumber(*unit) == slot);
		//Wyrmgus start
		} else if (!strcmp(value, "personal-name")) {
			unit->Name = LuaToString(l, 2, j + 1);
		} else if (!strcmp(value, "epithet")) {
			unit->epithet = epithet::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "surname")) {
			unit->surname = LuaToString(l, 2, j + 1);
		} else if (!strcmp(value, "site")) {
			unit->set_site(site::get(LuaToString(l, 2, j + 1)));
			if (unit->site->is_settlement()) {
				CMap::get()->add_settlement_unit(unit);
			}

			if (unit->site->get_orbit_center() != nullptr && unit->site->get_base_unit_type() != nullptr && unit->site->get_base_unit_type()->BoolFlag[CELESTIAL_BODY_INDEX].value) {
				CMap::get()->add_orbiting_celestial_body_unit(unit);
			}
		} else if (!strcmp(value, "settlement")) {
			unit->settlement = site::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "home_settlement")) {
			unit->home_settlement = site::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "trait")) {
			unit->Trait = CUpgrade::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "prefix")) {
			unit->Prefix = CUpgrade::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "suffix")) {
			unit->Suffix = CUpgrade::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "spell")) {
			unit->Spell = wyrmgus::spell::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "work")) {
			unit->Work = CUpgrade::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "elixir")) {
			unit->Elixir = CUpgrade::get(LuaToString(l, 2, j + 1));
		} else if (!strcmp(value, "unique")) {
			wyrmgus::unique_item *unique_item = wyrmgus::unique_item::get(LuaToString(l, 2, j + 1));
			unit->unique = unique_item;
			if (unique_item->get_unit_type()->BoolFlag[ITEM_INDEX].value) { //apply the unique item's prefix and suffix here, because it may have changed in the database in relation to when the game was last played
				unit->Type = unique_item->get_unit_type();
				type = unique_item->get_unit_type();
				if (unique_item->get_prefix() != nullptr) {
					unit->Prefix = unique_item->get_prefix();
				}
				if (unique_item->get_suffix() != nullptr) {
					unit->Suffix = unique_item->get_suffix();
				}
				if (unique_item->get_spell() != nullptr) {
					unit->Spell = unique_item->get_spell();
				}
				if (unique_item->get_work() != nullptr) {
					unit->Work = unique_item->get_work();
				}
				if (unique_item->get_elixir() != nullptr) {
					unit->Elixir = unique_item->get_elixir();
				}
			}
		} else if (!strcmp(value, "bound")) {
			unit->Bound = LuaToBoolean(l, 2, j + 1);
		} else if (!strcmp(value, "identified")) {
			unit->Identified = LuaToBoolean(l, 2, j + 1);
		} else if (!strcmp(value, "equipped")) {
			bool is_equipped = LuaToBoolean(l, 2, j + 1);
			if (is_equipped && unit->Container != nullptr) {
				unit->Container->EquippedItems[static_cast<int>(wyrmgus::get_item_class_slot(unit->Type->get_item_class()))].push_back(unit);
			}
		} else if (!strcmp(value, "sold-unit")) {
			bool is_sold = LuaToBoolean(l, 2, j + 1);
			if (is_sold && unit->Container != nullptr) {
				unit->Container->SoldUnits.push_back(unit);
			}
		} else if (!strcmp(value, "connecting-destination")) {
			unit->ConnectingDestination = &wyrmgus::unit_manager::get()->GetSlotUnit(LuaToNumber(l, 2, j + 1));
			unit->MapLayer->LayerConnectors.push_back(unit);
		//Wyrmgus end
		} else if (!strcmp(value, "current-sight-range")) {
			unit->CurrentSightRange = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "host-info")) {
			lua_rawgeti(l, 2, j + 1);
			//Wyrmgus start
//			if (!lua_istable(l, -1) || lua_rawlen(l, -1) != 4) {
			if (!lua_istable(l, -1) || lua_rawlen(l, -1) != 5) {
			//Wyrmgus end
				LuaError(l, "incorrect argument");
			}
			Vec2i pos;
			//Wyrmgus start
			int z;
			//Wyrmgus end
			int w;
			int h;

			//Wyrmgus start
//			pos.x = LuaToNumber(l, -1, 1);
//			pos.y = LuaToNumber(l, -1, 2);
//			w = LuaToNumber(l, -1, 3);
//			h = LuaToNumber(l, -1, 4);
//			MapSight<MapMarkTileSight>(*player, pos, w, h, unit->CurrentSightRange);
			z = LuaToNumber(l, -1, 1);
			pos.x = LuaToNumber(l, -1, 2);
			pos.y = LuaToNumber(l, -1, 3);
			w = LuaToNumber(l, -1, 4);
			h = LuaToNumber(l, -1, 5);
			MapSight<MapMarkTileSight>(*player, pos, w, h, unit->CurrentSightRange, z);
			//Wyrmgus end
			// Detectcloak works in container
			if (unit->Type->BoolFlag[DETECTCLOAK_INDEX].value) {
				//Wyrmgus start
//				MapSight<MapMarkTileDetectCloak>(*player, pos, w, h, unit->CurrentSightRange);
				MapSight<MapMarkTileDetectCloak>(*player, pos, w, h, unit->CurrentSightRange, z);
				//Wyrmgus end
			}
			//Wyrmgus start
			if (unit->Variable[ETHEREALVISION_INDEX].Value) {
				MapSight<MapMarkTileDetectEthereal>(*player, pos, w, h, unit->CurrentSightRange, z);
			}
			//Wyrmgus end
			// Radar(Jammer) not.
			lua_pop(l, 1);
		} else if (!strcmp(value, "map-layer")) {
			unit->MapLayer = CMap::get()->MapLayers[LuaToNumber(l, 2, j + 1)].get();
		} else if (!strcmp(value, "tile")) {
			lua_rawgeti(l, 2, j + 1);
			CclGetPos(l, &unit->tilePos.x , &unit->tilePos.y, -1);
			lua_pop(l, 1);
			unit->Offset = CMap::get()->get_pos_index(unit->tilePos, unit->MapLayer->ID);
		} else if (!strcmp(value, "seen-tile")) {
			lua_rawgeti(l, 2, j + 1);
			CclGetPos(l, &unit->Seen.tilePos.x , &unit->Seen.tilePos.y, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "stats")) {
			unit->Stats = &type->Stats[LuaToNumber(l, 2, j + 1)];
		} else if (!strcmp(value, "pixel")) {
			lua_rawgeti(l, 2, j + 1);
			Vec2i pixel_offset;
			CclGetPos(l, &pixel_offset.x, &pixel_offset.y, -1);
			unit->pixel_offset = pixel_offset;
			lua_pop(l, 1);
		} else if (!strcmp(value, "seen-pixel")) {
			lua_rawgeti(l, 2, j + 1);
			Vec2i pixel_offset;
			CclGetPos(l, &pixel_offset.x, &pixel_offset.y, -1);
			unit->Seen.pixel_offset = pixel_offset;
			lua_pop(l, 1);
		} else if (!strcmp(value, "frame")) {
			unit->Frame = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "seen")) {
			unit->Seen.Frame = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "not-seen")) {
			unit->Seen.Frame = UnitNotSeen;
			--j;
		} else if (!strcmp(value, "direction")) {
			unit->Direction = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "damage-type")) {
			unit->DamagedType = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "attacked")) {
			// FIXME : unsigned long should be better handled
			unit->Attacked = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "auto-repair")) {
			unit->AutoRepair = 1;
			--j;
		} else if (!strcmp(value, "burning")) {
			unit->Burning = 1;
			--j;
		} else if (!strcmp(value, "destroyed")) {
			unit->Destroyed = 1;
			--j;
		} else if (!strcmp(value, "removed")) {
			unit->Removed = 1;
			--j;
		} else if (!strcmp(value, "selected")) {
			unit->Selected = 1;
			--j;
		} else if (!strcmp(value, "summoned")) {
			unit->Summoned = 1;
			--j;
		} else if (!strcmp(value, "waiting")) {
			unit->Waiting = 1;
			--j;
		} else if (!strcmp(value, "mine-low")) {
			unit->MineLow = 1;
			--j;
		} else if (!strcmp(value, "player-from")) {
			unit->player_from = CPlayer::Players[LuaToNumber(l, 2, j + 1)].get();
		} else if (!strcmp(value, "seen-by-player")) {
			const char *s = LuaToString(l, 2, j + 1);
			unit->Seen.by_player.clear();
			for (int i = 0; i < PlayerMax && *s; ++i, ++s) {
				if (*s == '-' || *s == '_' || *s == ' ') {
					unit->Seen.by_player.erase(i);
				} else {
					unit->Seen.by_player.insert(i);
				}
			}
		} else if (!strcmp(value, "seen-destroyed")) {
			const char *s = LuaToString(l, 2, j + 1);
			unit->Seen.destroyed.clear();
			for (int i = 0; i < PlayerMax && *s; ++i, ++s) {
				if (*s == '-' || *s == '_' || *s == ' ') {
					unit->Seen.destroyed.erase(i);
				} else {
					unit->Seen.destroyed.insert(i);
				}
			}
		} else if (!strcmp(value, "under-construction")) {
			unit->UnderConstruction = 1;
			--j;
		} else if (!strcmp(value, "seen-under-construction")) {
			unit->Seen.UnderConstruction = 1;
			--j;
		} else if (!strcmp(value, "seen-state")) {
			unit->Seen.State = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "active")) {
			unit->Active = 1;
			--j;
		} else if (!strcmp(value, "ttl")) {
			// FIXME : unsigned long should be better handled
			unit->TTL = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "threshold")) {
			// FIXME : unsigned long should be better handled
			unit->Threshold = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "step-count")) {
			unit->step_count = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "group-id")) {
			unit->GroupId = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "last-group")) {
			unit->LastGroup = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "resources-held")) {
			unit->ResourcesHeld = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "current-resource")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->CurrentResource = CclGetResourceByName(l);
			lua_pop(l, 1);
		//Wyrmgus start
		} else if (!strcmp(value, "gives-resource")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->GivesResource = CclGetResourceByName(l);
			lua_pop(l, 1);
		//Wyrmgus end
		} else if (!strcmp(value, "pathfinder-input")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->pathFinderData->input.Load(l);
			lua_pop(l, 1);
		} else if (!strcmp(value, "pathfinder-output")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->pathFinderData->output.Load(l);
			lua_pop(l, 1);
		} else if (!strcmp(value, "wait")) {
			unit->Wait = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "anim-data")) {
			lua_rawgeti(l, 2, j + 1);
			wyrmgus::animation_set::LoadUnitAnim(l, *unit, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "wait-anim-data")) {
			lua_rawgeti(l, 2, j + 1);
			wyrmgus::animation_set::LoadWaitUnitAnim(l, *unit, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "blink")) {
			unit->Blink = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "moving")) {
			unit->Moving = 1;
			--j;
		} else if (!strcmp(value, "re-cast")) {
			unit->ReCast = 1;
			--j;
		} else if (!strcmp(value, "boarded")) {
			unit->Boarded = 1;
			--j;
		} else if (!strcmp(value, "resource-workers")) {
			int subargs;
			int k;
			lua_rawgeti(l, 2, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = lua_rawlen(l, -1);
			for (k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				CUnit *u = CclGetUnitFromRef(l);
				lua_pop(l, 1);

				assert_throw(u != nullptr);

				unit->Resource.Workers.push_back(u->acquire_ref());
			}
			lua_pop(l, 1);
		} else if (!strcmp(value, "resource-active")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->Resource.Active = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "units-boarded-count")) {
			unit->BoardCount = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "units-contained")) {
			int subargs;
			int k;
			lua_rawgeti(l, 2, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			subargs = lua_rawlen(l, -1);
			for (k = 0; k < subargs; ++k) {
				lua_rawgeti(l, -1, k + 1);
				CUnit *u = CclGetUnitFromRef(l);
				lua_pop(l, 1);
				//Wyrmgus start
				assert_throw(u != nullptr);
				assert_throw(unit != nullptr);
				//Wyrmgus end
				u->AddInContainer(*unit);
			}
			lua_pop(l, 1);
		} else if (!strcmp(value, "orders")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			CclParseOrders(l, *unit);
			lua_pop(l, 1);
			// now we know unit's action so we can assign it to a player
			assert_throw(player != nullptr);
			unit->AssignToPlayer(*player);
			if (unit->CurrentAction() == UnitAction::Built) {
				DebugPrint("HACK: the building is not ready yet\n");
				// HACK: the building is not ready yet
				unit->Player->DecreaseCountsForUnit(unit);
			}
		} else if (!strcmp(value, "critical-order")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->CriticalOrder = CclParseOrder(l, *unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "saved-order")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->SavedOrder = CclParseOrder(l, *unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "new-order")) {
			lua_rawgeti(l, 2, j + 1);
			lua_pushvalue(l, -1);
			unit->NewOrder = CclParseOrder(l, *unit);
			lua_pop(l, 1);
		} else if (!strcmp(value, "goal")) {
			unit->Goal = &wyrmgus::unit_manager::get()->GetSlotUnit(LuaToNumber(l, 2, j + 1));
		} else if (!strcmp(value, "auto-cast")) {
			const char *s = LuaToString(l, 2, j + 1);
			unit->add_autocast_spell(wyrmgus::spell::get(s));
		} else if (!strcmp(value, "spell-cooldown")) {
			lua_rawgeti(l, 2, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const size_t subargs = lua_rawlen(l, -1);
			for (size_t k = 0; k < subargs; ++k) {
				const wyrmgus::spell *spell = wyrmgus::spell::get(LuaToString(l, -1, k + 1));
				++k;
				const int cooldown = LuaToNumber(l, -1, k + 1);
				if (cooldown > 0) {
					unit->set_spell_cooldown_timer(spell, cooldown);
				}
			}
			lua_pop(l, 1);
		} else if (!strcmp(value, "status-effects")) {
			lua_rawgeti(l, 2, j + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const size_t subargs = lua_rawlen(l, -1);
			for (size_t k = 0; k < subargs; ++k) {
				const status_effect status_effect = string_to_status_effect(LuaToString(l, -1, k + 1));
				++k;
				const int cycles = LuaToNumber(l, -1, k + 1);
				unit->set_status_effect_timer(status_effect, cycles);
			}
			lua_pop(l, 1);
		//Wyrmgus start
		} else if (!strcmp(value, "variation")) {
			unit->Variation = LuaToNumber(l, 2, j + 1);
		} else if (!strcmp(value, "layer-variation")) {
			std::string image_layer_name = LuaToString(l, 2, j + 1);
			int image_layer = GetImageLayerIdByName(image_layer_name);
			if (image_layer != -1) {
				++j;
				unit->LayerVariation[image_layer] = LuaToNumber(l, 2, j + 1);
			} else {
				LuaError(l, "Image layer \"%s\" doesn't exist." _C_ image_layer_name.c_str());
			}
		} else if (!strcmp(value, "unit-stock")) {
			const unit_type *stocked_unit_type = wyrmgus::unit_type::get(LuaToString(l, 2, j + 1));
			++j;
			unit->set_unit_stock(stocked_unit_type, LuaToNumber(l, 2, j + 1));
		} else if (!strcmp(value, "unit-class-stock")) {
			const unit_class *stocked_unit_class = unit_class::get(LuaToString(l, 2, j + 1));
			++j;
			unit->set_unit_class_stock(stocked_unit_class, LuaToNumber(l, 2, j + 1));
		} else if (!strcmp(value, "unit-stock-replenishment-timer")) {
			const unit_type *stocked_unit_type = unit_type::get(LuaToString(l, 2, j + 1));
			++j;
			unit->set_unit_stock_replenishment_timer(stocked_unit_type, LuaToNumber(l, 2, j + 1));
		} else if (!strcmp(value, "unit-class-stock-replenishment-timer")) {
			const unit_class *stocked_unit_class = unit_class::get(LuaToString(l, 2, j + 1));
			++j;
			unit->set_unit_class_stock_replenishment_timer(stocked_unit_class, LuaToNumber(l, 2, j + 1));
		} else if (!strcmp(value, "character")) {
			unit->SetCharacter(LuaToString(l, 2, j + 1), false);
		} else if (!strcmp(value, "custom-hero")) {
			unit->SetCharacter(LuaToString(l, 2, j + 1), true);
		} else if (!strcmp(value, "individual-upgrade")) {
			CUpgrade *individual_upgrade = CUpgrade::get(LuaToString(l, 2, j + 1));
			++j;
			int individual_upgrade_quantity = LuaToNumber(l, 2, j + 1);
			unit->SetIndividualUpgrade(individual_upgrade, individual_upgrade_quantity);
		} else if (!strcmp(value, "rally_point")) {
			int rally_point_x = LuaToNumber(l, 2, j + 1);
			++j;
			int rally_point_y = LuaToNumber(l, 2, j + 1);
			unit->rally_point_pos = QPoint(rally_point_x, rally_point_y);
		} else if (!strcmp(value, "rally_point_map_layer")) {
			unit->rally_point_map_layer = CMap::get()->MapLayers[LuaToNumber(l, 2, j + 1)].get();
		//Wyrmgus end
		} else if (!strcmp(value, "last_created_player_unit")) {
			unit->Player->last_created_unit = unit;
			--j;
		} else {
			const int index = UnitTypeVar.VariableNameLookup[value];// User variables
			if (index != -1) { // Valid index
				lua_rawgeti(l, 2, j + 1);
				DefineVariableField(l, unit->Variable[index], -1);
				lua_pop(l, 1);
				continue;
			}
			//Wyrmgus start
//			LuaError(l, "Unit: Unsupported tag: %s" _C_ value);
			fprintf(stderr, "Unit: Unsupported tag: %s\n", value);
			//Wyrmgus end
		}
	}

	// Unit may not have been assigned to a player before now. If not,
	// do so now. It is only assigned earlier if we have orders.
	// for loading of units from a MAP, and not a savegame, we won't
	// have orders for those units.  They should appear here as if
	// they were just created.
	if (!unit->Player) {
		assert_throw(player != nullptr);
		unit->AssignToPlayer(*player);
		UpdateForNewUnit(*unit, 0);
	}

	//  Revealers are units that can see while removed
	if (unit->Removed && unit->Type->BoolFlag[REVEALER_INDEX].value) {
		MapMarkUnitSight(*unit);
	}

	return 0;
}

/**
**  Move a unit on map.
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made placed.
*/
static int CclMoveUnit(lua_State *l)
{
	LuaCheckArgs(l, 2);

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);

	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 2);

	if (UnitCanBeAt(*unit, ipos, unit->MapLayer->ID)) {
		unit->Place(ipos, unit->MapLayer->ID);
	} else {
		const int heading = SyncRand(256);

		unit->tilePos = ipos;
		unit->drop_out_on_side(heading, nullptr);
	}
	lua_pushvalue(l, 1);
	return 1;
}

/**
**  Remove unit from the map.
**
**  @param l  Lua state.
**
**  @return   Returns 1.
*/
static int CclRemoveUnit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		return 0;
	}
	//Wyrmgus end
	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	unit->Remove(nullptr);
	lua_pushvalue(l, 1);
	return 1;
}

/**
**  Create a unit and place it on the map
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made unit.
*/
static int CclCreateUnit(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 3);
	const int nargs = lua_gettop(l);
	if (nargs < 3 || nargs > 4) {
		LuaError(l, "incorrect argument\n");
	}
	
	int z = 0;
	if (nargs >= 4) {
		z = LuaToNumber(l, 4);
	}
	//Wyrmgus end

	lua_pushvalue(l, 1);
	const unit_type *unittype = CclGetUnitType(l);
	lua_pop(l, 1);

	if (unittype == nullptr) {
		log::log_error("Failed to create unit: invalid unit type.");
		return 0;
	}

	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	//Wyrmgus start
	if (!CMap::get()->Info->IsPointOnMap(ipos, z)) {
		log::log_error("Point on map " + point::to_string(ipos) + " (map layer " + std::to_string(z) + ") is invalid.");
		return 0;
	}
	//Wyrmgus end

	lua_pushvalue(l, 2);
	const int playerno = TriggerGetPlayer(l);
	lua_pop(l, 1);
	if (playerno == -1) {
		printf("CreateUnit: You cannot use \"any\" in create-unit, specify a player\n");
		LuaError(l, "bad player");
		return 0;
	}
	if (CPlayer::Players[playerno]->get_type() == player_type::nobody) {
		printf("CreateUnit: player %d does not exist\n", playerno);
		LuaError(l, "bad player");
		return 0;
	}
	CUnit *unit = MakeUnit(*unittype, CPlayer::Players[playerno].get());
	if (unit == nullptr) {
		return 0;
	} else {
		//Wyrmgus start
//		if (UnitCanBeAt(*unit, ipos)
		if (UnitCanBeAt(*unit, ipos, z)
		//Wyrmgus end
			//Wyrmgus start
//			|| (unit->Type->BoolFlag[BUILDING_INDEX].value && CanBuildUnitType(nullptr, *unit->Type, ipos, 0))) {
//			unit->Place(ipos);
			|| (unit->Type->BoolFlag[BUILDING_INDEX].value && CanBuildUnitType(nullptr, *unit->Type, ipos, 0, true, z))) {
			unit->Place(ipos, z);
			//Wyrmgus end
		} else {
			const int heading = SyncRand(256);

			QPoint res_pos;
			try {
				res_pos = FindNearestDrop(*unit->Type, ipos, heading, z, unit->Type->BoolFlag[BUILDING_INDEX].value && GameCycle > 0, GameCycle == 0); //place buildings with a certain distance of each other, if the game cycle is greater than 0 (so if they weren't intentionally placed side-by-side for a map)
			} catch (const std::exception &exception) {
				wyrmgus::exception::report(exception);

				//no suitable location found
				unit->Remove(nullptr);
				LetUnitDie(*unit);
				lua_pushnumber(l, 0);
				return 1;
			}

			unit->Place(res_pos, z);
		}
		UpdateForNewUnit(*unit, 0);

		lua_pushnumber(l, UnitNumber(*unit));
		return 1;
	}
}

//Wyrmgus start
/**
**  Create a unit and place it within a transporter unit
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made unit.
*/
static int CclCreateUnitInTransporter(lua_State *l)
{
	LuaCheckArgs(l, 3);
	
	if (lua_isnil(l, 3)) {
		return 0;
	}

	lua_pushvalue(l, 1);
	wyrmgus::unit_type *unittype = CclGetUnitType(l);
	if (unittype == nullptr) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);

	lua_pushvalue(l, 2);
	const int playerno = TriggerGetPlayer(l);
	lua_pop(l, 1);

	lua_pushvalue(l, 3);
	CUnit *transporter = CclGetUnit(l);
	lua_pop(l, 1);

	Vec2i ipos;
	ipos.x = transporter->tilePos.x;
	ipos.y = transporter->tilePos.y;

	if (playerno == -1) {
		printf("CreateUnit: You cannot use \"any\" in create-unit, specify a player\n");
		LuaError(l, "bad player");
		return 0;
	}
	if (CPlayer::Players[playerno]->get_type() == player_type::nobody) {
		printf("CreateUnit: player %d does not exist\n", playerno);
		LuaError(l, "bad player");
		return 0;
	}
	CUnit *unit = MakeUnit(*unittype, CPlayer::Players[playerno].get());
	if (unit == nullptr || !CanTransport(*transporter, *unit)) {
		return 0;
	} else {
		if (UnitCanBeAt(*unit, ipos, transporter->MapLayer->ID)
			|| (unit->Type->BoolFlag[BUILDING_INDEX].value && CanBuildUnitType(nullptr, *unit->Type, ipos, 0, true, transporter->MapLayer->ID))) {
			unit->Place(ipos, transporter->MapLayer->ID);
		} else {
			const int heading = SyncRand(256);

			unit->tilePos = ipos;
			unit->MapLayer = transporter->MapLayer;
			unit->drop_out_on_side(heading, nullptr);
		}

		// Place the unit inside the transporter.
		unit->Remove(transporter);
		transporter->BoardCount += unit->Type->BoardSize;
		unit->Boarded = 1;
		transporter->UpdateContainerAttackRange();

		UpdateForNewUnit(*unit, 0);

		lua_pushnumber(l, UnitNumber(*unit));
		return 1;
	}
}

/**
**  Create a unit on top of another (such as a gold mine on top of a gold deposit)
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the created unit.
*/
static int CclCreateUnitOnTop(lua_State *l)
{
	LuaCheckArgs(l, 3);
	
	if (lua_isnil(l, 3)) {
		return 0;
	}

	lua_pushvalue(l, 1);
	wyrmgus::unit_type *unittype = CclGetUnitType(l);
	if (unittype == nullptr) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);

	lua_pushvalue(l, 2);
	const int playerno = TriggerGetPlayer(l);
	lua_pop(l, 1);

	lua_pushvalue(l, 3);
	CUnit *on_top = CclGetUnit(l);
	lua_pop(l, 1);

	Vec2i ipos;
	ipos.x = on_top->tilePos.x;
	ipos.y = on_top->tilePos.y;
	int z = on_top->MapLayer->ID;

	if (playerno == -1) {
		printf("CreateUnit: You cannot use \"any\" in create-unit, specify a player\n");
		LuaError(l, "bad player");
		return 0;
	}
	if (CPlayer::Players[playerno]->get_type() == player_type::nobody) {
		printf("CreateUnit: player %d does not exist\n", playerno);
		LuaError(l, "bad player");
		return 0;
	}
	CUnit *unit = MakeUnit(*unittype, CPlayer::Players[playerno].get());
	if (unit == nullptr) {
		return 0;
	} else {
		unit->MapLayer = CMap::get()->MapLayers[z].get();
		unit->ReplaceOnTop(*on_top);
		unit->Place(ipos, z);
		UpdateForNewUnit(*unit, 0);

		lua_pushnumber(l, UnitNumber(*unit));
		return 1;
	}
}

/**
**  Create a building and place it on a random location map
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made unit.
*/
static int CclCreateBuildingAtRandomLocationNear(lua_State *l)
{
	LuaCheckArgs(l, 4);

	lua_pushvalue(l, 1);
	wyrmgus::unit_type *unittype = CclGetUnitType(l);
	if (unittype == nullptr) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	lua_pushvalue(l, 4);
	CUnit *worker = CclGetUnit(l);
	lua_pop(l, 1);
	if (worker == nullptr) {
		LuaError(l, "Worker unit is null");
		return 0;
	}
	
	lua_pushvalue(l, 2);
	const int playerno = TriggerGetPlayer(l);
	lua_pop(l, 1);
	if (playerno == -1) {
		printf("CreateUnit: You cannot use \"any\" in create-unit, specify a player\n");
		LuaError(l, "bad player");
		return 0;
	}
	if (CPlayer::Players[playerno]->get_type() == player_type::nobody) {
		printf("CreateUnit: player %d does not exist\n", playerno);
		LuaError(l, "bad player");
		return 0;
	}
	Vec2i new_pos;
	AiFindBuildingPlace(*worker, *unittype, ipos, &new_pos, true, worker->MapLayer->ID);
	
	if (!CMap::get()->Info->IsPointOnMap(new_pos, worker->MapLayer)) {
		new_pos = CPlayer::Players[playerno]->StartPos;
	}
	
	CUnit *unit = MakeUnit(*unittype, CPlayer::Players[playerno].get());
	
	if (unit == nullptr) {
		return 0;
	} else {
		if (UnitCanBeAt(*unit, new_pos, worker->MapLayer->ID)
			|| (unit->Type->BoolFlag[BUILDING_INDEX].value && CanBuildUnitType(nullptr, *unit->Type, new_pos, 0, true, worker->MapLayer->ID))) {
			unit->Place(new_pos, worker->MapLayer->ID);
		} else {
			const int heading = SyncRand(256);

			unit->tilePos = new_pos;
			unit->MapLayer = worker->MapLayer;
			unit->drop_out_on_side(heading, nullptr);
		}
		UpdateForNewUnit(*unit, 0);
		//Wyrmgus end
		
		lua_pushnumber(l, UnitNumber(*unit));
		return 1;
	}
}
//Wyrmgus end

/**
**  'Upgrade' a unit in place to a unit of different type.
**
**  @param l  Lua state.
**
**  @return   Returns success.
*/
static int CclTransformUnit(lua_State *l)
{
	lua_pushvalue(l, 1);
	CUnit *targetUnit = CclGetUnit(l);
	lua_pop(l, 1);
	lua_pushvalue(l, 2);
	const wyrmgus::unit_type *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	CommandUpgradeTo(*targetUnit, *(wyrmgus::unit_type *)unittype, 1);
	lua_pushvalue(l, 1);
	return 1;
}

/**
**  Damages unit, additionally using another unit as first's attacker
**
**  @param l  Lua state.
**
**  @return   Returns the slot number of the made unit.
*/
static int CclDamageUnit(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int attacker = LuaToNumber(l, 1);
	CUnit *attackerUnit = nullptr;
	if (attacker != -1) {
		attackerUnit = &wyrmgus::unit_manager::get()->GetSlotUnit(attacker);
	}
	lua_pushvalue(l, 2);
	CUnit *targetUnit = CclGetUnit(l);
	lua_pop(l, 1);
	const int damage = LuaToNumber(l, 3);
	HitUnit(attackerUnit, *targetUnit, damage);

	return 1;
}

/**
**  Set resources held by a unit
**
**  @param l  Lua state.
*/
static int CclSetResourcesHeld(lua_State *l)
{
	LuaCheckArgs(l, 2);

	if (lua_isnil(l, 1)) {
		return 0;
	}

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	const int value = LuaToNumber(l, 2);
	//Wyrmgus start
//	unit->ResourcesHeld = value;
	unit->SetResourcesHeld(value);
	//Wyrmgus end
	unit->Variable[GIVERESOURCE_INDEX].Value = value;
	unit->Variable[GIVERESOURCE_INDEX].Max = value;
	unit->Variable[GIVERESOURCE_INDEX].Enable = 1;

	return 0;
}

/**
**  Set teleport deastination for teleporter unit
**
**  @param l  Lua state.
*/
static int CclSetTeleportDestination(lua_State *l)
{
	LuaCheckArgs(l, 2);

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	if (unit->Type->BoolFlag[TELEPORTER_INDEX].value == false) {
		LuaError(l, "Unit not a teleporter");
	}
	lua_pushvalue(l, 2);
	CUnit *dest = CclGetUnit(l);
	lua_pop(l, 1);
	if (dest->IsAliveOnMap()) {
		unit->Goal = dest;
	}

	return 0;
}

/**
**  Order a unit
**
**  @param l  Lua state.
**
**  OrderUnit(player, unit-type, sloc, dloc, order)
*/
static int CclOrderUnit(lua_State *l)
{
	LuaCheckArgs(l, 7);

	lua_pushvalue(l, 1);
	const int plynr = TriggerGetPlayer(l);
	lua_pop(l, 1);
	lua_pushvalue(l, 2);
	const wyrmgus::unit_type *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	if (!lua_istable(l, 3)) {
		LuaError(l, "incorrect argument");
	}
	Vec2i pos1;
	pos1.x = LuaToNumber(l, 3, 1);
	pos1.y = LuaToNumber(l, 3, 2);
	
	Vec2i pos2;
	if (lua_rawlen(l, 3) == 4) {
		pos2.x = LuaToNumber(l, 3, 3);
		pos2.y = LuaToNumber(l, 3, 4);
	} else {
		pos2 = pos1;
	}
	
	int z = LuaToNumber(l, 4);

	Vec2i dpos1;
	Vec2i dpos2;
	if (lua_istable(l, 5)) {
		dpos1.x = LuaToNumber(l, 5, 1);
		dpos1.y = LuaToNumber(l, 5, 2);
		if (lua_rawlen(l, 5) == 4) {
			dpos2.x = LuaToNumber(l, 5, 3);
			dpos2.y = LuaToNumber(l, 5, 4);
		} else {
			dpos2 = dpos1;
		}
	}
	
	int d_z = 0;
	if (lua_isnumber(l, 6)) {
		d_z = LuaToNumber(l, 6);
	}

	const char *order = LuaToString(l, 7);
	std::vector<CUnit *> table;
	Select(pos1, pos2, table, z);
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &unit = *table[i];

		if (unittype == ANY_UNIT
			|| (unittype == ALL_FOODUNITS && !unit.Type->BoolFlag[BUILDING_INDEX].value)
			|| (unittype == ALL_BUILDINGS && unit.Type->BoolFlag[BUILDING_INDEX].value)
			|| unittype == unit.Type) {
			if (plynr == -1 || plynr == unit.Player->get_index()) {
				if (!strcmp(order, "move")) {
					CommandMove(unit, (dpos1 + dpos2) / 2, 1, d_z);
				} else if (!strcmp(order, "attack")) {
					CUnit *attack = TargetOnMap(unit, dpos1, dpos2, d_z);

					CommandAttack(unit, (dpos1 + dpos2) / 2, attack, 1, d_z);
				} else if (!strcmp(order, "attack-move")) {
					CommandAttack(unit, (dpos1 + dpos2) / 2, NoUnitP, 1, d_z);
				} else if (!strcmp(order, "attack-ground")) {
					CommandAttackGround(unit, (dpos1 + dpos2) / 2, 1, d_z);
				} else if (!strcmp(order, "patrol")) {
					CommandPatrolUnit(unit, (dpos1 + dpos2) / 2, 1, d_z);
				} else if (!strcmp(order, "board")) {
					CUnit &transporter = *TargetOnMap(unit, dpos1, dpos2, d_z);

					CommandBoard(unit, transporter, 1);
				} else if (!strcmp(order, "follow")) {
					CommandMove(unit, (dpos1 + dpos2) / 2, 1, d_z);
				} else if (!strcmp(order, "unload")) {
					CommandUnload(unit, (dpos1 + dpos2) / 2, nullptr, 1, d_z);
				} else if (!strcmp(order, "stop")) {					
					CommandStopUnit(unit);
				} else {
					LuaError(l, "Unsupported order: %s" _C_ order);
				}
			}
		}
	}
	return 0;
}

class HasSameUnitTypeAs
{
public:
	explicit HasSameUnitTypeAs(const wyrmgus::unit_type *_type) : type(_type) {}
	bool operator()(const CUnit *unit) const
	{
		return (type == ANY_UNIT || type == unit->Type
				|| (type == ALL_FOODUNITS && !unit->Type->BoolFlag[BUILDING_INDEX].value)
				|| (type == ALL_BUILDINGS && unit->Type->BoolFlag[BUILDING_INDEX].value));
	}
private:
	const wyrmgus::unit_type *type;
};


/**
**  Kill a unit
**
**  @param l  Lua state.
**
**  @return   Returns true if a unit was killed.
*/
static int CclKillUnit(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 2);
	LuaCheckArgs(l, 1);
	//Wyrmgus end

	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);	
	
	if (unit && unit->IsAlive()) {
		LetUnitDie(*unit);
		lua_pushboolean(l, 1);
		return 1;
	}
	//Wyrmgus end
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Kill a unit at a location
**
**  @param l  Lua state.
**
**  @return   Returns the number of units killed.
*/
static int CclKillUnitAt(lua_State *l)
{
	LuaCheckArgs(l, 5);

	lua_pushvalue(l, 1);
	const wyrmgus::unit_type *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	lua_pushvalue(l, 2);
	int plynr = TriggerGetPlayer(l);
	lua_pop(l, 1);
	int q = LuaToNumber(l, 3);

	if (!lua_istable(l, 4) || !lua_istable(l, 5)) {
		LuaError(l, "incorrect argument");
	}
	Vec2i pos1;
	Vec2i pos2;
	CclGetPos(l, &pos1.x, &pos1.y, 4);
	CclGetPos(l, &pos2.x, &pos2.y, 5);

	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(pos1, pos2, table);
	Select(pos1, pos2, table, 0);
	//Wyrmgus end

	int s = 0;
	for (std::vector<CUnit *>::iterator it = table.begin(); it != table.end() && s < q; ++it) {
		CUnit &unit = **it;

		if (unittype == ANY_UNIT
			|| (unittype == ALL_FOODUNITS && !unit.Type->BoolFlag[BUILDING_INDEX].value)
			|| (unittype == ALL_BUILDINGS && unit.Type->BoolFlag[BUILDING_INDEX].value)
			|| unittype == unit.Type) {
			if ((plynr == -1 || plynr == unit.Player->get_index()) && unit.IsAlive()) {
				LetUnitDie(unit);
				++s;
			}
		}
	}
	lua_pushnumber(l, s);
	return 1;
}

//Wyrmgus start
/**
**  Change the owner of a unit
**
**  @param l  Lua state.
*/
static int CclChangeUnitOwner(lua_State *l)
{
	LuaCheckArgs(l, 2);

	if (lua_isnil(l, 1)) {
		return 0;
	}

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	const int value = LuaToNumber(l, 2);
	unit->ChangeOwner(*CPlayer::Players[value], true);

	return 0;
}

/**
**  Convert unit to a new unit type
**
**  @param l  Lua state.
*/
static int CclConvertUnit(lua_State *l)
{
	LuaCheckArgs(l, 2);

	if (lua_isnil(l, 1)) {
		return 0;
	}

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);

	lua_pushvalue(l, 2);
	wyrmgus::unit_type *unittype = CclGetUnitType(l);
	if (unittype == nullptr) {
		LuaError(l, "Bad unittype");
	}
	CommandTransformIntoType(*unit, *unittype);

	return 0;
}

/**
**  Increase unit level
**
**  @param l  Lua state.
*/
static int CclIncreaseUnitLevel(lua_State *l)
{
	LuaCheckArgs(l, 2);

	if (lua_isnil(l, 1)) {
		return 0;
	}

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);

	const int value = LuaToNumber(l, 2);
	unit->IncreaseLevel(value);

	return 0;
}

/**
**  Perform a level check for a unit
**
**  @param l  Lua state.
*/
static int CclUnitLevelCheck(lua_State *l)
{
	LuaCheckArgs(l, 2);

	if (lua_isnil(l, 1)) {
		return 1;
	}

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	const int level = LuaToNumber(l, 2);
	lua_pushboolean(l, unit->LevelCheck(level));
	return 1;
}
//Wyrmgus end

/**
**  Get a player's units
**
**  @param l  Lua state.
**
**  @return   Array of units.
*/
static int CclGetUnits(lua_State *l)
{
	LuaCheckArgs(l, 1);

	const int plynr = TriggerGetPlayer(l);

	lua_newtable(l);
	int index = 0;
	if (plynr == -1) {
		const std::vector<CUnit *> &units = wyrmgus::unit_manager::get()->get_units();
		for (const CUnit *unit : units) {
			if (!unit->IsAlive()) {
				continue;
			}

			lua_pushnumber(l, UnitNumber(*unit));
			lua_rawseti(l, -2, index + 1);
			++index;
		}
	} else {
		for (int i = 0; i < CPlayer::Players[plynr]->GetUnitCount(); ++i) {
			const CUnit &unit = CPlayer::Players[plynr]->GetUnit(i);
			if (!unit.IsAlive()) {
				continue;
			}

			lua_pushnumber(l, UnitNumber(unit));
			lua_rawseti(l, -2, index + 1);
			++index;
		}
	}
	return 1;
}

/**
**  Get a player's units in rectangle box specified with 2 coordinates
**
**  @param l  Lua state.
**
**  @return   Array of units.
*/
static int CclGetUnitsAroundUnit(lua_State *l)
{
	const int nargs = lua_gettop(l);
	if (nargs != 2 && nargs != 3) {
		LuaError(l, "incorrect argument\n");
	}
	
	const int slot = LuaToNumber(l, 1);
	const CUnit &unit = wyrmgus::unit_manager::get()->GetSlotUnit(slot);
	const int range = LuaToNumber(l, 2);
	bool allUnits = false;
	if (nargs == 3) {
		allUnits = LuaToBoolean(l, 3);
	}
	lua_newtable(l);
	std::vector<CUnit *> table;
	if (allUnits) {
		//Wyrmgus start
//		SelectAroundUnit(unit, range, table, HasNotSamePlayerAs(*CPlayer::get_neutral_player()));
		SelectAroundUnit(unit, range, table);
		//Wyrmgus end
	} else {
		SelectAroundUnit(unit, range, table, HasSamePlayerAs(*unit.Player));
	}
	size_t n = 0;
	for (size_t i = 0; i < table.size(); ++i) {
		if (table[i]->IsAliveOnMap()) {
			lua_pushnumber(l, UnitNumber(*table[i]));
			lua_rawseti(l, -2, ++n);
		}
	}
	return 1;
}

//Wyrmgus start
/**
**  Get the players who have units in rectangle box specified with 2 coordinates
**
**  @param l  Lua state.
**
**  @return   Array of units.
*/
static int CclGetPlayersAroundUnit(lua_State *l)
{
	const int nargs = lua_gettop(l);
	if (nargs != 2) {
		LuaError(l, "incorrect argument\n");
	}
	
	const int slot = LuaToNumber(l, 1);
	const CUnit &unit = wyrmgus::unit_manager::get()->GetSlotUnit(slot);
	const int range = LuaToNumber(l, 2);
	lua_newtable(l);
	std::vector<CUnit *> table;
	SelectAroundUnit(unit, range, table, MakeAndPredicate(HasNotSamePlayerAs(*CPlayer::get_neutral_player()), HasNotSamePlayerAs(*unit.Player)));
	std::vector<int> players_around;
	for (size_t i = 0; i < table.size(); ++i) {
		if (table[i]->IsAliveOnMap() && std::find(players_around.begin(), players_around.end(), table[i]->Player->get_index()) == players_around.end()) {
			players_around.push_back(table[i]->Player->get_index());
		}
	}
	size_t n = 0;
	for (size_t i = 0; i < players_around.size(); ++i) {
		lua_pushnumber(l, players_around[i]);
		lua_rawseti(l, -2, ++n);
	}
	return 1;
}

/**
**  Get a player's units inside another unit
**
**  @param l  Lua state.
**
**  @return   Array of units.
*/
static int CclGetUnitsInsideUnit(lua_State *l)
{
	LuaCheckArgs(l, 1);

	const int slot = LuaToNumber(l, 1);
	const CUnit &transporter = wyrmgus::unit_manager::get()->GetSlotUnit(slot);
	
	lua_newtable(l);
	
	for (size_t i = 0; i < transporter.get_units_inside().size(); ++i) {
		const CUnit *unit = transporter.get_units_inside().at(i);
		lua_pushnumber(l, UnitNumber(*unit));
		lua_rawseti(l, -2, i + 1);
	}
	return 1;
}

/**
**  Get selected units
**
**  @param l  Lua state.
**
**  @return   Array of selected units
*/
static int CclGetSelectedUnits(lua_State *l)
{
	lua_newtable(l);
	if (!Selected.empty()) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			lua_pushnumber(l, UnitNumber(*Selected[i]));
			lua_rawseti(l, -2, i + 1);
		}
	}
	return 1;
}
//Wyrmgus end

/**
**
**  Get the value of the unit bool-flag.
**
**  @param l  Lua state.
**
**  @return   The value of the bool-flag of the unit.
*/
static int CclGetUnitBoolFlag(lua_State *l)
{
	LuaCheckArgs(l, 2);

	lua_pushvalue(l, 1);
	const CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);

	const char *const value = LuaToString(l, 2);
	int index = UnitTypeVar.BoolFlagNameLookup[value];// User bool flags
	if (index == -1) {
		LuaError(l, "Bad bool-flag name '%s'\n" _C_ value);
	}
	lua_pushboolean(l, unit->Type->BoolFlag[index].value);
	return 1;
}

/**
**  Get the value of the unit variable.
**
**  @param l  Lua state.
**
**  @return   The value of the variable of the unit.
*/
static int CclGetUnitVariable(lua_State *l)
{
	const int nargs = lua_gettop(l);
	assert_throw(nargs == 2 || nargs == 3);

	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	if (unit == nullptr) {
		return 1;
	}
	UpdateUnitVariables(*unit);
	lua_pop(l, 1);

	const char *const value = LuaToString(l, 2);
	if (!strcmp(value, "RegenerationRate")) {
		lua_pushnumber(l, unit->Variable[HP_INDEX].Increase);
	} else if (!strcmp(value, "Ident")) {
		lua_pushstring(l, unit->Type->get_identifier().c_str());
	} else if (!strcmp(value, "ResourcesHeld")) {
		lua_pushnumber(l, unit->ResourcesHeld);
	} else if (!strcmp(value, "GiveResourceType")) {
		//Wyrmgus start
//		lua_pushnumber(l, unit->Type->GivesResource);
		lua_pushnumber(l, unit->GivesResource);
		//Wyrmgus end
	} else if (!strcmp(value, "CurrentResource")) {
		lua_pushnumber(l, unit->CurrentResource);
	} else if (!strcmp(value, "Name")) {
	//Wyrmgus start
//		lua_pushstring(l, unit->Type->Name.c_str());
		lua_pushstring(l, unit->get_full_name().c_str());
	} else if (!strcmp(value, "Character")) {
		if (unit->get_character() != nullptr) {
			lua_pushstring(l, unit->get_character()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "CustomCharacter")) {
		if (unit->get_character() != nullptr && unit->get_character()->is_custom()) {
			lua_pushboolean(l, true);
		} else {
			lua_pushboolean(l, false);
		}
	} else if (!strcmp(value, "Settlement")) {
		if (unit->get_settlement() != nullptr) {
			lua_pushstring(l, unit->get_settlement()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "GiveResourceTypeName")) {
		//Wyrmgus start
//		lua_pushstring(l, DefaultResourceNames[unit->Type->GivesResource].c_str());
		lua_pushstring(l, DefaultResourceNames[unit->GivesResource].c_str());
		//Wyrmgus end
	} else if (!strcmp(value, "CurrentResourceName")) {
		lua_pushstring(l, DefaultResourceNames[unit->CurrentResource].c_str());
	} else if (!strcmp(value, "TypeName")) {
		lua_pushstring(l, unit->get_type_name().c_str());
	} else if (!strcmp(value, "Trait")) {
		if (unit->Trait != nullptr) {
			lua_pushstring(l, unit->Trait->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "Prefix")) {
		if (unit->Prefix != nullptr) {
			lua_pushstring(l, unit->Prefix->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "Suffix")) {
		if (unit->Suffix != nullptr) {
			lua_pushstring(l, unit->Suffix->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "Unique")) {
		if (unit->get_unique() != nullptr) {
			lua_pushstring(l, unit->get_unique()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "Work")) {
		if (unit->Work != nullptr) {
			lua_pushstring(l, unit->Work->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "Elixir")) {
		if (unit->Elixir != nullptr) {
			lua_pushstring(l, unit->Elixir->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
	} else if (!strcmp(value, "Icon")) {
		if (unit->get_icon() != nullptr) {
			lua_pushstring(l, unit->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(value, "PlayerType")) {
		lua_pushinteger(l, static_cast<int>(unit->Player->get_type()));
	} else if (!strcmp(value, "IndividualUpgrade")) {
		LuaCheckArgs(l, 3);
		std::string upgrade_ident = LuaToString(l, 3);
		CUpgrade *upgrade = CUpgrade::get(upgrade_ident);
		lua_pushnumber(l, unit->GetIndividualUpgrade(upgrade));
		return 1;
	} else if (!strcmp(value, "Active")) {
		lua_pushboolean(l, unit->Active);
		return 1;
	} else if (!strcmp(value, "Idle")) {
		lua_pushboolean(l, unit->IsIdle());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(value, "Removed")) {
		lua_pushboolean(l, unit->Removed);
		return 1;
	} else if (!strcmp(value, "Built")) {
		lua_pushboolean(l, unit->CurrentAction() != UnitAction::Built);
		return 1;
	} else if (!strcmp(value, "Container")) {
		if (unit->Container != nullptr) {
			lua_pushnumber(l, UnitNumber(*unit->Container));
		} else {
			lua_pushnumber(l, -1);
		}
		return 1;
	} else if (!strcmp(value, "MapLayer")) {
		lua_pushnumber(l, unit->MapLayer->ID);
	} else if (!strcmp(value, "EffectiveResourceSellPrice")) {
		LuaCheckArgs(l, 3);
		const std::string resource_ident = LuaToString(l, 3);
		const resource *resource = resource::get(resource_ident);
		lua_pushnumber(l, unit->Player->get_effective_resource_sell_price(resource));
		return 1;
	} else if (!strcmp(value, "EffectiveResourceBuyPrice")) {
		LuaCheckArgs(l, 3);
		const std::string resource_ident = LuaToString(l, 3);
		const resource *resource = resource::get(resource_ident);
		lua_pushnumber(l, unit->Player->get_effective_resource_buy_price(resource));
		return 1;
	//Wyrmgus end
	} else {
		int index = UnitTypeVar.VariableNameLookup[value];// User variables
		if (index == -1) {
			LuaError(l, "Bad variable name '%s'\n" _C_ value);
		}
		if (nargs == 2) {
			lua_pushnumber(l, unit->Variable[index].Value);
		} else {
			const char *const type = LuaToString(l, 3);
			if (!strcmp(type, "Value")) {
				//Wyrmgus start
//				lua_pushnumber(l, unit->Variable[index].Value);
				lua_pushnumber(l, unit->GetModifiedVariable(index, VariableAttribute::Value));
				//Wyrmgus end
			} else if (!strcmp(type, "Max")) {
				//Wyrmgus start
//				lua_pushnumber(l, unit->Variable[index].Max);
				lua_pushnumber(l, unit->GetModifiedVariable(index, VariableAttribute::Max));
				//Wyrmgus end
			} else if (!strcmp(type, "Increase")) {
				//Wyrmgus start
//				lua_pushnumber(l, unit->Variable[index].Increase);
				lua_pushnumber(l, unit->GetModifiedVariable(index, VariableAttribute::Increase));
				//Wyrmgus end
			} else if (!strcmp(type, "Enable")) {
				lua_pushnumber(l, unit->Variable[index].Enable);
			} else {
				LuaError(l, "Bad variable type '%s'\n" _C_ type);
			}
		}
	}
	return 1;
}

/**
**  Set the value of the unit variable.
**
**  @param l  Lua state.
**
**  @return The new value of the unit.
*/
static int CclSetUnitVariable(lua_State *l)
{
	const int nargs = lua_gettop(l);
	//Wyrmgus start
//	assert_throw(nargs >= 3 && nargs <= 5);
	assert_throw(nargs >= 2 && nargs <= 5);
	//Wyrmgus end

	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		log::log_error("Failed to set unit variable: unit is null.");
		return 0;
	}
	//Wyrmgus end
	
	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	const char *const name = LuaToString(l, 2);
	//Wyrmgus start
//	int value;
	int value = 0;
	//Wyrmgus end
	if (!strcmp(name, "Player")) {
		value = LuaToNumber(l, 3);
		unit->AssignToPlayer(*CPlayer::Players[value]);
	//Wyrmgus start
	} else if (!strcmp(name, "Name")) {
		unit->Name = LuaToString(l, 3);
	//Wyrmgus end
	} else if (!strcmp(name, "RegenerationRate")) {
		value = LuaToNumber(l, 3);
		unit->Variable[HP_INDEX].Increase = std::min(unit->Variable[HP_INDEX].Max, value);
	} else if (!strcmp(name, "IndividualUpgrade")) {
		LuaCheckArgs(l, 4);
		std::string upgrade_ident = LuaToString(l, 3);
		bool has_upgrade = LuaToBoolean(l, 4);
		if (has_upgrade && unit->GetIndividualUpgrade(CUpgrade::get(upgrade_ident)) == 0) {
			IndividualUpgradeAcquire(*unit, CUpgrade::get(upgrade_ident));
		} else if (!has_upgrade && unit->GetIndividualUpgrade(CUpgrade::get(upgrade_ident))) {
			IndividualUpgradeLost(*unit, CUpgrade::get(upgrade_ident));
		}
	} else if (!strcmp(name, "Active")) {
		const bool ai_active = LuaToBoolean(l, 3);
		if (ai_active != unit->is_ai_active()) {
			if (ai_active) {
				unit->Player->ChangeUnitTypeAiActiveCount(unit->Type, 1);
			} else {
				unit->Player->ChangeUnitTypeAiActiveCount(unit->Type, -1);
				if (unit->Player->GetUnitTypeAiActiveCount(unit->Type) < 0) { // if unit AI active count is negative, something wrong happened
					fprintf(stderr, "Player %d has a negative %s AI active count of %d.\n", unit->Player->get_index(), unit->Type->get_identifier().c_str(), unit->Player->GetUnitTypeAiActiveCount(unit->Type));
				}
			}
		}
		unit->Active = ai_active;
	//Wyrmgus start
	} else if (!strcmp(name, "Character")) {
		unit->SetCharacter(LuaToString(l, 3));
	} else if (!strcmp(name, "CustomHero")) {
		unit->SetCharacter(LuaToString(l, 3), true);
	} else if (!strcmp(name, "Variation")) {
		size_t variation_index = LuaToNumber(l, 3);
		if (variation_index >= 0 && variation_index < unit->Type->get_variations().size()) {
			unit->SetVariation(unit->Type->get_variations()[variation_index].get());
			unit->Variable[VARIATION_INDEX].Value = unit->Variation;
		}
	} else if (!strcmp(name, "LayerVariation")) {
		LuaCheckArgs(l, 4);
		std::string image_layer_name = LuaToString(l, 3);
		int image_layer = GetImageLayerIdByName(image_layer_name);
		if (image_layer != -1) {
			size_t variation_index = LuaToNumber(l, 4);
			if (variation_index >= 0 && variation_index < unit->Type->LayerVariations[image_layer].size()) {
				unit->SetVariation(unit->Type->LayerVariations[image_layer][variation_index].get(), image_layer);
			}
		} else {
			LuaError(l, "Image layer \"%s\" doesn't exist." _C_ image_layer_name.c_str());
		}
	} else if (!strcmp(name, "Prefix")) {
		//add an item prefix to the unit
		LuaCheckArgs(l, 3);
		const std::string upgrade_ident = LuaToString(l, 3);
		if (upgrade_ident.empty()) {
			unit->SetPrefix(nullptr);
		} else {
			unit->SetPrefix(CUpgrade::try_get(upgrade_ident));
		}
	} else if (!strcmp(name, "Suffix")) {
		//add an item suffix to the unit
		LuaCheckArgs(l, 3);
		const std::string upgrade_ident = LuaToString(l, 3);
		if (upgrade_ident.empty()) {
			unit->SetSuffix(nullptr);
		} else {
			unit->SetSuffix(CUpgrade::try_get(upgrade_ident));
		}
	} else if (!strcmp(name, "Spell")) { //add a spell to the item unit
		LuaCheckArgs(l, 3);
		std::string spell_ident = LuaToString(l, 3);
		if (spell_ident.empty()) {
			unit->SetSpell(nullptr);
		} else {
			unit->SetSpell(wyrmgus::spell::get(spell_ident));
		}
	} else if (!strcmp(name, "Work")) { //add a literary work property to the unit
		LuaCheckArgs(l, 3);
		std::string upgrade_ident = LuaToString(l, 3);
		if (upgrade_ident.empty()) {
			unit->SetWork(nullptr);
		} else {
			unit->SetWork(CUpgrade::get(upgrade_ident));
		}
	} else if (!strcmp(name, "Elixir")) { //add an elixir property to the unit
		LuaCheckArgs(l, 3);
		std::string upgrade_ident = LuaToString(l, 3);
		if (upgrade_ident.empty()) {
			unit->SetElixir(nullptr);
		} else {
			unit->SetElixir(CUpgrade::get(upgrade_ident));
		}
	} else if (!strcmp(name, "Unique")) { //set the unit to a particular unique unit
		LuaCheckArgs(l, 3);
		const std::string unique_name = LuaToString(l, 3);
		if (unique_name.empty()) {
			unit->set_unique(nullptr);
		} else {
			unit->set_unique(wyrmgus::unique_item::get(unique_name));
		}
	} else if (!strcmp(name, "GenerateSpecialProperties")) {
		CPlayer *dropper_player = unit->Player;
		bool always_magic = false;
		if (nargs >= 3) {
			int dropper_player_index = LuaToNumber(l, 3);
			dropper_player = CPlayer::Players[dropper_player_index].get();
		}
		if (nargs >= 4) {
			always_magic = LuaToBoolean(l, 4);
		}
		unit->generate_special_properties(nullptr, dropper_player, true, false, always_magic);
	} else if (!strcmp(name, "TTL")) {
		unit->TTL = GameCycle + LuaToNumber(l, 3);
	} else if (!strcmp(name, "Identified")) {
		unit->Identified = LuaToBoolean(l, 3);
	//Wyrmgus end
	} else if (!strcmp(name, "ResourcesHeld")) {
		unit->ResourcesHeld = LuaToNumber(l, 3);
	} else {
		const int index = UnitTypeVar.VariableNameLookup[name];// User variables
		if (index == -1) {
			LuaError(l, "Bad variable name '%s'\n" _C_ name);
		}

		const int old_value = unit->Variable[index].Value;

		value = LuaToNumber(l, 3);
		if (nargs == 3) {
			//Wyrmgus start
//			unit->Variable[index].Value = std::min(unit->Variable[index].Max, value);
			unit->Variable[index].Value = std::min(unit->GetModifiedVariable(index, VariableAttribute::Max), value);
			//Wyrmgus end
		} else {
			const char *const type = LuaToString(l, 4);
			if (!strcmp(type, "Value")) {
				//Wyrmgus start
//				unit->Variable[index].Value = std::min(unit->Variable[index].Max, value);
				unit->Variable[index].Value = std::min(unit->GetModifiedVariable(index, VariableAttribute::Max), value);
				//Wyrmgus end
			} else if (!strcmp(type, "Max")) {
				unit->Variable[index].Max = value;
			} else if (!strcmp(type, "Increase")) {
				unit->Variable[index].Increase = value;
			} else if (!strcmp(type, "Enable")) {
				unit->Variable[index].Enable = value;
			} else {
				LuaError(l, "Bad variable type '%s'\n" _C_ type);
			}
		}

		unit->on_variable_changed(index, unit->Variable[index].Value - old_value);
	}
	lua_pushnumber(l, value);
	//Wyrmgus start
	if (IsOnlySelected(*unit)) {
		UI.ButtonPanel.Update();
	}
	//Wyrmgus end
	return 1;
}

/**
**  Get the usage of unit slots during load to allocate memory
**
**  @param l  Lua state.
*/
static int CclSlotUsage(lua_State *l)
{
	wyrmgus::unit_manager::get()->Load(l);
	return 0;
}

static int CclSelectSingleUnit(lua_State *l)
{
	const int nargs = lua_gettop(l);
	assert_throw(nargs == 1);
	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	SelectSingleUnit(*unit);
	SelectionChanged();
	return 0;
}

//Wyrmgus start
/**
**  Return whether the unit is at a certain location
*/
static int CclUnitIsAt(lua_State *l)
{
	LuaCheckArgs(l, 3);

	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = CclGetUnit(l);
	lua_pop(l, 1);
	
	Vec2i minPos;
	Vec2i maxPos;
	CclGetPos(l, &minPos.x, &minPos.y, 2);
	CclGetPos(l, &maxPos.x, &maxPos.y, 3);

	if (unit->tilePos.x >= minPos.x && unit->tilePos.x <= maxPos.x && unit->tilePos.y >= minPos.y && unit->tilePos.y <= maxPos.y) {
		lua_pushboolean(l, true);
	} else {
		lua_pushboolean(l, false);
	}
	return 1;
}
//Wyrmgus end

/**
**  Register CCL features for unit.
*/
void UnitCclRegister()
{
	lua_register(Lua, "SetTrainingQueue", CclSetTrainingQueue);
	lua_register(Lua, "SetRevealAttacker", CclSetRevealAttacker);
	lua_register(Lua, "ResourcesMultiBuildersMultiplier", CclResourcesMultiBuildersMultiplier);

	lua_register(Lua, "Unit", CclUnit);

	lua_register(Lua, "MoveUnit", CclMoveUnit);
	lua_register(Lua, "RemoveUnit", CclRemoveUnit);
	lua_register(Lua, "CreateUnit", CclCreateUnit);
	lua_register(Lua, "TransformUnit", CclTransformUnit);
	lua_register(Lua, "DamageUnit", CclDamageUnit);
	lua_register(Lua, "SetResourcesHeld", CclSetResourcesHeld);
	lua_register(Lua, "SetTeleportDestination", CclSetTeleportDestination);
	lua_register(Lua, "OrderUnit", CclOrderUnit);
	lua_register(Lua, "KillUnit", CclKillUnit);
	lua_register(Lua, "KillUnitAt", CclKillUnitAt);
	//Wyrmgus start
	lua_register(Lua, "CreateUnitInTransporter", CclCreateUnitInTransporter);
	lua_register(Lua, "CreateUnitOnTop", CclCreateUnitOnTop);
	lua_register(Lua, "CreateBuildingAtRandomLocationNear", CclCreateBuildingAtRandomLocationNear);
	lua_register(Lua, "ChangeUnitOwner", CclChangeUnitOwner);
	lua_register(Lua, "ConvertUnit", CclConvertUnit);
	lua_register(Lua, "IncreaseUnitLevel", CclIncreaseUnitLevel);
	lua_register(Lua, "UnitLevelCheck", CclUnitLevelCheck);
	//Wyrmgus end

	lua_register(Lua, "GetUnits", CclGetUnits);
	lua_register(Lua, "GetUnitsAroundUnit", CclGetUnitsAroundUnit);
	//Wyrmgus start
	lua_register(Lua, "GetPlayersAroundUnit", CclGetPlayersAroundUnit);
	lua_register(Lua, "GetUnitsInsideUnit", CclGetUnitsInsideUnit);
	lua_register(Lua, "GetSelectedUnits", CclGetSelectedUnits);
	//Wyrmgus end

	// unit member access functions
	lua_register(Lua, "GetUnitBoolFlag", CclGetUnitBoolFlag);
	lua_register(Lua, "GetUnitVariable", CclGetUnitVariable);
	lua_register(Lua, "SetUnitVariable", CclSetUnitVariable);

	lua_register(Lua, "SlotUsage", CclSlotUsage);
	
	lua_register(Lua, "SelectSingleUnit", CclSelectSingleUnit);
	
	//Wyrmgus start
	lua_register(Lua, "UnitIsAt", CclUnitIsAt);
	//Wyrmgus end
}
