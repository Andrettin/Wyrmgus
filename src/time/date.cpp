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

#include "time/date.h"

#include "civilization.h"
#include "game.h"
#include "player.h"
#include "time/calendar.h"
#include "time/timeline.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

unsigned long long CDate::CurrentTotalHours = 0;
QCalendar CDate::calendar;

CDate CDate::FromString(const std::string &date_str)
{
	CDate date;
	
	std::vector<std::string> date_vector = SplitString(date_str, ".");
	
	wyrmgus::calendar *calendar = nullptr;
	size_t offset = 0;
	
	if (date_vector.size() >= 1 && !wyrmgus::string::is_number(date_vector[0])) {
		calendar = wyrmgus::calendar::try_get(date_vector[0]);
		if (calendar) {
			offset += 1;
		} else if (wyrmgus::timeline::try_get(date_vector[0]) == nullptr) { //is neither a calendar nor a timeline
			fprintf(stderr, "Calendar \"%s\" does not exist.\n", date_vector[0].c_str());
		}
	}
	
	if (date_vector.size() >= (1 + offset) && !wyrmgus::string::is_number(date_vector[0 + offset])) {
		wyrmgus::timeline *timeline = wyrmgus::timeline::get(date_vector[0 + offset]);
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
	this->Year = 0;
	this->Month = 1;
	this->Day = 1;
	this->Hour = DEFAULT_HOURS_PER_DAY / 2;
}

bool CDate::ContainsDate(const CDate &date) const
{
	return *this >= date;
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

void CDate::AddMonths(const int months)
{
	this->Month += months;
	
	if (this->Month > 0) {
		while (this->Month > CDate::months_per_year) {
			this->Month -= CDate::months_per_year;
			this->AddYears(1);
		}
	} else {
		while (this->Month <= 0) {
			this->Month += CDate::months_per_year;
			this->AddYears(-1);
		}
	}
}

void CDate::AddDays(const int days, const int day_multiplier)
{
	int current_day = this->Day;
	current_day += days * day_multiplier;
	
	if (current_day > 0) {
		while (current_day > CDate::days_per_year) {
			current_day -= CDate::days_per_year;
			this->AddYears(1);
		}
		
		while (current_day > CDate::calendar.daysInMonth(this->Month)) {
			current_day -= CDate::calendar.daysInMonth(this->Month);
			this->AddMonths(1);
		}
	} else {
		while (current_day <= (-CDate::days_per_year + 1)) {
			current_day += CDate::days_per_year;
			this->AddYears(-1);
		}
		
		while (current_day <= 0) {
			current_day += CDate::calendar.daysInMonth(this->Month);
			this->AddMonths(-1);
		}
	}

	this->Day = current_day;
}

void CDate::AddHours(const long long int hours, const int day_multiplier)
{
	this->AddDays(hours / CDate::hours_per_day, day_multiplier);
	
	this->Hour += hours % CDate::hours_per_day;
	
	if (this->Hour >= 0) {
		while (this->Hour >= CDate::hours_per_day) {
			this->Hour -= CDate::hours_per_day;
			this->AddDays(1, day_multiplier);
		}
	} else {
		while (this->Hour < 0) {
			this->Hour += CDate::hours_per_day;
			this->AddDays(-1, day_multiplier);
		}
	}
}

CDate CDate::ToCalendar(wyrmgus::calendar *current_calendar, wyrmgus::calendar *new_calendar) const
{
	if (current_calendar == new_calendar) {
		return *this;
	}

	const int old_year_offset = current_calendar ? current_calendar->get_year_offset() : 0;
	const int new_year_offset = new_calendar ? new_calendar->get_year_offset() : 0;
	
	CDate date;
	date.Year = this->Year + new_year_offset - old_year_offset;
	date.Month = this->Month;
	date.Day = this->Day;
	date.Hour = this->Hour;
	
	return date;
}

CDate CDate::ToBaseCalendar(wyrmgus::calendar *current_calendar) const
{
	CDate date;
	date.Year = this->Year - current_calendar->get_year_offset();
	date.Month = this->Month;
	date.Day = this->Day;
	date.Hour = this->Hour;
	
	return date;
}

std::string CDate::ToString() const
{
	std::string date_string;
	
	date_string += std::to_string(this->Year);
	date_string += "." + std::to_string(this->Month);
	date_string += "." + std::to_string(this->Day);
	date_string += "." + std::to_string(this->Hour);
	
	return date_string;
}

std::string CDate::ToDisplayString(const wyrmgus::calendar *calendar, const bool year_only) const
{
	std::string display_string;
	
	if (!year_only) {
		display_string += std::to_string(this->Day) + "." + std::to_string(this->Month) + ".";
	}
	
	display_string += std::to_string(abs(this->Year));
	
	display_string += " ";
	if (this->Year < 0) {
		if (calendar == nullptr || !calendar->get_negative_year_label().empty()) {
			display_string += calendar->get_negative_year_label();
		} else {
			display_string += CDate::default_negative_year_label;
		}
	} else {
		if (calendar == nullptr || !calendar->get_year_label().empty()) {
			display_string += calendar->get_year_label();
		} else {
			display_string += CDate::default_year_label;
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
int CDate::GetTotalDays() const
{
	int days = 0;
	
	days += (this->Year < 0 ? this->Year : this->Year - 1) * CDate::days_per_year;
	for (int i = 1; i <= this->Month; ++i) {
		days += CDate::calendar.daysInMonth(i);
	}
	days += this->Day - 1;
	
	return days;
}

unsigned long long CDate::GetTotalHours() const
{
	unsigned long long hours = this->Hour;
	
	unsigned long long days = 0;
	days += (static_cast<unsigned long long>(this->Year) + BaseCalendarYearOffsetForHours - 1) * CDate::days_per_year;
	for (int i = 1; i <= this->Month; ++i) {
		days += calendar.daysInMonth(i);
	}
	days += static_cast<unsigned long long>(this->Day) - 1;
	hours += days * CDate::hours_per_day;
	
	return hours;
}

/**
**	@brief	Set the current date for a particular calendar
**
**	@param	calendar_ident	The calendar's string identifier
**	@param	date_string		The date's string representation
*/
void SetCurrentDate(const std::string &date_string)
{
	wyrmgus::game::get()->set_current_date(wyrmgus::string::to_date(date_string));
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
