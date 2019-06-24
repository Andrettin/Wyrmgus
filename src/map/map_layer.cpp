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
/**@name map_layer.cpp - The map layer source file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#include "map/map_layer.h"

#include "map/map.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "sound/sound_server.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "wyrmgus.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Constructor
*/
CMapLayer::CMapLayer(const int index, const int width, const int height, CPlane *plane, CWorld *world, const int surface_layer)
	: Index(index), Width(width), Height(height), Plane(plane), World(world), SurfaceLayer(surface_layer)
{
	const int max_tile_index = this->Width * this->Height;

	this->Fields = new CMapField[max_tile_index];
}

/**
**	@brief	Destructor
*/
CMapLayer::~CMapLayer()
{
	if (this->Fields) {
		delete[] this->Fields;
	}
}

/**
**	@brief	Get the map field at a given location
**
**	@param	index	The index of the map field
**
**	@return	The map field
*/
CMapField *CMapLayer::Field(const unsigned int index) const
{
	return &this->Fields[index];
}

/**
**	@brief	Perform the map layer's per-hour loop
*/
void CMapLayer::DoPerHourLoop()
{
	this->DecrementRemainingSeasonHours();
	this->DecrementRemainingTimeOfDayHours();
}

/**
**	@brief	Regenerate forest tiles in the map layer
*/
void CMapLayer::RegenerateForest()
{
	for (size_t i = 0; i < this->DestroyedForestTiles.size();) {
		const Vec2i &pos = this->DestroyedForestTiles[i];
		CMapField &mf = *this->Field(pos);
		if (!mf.IsDestroyedForestTile()) { //the destroyed forest tile may have become invalid, e.g. because the terrain changed, or because of the regeneration itself; we keep the removal of elements centralized here so that we can loop through the tiles reliably
			this->DestroyedForestTiles.erase(this->DestroyedForestTiles.begin() + i);
		} else {
			this->RegenerateForestTile(pos);
			++i;
		}
	}
}

/**
**	@brief	Regenerate a forest tile
**
**	@param	pos		Map tile position
**
**	@return	True if the forest tile was regenerated, or false otherwise
*/
void CMapLayer::RegenerateForestTile(const Vec2i &pos)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, this->GetIndex()));
	
	CMapField &mf = *this->Field(pos);

	//  Increment each value of no wood.
	//  If grown up, place new wood.
	//  FIXME: a better looking result would be fine
	//    Allow general updates to any tiletype that regrows

	const uint16_t permanent_occupied_flag = (MapFieldWall | MapFieldUnpassable | MapFieldBuilding);
	const uint16_t occupied_flag = (permanent_occupied_flag | MapFieldLandUnit | MapFieldItem);
	
	if ((mf.GetFlags() & permanent_occupied_flag)) { //if the tree tile is permanently occupied by buildings and the like, reset the regeneration process
		mf.Value = 0;
		return;
	}

	if (mf.GetFlags() & occupied_flag) { // if the tree tile is temporarily occupied (e.g. by an item or unit), don't continue the regrowing process while the occupation occurs, but don't reset it either
		return;
	}
	
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	
	//Wyrmgus start
//	const Vec2i offset(0, -1);
//	CMapField &topMf = *(&mf - this->Info.MapWidth);

	for (int x_offset = -1; x_offset <= 1; x_offset+=2) { //increment by 2 to avoid instances where it is 0
		for (int y_offset = -1; y_offset <= 1; y_offset+=2) {
			const Vec2i verticalOffset(0, y_offset);
			CMapField &verticalMf = *this->Field(pos + verticalOffset);
			const Vec2i horizontalOffset(x_offset, 0);
			CMapField &horizontalMf = *this->Field(pos + horizontalOffset);
			const Vec2i diagonalOffset(x_offset, y_offset);
			CMapField &diagonalMf = *this->Field(pos + diagonalOffset);
			
			if (
				CMap::Map.Info.IsPointOnMap(pos + diagonalOffset, this->GetIndex())
				&& CMap::Map.Info.IsPointOnMap(pos + verticalOffset, this->GetIndex())
				&& CMap::Map.Info.IsPointOnMap(pos + horizontalOffset, this->GetIndex())
				&& ((verticalMf.IsDestroyedForestTile() && verticalMf.Value >= ForestRegeneration && !(verticalMf.GetFlags() & occupied_flag)) || verticalMf.ForestOnMap())
				&& ((diagonalMf.IsDestroyedForestTile() && diagonalMf.Value >= ForestRegeneration && !(diagonalMf.GetFlags() & occupied_flag)) || diagonalMf.ForestOnMap())
				&& ((horizontalMf.IsDestroyedForestTile() && horizontalMf.Value >= ForestRegeneration && !(horizontalMf.GetFlags() & occupied_flag)) || horizontalMf.ForestOnMap())
			) {
				DebugPrint("Real place wood\n");
				CMap::Map.SetOverlayTerrainDestroyed(pos + verticalOffset, false, this->GetIndex());
				CMap::Map.SetOverlayTerrainDestroyed(pos + diagonalOffset, false, this->GetIndex());
				CMap::Map.SetOverlayTerrainDestroyed(pos + horizontalOffset, false, this->GetIndex());
				CMap::Map.SetOverlayTerrainDestroyed(pos, false, this->GetIndex());
				
				return;
			}
		}
	}
}

