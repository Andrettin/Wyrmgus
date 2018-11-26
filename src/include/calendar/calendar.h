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
/**@name calendar.h - The calendar header file. */
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

#ifndef __CALENDAR_H__
#define __CALENDAR_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>
#include <vector>

#include "calendar/date.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCalendar;
class CConfigData;

class CDayOfTheWeek
{
public:
	CDayOfTheWeek() :
		ID(-1), Calendar(NULL)
	{
	}
	
	void ProcessConfigData(const CConfigData *config_data);

	std::string Ident;
	std::string Name;
	int ID;
	CCalendar *Calendar;
};

class CMonth
{
public:
	CMonth() :
		Days(0)
	{
	}
	
	void ProcessConfigData(const CConfigData *config_data);

	std::string Name;
	int Days;
};

class CCalendar
{
public:
	CCalendar() :
		Initialized(false), HoursPerDay(DefaultHoursPerDay), DaysPerYear(0), BaseDayOfTheWeek(NULL), CurrentDayOfTheWeek(-1)
	{
	}
	
	~CCalendar();
	
	static CCalendar *GetCalendar(const std::string &ident);
	static CCalendar *GetOrAddCalendar(const std::string &ident);
	static void ClearCalendars();
	static int GetTimeOfDay(const unsigned long long hours, const int hours_per_day);
	
	static std::vector<CCalendar *> Calendars;
	static std::map<std::string, CCalendar *> CalendarsByIdent;
	static CCalendar *BaseCalendar;
	
	void ProcessConfigData(const CConfigData *config_data);
private:
	CDayOfTheWeek *GetDayOfTheWeekByIdent(const std::string &ident);
	void AddChronologicalIntersection(CCalendar *intersecting_calendar, const CDate &date, const CDate &intersecting_date);
	void InheritChronologicalIntersectionsFromCalendar(CCalendar *intersecting_calendar);
public:
	std::pair<CDate, CDate> GetBestChronologicalIntersectionForDate(CCalendar *calendar, const CDate &date) const;
	
	std::string Ident;
	std::string Name;
	bool Initialized;
	int HoursPerDay;
	int DaysPerYear;
	std::string YearLabel;														/// label used for years (e.g. AD)
	std::string NegativeYearLabel;												/// label used for "negative" years (e.g. BC)
	CDayOfTheWeek *BaseDayOfTheWeek;											/// the day of the week for the first day of the year in the calendar
	CDate CurrentDate;															/// the current date in this calendar within a game
	int CurrentDayOfTheWeek;													/// the current day of the week in this calendar within a game
	std::vector<CDayOfTheWeek *> DaysOfTheWeek;									/// the days of the week in the calendar
	std::vector<CMonth *> Months;												/// the months in the calendar
private:
	std::map<std::string, CDayOfTheWeek *> DaysOfTheWeekByIdent;
	std::map<CCalendar *, std::map<CDate, CDate>> ChronologicalIntersections;	/// chronological intersection points between this calendar and other calendars
};

//@}

#endif // !__CALENDAR_H__
