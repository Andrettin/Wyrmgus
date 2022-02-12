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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "action/action_unload.h"

#include "animation/animation.h"
#include "animation/animation_set.h"
//Wyrmgus start
#include "character.h"
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "game/game.h"
//Wyrmgus end
#include "iolib.h"
#include "map/landmass.h"
#include "map/map.h"
#include "map/map_layer.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "pathfinder/pathfinder.h"
#include "player/player.h"
#include "script.h"
#include "spell/status_effect.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_domain.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/point_util.h"
#include "video/video.h"

std::unique_ptr<COrder> COrder::NewActionUnload(const Vec2i &pos, CUnit *what, int z, const landmass *landmass)
{
	auto order = std::make_unique<COrder_Unload>();

	order->goalPos = pos;
	//Wyrmgus start
	order->MapLayer = z;
	order->landmass = landmass;
	//Wyrmgus end
	if (what && !what->Destroyed) {
		order->set_goal(what);
	}
	return order;
}

void COrder_Unload::Save(CFile &file, const CUnit &unit) const
{
	Q_UNUSED(unit)

	file.printf("{\"action-unload\",");
	if (this->Finished) {
		file.printf(" \"finished\", ");
	}
	file.printf(" \"range\", %d,", this->Range);
	if (this->has_goal()) {
		file.printf(" \"goal\", \"%s\",", UnitReference(this->get_goal()).c_str());
	}
	file.printf(" \"tile\", {%d, %d}, ", this->goalPos.x, this->goalPos.y);
	//Wyrmgus start
	file.printf(" \"map-layer\", %d,", this->MapLayer);
	//Wyrmgus end

	if (this->landmass != nullptr) {
		file.printf(" \"landmass\", %zu,", this->landmass->get_index());
	}

	file.printf("\"state\", %d", this->State);
	file.printf("}");
}

bool COrder_Unload::ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit)
{
	Q_UNUSED(unit)

	if (!strcmp("state", value)) {
		++j;
		this->State = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "range")) {
		++j;
		this->Range = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "tile")) {
		++j;
		lua_rawgeti(l, -1, j + 1);
		CclGetPos(l, &this->goalPos.x , &this->goalPos.y);
		lua_pop(l, 1);
	//Wyrmgus start
	} else if (!strcmp(value, "map-layer")) {
		++j;
		this->MapLayer = LuaToNumber(l, -1, j + 1);
	} else if (!strcmp(value, "landmass")) {
		++j;
		const int landmass_index = LuaToNumber(l, -1, j + 1);
		this->landmass = CMap::get()->get_landmasses()[landmass_index].get();
	//Wyrmgus end
	} else {
		return false;
	}
	return true;
}

bool COrder_Unload::IsValid() const
{
	return true;
}

PixelPos COrder_Unload::Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	if (this->MapLayer != UI.CurrentMapLayer->ID) {
		return lastScreenPos;
	}

	const PixelPos targetPos = vp.TilePosToScreen_Center(this->goalPos);

	if (preferences::get()->are_pathlines_enabled()) {
		Video.FillCircleClip(ColorGreen, lastScreenPos, (2 * preferences::get()->get_scale_factor()).to_int(), render_commands);
		Video.DrawLineClip(ColorGreen, lastScreenPos, targetPos, render_commands);
		Video.FillCircleClip(ColorGreen, targetPos, (3 * preferences::get()->get_scale_factor()).to_int(), render_commands);
	}

	return targetPos;
}

void COrder_Unload::UpdatePathFinderData(PathFinderInput &input)
{
	input.SetMinRange(0);
	input.SetMaxRange(0);

	const Vec2i tileSize(0, 0);

	input.SetGoal(this->goalPos, tileSize, this->MapLayer);
}