/**
**	@brief	Get whether a map tile is a wall.
**
**	@param	pos		Map tile position.
**
**	@return	True if the tile is a wall, or false otherwise.
*/
bool CMapLayer::WallOnMap(const Vec2i &pos) const
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, this->GetIndex()));
	return this->Field(pos)->isAWall();
}

/**
**	@brief	Get whether a tile block has a tree tile
**
**	@param	pos		The tile block's top-left position
**	@param	size	The tile block's size
**
**	@return	True if the tile block borders has a tree tile, or false otherwise
*/
bool CMapLayer::TileBlockHasTree(const Vec2i &pos, const Vec2i &size) const
{
	Vec2i max_pos = pos + size - 1;
	for (int x = pos.x; x <= max_pos.x; ++x) {
		for (int y = pos.y; y <= max_pos.y; ++y) {
			const Vec2i tile_pos(x, y);
			
			if (!CMap::Map.Info.IsPointOnMap(tile_pos, this->GetIndex())) {
				continue;
			}
			
			const CMapField &mf = *this->Field(tile_pos);
			
			if (mf.ForestOnMap()) {
				return true;
			}
		}
	}
		
	return false;
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
void CMapLayer::SetTimeOfDay(const CScheduledTimeOfDay *time_of_day)
{
	if (this->TimeOfDay == time_of_day) {
		return;
	}
	
	const CScheduledTimeOfDay *old_time_of_day = this->TimeOfDay;
	this->TimeOfDay = time_of_day;
	
	if (GameRunning && this == UI.CurrentMapLayer) {
		Wyrmgus::GetInstance()->emit_signal("time_of_day_changed", old_time_of_day ? old_time_of_day->TimeOfDay : nullptr, this->TimeOfDay ? this->TimeOfDay->TimeOfDay : nullptr);
	}

	const bool is_day_changed = (this->TimeOfDay && this->TimeOfDay->TimeOfDay->IsDay()) != (old_time_of_day && old_time_of_day->TimeOfDay->IsDay());
	const bool is_night_changed = (this->TimeOfDay && this->TimeOfDay->TimeOfDay->IsNight()) != (old_time_of_day && old_time_of_day->TimeOfDay->IsNight());
	
	//update the sight of all units
	if (is_day_changed || is_night_changed) {
		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			CUnit *unit = *it;
			if (
				unit && unit->IsAlive() && unit->GetMapLayer() == this &&
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
const CTimeOfDay *CMapLayer::GetTimeOfDay() const
{
	if (this->TimeOfDay == nullptr) {
		return nullptr;
	}
	
	return this->TimeOfDay->TimeOfDay;
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

void CMapLayer::SetSeason(const CScheduledSeason *season)
{
	if (season == this->Season) {
		return;
	}
	
	CSeason *old_season = this->Season ? this->Season->Season : nullptr;
	CSeason *new_season = season ? season->Season : nullptr;
	
	this->Season = season;
	
	//update map layer tiles affected by the season change
	for (int x = 0; x < this->Width; x++) {
		for (int y = 0; y < this->Height; y++) {
			const CMapField &mf = *this->Field(x, y);
			
			//check if the tile's terrain graphics have changed due to the new season and if so, update the minimap
			if (
				(mf.playerInfo.SeenTerrain && mf.playerInfo.SeenTerrain->GetGraphics(old_season) != mf.playerInfo.SeenTerrain->GetGraphics(new_season))
				|| (mf.playerInfo.SeenOverlayTerrain && mf.playerInfo.SeenOverlayTerrain->GetGraphics(old_season) != mf.playerInfo.SeenOverlayTerrain->GetGraphics(new_season))
			) {
				UI.Minimap.UpdateXY(Vec2i(x, y), this->GetIndex());
			}
		}
	}
	
	//update units which may have had their variation become invalid due to the season change
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit *unit = *it;
		if (unit && unit->IsAlive() && unit->GetMapLayer() == this) {
			const UnitTypeVariation *variation = unit->GetVariation();
			if (variation && !unit->CheckSeasonForVariation(variation)) {
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
const CSeason *CMapLayer::GetSeason() const
{
	if (!this->Season) {
		return nullptr;
	}
	
	return this->Season->Season;
}

void CMapLayer::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_index"), &CMapLayer::GetIndex);
	ClassDB::bind_method(D_METHOD("get_tile_index", "tile_pos"), +[](const CMapLayer *map_layer, const Vector2 &tile_pos){ return map_layer->GetTileIndex(Vector2i(tile_pos)); });
}
