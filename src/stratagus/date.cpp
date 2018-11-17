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

#include "date.h"

#include "calendar.h"
#include "timeline.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CDate CDate::FromString(std::string date_str)
{
	CDate date;
	
	std::vector<std::string> date_vector = SplitString(date_str, ".");
	
	CCalendar *calendar = NULL;
	size_t offset = 0;
	
	if (date_vector.size() >= 1 && !IsStringNumber(date_vector[0])) {
		calendar = CCalendar::GetCalendar(date_vector[0]);
		if (calendar) {
			offset += 1;
		} else if (!CTimeline::GetTimeline(date_vector[0])) { //is neither a calendar nor a timeline
			fprintf(stderr, "Calendar \"%s\" does not exist.\n", date_vector[0].c_str());
		}
	}
	
	if (date_vector.size() >= (1 + offset) && !IsStringNumber(date_vector[0 + offset])) {
		CTimeline *timeline = CTimeline::GetTimeline(date_vector[0 + offset]);
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
	
	if (calendar) {
		date = date.ToBaseCalendar(calendar);
	}
	
	return date;
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

CDate CDate::ToBaseCalendar(const CCalendar *current_calendar) const
{
	CDate date;
	date.Timeline = this->Timeline;
	date.Year = this->Year;
	date.Month = this->Month;
	date.Day = this->Day;
	
	if (CCalendar::BaseCalendar) {
		if (current_calendar->YearDifferences.find(CCalendar::BaseCalendar) != current_calendar->YearDifferences.end()) {
			date.Year += current_calendar->YearDifferences.find(CCalendar::BaseCalendar)->second;
		} else {
			fprintf(stderr, "Dates in calendar \"%s\" cannot be converted to the base calendar.\n", current_calendar->Ident.c_str());
		}
	} else {
		fprintf(stderr, "No base calendar has been defined.\n", current_calendar->Ident.c_str());
	}
	
	return date;
}

std::string CDate::ToDisplayString(const CCalendar *calendar) const
{
	std::string display_string;
	
	display_string += std::to_string((long long) abs(this->Year)) + "." + std::to_string((long long) this->Month) + "." + std::to_string((long long) this->Day);
	
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

//@}
