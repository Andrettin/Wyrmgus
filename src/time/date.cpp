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
/**@name date.cpp - The date source file. */
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

#include "time/date.h"

#include "time/calendar.h"
#include "time/timeline.h"
#include "civilization.h"
#include "player.h"
#include "quest.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

unsigned long long CDate::CurrentTotalHours = 0;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CDate CDate::FromString(const std::string &date_str)
{
	CDate date;
	
	std::vector<std::string> date_vector = SplitString(date_str, ".");
	
	CCalendar *calendar = nullptr;
	size_t offset = 0;
	
	if (date_vector.size() >= 1 && !IsStringNumber(date_vector[0])) {
		calendar = CCalendar::GetCalendar(date_vector[0]);
		if (calendar) {
			offset += 1;
		} else if (!CTimeline::Get(date_vector[0])) { //is neither a calendar nor a timeline
			fprintf(stderr, "Calendar \"%s\" does not exist.\n", date_vector[0].c_str());
		}
	}
	
	if (date_vector.size() >= (1 + offset) && !IsStringNumber(date_vector[0 + offset])) {
		CTimeline *timeline = CTimeline::Get(date_vector[0 + offset]);
		if (timeline) {
			date.Timeline = timeline;
		} else {
			fprintf(stderr, "Timeline \"%s\" does not exist.\n", date_vector[0 + offset].c_str());
		}
		offset += 1;
	}
	
	if (date_vector.size() >= (1 + offset)) {
		date.Year = std::stoi(date_vector[0 + offset]);
	}
	
	if (date_vector.size() >= (2 + offset)) {
		date.Month = std::stoi(date_vector[1 + offset]);
	}
	
	if (date_vector.size() >= (3 + offset)) {
		date.Day = std::stoi(date_vector[2 + offset]);
	}
	
	if (date_vector.size() >= (4 + offset)) {
		date.Hour = std::stoi(date_vector[3 + offset]);
	}
	
	if (calendar) {
		date = date.ToBaseCalendar(calendar);
	}
	
	return date;
}

void CDate::Clear()
{
	Year = 0;
	Month = 1;
	Day = 1;
	Hour = DEFAULT_HOURS_PER_DAY / 2;
	Timeline = nullptr;
}

bool CDate::ContainsDate(const CDate &date) const
{
	if (this->Timeline == date.Timeline) {
		return *this >= date;
	}
	
	if (this->Timeline) {
		return this->Timeline->PointOfDivergence.ContainsDate(date);
	}
	
	return false;
}

void CDate::AddYears(const int years)
{
	this->Year += years;
	
	if (this->Year == 0) {
		if (years < 0) {
			this->Year = -1;
		} else {
			this->Year = 1;
		}
	}
}

void CDate::AddMonths(const CCalendar *calendar, const int months)
{
	this->Month += months;
	
	if (this->Month > 0) {
		while (this->Month > (int) calendar->Months.size()) {
			this->Month -= static_cast<char>(calendar->Months.size());
			this->AddYears(1);
		}
	} else {
		while (this->Month <= 0) {
			this->Month += static_cast<char>(calendar->Months.size());
			this->AddYears(-1);
		}
	}
}

void CDate::AddDays(const CCalendar *calendar, const int days, const int day_multiplier)
{
	int current_day = this->Day;
	current_day += days * day_multiplier;
	
	if (current_day > 0) {
		while (current_day > calendar->DaysPerYear) {
			current_day -= calendar->DaysPerYear;
			this->AddYears(1);
		}
		
		while (current_day > calendar->Months[this->Month - 1]->Days) {
			current_day -= calendar->Months[this->Month - 1]->Days;
			this->AddMonths(calendar, 1);
		}
	} else {
		while (current_day <= (-calendar->DaysPerYear + 1)) {
			current_day += calendar->DaysPerYear;
			this->AddYears(-1);
		}
		
		while (current_day <= 0) {
			current_day += calendar->Months[this->Month - 1]->Days;
			this->AddMonths(calendar, -1);
		}
	}

	this->Day = current_day;
}

void CDate::AddHours(const CCalendar *calendar, const long long int hours, const int day_multiplier)
{
	this->AddDays(calendar, hours / calendar->HoursPerDay, day_multiplier);
	
	this->Hour += hours % calendar->HoursPerDay;
	
	if (this->Hour >= 0) {
		while (this->Hour >= calendar->HoursPerDay) {
			this->Hour -= calendar->HoursPerDay;
			this->AddDays(calendar, 1, day_multiplier);
		}
	} else {
		while (this->Hour < 0) {
			this->Hour += calendar->HoursPerDay;
			this->AddDays(calendar, -1, day_multiplier);
		}
	}
}

CDate CDate::ToCalendar(CCalendar *current_calendar, CCalendar *new_calendar) const
{
	if (current_calendar == new_calendar) {
		return *this;
	}
	
	CDate date;
	date.Timeline = this->Timeline;
	date.Year = this->Year;
	date.Month = this->Month;
	date.Day = this->Day;
	date.Hour = this->Hour;
	
	std::pair<CDate, CDate> chronological_intersection = current_calendar->GetBestChronologicalIntersectionForDate(new_calendar, *this);
	
	if (chronological_intersection.first.Year != 0) { //whether the chronological intersection returned is valid
		if (current_calendar->DaysPerYear == new_calendar->DaysPerYear) { //if the quantity of days per year is the same in both calendars, then we can just use the year difference in the intersection to get the resulting year for this date in the new calendar
			date.Year += chronological_intersection.second.Year - chronological_intersection.first.Year;
		} else if (current_calendar->HoursPerDay == new_calendar->HoursPerDay) { //if the quantity of days per years is different, but the hours in a day are the same, add the year difference in days
			date.AddDays(new_calendar, chronological_intersection.second.Year * new_calendar->DaysPerYear - chronological_intersection.first.Year * current_calendar->DaysPerYear);
		} else {
			date.AddHours(new_calendar, (long long int) chronological_intersection.second.Year * new_calendar->DaysPerYear * new_calendar->HoursPerDay - (long long int) chronological_intersection.first.Year * current_calendar->DaysPerYear * current_calendar->HoursPerDay);
		}
	} else {
		fprintf(stderr, "Dates in calendar \"%s\" cannot be converted to calendar \"%s\", as no chronological intersections are present.\n", current_calendar->Ident.c_str(), new_calendar->Ident.c_str());
	}
	
	return date;
}

