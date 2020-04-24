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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "include/data_type.h"
#include "time/date.h"

namespace stratagus {
	class calendar;
}

class CDayOfTheWeek
{
public:
	void ProcessConfigData(const CConfigData *config_data);

	std::string Ident;
	std::string Name;
	int ID = -1;
	stratagus::calendar *Calendar = nullptr;
};

class CMonth
{
public:
	void ProcessConfigData(const CConfigData *config_data);

	std::string Name;
	int Days = 0;
};

namespace stratagus {

class calendar : public named_data_entry, public data_type<calendar>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(stratagus::calendar* base_calendar MEMBER base_calendar)
	Q_PROPERTY(int year_offset MEMBER year_offset READ get_year_offset)

public:
	static constexpr const char *class_identifier = "calendar";
	static constexpr const char *database_folder = "calendars";

	static void clear()
	{
		data_type::clear();

		calendar::default_calendar = nullptr;
	}

	static calendar *default_calendar;

	calendar(const std::string &identifier);
	~calendar();

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	virtual void check() const override;

private:
	CDayOfTheWeek *GetDayOfTheWeekByIdent(const std::string &ident);
	void AddChronologicalIntersection(calendar *intersecting_calendar, const CDate &date, const CDate &intersecting_date);
	void InheritChronologicalIntersectionsFromCalendar(calendar *intersecting_calendar);
public:
	std::pair<CDate, CDate> GetBestChronologicalIntersectionForDate(calendar *calendar, const CDate &date) const;

	bool is_any_base_calendar(calendar *calendar) const
	{
		if (this->base_calendar == nullptr) {
			return false;
		}

		if (this->base_calendar == calendar) {
			return true;
		}

		return this->base_calendar->is_any_base_calendar(calendar);
	}

	int get_year_offset() const
	{
		return this->year_offset;
	}
	
private:
	calendar *base_calendar = nullptr; //the base calendar, used to ultimately calculate the year offset to the Gregorian calendar from
	int year_offset = 0; //the offset from the Gregorian calendar, in years
public:
	int HoursPerDay = DEFAULT_HOURS_PER_DAY;
	int DaysPerYear = 0;
	std::string YearLabel;														/// label used for years (e.g. AD)
	std::string NegativeYearLabel;												/// label used for "negative" years (e.g. BC)
	CDayOfTheWeek *BaseDayOfTheWeek = nullptr;									/// the day of the week for the first day of the year in the calendar
	int CurrentDayOfTheWeek = -1;												/// the current day of the week in this calendar within a game
	std::vector<CDayOfTheWeek *> DaysOfTheWeek;									/// the days of the week in the calendar
	std::vector<CMonth *> Months;												/// the months in the calendar
private:
	std::map<std::string, CDayOfTheWeek *> DaysOfTheWeekByIdent;
	std::map<calendar *, std::map<CDate, CDate>> ChronologicalIntersections;	/// chronological intersection points between this calendar and other calendars
};

}