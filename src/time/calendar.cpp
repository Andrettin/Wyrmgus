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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "time/calendar.h"

#include "config.h"
#include "util/string_util.h"

namespace stratagus {

calendar *calendar::default_calendar = nullptr;

}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDayOfTheWeek::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid day of the week property: \"%s\".\n", key.c_str());
		}
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CMonth::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "days") {
			this->Days = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid month property: \"%s\".\n", key.c_str());
		}
	}
}

namespace stratagus {

calendar::calendar(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
{
}

calendar::~calendar()
{
	for (size_t i = 0; i < DaysOfTheWeek.size(); ++i) {
		delete DaysOfTheWeek[i];
	}
	
	for (size_t i = 0; i < Months.size(); ++i) {
		delete Months[i];
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void calendar::ProcessConfigData(const CConfigData *config_data)
{
	std::string base_day_of_the_week;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "base_calendar") {
			bool is_base_calendar = string::to_bool(value);
			if (is_base_calendar) {
				calendar::default_calendar = this;
			}
		} else if (key == "year_label") {
			this->YearLabel = value;
		} else if (key == "negative_year_label") {
			this->NegativeYearLabel = value;
		} else if (key == "base_day_of_the_week") {
			value = FindAndReplaceString(value, "_", "-");
			base_day_of_the_week = value;
		} else {
			fprintf(stderr, "Invalid calendar property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "day_of_the_week") {
			CDayOfTheWeek *day_of_the_week = new CDayOfTheWeek;
			day_of_the_week->ID = this->DaysOfTheWeek.size();
			this->DaysOfTheWeek.push_back(day_of_the_week);
			day_of_the_week->Calendar = this;
			day_of_the_week->Ident = child_config_data->Ident;
			this->DaysOfTheWeekByIdent[day_of_the_week->Ident] = day_of_the_week;
			day_of_the_week->ProcessConfigData(child_config_data);
		} else if (child_config_data->Tag == "month") {
			CMonth *month = new CMonth;
			month->ProcessConfigData(child_config_data);
			this->Months.push_back(month);
			this->DaysPerYear += month->Days;
		} else if (child_config_data->Tag == "chronological_intersection") {
			calendar *calendar = nullptr;
			CDate date;
			CDate intersecting_date;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "calendar") {
					calendar = calendar::get(value);
				} else if (key == "date") {
					date = CDate::FromString(value);
				} else if (key == "intersecting_date") {
					intersecting_date = CDate::FromString(value);
				} else {
					fprintf(stderr, "Invalid year difference property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!calendar) {
				fprintf(stderr, "Chronological intersection has no \"calendar\" property.\n");
				continue;
			}
			
			if (date.Year == 0) {
				fprintf(stderr, "Chronological intersection has no \"date\" property.\n");
				continue;
			}
			
			if (intersecting_date.Year == 0) {
				fprintf(stderr, "Chronological intersection has no \"intersecting_date\" property.\n");
				continue;
			}
			
			this->AddChronologicalIntersection(calendar, date, intersecting_date);
		} else {
			fprintf(stderr, "Invalid calendar property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}

	if (!base_day_of_the_week.empty()) {
		this->BaseDayOfTheWeek = this->GetDayOfTheWeekByIdent(base_day_of_the_week);
	}

	this->initialize();
}

void calendar::initialize()
{
	if (this->base_calendar != nullptr) {
		if (this->base_calendar == this) {
			throw std::runtime_error("The base calendar of \"" + this->get_identifier() + "\" is itself.");
		}

		if (this->base_calendar->is_any_base_calendar(this)) {
			throw std::runtime_error("The base calendar of \"" + this->get_identifier() + "\" is \"" + this->base_calendar->get_identifier() + "\", which creates a circular dependency.");
		}

		if (!this->base_calendar->is_initialized()) {
			this->base_calendar->initialize();
		}

		//calculate the year offset to the Gregorian calendar based on the offset to the base calendar
		this->year_offset += this->base_calendar->get_year_offset();
		this->base_calendar = nullptr;
	}

	//inherit the intersection points from the intersecting calendar that it has with third calendars
	for (std::map<calendar *, std::map<CDate, CDate>>::iterator iterator = this->ChronologicalIntersections.begin(); iterator != this->ChronologicalIntersections.end(); ++iterator) {
		calendar *intersecting_calendar = iterator->first;

		this->InheritChronologicalIntersectionsFromCalendar(intersecting_calendar);
	}

	data_entry::initialize();
}

void calendar::check() const
{
	if (this->Months.empty()) {
		throw std::runtime_error("No months have been defined for calendar \"" + this->get_identifier() + "\".");
	}
}

/**
**	@brief	Get a day of the week
**
**	@param	ident	The day of the week's string identifier
**
**	@return	A pointer to the day of the week if found, null otherwise
*/
CDayOfTheWeek *calendar::GetDayOfTheWeekByIdent(const std::string &ident)
{
	if (this->DaysOfTheWeekByIdent.find(ident) != this->DaysOfTheWeekByIdent.end()) {
		return this->DaysOfTheWeekByIdent.find(ident)->second;
	}
	
	return nullptr;
}

/**
**	@brief	Add a chronological intersection between this calendar and another one
**
**	@param	intersecting_calendar	A pointer to the calendar that is being intersected with
**	@param	date					The date in this calendar for the intersection
**	@param	intersecting_date		The date in the other calendar for the intersection
*/
void calendar::AddChronologicalIntersection(calendar *intersecting_calendar, const CDate &date, const CDate &intersecting_date)
{
	if (!intersecting_calendar) {
		return;
	}
	
	if (this->ChronologicalIntersections[intersecting_calendar].find(date) != this->ChronologicalIntersections[intersecting_calendar].end()) {
		return; //already defined
	}
	
	this->ChronologicalIntersections[intersecting_calendar][date] = intersecting_date;
	
	intersecting_calendar->AddChronologicalIntersection(this, intersecting_date, date);
	
	InheritChronologicalIntersectionsFromCalendar(intersecting_calendar);
}

/**
**	@brief	Inherit the chronological intersections with third calendars from another calendar
**
**	@param	intersecting_calendar	A pointer to the calendar from which the intersections are being inherited
*/
void calendar::InheritChronologicalIntersectionsFromCalendar(calendar *intersecting_calendar)
{
	if (!intersecting_calendar || !intersecting_calendar->is_initialized()) {
		return;
	}
	
	for (std::map<calendar *, std::map<CDate, CDate>>::iterator iterator = intersecting_calendar->ChronologicalIntersections.begin(); iterator != intersecting_calendar->ChronologicalIntersections.end(); ++iterator) {
		calendar *third_calendar = iterator->first;
		
		if (third_calendar == this || !third_calendar->is_initialized()) {
			continue;
		}
		
		for (std::map<CDate, CDate>::iterator sub_iterator = iterator->second.begin(); sub_iterator != iterator->second.end(); ++sub_iterator) {
			this->AddChronologicalIntersection(third_calendar, sub_iterator->first.ToCalendar(intersecting_calendar, this), sub_iterator->second);
		}
	}
}

/**
**	@brief	Get the closest chronological intersection for a given date
**
**	@param	calendar	A pointer to the calendar that is being intersected with
**	@param	date		A given date in this calendar
**
**	@return	The closest chronological intersection with the other calendar for the given date
*/
std::pair<CDate, CDate> calendar::GetBestChronologicalIntersectionForDate(calendar *calendar, const CDate &date) const
{
	std::pair<CDate, CDate> chronological_intersection(*(new CDate), *(new CDate));
	
	if (this->ChronologicalIntersections.find(calendar) == this->ChronologicalIntersections.end()) {
		return chronological_intersection;
	}
	
	const std::map<CDate, CDate> &chronological_intersections_submap = this->ChronologicalIntersections.find(calendar)->second;
	
	std::map<CDate, CDate>::const_iterator iterator = chronological_intersections_submap.lower_bound(date); //get the lower bound for the chronological intersection submap
	
	if (iterator == chronological_intersections_submap.end()) { //no chronological intersections found for the lower bound
		if (!chronological_intersections_submap.empty()) {
			iterator--; //get the element just before the end
			chronological_intersection.first = iterator->first; //date in the given calendar
			chronological_intersection.second = iterator->second; //date in the calendar we want to convert to
		}
		return chronological_intersection;
	} else {
		chronological_intersection.first = iterator->first; //date in the given calendar
		chronological_intersection.second = iterator->second; //date in the calendar we want to convert to
		
		if (iterator->first == date) { //exact match, just return this intersection
			return chronological_intersection;
		}
		
		//see whether the next element has a smaller year difference
		int first_year_difference = abs(date.Year - iterator->first.Year);
		
		iterator++;
		
		if (iterator != chronological_intersections_submap.end()) {
			int second_year_difference = abs(date.Year - iterator->first.Year);
			
			if (second_year_difference < first_year_difference) {
				chronological_intersection.first = iterator->first; //date in the given calendar
				chronological_intersection.second = iterator->second; //date in the calendar we want to convert to
			}
		}
	}
	
	return chronological_intersection;
}

}