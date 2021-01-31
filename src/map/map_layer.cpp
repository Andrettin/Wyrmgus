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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "map/map_layer.h"

#include "database/defines.h"
#include "map/map.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "sound/music.h"
#include "sound/sound_server.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "util/exception_util.h"

CMapLayer::CMapLayer(const QSize &size) : size(size)
{
	if (size.width() > MaxMapWidth) {
		exception::throw_with_trace(std::runtime_error("Tried to create a map layer with width (" + std::to_string(size.width()) + ") greater than the maximum (" + std::to_string(MaxMapWidth) + ")."));
	}

	if (size.height() > MaxMapHeight) {
		exception::throw_with_trace(std::runtime_error("Tried to create a map layer with height (" + std::to_string(size.height()) + ") greater than the maximum (" + std::to_string(MaxMapHeight) + ")."));
	}

	const int max_tile_index = size.width() * size.height();

	try {
		this->Fields = std::make_unique<wyrmgus::tile[]>(max_tile_index);
	} catch (const std::bad_alloc &) {
		std::throw_with_nested(std::runtime_error("Failed to allocate map layer with a tile area of " + std::to_string(max_tile_index) + ", for " + std::to_string(max_tile_index * sizeof(wyrmgus::tile)) + " bytes in total."));
	}
}

CMapLayer::~CMapLayer()
{
}

/**
**	@brief	Get the map field at a given location
**
**	@param	index	The index of the map field
**
**	@return	The map field
*/
wyrmgus::tile *CMapLayer::Field(const unsigned int index) const
{
	return &this->Fields[index];
}

/**
**	@brief	Perform the map layer's per-cycle loop
*/
void CMapLayer::DoPerCycleLoop()
{
	if (GameCycle > 0) {
		//do tile animation
		if (GameCycle % (CYCLES_PER_SECOND / 4) == 0) { // same speed as color-cycling
			const int max_tile_index = this->get_width() * this->get_height();
			for (int i = 0; i < max_tile_index; ++i) {
				wyrmgus::tile &mf = *this->Field(i);
				
				if (mf.get_terrain() != nullptr && mf.get_terrain()->SolidAnimationFrames > 0) {
					mf.AnimationFrame += 1;
					if (mf.AnimationFrame >= mf.get_terrain()->SolidAnimationFrames) {
						mf.AnimationFrame = 0;
					}
				}
				
				if (mf.get_overlay_terrain() != nullptr && mf.get_overlay_terrain()->SolidAnimationFrames > 0) {
					mf.OverlayAnimationFrame += 1;
					if (mf.OverlayAnimationFrame >= mf.get_overlay_terrain()->SolidAnimationFrames) {
						mf.OverlayAnimationFrame = 0;
					}
				}
			}
		}
	}
}

/**
**	@brief	Perform the map layer's per-hour loop
*/
void CMapLayer::DoPerHourLoop()
{
	this->DecrementRemainingSeasonHours();
	this->DecrementRemainingTimeOfDayHours();
}

void CMapLayer::handle_destroyed_overlay_terrain()
{
	if (wyrmgus::defines::get()->get_destroyed_overlay_terrain_decay_threshold() == 0) {
		return;
	}

	for (size_t i = 0; i < this->destroyed_overlay_terrain_tiles.size();) {
		const QPoint &pos = this->destroyed_overlay_terrain_tiles[i];
		wyrmgus::tile &mf = *this->Field(pos);

		if (mf.get_overlay_terrain() == nullptr || !mf.OverlayTerrainDestroyed || (mf.get_flags() & MapFieldStumps)) {
			//the destroyed overlay terrain tile may have become invalid, e.g. because the terrain changed, or because of the handling of destroyed overlay tiles itself; we keep the removal of elements centralized here so that we can loop through the tiles reliably
			this->destroyed_overlay_terrain_tiles.erase(this->destroyed_overlay_terrain_tiles.begin() + i);
		} else {
			this->decay_destroyed_overlay_terrain_tile(pos);
			++i;
		}
	}
}

void CMapLayer::decay_destroyed_overlay_terrain_tile(const QPoint &pos)
{
	if (!CMap::Map.Info.IsPointOnMap(pos, this->ID)) {
		exception::throw_with_trace(std::runtime_error("Tried to decay a destroyed overlay terrain tile for an invalid tile position."));
	}

	wyrmgus::tile &mf = *this->Field(pos);

	mf.decrement_value();

	if (mf.get_value() > wyrmgus::defines::get()->get_destroyed_overlay_terrain_decay_threshold()) {
		return;
	}

	mf.set_value(wyrmgus::defines::get()->get_destroyed_overlay_terrain_decay_threshold());
	CMap::get()->RemoveTileOverlayTerrain(pos, this->ID);
}

