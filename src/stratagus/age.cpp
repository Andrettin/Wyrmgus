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
/**@name age.cpp - The age source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "age.h"

#include "config.h"
#include "game.h"
#include "mod.h"
#include "player.h"
#include "time/calendar.h"
#include "unit/unittype.h"
#include "upgrade/dependency.h"
#include "upgrade/upgrade_structs.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CAge *> CAge::Ages;
std::map<std::string, CAge *> CAge::AgesByIdent;
CAge *CAge::CurrentAge = nullptr;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get an age
**
**	@param	ident	The age's string identifier
**
**	@return	The age if found, or null otherwise
*/
CAge *CAge::GetAge(const std::string &ident, const bool should_find)
{
	std::map<std::string, CAge *>::const_iterator find_iterator = AgesByIdent.find(ident);
	
	if (find_iterator != AgesByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid age: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add an age
**
**	@param	ident	The age's string identifier
**
**	@return	The age if found, otherwise a new age is created and returned
*/
CAge *CAge::GetOrAddAge(const std::string &ident)
{
	CAge *age = GetAge(ident, false);
	
	if (!age) {
		age = new CAge;
		age->Ident = ident;
		Ages.push_back(age);
		AgesByIdent[ident] = age;
	}
	
	return age;
}

/**
**	@brief	Remove the existing ages
*/
void CAge::ClearAges()
{
	for (size_t i = 0; i < Ages.size(); ++i) {
		delete Ages[i];
	}
	Ages.clear();
}

/**
**	@brief	Destructor
*/
CAge::~CAge()
{
	if (this->G) {
		CGraphic::Free(this->G);
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CAge::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "priority") {
			this->Priority = std::stoi(value);
		} else if (key == "year_boost") {
			this->YearBoost = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid age property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "image") {
			std::string file;
			Vec2i size(0, 0);
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "file") {
					file = CMod::GetCurrentModPath() + value;
				} else if (key == "width") {
					size.x = std::stoi(value);
				} else if (key == "height") {
					size.y = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid image property: \"%s\".\n", key.c_str());
				}
			}
			
			if (file.empty()) {
				fprintf(stderr, "Image has no file.\n");
				continue;
			}
			
			if (size.x == 0) {
				fprintf(stderr, "Image has no width.\n");
				continue;
			}
			
			if (size.y == 0) {
				fprintf(stderr, "Image has no height.\n");
				continue;
			}
			
			this->G = CGraphic::New(file, size.x, size.y);
			this->G->Load();
			this->G->UseDisplayFormat();
		} else if (child_config_data->Tag == "predependencies") {
			this->Predependency = new CAndDependency;
			this->Predependency->ProcessConfigData(child_config_data);
		} else if (child_config_data->Tag == "dependencies") {
			this->Dependency = new CAndDependency;
			this->Dependency->ProcessConfigData(child_config_data);
		} else {
			fprintf(stderr, "Invalid age property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	std::sort(CAge::Ages.begin(), CAge::Ages.end(), [](CAge *a, CAge *b) {
		if (a->Priority != b->Priority) {
			return a->Priority > b->Priority;
		} else {
			return a->Ident < b->Ident;
		}
	});
}

/**
**	@brief	Set the current age
*/
void CAge::SetCurrentAge(CAge *age)
{
	if (age == CAge::CurrentAge) {
		return;
	}
	
	CAge::CurrentAge = age;
	
	if (GameCycle > 0 && !SaveGameLoading) {
		if (CAge::CurrentAge && CAge::CurrentAge->YearBoost > 0) {
			//boost the current date's year by a certain amount; this is done so that we can both have a slower passage of years in-game (for seasons and character lifetimes to be more sensible), while still not having overly old dates in later ages
			for (CCalendar *calendar : CCalendar::Calendars) {
				calendar->CurrentDate.AddHours(calendar, (long long) CAge::CurrentAge->YearBoost * DEFAULT_DAYS_PER_YEAR * DEFAULT_HOURS_PER_DAY);
			}
		}
	}
}

/**
**	@brief	Check which age fits the current overall situation best, out of the ages each player is in
*/
void CAge::CheckCurrentAge()
{
	CAge *best_age = CAge::CurrentAge;
	
	for (int p = 0; p < PlayerMax; ++p) {
		if (Players[p].Age && (!best_age || Players[p].Age->Priority > best_age->Priority)) {
			best_age = Players[p].Age;
		}
	}
	
	if (best_age != CAge::CurrentAge) {
		CAge::SetCurrentAge(best_age);
	}
}

/**
**	@brief	Set the current overall in-game age
**
**	@param	age_ident	The age's string identifier
*/
void SetCurrentAge(const std::string &age_ident)
{
	CAge *age = CAge::GetAge(age_ident);
	
	if (!age) {
		return;
	}
	
	CAge::CurrentAge = age;
}
