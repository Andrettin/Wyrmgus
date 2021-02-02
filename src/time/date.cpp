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
		if (timeline) {
			offset += 1;
		}
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

bool CDate::ContainsDate(const CDate &date) const
{
	return *this >= date;
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
**	@brief	Set the current date for a particular calendar
**
**	@param	calendar_ident	The calendar's string identifier
**	@param	date_string		The date's string representation
*/
void SetCurrentDate(const std::string &date_string)
{
	game::get()->set_current_date(string::to_date(date_string));
}

/**
**	@brief	Set the current total in-game hours
**
**	@param	hours	The amount of hours
*/
void SetCurrentTotalHours(const unsigned long long hours)
{
	game::get()->set_current_total_hours(hours);
}