void CMapLayer::regenerate_forests()
{
	if (wyrmgus::defines::get()->get_forest_regeneration_threshold() == 0) {
		return;
	}

	for (size_t i = 0; i < this->destroyed_tree_tiles.size();) {
		const QPoint &pos = this->destroyed_tree_tiles[i];
		wyrmgus::tile &mf = *this->Field(pos);
		if (!mf.is_destroyed_tree_tile()) { //the destroyed forest tile may have become invalid, e.g. because the terrain changed, or because of the regeneration itself; we keep the removal of elements centralized here so that we can loop through the tiles reliably
			this->destroyed_tree_tiles.erase(this->destroyed_tree_tiles.begin() + i);
		} else {
			this->regenerate_tree_tile(pos);
			++i;
		}
	}
}

void CMapLayer::regenerate_tree_tile(const QPoint &pos)
{
	if (!CMap::Map.Info.IsPointOnMap(pos, this->ID)) {
		exception::throw_with_trace(std::runtime_error("Tried to regenerate a tree tile for an invalid tile position."));
	}
	
	wyrmgus::tile &mf = *this->Field(pos);

	//  Increment each value of no wood.
	//  If grown up, place new wood.
	//  FIXME: a better looking result would be fine
	//    Allow general updates to any tiletype that regrows

	const unsigned long permanent_occupied_flag = (MapFieldWall | MapFieldUnpassable | MapFieldBuilding);
	const unsigned long occupied_flag = (permanent_occupied_flag | MapFieldLandUnit | MapFieldItem);
	
	if ((mf.Flags & permanent_occupied_flag)) { //if the tree tile is permanently occupied by buildings and the like, reset the regeneration process
		mf.set_value(0);
		return;
	}

	if (mf.Flags & occupied_flag) { // if the tree tile is temporarily occupied (e.g. by an item or unit), don't continue the regrowing process while the occupation occurs, but don't reset it either
		return;
	}
	
	mf.increment_value();

	const int forest_regeneration_threshold = wyrmgus::defines::get()->get_forest_regeneration_threshold();

	if (mf.get_value() < forest_regeneration_threshold) {
		return;
	}
	mf.set_value(forest_regeneration_threshold);
	
	//Wyrmgus start
//	const Vec2i offset(0, -1);
//	wyrmgus::tile &topMf = *(&mf - this->Info.MapWidth);

	for (int x_offset = -1; x_offset <= 1; x_offset+=2) { //increment by 2 to avoid instances where it is 0
		for (int y_offset = -1; y_offset <= 1; y_offset+=2) {
			const Vec2i verticalOffset(0, y_offset);
			wyrmgus::tile &verticalMf = *this->Field(pos + verticalOffset);
			const Vec2i horizontalOffset(x_offset, 0);
			wyrmgus::tile &horizontalMf = *this->Field(pos + horizontalOffset);
			const Vec2i diagonalOffset(x_offset, y_offset);
			wyrmgus::tile &diagonalMf = *this->Field(pos + diagonalOffset);
			
			if (
				CMap::Map.Info.IsPointOnMap(pos + diagonalOffset, this->ID)
				&& CMap::Map.Info.IsPointOnMap(pos + verticalOffset, this->ID)
				&& CMap::Map.Info.IsPointOnMap(pos + horizontalOffset, this->ID)
				&& ((verticalMf.is_destroyed_tree_tile() && verticalMf.get_value() >= forest_regeneration_threshold && !(verticalMf.Flags & occupied_flag)) || (verticalMf.get_flags() & MapFieldForest))
				&& ((diagonalMf.is_destroyed_tree_tile() && diagonalMf.get_value() >= forest_regeneration_threshold && !(diagonalMf.Flags & occupied_flag)) || (diagonalMf.get_flags() & MapFieldForest))
				&& ((horizontalMf.is_destroyed_tree_tile() && horizontalMf.get_value() >= forest_regeneration_threshold && !(horizontalMf.Flags & occupied_flag)) || (horizontalMf.get_flags() & MapFieldForest))
			) {
				DebugPrint("Real place wood\n");
				CMap::Map.SetOverlayTerrainDestroyed(pos + verticalOffset, false, this->ID);
				CMap::Map.SetOverlayTerrainDestroyed(pos + diagonalOffset, false, this->ID);
				CMap::Map.SetOverlayTerrainDestroyed(pos + horizontalOffset, false, this->ID);
				CMap::Map.SetOverlayTerrainDestroyed(pos, false, this->ID);
				
				return;
			}
		}
	}

	/*
	if (topMf.getGraphicTile() == this->Tileset->getRemovedTreeTile()
		&& topMf.get_value() >= forest_regeneration_threshold
		&& !(topMf.Flags & occupied_flag)) {
		DebugPrint("Real place wood\n");
		topMf.setTileIndex(*Map.Tileset, Map.Tileset->getTopOneTreeTile(), 0);
		topMf.setGraphicTile(Map.Tileset->getTopOneTreeTile());
		topMf.player_info->SeenTile = topMf.getGraphicTile();
		topMf.set_value(0);
		topMf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.get_minimap()->UpdateSeenXY(pos + offset);
		UI.get_minimap()->UpdateXY(pos + offset);
		
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getBottomOneTreeTile(), 0);
		mf.setGraphicTile(Map.Tileset->getBottomOneTreeTile());
		mf.player_info->SeenTile = mf.getGraphicTile();
		mf.set_value(0);
		mf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.get_minimap()->UpdateSeenXY(pos);
		UI.get_minimap()->UpdateXY(pos);
		
		if (mf.player_info->IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->player_info->IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(topMf);
		}
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	}
	*/
}