CDate CDate::ToBaseCalendar(CCalendar *current_calendar) const
{
	if (!CCalendar::BaseCalendar) {
		fprintf(stderr, "No base calendar has been defined.\n");
		return *this;
	}
	
	return this->ToCalendar(current_calendar, CCalendar::BaseCalendar);
}

std::string CDate::ToString(const CCalendar *calendar) const
{
	std::string date_string;
	
	date_string += std::to_string((long long) this->Year);
	date_string += "." + std::to_string((long long) this->Month);
	date_string += "." + std::to_string((long long) this->Day);
	date_string += "." + std::to_string((long long) this->Hour);
	
	return date_string;
}

std::string CDate::ToDisplayString(const CCalendar *calendar, const bool year_only) const
{
	std::string display_string;
	
	if (!year_only) {
		display_string += std::to_string((long long) this->Day) + "." + std::to_string((long long) this->Month) + ".";
	}
	
	display_string += std::to_string((long long) abs(this->Year));
	
	if (!calendar) {
		fprintf(stderr, "Calendar does not exist.\n");
		return display_string;
	}
	
	if (this->Year < 0) {
		if (!calendar->NegativeYearLabel.empty()) {
			display_string += " " + calendar->NegativeYearLabel;
		} else {
			fprintf(stderr, "Calendar \"%s\" has no negative year label.\n", calendar->Ident.c_str());
		}
	} else {
		if (!calendar->YearLabel.empty()) {
			display_string += " " + calendar->YearLabel;
		} else {
			fprintf(stderr, "Calendar \"%s\" has no year label.\n", calendar->Ident.c_str());
		}
	}
	
	return display_string;
}

/**
**	@brief	Get the total amount of days counting from the date 1.1.1 of the calendar this date is presumed to use
**
**	@param	calendar	The calendar
**
**	@return	The amount of days
*/
int CDate::GetTotalDays(const CCalendar *calendar) const
{
	int days = 0;
	
	days += (this->Year < 0 ? this->Year : this->Year - 1) * calendar->DaysPerYear;
	for (int i = 0; i < (this->Month - 1); ++i) {
		days += calendar->Months[i]->Days;
	}
	days += this->Day - 1;
	
	return days;
}

unsigned long long CDate::GetTotalHours(CCalendar *calendar) const
{
	CDate date(this->ToBaseCalendar(calendar));
	
	unsigned long long hours = date.Hour;
	
	unsigned long long days = 0;
	days += (date.Year - 1 + BASE_CALENDAR_YEAR_OFFSET_FOR_HOURS) * calendar->DaysPerYear;
	for (int i = 0; i < (date.Month - 1); ++i) {
		days += calendar->Months[i]->Days;
	}
	days += date.Day - 1;
	hours += days * calendar->HoursPerDay;
	
	return hours;
}

/**
**	@brief	Get the day of the week for this date in a calendar (this date is presumed to already be in the calendar)
**
**	@param	calendar	The calendar
**
**	@return	The ID of the day of the week
*/
int CDate::GetDayOfTheWeek(const CCalendar *calendar) const
{
	if (!calendar->DaysOfTheWeek.empty() && calendar->BaseDayOfTheWeek) {
		int day_of_the_week = calendar->BaseDayOfTheWeek->ID;
		
		day_of_the_week += this->GetTotalDays(calendar) % calendar->DaysOfTheWeek.size();
		if (day_of_the_week < 0) {
			day_of_the_week += calendar->DaysOfTheWeek.size();
		}
		
		return day_of_the_week;
	}
	
	return -1;
}

/**
**	@brief	Set the current date for a particular calendar
**
**	@param	calendar_ident	The calendar's string identifier
**	@param	date_string		The date's string representation
*/
void SetCurrentDate(const std::string &calendar_ident, const std::string &date_string)
{
	CCalendar *calendar = CCalendar::GetCalendar(calendar_ident);
	
	if (!calendar) {
		return;
	}
	
	calendar->CurrentDate = CDate::FromString(date_string);
}

/**
**	@brief	Set the current day of the week for a particular calendar
**
**	@param	calendar_ident	The calendar's string identifier
**	@param	day_of_the_week	The day of the week's ID
*/
void SetCurrentDayOfTheWeek(const std::string &calendar_ident, const int day_of_the_week)
{
	CCalendar *calendar = CCalendar::GetCalendar(calendar_ident);
	
	if (!calendar || calendar->DaysOfTheWeek.empty() || !calendar->BaseDayOfTheWeek) {
		return;
	}
	
	calendar->CurrentDayOfTheWeek = day_of_the_week;
}

/**
**	@brief	Set the current total in-game hours
**
**	@param	hours	The amount of hours
*/
void SetCurrentTotalHours(const unsigned long long hours)
{
	CDate::CurrentTotalHours = hours;
}