/**
**  Find a free position close to startPos
**
**  @param transporter
**  @param unit         Unit to unload.
**  @param startPos     Original search position
**  @param maxrange     maximum range to unload.
**  @param res          Unload position.
**
**  @return      True if a position was found, False otherwise.
**  @note        res is undefined if a position is not found.
**
**  @bug         FIXME: Place unit only on fields reachable from the transporter
*/
//Wyrmgus start
//static bool FindUnloadPosition(const CUnit &transporter, const CUnit &unit, const Vec2i startPos, int maxRange, Vec2i *res)
static bool FindUnloadPosition(const CUnit &transporter, const CUnit &unit, const Vec2i startPos, int maxRange, Vec2i *res, int z, const landmass *landmass = nullptr, const std::optional<QPoint> &target_pos = std::nullopt)
//Wyrmgus end
{
	Vec2i pos = startPos;

	pos -= Vec2i(unit.Type->get_tile_size() - QSize(1, 1));
	int addx = transporter.Type->get_tile_width() + unit.Type->get_tile_width() - 1;
	int addy = transporter.Type->get_tile_height() + unit.Type->get_tile_height() - 1;

	bool found = false;
	int best_distance = INT_MAX;

	--pos.x;
	for (int range = 0; range < maxRange; ++range) {
		for (int i = addy; i--; ++pos.y) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z) && (!landmass || CMap::get()->get_tile_landmass(pos, z) == landmass)) {
			//Wyrmgus end
				if (target_pos.has_value()) {
					const int distance = wyrmgus::point::square_distance_to(pos, target_pos.value());
					if (distance < best_distance) {
						best_distance = distance;
						*res = pos;
						found = true;
					}
				} else {
					*res = pos;
					return true;
				}
			}
		}
		++addx;

		for (int i = addx; i--; ++pos.x) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z) && (!landmass || CMap::get()->get_tile_landmass(pos, z) == landmass)) {
			//Wyrmgus end
				if (target_pos.has_value()) {
					const int distance = wyrmgus::point::square_distance_to(pos, target_pos.value());
					if (distance < best_distance) {
						best_distance = distance;
						*res = pos;
						found = true;
					}
				} else {
					*res = pos;
					return true;
				}
			}
		}
		++addy;

		for (int i = addy; i--; --pos.y) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z) && (!landmass || CMap::get()->get_tile_landmass(pos, z) == landmass)) {
			//Wyrmgus end
				if (target_pos.has_value()) {
					const int distance = wyrmgus::point::square_distance_to(pos, target_pos.value());
					if (distance < best_distance) {
						best_distance = distance;
						*res = pos;
						found = true;
					}
				} else {
					*res = pos;
					return true;
				}
			}
		}
		++addx;

		for (int i = addx; i--; --pos.x) {
			//Wyrmgus start
//			if (UnitCanBeAt(unit, pos)) {
			if (UnitCanBeAt(unit, pos, z) && (!landmass || CMap::get()->get_tile_landmass(pos, z) == landmass)) {
			//Wyrmgus end
				if (target_pos.has_value()) {
					const int distance = wyrmgus::point::square_distance_to(pos, target_pos.value());
					if (distance < best_distance) {
						best_distance = distance;
						*res = pos;
						found = true;
					}
				} else {
					*res = pos;
					return true;
				}
			}
		}
		++addy;
	}

	return found;
}

