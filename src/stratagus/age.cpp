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
#include "config_operator.h"
#include "game/game.h"
#include "mod.h"
#include "player.h"
#include "time/calendar.h"
#include "unit/unit_type.h"
#include "upgrade/dependency.h"
#include "upgrade/upgrade_structs.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CAge *CAge::CurrentAge = nullptr;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CAge::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "image") {
		std::string file;
		Vec2i size(0, 0);
		
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			std::string value = property.Value;
			
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
			return true; //returns true, because false is only for if the property doesn't exist
		}
		
		if (size.x == 0) {
			fprintf(stderr, "Image has no width.\n");
			return true;
		}
		
		if (size.y == 0) {
			fprintf(stderr, "Image has no height.\n");
			return true;
		}
		
		this->G = CGraphic::New(file, size.x, size.y);
		this->G->Load();
		this->G->UseDisplayFormat();
	} else if (section->Tag == "predependencies") {
		this->Predependency = new CAndDependency;
		this->Predependency->ProcessConfigData(section);
	} else if (section->Tag == "dependencies") {
		this->Dependency = new CAndDependency;
		this->Dependency->ProcessConfigData(section);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the age
*/
void CAge::Initialize()
{
	this->Initialized = true;
	
	if (CAge::AreAllInitialized()) {
		std::sort(CAge::Instances.begin(), CAge::Instances.end(), [](CAge *a, CAge *b) {
			if (a->Priority != b->Priority) {
				return a->Priority > b->Priority;
			} else {
				return a->Ident < b->Ident;
			}
		});
		CAge::UpdateIndexes();
	}
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
		if (CPlayer::Players[p]->Age && (!best_age || CPlayer::Players[p]->Age->Priority > best_age->Priority)) {
			best_age = CPlayer::Players[p]->Age;
		}
	}
	
	if (best_age != CAge::CurrentAge) {
		CAge::SetCurrentAge(best_age);
	}
}

void CAge::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), [](CAge *age, const int priority){ age->Priority = priority; });
	ClassDB::bind_method(D_METHOD("get_priority"), &CAge::GetPriority);
	ClassDB::bind_method(D_METHOD("set_year_boost", "year_boost"), [](CAge *age, const int year_boost){ age->YearBoost = year_boost; });
	ClassDB::bind_method(D_METHOD("get_year_boost"), &CAge::GetYearBoost);
	
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "year_boost"), "set_year_boost", "get_year_boost");
}

/**
**	@brief	Set the current overall in-game age
**
**	@param	age_ident	The age's string identifier
*/
void SetCurrentAge(const std::string &age_ident)
{
	CAge *age = CAge::Get(age_ident);
	
	if (!age) {
		return;
	}
	
	CAge::CurrentAge = age;
}
