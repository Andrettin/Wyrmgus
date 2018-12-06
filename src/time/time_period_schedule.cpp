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
/**@name time_period_schedule.cpp - The time period schedule source file. */
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

#include "time/time_period_schedule.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Calculate the hour multiplier used to for the passage of in-game hours relating to this schedule
*/
void CTimePeriodSchedule::CalculateHourMultiplier()
{
	int multiplier = 1;
	
	int standard_days = this->TotalHours / DefaultHoursPerDay;
	if (standard_days > DefaultDaysPerWeek) {
		multiplier += DefaultDayMultiplierForYear;
		multiplier += (standard_days * 100 / DefaultDaysPerYear - 100) * DefaultDayMultiplierForYear / DayMultiplierDivider / 100; //this makes duration increases effectively level off after the default number of days per year; at the same time, this formula also serves to calculate the hour multiplier for duration lengths greater than a week and smaller than a year
	}
	
	this->HourMultiplier = multiplier;
}

//@}
