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
/**@name date.h - The date header file. */
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

#ifndef __DATE_H__
#define __DATE_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CTimeline;

#define BaseCalendarYearOffsetForHours 10000 //essentially the Human Era

class CDate
{
public:
	CDate() :
		Year(0), Month(1), Day(1), Hour(DefaultHoursPerDay / 2), Timeline(NULL)
	{
	}
	
	static CDate FromString(std::string date_str);

	void Clear();
	bool ContainsDate(const CDate &date) const;	/// whether this date "contains" another (i.e. if it is subsequent to another, and in an appropriate timeline)
	void AddMonths(CCalendar *calendar, const int months);
	void AddDays(CCalendar *calendar, const int days);
	void AddHours(CCalendar *calendar, const long long int hours);
	CDate ToCalendar(CCalendar *current_calendar, CCalendar *new_calendar) const;
	CDate ToBaseCalendar(CCalendar *current_calendar) const;
	std::string ToDisplayString(const CCalendar *calendar) const;
	unsigned long long GetTotalHours(CCalendar *calendar) const;	/// gets the total amount of hours for the particular calendar in this date, counting from -10,000 in the base calendar
	
	int Year;
	char Month;
	char Day;
	char Hour;
	CTimeline *Timeline;
	
	bool operator <(const CDate& rhs) const {
		if (Year < rhs.Year) {
			return true;
        } else if (Year == rhs.Year) {
			if (Month < rhs.Month) {
				return true;
			} else if (Month == rhs.Month) {
				if (Day < rhs.Day) {
					return true;
				} else if (Day == rhs.Day) {
					return Hour < rhs.Hour;
				}
			}
		}

		return false;
	}
	
	bool operator <=(const CDate& rhs) const {
		if (Year < rhs.Year) {
			return true;
        } else if (Year == rhs.Year) {
			if (Month < rhs.Month) {
				return true;
			} else if (Month == rhs.Month) {
				if (Day < rhs.Day) {
					return true;
				} else if (Day == rhs.Day) {
					return Hour <= rhs.Hour;
				}
			}
		}

		return false;
	}
	
	bool operator >(const CDate& rhs) const {
		if (Year > rhs.Year) {
			return true;
        } else if (Year == rhs.Year) {
			if (Month > rhs.Month) {
				return true;
			} else if (Month == rhs.Month) {
				if (Day > rhs.Day) {
					return true;
				} else if (Day == rhs.Day) {
					return Hour > rhs.Hour;
				}
			}
		}

		return false;
	}
	
	bool operator >=(const CDate& rhs) const {
		if (Year > rhs.Year) {
			return true;
        } else if (Year == rhs.Year) {
			if (Month > rhs.Month) {
				return true;
			} else if (Month == rhs.Month) {
				if (Day > rhs.Day) {
					return true;
				} else if (Day == rhs.Day) {
					return Hour >= rhs.Hour;
				}
			}
		}

		return false;
	}
	
	bool operator ==(const CDate& rhs) const {
		return Year == rhs.Year && Month == rhs.Month && Day == rhs.Day && Hour == rhs.Hour;
	}
};

//@}

#endif // !__DATE_H__