/**
**  Reappear unit on map.
**
**  @param unit  Unit to drop out.
**
**  @return      True if unit is unloaded.
**
**  @bug         FIXME: Place unit only on fields reachable from the transporter
*/
//Wyrmgus start
//static int UnloadUnit(CUnit &transporter, CUnit &unit)
static int UnloadUnit(CUnit &transporter, CUnit &unit, const landmass *landmass)
//Wyrmgus end
{
	static constexpr int max_range = 1;
	Vec2i pos;

	std::optional<QPoint> target_unload_pos;
	if (transporter.has_rally_point() && transporter.get_rally_point_map_layer() == transporter.MapLayer) {
		target_unload_pos = transporter.get_rally_point_pos();
	}

	assert_throw(unit.Removed);
	//Wyrmgus start
//	if (!FindUnloadPosition(transporter, unit, transporter.tilePos, max_range, &pos)) {
	if (!FindUnloadPosition(transporter, unit, transporter.tilePos, max_range, &pos, transporter.MapLayer->ID, landmass, target_unload_pos)) {
	//Wyrmgus end
		return false;
	}
	//Wyrmgus start
	if (unit.Type->BoolFlag[ITEM_INDEX].value && transporter.HasInventory() && transporter.IsItemEquipped(&unit)) { //if the unit is an equipped item in the transporter's inventory, deequip it
		transporter.DeequipItem(unit);
	}
	
	if (game::get()->is_persistency_enabled() && transporter.get_character() != nullptr && transporter.Player == CPlayer::GetThisPlayer() && unit.Type->BoolFlag[ITEM_INDEX].value) { //if the transporter has a character and the unit is an item, remove it from the character's item list
		wyrmgus::persistent_item *item = transporter.get_character()->get_item(&unit);
		transporter.get_character()->remove_item(item);
		transporter.get_character()->save();
	}
	
	//Wyrmgus start
	//if this is a naval transporter and the unit has a disembarkment bonus, apply it
	if (transporter.Type->get_domain() == unit_domain::water && unit.Variable[DISEMBARKMENTBONUS_INDEX].Value > 0) {
		unit.apply_status_effect(status_effect::inspire, 1000);
	}
	//Wyrmgus end
	
	/*
	unit.Boarded = 0;
	unit.Place(pos);
	transporter.BoardCount -= unit.Type->BoardSize;
	*/
	if (unit.Boarded) {
		unit.Boarded = 0;
		transporter.BoardCount -= unit.Type->BoardSize;
	}
	unit.Place(pos, transporter.MapLayer->ID);

	if (unit.Type->BoolFlag[ITEM_INDEX].value && unit.get_unique() == nullptr) { //destroy items if they have been on the ground for too long
		int ttl_cycles = (5 * 60 * CYCLES_PER_SECOND);
		if (unit.Prefix != nullptr || unit.Suffix != nullptr || unit.Spell != nullptr || unit.Work != nullptr || unit.Elixir != nullptr) {
			ttl_cycles *= 4;
		}
		unit.TTL = GameCycle + ttl_cycles;
	}
	//Wyrmgus end
	
	//Wyrmgus start
	//if transporter has a rally point (useful for towers), send the unloaded unit there
	if (transporter.has_rally_point() && unit.CanMove()) {
		CommandMove(unit, transporter.get_rally_point_pos(), FlushCommands, transporter.get_rally_point_map_layer()->ID);
	}
	//Wyrmgus end
	return true;
}

/**
**  Return true if position is a correct place to drop out units.
**
**  @param transporter  Transporter unit.
**  @param pos          position to drop out units.
*/
//Wyrmgus start
//static bool IsDropZonePossible(const CUnit &transporter, const Vec2i &pos)
static bool IsDropZonePossible(const CUnit &transporter, const Vec2i &pos, int z, const landmass *landmass = nullptr, CUnit *goal = nullptr)
//Wyrmgus end
{
	const int maxUnloadRange = 1;

	//Wyrmgus start
//	if (!UnitCanBeAt(transporter, pos)) {
	if (!UnitCanBeAt(transporter, pos, z)) {
	//Wyrmgus end
		return false;
	}
	Vec2i dummyPos;
	//Wyrmgus start
	/*
	for (const CUnit *unit : transporter.get_units_inside()) {
		//Wyrmgus start
//		if (FindUnloadPosition(transporter, *unit, pos, maxUnloadRange, &dummyPos)) {
		if (FindUnloadPosition(transporter, *unit, pos, maxUnloadRange, &dummyPos, z, landmass)) {
		//Wyrmgus end
			return true;
		}
	}
	*/
	if (goal) {
		if (FindUnloadPosition(transporter, *goal, pos, maxUnloadRange, &dummyPos, z, landmass)) {
			return true;
		}
	} else {
		for (const CUnit *unit : transporter.get_units_inside()) {
			if (FindUnloadPosition(transporter, *unit, pos, maxUnloadRange, &dummyPos, z, landmass)) {
				return true;
			}
		}
	}
	//Wyrmgus end
	// Check unit can be droped from here.
	return false;
}


