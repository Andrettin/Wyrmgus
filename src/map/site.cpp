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
/**@name site.cpp - The site source file. */
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

#include "map/site.h"

#include "civilization.h"
#include "config.h"
#include "faction.h"
#include "map/map_template.h"
#include "unit/unit_type.h"
#include "world/province.h" //for regions

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CSite::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "name") {
		this->Name = value;
	} else if (key == "major") {
		this->Major = StringToBool(value);
	} else if (key == "position_x") {
		this->Position.x = std::stoi(value);
	} else if (key == "position_y") {
		this->Position.y = std::stoi(value);
	} else if (key == "map_template") {
		value = FindAndReplaceString(value, "_", "-");
		this->MapTemplate = CMapTemplate::Get(value);
	} else if (key == "core") {
		value = FindAndReplaceString(value, "_", "-");
		
		CFaction *faction = CFaction::Get(value);
		if (faction != nullptr) {
			this->Cores.push_back(faction);
			faction->Cores.push_back(this);
			faction->Sites.push_back(this);
			if (faction->Civilization) {
				faction->Civilization->Sites.push_back(this);
			}
		}
	} else if (key == "region") {
		value = FindAndReplaceString(value, "_", "-");
		
		CRegion *region = GetRegion(value);
		if (region != nullptr) {
			this->Regions.push_back(region);
			region->Sites.push_back(this);
		} else {
			fprintf(stderr, "Invalid region: \"%s\".\n", value.c_str());
		}
	} else {
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CSite::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "cultural_names") {
		for (size_t j = 0; j < section->Properties.size(); ++j) {
			std::string key = section->Properties[j].first;
			std::string value = section->Properties[j].second;
			
			key = FindAndReplaceString(key, "_", "-");
			
			const CCivilization *civilization = CCivilization::Get(key);
			
			if (civilization) {
				this->CulturalNames[civilization] = value;
			}
		}
	} else if (section->Tag == "historical_owner") {
		CDate date;
		CFaction *owner_faction = nullptr;
			
		for (size_t j = 0; j < section->Properties.size(); ++j) {
			std::string key = section->Properties[j].first;
			std::string value = section->Properties[j].second;
			
			if (key == "date") {
				value = FindAndReplaceString(value, "_", "-");
				date = CDate::FromString(value);
			} else if (key == "faction") {
				value = FindAndReplaceString(value, "_", "-");
				owner_faction = CFaction::Get(value);
			} else {
				fprintf(stderr, "Invalid historical owner property: \"%s\".\n", key.c_str());
			}
		}
		
		this->HistoricalOwners[date] = owner_faction;
	} else if (section->Tag == "historical_building") {
		CDate start_date;
		CDate end_date;
		int building_class_id = -1;
		CUniqueItem *unique = nullptr;
		CFaction *building_owner_faction = nullptr;
			
		for (size_t j = 0; j < section->Properties.size(); ++j) {
			std::string key = section->Properties[j].first;
			std::string value = section->Properties[j].second;
			
			if (key == "start_date") {
				value = FindAndReplaceString(value, "_", "-");
				start_date = CDate::FromString(value);
			} else if (key == "end_date") {
				value = FindAndReplaceString(value, "_", "-");
				end_date = CDate::FromString(value);
			} else if (key == "building_class") {
				value = FindAndReplaceString(value, "_", "-");
				building_class_id = GetUnitTypeClassIndexByName(value);
				if (building_class_id == -1) {
					fprintf(stderr, "Invalid unit class: \"%s\".\n", value.c_str());
				}
			} else if (key == "unique") {
				value = FindAndReplaceString(value, "_", "-");
				unique = GetUniqueItem(value);
				if (!unique) {
					fprintf(stderr, "Invalid unique: \"%s\".\n", value.c_str());
				}
			} else if (key == "faction") {
				value = FindAndReplaceString(value, "_", "-");
				building_owner_faction = CFaction::Get(value);
			} else {
				fprintf(stderr, "Invalid historical building property: \"%s\".\n", key.c_str());
			}
		}
		
		if (building_class_id == -1) {
			fprintf(stderr, "Historical building has no building class.\n");
			return true;
		}
		
		this->HistoricalBuildings.push_back(std::tuple<CDate, CDate, int, CUniqueItem *, CFaction *>(start_date, end_date, building_class_id, unique, building_owner_faction));
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the site
*/
void CSite::Initialize()
{
	if (!this->Major && !this->Cores.empty()) { //if the site is a minor one, but has faction cores, remove them
		for (CFaction *core_faction : this->Cores) {
			core_faction->Cores.erase(std::remove(core_faction->Cores.begin(), core_faction->Cores.end(), this), core_faction->Cores.end());
		}
		this->Cores.clear();
	}
	
	if (this->MapTemplate) {
		if (this->Position.x != -1 && this->Position.y != -1) {
			if (this->MapTemplate->SitesByPosition.find(std::pair<int, int>(this->Position.x, this->Position.y)) == this->MapTemplate->SitesByPosition.end()) {
				this->MapTemplate->SitesByPosition[std::pair<int, int>(this->Position.x, this->Position.y)] = this;
			} else {
				fprintf(stderr, "Position (%d, %d) of map template \"%s\" already has a site.", this->Position.x, this->Position.y, this->MapTemplate->Ident.c_str());
			}
		}
		
		this->MapTemplate->Sites.push_back(this);
	}
	
	this->Initialized = true;
}

/**
**	@brief	Get a site's cultural name
**
**	@param	civilization	The civilization for which to get the cultural name
**
**	@return	The cultural name if present, or the default name otherwise
*/
/**
**  
*/
std::string CSite::GetCulturalName(const CCivilization *civilization) const
{
	if (civilization != nullptr) {
		std::map<const CCivilization *, std::string>::const_iterator find_iterator = this->CulturalNames.find(civilization);
		if (find_iterator != this->CulturalNames.end()) {
			return find_iterator->second;
		}
	}
	
	return this->Name;
}
