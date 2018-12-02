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
//      (c) Copyright 2018 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "map/map_layer.h"

#include "map/terrain_type.h"
#include "map/tile.h"
#include "season.h"
#include "season_schedule.h"
#include "sound_server.h"
#include "time_of_day.h"
#include "time_of_day_schedule.h"
#include "unit.h"
#include "unit_manager.h"

#ifdef USE_OAML
#include <oaml.h>

extern oamlApi *oaml;
extern bool enableOAML;
#endif

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
**	@brief	Get the map field at a given location
**
**	@param	x	The x coordinate of the map field
**	@param	y	The y coordinate of the map field
**
**	@return	The map field
*/
CMapField *CMapLayer::Field(const int x, const int y) const
{
	return this->Field(x + y * this->Width);
}

/**
**	@brief	Get the map field at a given location
**
**	@param	pos	The coordinates of the map field
**
**	@return	The map field
*/
CMapField *CMapLayer::Field(const Vec2i &pos) const
{
	return this->Field(pos.x, pos.y);
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
	
#ifdef USE_OAML
	if (enableOAML && oaml && this == UI.CurrentMapLayer && this->GetTimeOfDay()) {
		// Time of day can change our main music loop, if the current playing track is set for this
		SetMusicCondition(OAML_CONDID_MAIN_LOOP, this->GetTimeOfDay()->ID);
	}
#endif

	const bool is_day_changed = (this->TimeOfDay && this->TimeOfDay->TimeOfDay->Day) != (old_time_of_day && old_time_of_day->TimeOfDay->Day);
	const bool is_night_changed = (this->TimeOfDay && this->TimeOfDay->TimeOfDay->Night) != (old_time_of_day && old_time_of_day->TimeOfDay->Night);
	
	//update the sight of all units
	if (is_day_changed || is_night_changed) {
		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			CUnit *unit = *it;
			if (
				unit && unit->IsAlive() && unit->MapLayer == this &&
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
CTimeOfDay *CMapLayer::GetTimeOfDay() const
{
	if (!this->TimeOfDay) {
		return nullptr;
	}
	
	return this->TimeOfDay->TimeOfDay;
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
				UI.Minimap.UpdateXY(Vec2i(x, y), this->ID);
			}
		}
	}
}

/**
**	@brief	Get the current season the map layer is in
**
**	@return	The map layer's current season
*/
CSeason *CMapLayer::GetSeason() const
{
	if (!this->Season) {
		return nullptr;
	}
	
	return this->Season->Season;
}

//@}