/**
**  Find the closest available drop zone for a transporter.
**  Fail if transporter don't transport any unit..
**
**  @param  transporter  the transporter
**  @param  startPos     start location for the search
**	@param  maxRange     The maximum distance from initial position to search...
**  @param  resPos       drop zone position
**
**  @return              true if a location was found, false otherwise
**  @note to be called only from ClosestFreeDropZone.
*/
//Wyrmgus start
//static bool ClosestFreeDropZone_internal(const CUnit &transporter, const Vec2i &startPos, int maxRange, Vec2i *resPos)
static bool ClosestFreeDropZone_internal(const CUnit &transporter, const Vec2i &startPos, int maxRange, Vec2i *resPos, int z, const landmass *landmass, CUnit *goal)
//Wyrmgus end
{
	int addx = 0;
	int addy = 1;
	Vec2i pos = startPos;

	for (int range = 0; range < maxRange; ++range) {
		for (int i = addy; i--; ++pos.y) {
			//Wyrmgus start
//			if (IsDropZonePossible(transporter, pos)) {
			if (IsDropZonePossible(transporter, pos, z, landmass, goal)) {
			//Wyrmgus end
				*resPos = pos;
				return true;
			}
		}
		++addx;
		for (int i = addx; i--; ++pos.x) {
			//Wyrmgus start
//			if (IsDropZonePossible(transporter, pos)) {
			if (IsDropZonePossible(transporter, pos, z, landmass, goal)) {
			//Wyrmgus end
				*resPos = pos;
				return true;
			}
		}
		++addy;
		for (int i = addy; i--; --pos.y) {
			//Wyrmgus start
//			if (IsDropZonePossible(transporter, pos)) {
			if (IsDropZonePossible(transporter, pos, z, landmass, goal)) {
			//Wyrmgus end
				*resPos = pos;
				return true;
			}
		}
		++addx;
		for (int i = addx; i--; --pos.x) {
			//Wyrmgus start
//			if (IsDropZonePossible(transporter, pos)) {
			if (IsDropZonePossible(transporter, pos, z, landmass, goal)) {
			//Wyrmgus end
				*resPos = pos;
				return true;
			}
		}
		++addy;
	}
	DebugPrint("Try clicking closer to an actual coast.\n");
	return false;
}

/**
**  Find the closest available drop zone for a transporter.
**  Fail if transporter don't transport any unit..
**
**  @param  transporter  the transporter
**  @param  startPos     start location for the search
**	@param  maxRange     The maximum distance from initial position to search...
**  @param  resPos       drop zone position
**
**  @return              1 if a location was found, 0 otherwise
*/
//Wyrmgus start
//static int ClosestFreeDropZone(CUnit &transporter, const Vec2i &startPos, int maxRange, Vec2i *resPos)
static int ClosestFreeDropZone(CUnit &transporter, const Vec2i &startPos, int maxRange, Vec2i *resPos, int z, const landmass *landmass, CUnit *goal)
//Wyrmgus end
{
	// Check there are units onboard
	if (!transporter.has_units_inside()) {
		return 0;
	}

	const bool isTransporterRemoved = transporter.Removed;
	const bool selected = transporter.Selected;

	if (!isTransporterRemoved) {
		// Remove transporter to avoid "collision" with itself.
		//Wyrmgus start
//		transporter.Remove(nullptr);
		UnmarkUnitFieldFlags(transporter);
		//Wyrmgus end
	}
	//Wyrmgus start
//	const bool res = ClosestFreeDropZone_internal(transporter, startPos, maxRange, resPos);
	const bool res = ClosestFreeDropZone_internal(transporter, startPos, maxRange, resPos, z, landmass, goal);
	//Wyrmgus end
	if (!isTransporterRemoved) {
		//Wyrmgus start
//		transporter.Place(transporter.tilePos);
		MarkUnitFieldFlags(transporter);
		//Wyrmgus end
		if (selected) {
			SelectUnit(transporter);
			SelectionChanged();
		}
	}
	return res;
}


/**
**  Move to dropzone.
**
**  @param unit  Pointer to unit.
**
**  @return      -1 if unreachable, True if reached, False otherwise.
*/
static int MoveToDropZone(CUnit &unit)
{
	switch (DoActionMove(unit)) { // reached end-point?
		case PF_UNREACHABLE:
			return PF_UNREACHABLE;
		case PF_REACHED:
			break;
		default:
			return 0;
	}

	assert_throw(unit.CurrentAction() == UnitAction::Unload);
	return 1;
}

