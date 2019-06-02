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

#ifndef __CALENDAR_H__
#define __CALENDAR_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "time/date.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCalendar;

class CDayOfTheWeek
{
public:
	void ProcessConfigData(const CConfigData *config_data);

	std::string Ident;
	std::string Name;
	int ID = -1;
	CCalendar *Calendar = nullptr;
};

class CMonth
{
public:
	void ProcessConfigData(const CConfigData *config_data);

	std::string Name;
	int Days = 0;
};

class CCalendar : public DataElement, public DataType<CCalendar>
{
	GDCLASS(CCalendar, DataElement)
	
public:
	~CCalendar();
	
	static constexpr const char *ClassIdentifier = "calendar";

	static void Clear();
	
	static CCalendar *BaseCalendar;
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	
private:
	CDayOfTheWeek *GetDayOfTheWeekByIdent(const std::string &ident);
	void AddChronologicalIntersection(CCalendar *intersecting_calendar, const CDate &date, const CDate &intersecting_date);
	void InheritChronologicalIntersectionsFromCalendar(CCalendar *intersecting_calendar);
public:
	std::pair<CDate, CDate> GetBestChronologicalIntersectionForDate(CCalendar *calendar, const CDate &date) const;
	
	int HoursPerDay = DEFAULT_HOURS_PER_DAY;
	int DaysPerYear = 0;
	std::string YearLabel;														/// label used for years (e.g. AD)
	std::string NegativeYearLabel;												/// label used for "negative" years (e.g. BC)
	CDayOfTheWeek *BaseDayOfTheWeek = nullptr;									/// the day of the week for the first day of the year in the calendar
	CDate CurrentDate;															/// the current date in this calendar within a game
	int CurrentDayOfTheWeek = -1;												/// the current day of the week in this calendar within a game
	std::vector<CDayOfTheWeek *> DaysOfTheWeek;									/// the days of the week in the calendar
	std::vector<CMonth *> Months;												/// the months in the calendar
private:
	std::map<std::string, CDayOfTheWeek *> DaysOfTheWeekByIdent;
	std::map<CCalendar *, std::map<CDate, CDate>> ChronologicalIntersections;	/// chronological intersection points between this calendar and other calendars

protected:
	static inline void _bind_methods() {}
};

#endif