/**
**	@brief	Decrement the current time of day's remaining hours
*/
void CMapLayer::DecrementRemainingTimeOfDayHours()
{
	if (!this->TimeOfDaySchedule) {
		return;
	}
	
	this->RemainingTimeOfDayHours -= this->TimeOfDaySchedule->HourMultiplier;
	
	if (this->RemainingTimeOfDayHours <= 0) {
		this->IncrementTimeOfDay();
	}
}

/**
**	@brief	Increment the current time of day
*/
void CMapLayer::IncrementTimeOfDay()
{
	unsigned current_time_of_day_id = this->TimeOfDay->ID;
	current_time_of_day_id++;
	if (current_time_of_day_id >= this->TimeOfDaySchedule->ScheduledTimesOfDay.size()) {
		current_time_of_day_id = 0;
	}
	
	this->SetTimeOfDay(this->TimeOfDaySchedule->ScheduledTimesOfDay[current_time_of_day_id]);
	this->RemainingTimeOfDayHours += this->TimeOfDay->GetHours(this->GetSeason());
}

/**
**	@brief	Set the time of day corresponding to an amount of hours
**
**	@param	hours	The quantity of hours
*/
void CMapLayer::SetTimeOfDayByHours(const unsigned long long hours)
{
	if (!this->TimeOfDaySchedule) {
		return;
	}
	
	int remaining_hours = hours % this->TimeOfDaySchedule->TotalHours;
	this->SetTimeOfDay(this->TimeOfDaySchedule->ScheduledTimesOfDay.front());
	this->RemainingTimeOfDayHours = this->TimeOfDay->GetHours(this->GetSeason());
	this->RemainingTimeOfDayHours -= remaining_hours;
	
	while (this->RemainingTimeOfDayHours <= 0) {
		this->IncrementTimeOfDay();
	}
}

/**
**	@brief	Set the current time of day
**
**	@param	time_of_day	The time of day
*/
void CMapLayer::SetTimeOfDay(CScheduledTimeOfDay *time_of_day)
{
	if (this->TimeOfDay == time_of_day) {
		return;
	}
	
	CScheduledTimeOfDay *old_time_of_day = this->TimeOfDay;
	this->TimeOfDay = time_of_day;
	
	const bool is_day_changed = (this->TimeOfDay && this->TimeOfDay->TimeOfDay->is_day()) != (old_time_of_day && old_time_of_day->TimeOfDay->is_day());
	const bool is_night_changed = (this->TimeOfDay && this->TimeOfDay->TimeOfDay->is_night()) != (old_time_of_day && old_time_of_day->TimeOfDay->is_night());
	
	//update the sight of all units
	if (is_day_changed || is_night_changed) {
		for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
			if (
				unit->IsAlive() && unit->MapLayer == this &&
				(
					(is_day_changed && unit->Variable[DAYSIGHTRANGEBONUS_INDEX].Value != 0) // if has day sight bonus and is entering or exiting day
					|| (is_night_changed && unit->Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value != 0) // if has night sight bonus and is entering or exiting night
				)
			) {
				MapUnmarkUnitSight(*unit);
				UpdateUnitSightRange(*unit);
				MapMarkUnitSight(*unit);
			}
		}
	}
}