/**
**  Make one or more unit leave the transporter.
**
**  @return false if action should continue
*/
bool COrder_Unload::LeaveTransporter(CUnit &transporter)
{
	int stillonboard = 0;

	// Goal is the specific unit unit that you want to unload.
	// This can be null, in case you want to unload everything.
	if (this->has_goal()) {
		CUnit &goal = *this->get_goal();

		if (goal.Destroyed) {
			DebugPrint("destroyed unit unloading?\n");
			this->clear_goal();
			return true;
		}
		//Wyrmgus start
//		transporter.CurrentOrder()->ClearGoal();
		//Wyrmgus end
		// Try to unload the unit. If it doesn't work there is no problem.
		//Wyrmgus start
//		if (UnloadUnit(transporter, goal)) {
		if (UnloadUnit(transporter, goal, this->landmass)) {
		//Wyrmgus end
			this->clear_goal();
		} else {
			++stillonboard;
		}
	} else {
		//copy the vector since we may modify it
		const std::vector<CUnit *> units_inside = transporter.get_units_inside();

		//unload all units
		for (CUnit *goal : units_inside) {
			if (goal->Boarded) {
				//Wyrmgus start
//				if (!UnloadUnit(transporter, *goal)) {
				if (!UnloadUnit(transporter, *goal, this->landmass)) {
				//Wyrmgus end
					++stillonboard;
				}
			}
		}
	}
	if (IsOnlySelected(transporter)) {
		SelectedUnitChanged();
	}

	// We still have some units to unload, find a piece of free coast.
	if (stillonboard) {
		// We tell it to unload at it's current position. This can't be done,
		// so it will search for a piece of free coast nearby.
		this->State = 0;
		return false;
	} else {
		return true;
	}
}

void COrder_Unload::Execute(CUnit &unit)
{
	const int maxSearchRange = 20;

	if (!unit.CanMove()) {
		this->State = 2;
	}
	
	if (unit.Wait) {
		if (!unit.Waiting) {
			unit.Waiting = 1;
			unit.WaitBackup = unit.Anim;
		}
		UnitShowAnimation(unit, unit.get_animation_set()->Still);
		unit.Wait--;
		return;
	}
	if (unit.Waiting) {
		unit.Anim = unit.WaitBackup;
		unit.Waiting = 0;
	}
	if (this->State == 1 && this->Range >= 5) {
		// failed to reach the goal
		this->State = 2;
	}
	switch (this->State) {
		case 0: // Choose destination
			//Wyrmgus start
//			if (!this->HasGoal()) {
			if (true) {
			//Wyrmgus end
				Vec2i pos;

				//Wyrmgus start
//				if (!ClosestFreeDropZone(unit, this->goalPos, maxSearchRange, &pos)) {
				if (!ClosestFreeDropZone(unit, this->goalPos, maxSearchRange, &pos, this->MapLayer, this->landmass, this->get_goal())) {
				//Wyrmgus end
					this->Finished = true;
					return;
				}
				this->goalPos = pos;
			}

			this->State = 1;
		// follow on next case
		case 1: // Move unit to destination
			// The Goal is the unit that we have to unload.
			//Wyrmgus start
//			if (!this->HasGoal()) {
			if (true) {
			//Wyrmgus end
				const int moveResult = MoveToDropZone(unit);

				// We have to unload everything
				if (moveResult) {
					if (moveResult == PF_REACHED) {
						if (++this->State == 1) {
							this->Finished = true;
							return;
						}
					} else if (moveResult == PF_UNREACHABLE) {
						unit.Wait = 30;
						this->Range++;
						break;
					} else {
						this->State = 2;
					}
				}
				return;
			}
		case 2: { // Leave the transporter
			// FIXME: show still animations ?
			if (LeaveTransporter(unit)) {
				this->Finished = true;
				return;
			}

			return;
		}
		default:
			return;
	}
}
