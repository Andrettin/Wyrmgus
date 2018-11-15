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
/**@name calendar.cpp - The calendar source file. */
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

#include "calendar.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CCalendar *> CCalendar::Calendars;
std::map<std::string, CCalendar *> CCalendar::CalendarsByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Get a calendar
*/
CCalendar *CCalendar::GetCalendar(std::string ident)
{
	if (CalendarsByIdent.find(ident) != CalendarsByIdent.end()) {
		return CalendarsByIdent.find(ident)->second;
	}
	
	return NULL;
}

CCalendar *CCalendar::GetOrAddCalendar(std::string ident)
{
	CCalendar *calendar = GetCalendar(ident);
	
	if (!calendar) {
		calendar = new CCalendar;
		calendar->Ident = ident;
		Calendars.push_back(calendar);
		CalendarsByIdent[ident] = calendar;
	}
	
	return calendar;
}

void CCalendar::ClearCalendars()
{
	for (size_t i = 0; i < Calendars.size(); ++i) {
		delete Calendars[i];
	}
	Calendars.clear();
}

void CCalendar::ProcessConfigData(CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid calendar property: \"%s\".\n", key.c_str());
		}
	}
	
	for (size_t i = 0; i < config_data->Children.size(); ++i) {
		CConfigData *child_config_data = config_data->Children[i];
		
		if (child_config_data->Tag == "year_difference") {
			CCalendar *calendar = NULL;
			int difference = 0;
			bool difference_changed = false;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "calendar") {
					value = FindAndReplaceString(value, "_", "-");
					calendar = CCalendar::GetCalendar(value);
				} else if (key == "difference") {
					difference = std::stoi(value);
					difference_changed = true;
				} else {
					fprintf(stderr, "Invalid year difference property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!calendar) {
				//could be base calendar
//				fprintf(stderr, "Year difference has no \"calendar\" property.\n");
//				continue;
			}
			
			if (!difference_changed) {
				fprintf(stderr, "Year difference has no \"difference\" property.\n");
				continue;
			}
			
			this->YearDifferences[calendar] = difference;
			
			if (calendar) {
				//get the other year differences from the other calendar as well
				for (std::map<CCalendar *, int>::const_iterator iterator = calendar->YearDifferences.begin(); iterator != calendar->YearDifferences.end(); ++iterator) {
					if (iterator->first != this && this->YearDifferences.find(iterator->first) == this->YearDifferences.end()) {
						this->YearDifferences[iterator->first] = difference + iterator->second;
					}
				}
			}
		} else {
			fprintf(stderr, "Invalid calendar property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

//@}