/**
**	@brief	Get the current time of day the map layer is in
**
**	@return	The map layer's current time of day
*/
wyrmgus::time_of_day *CMapLayer::GetTimeOfDay() const
{
	if (!this->TimeOfDay) {
		return nullptr;
	}
	
	return this->TimeOfDay->TimeOfDay;
}

wyrmgus::time_of_day *CMapLayer::get_tile_time_of_day(const QPoint &tile_pos) const
{
	const wyrmgus::tile *tile = this->Field(tile_pos);

	if (tile->Flags & MapFieldSpace) {
		return nullptr;
	}

	if (tile->Flags & MapFieldUnderground) {
		return wyrmgus::defines::get()->get_underground_time_of_day();
	}

	return this->GetTimeOfDay();
}

/**
**	@brief	Decrement the current season's remaining hours
*/
void CMapLayer::DecrementRemainingSeasonHours()
{
	if (!this->SeasonSchedule) {
		return;
	}
	
	this->RemainingSeasonHours -= this->SeasonSchedule->HourMultiplier;
	
	if (this->RemainingSeasonHours <= 0) {
		this->IncrementSeason();
	}
}

/**
**	@brief	Increment the current season
*/
void CMapLayer::IncrementSeason()
{
	unsigned current_season_id = this->Season->ID;
	current_season_id++;
	if (current_season_id >= this->SeasonSchedule->ScheduledSeasons.size()) {
		current_season_id = 0;
	}
	
	this->SetSeason(this->SeasonSchedule->ScheduledSeasons[current_season_id]);
	this->RemainingSeasonHours += this->Season->Hours;
}

/**
**	@brief	Set the season corresponding to an amount of hours
**
**	@param	hours	The quantity of hours
*/
void CMapLayer::SetSeasonByHours(const unsigned long long hours)
{
	if (!this->SeasonSchedule) {
		return;
	}
	
	int remaining_hours = hours % this->SeasonSchedule->TotalHours;
	this->SetSeason(this->SeasonSchedule->ScheduledSeasons.front());
	this->RemainingSeasonHours = this->Season->Hours;
	this->RemainingSeasonHours -= remaining_hours;
	
	while (this->RemainingSeasonHours <= 0) {
		this->IncrementSeason();
	}
}

void CMapLayer::SetSeason(CScheduledSeason *season)
{
	if (season == this->Season) {
		return;
	}
	
	wyrmgus::season *old_season = this->Season ? this->Season->Season : nullptr;
	wyrmgus::season *new_season = season ? season->Season : nullptr;
	
	this->Season = season;
	
	//update map layer tiles affected by the season change
	for (int x = 0; x < this->get_width(); x++) {
		for (int y = 0; y < this->get_height(); y++) {
			const wyrmgus::tile &mf = *this->Field(x, y);
			
			//check if the tile's terrain graphics have changed due to the new season and if so, update the minimap
			if (
				(mf.player_info->SeenTerrain && mf.player_info->SeenTerrain->get_graphics(old_season) != mf.player_info->SeenTerrain->get_graphics(new_season))
				|| (mf.player_info->SeenOverlayTerrain && mf.player_info->SeenOverlayTerrain->get_graphics(old_season) != mf.player_info->SeenOverlayTerrain->get_graphics(new_season))
			) {
				UI.get_minimap()->UpdateXY(Vec2i(x, y), this->ID);
			}
		}
	}
	
	//update units which may have had their variation become invalid due to the season change
	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		if (unit->IsAlive() && unit->MapLayer == this) {
			const wyrmgus::unit_type_variation *variation = unit->GetVariation();
			if (variation != nullptr && !unit->can_have_variation(variation)) {
				unit->ChooseVariation(); //choose a new variation, as the old one has become invalid due to the season change
			}
		}
	}
}

/**
**	@brief	Get the current season the map layer is in
**
**	@return	The map layer's current season
*/
wyrmgus::season *CMapLayer::GetSeason() const
{
	if (!this->Season) {
		return nullptr;
	}
	
	return this->Season->Season;
}
