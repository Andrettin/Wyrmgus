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
/**@name time_of_day.h - The time of day header file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "color.h"
#include "data_type.h"

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGraphic;

class CTimeOfDay : public CDataType
{
public:
	static CTimeOfDay *GetTimeOfDay(const std::string &ident, const bool should_find = true);
	static CTimeOfDay *GetOrAddTimeOfDay(const std::string &ident);
	static void ClearTimesOfDay();
	
	static std::vector<CTimeOfDay *> TimesOfDay;	/// Times of day
	static std::map<std::string, CTimeOfDay *> TimesOfDayByIdent;
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	bool HasColorModification() const;

	std::string Name;							/// Name of the time of day
	int ID = -1;								/// The ID of this time of day
	bool Dawn = false;							/// Whether this is a dawn time of day
	bool Day = false;							/// Whether this is a day time of day
	bool Dusk = false;							/// Whether this is a dusk time of day
	bool Night = false;							/// Whether this is a night time of day
	CColor ColorModification;					/// The color modification applied to graphics when the time of day is active
	CGraphic *G = nullptr;
};
