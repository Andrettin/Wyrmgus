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

#include "sound_server.h"
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
**	@brief	Increment the current time of day
*/
void CMapLayer::IncrementTimeOfDay()
{
	int time_of_day = this->TimeOfDay + 1;
	if (time_of_day == MaxTimesOfDay) {
		time_of_day = 1;
	}
	this->SetTimeOfDay(time_of_day);
}

/**
**	@brief	Set the current time of day
**
**	@param	time_of_day	The time of day
*/
void CMapLayer::SetTimeOfDay(const int time_of_day)
{
	if (this->TimeOfDay == time_of_day) {
		return;
	}
	
	int old_time_of_day = this->TimeOfDay;
	this->TimeOfDay = time_of_day;
	
#ifdef USE_OAML
	if (enableOAML && oaml && this->ID == CurrentMapLayer) {
		// Time of day can change our main music loop, if the current playing track is set for this
		SetMusicCondition(OAML_CONDID_MAIN_LOOP, this->TimeOfDay);
	}
#endif

	bool is_day_changed = (this->TimeOfDay == MorningTimeOfDay || this->TimeOfDay == MiddayTimeOfDay || this->TimeOfDay == AfternoonTimeOfDay) != (old_time_of_day == MorningTimeOfDay || old_time_of_day == MiddayTimeOfDay || old_time_of_day == AfternoonTimeOfDay);
	bool is_night_changed = (this->TimeOfDay == FirstWatchTimeOfDay || this->TimeOfDay == MidnightTimeOfDay || this->TimeOfDay == SecondWatchTimeOfDay) != (old_time_of_day == FirstWatchTimeOfDay || old_time_of_day == MidnightTimeOfDay || old_time_of_day == SecondWatchTimeOfDay);
	
	//update the sight of all units
	if (is_day_changed || is_night_changed) {
		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			CUnit *unit = *it;
			if (
				unit && unit->IsAlive() && unit->MapLayer == this->ID &&
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
**	@brief	Get the quantity of in-game hours necessary for the passage of a time of day for this map layer
**
**	@return	The quantity of in-game hours for the passage of a time of day in this map layer
*/
unsigned CMapLayer::GetHoursPerTimeOfDay() const
{
	unsigned hours_per_time_of_day = this->HoursPerDay;
	hours_per_time_of_day /= MaxTimesOfDay - 1;
	return hours_per_time_of_day;
}

/**
**	@brief	Increment the current season
*/
void CMapLayer::IncrementSeason()
{
	this->Season++;
	if (this->Season == MaxSeasons) {
		this->Season = 1;
	}
}

/**
**	@brief	Get the quantity of in-game hours necessary for the passage of a season for this map layer
**
**	@return	The quantity of in-game hours for the passage of a season in this map layer
*/
unsigned CMapLayer::GetHoursPerSeason() const
{
	unsigned hours_per_season = this->DaysPerYear;
	hours_per_season *= this->HoursPerDay;
	hours_per_season /= DayMultiplier;
	hours_per_season /= MaxSeasons - 1;
	return hours_per_season;
}

/**
**	@brief	Get the current season the map layer is in
**
**	@return	The map layer's current season
*/
int CMapLayer::GetSeason() const
{
	return this->Season;
}

//@}
