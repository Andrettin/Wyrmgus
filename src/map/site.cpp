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

#include "stratagus.h"

#include "map/site.h"

#include "civilization.h"
#include "config.h"
#include "map/map_template.h"
#include "player.h" //for factions
#include "province.h" //for regions
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

namespace stratagus {

void site::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "major") {
			this->Major = string::to_bool(value);
		} else if (key == "position_x") {
			this->Position.x = std::stoi(value);
		} else if (key == "position_y") {
			this->Position.y = std::stoi(value);
		} else if (key == "map_template") {
			this->map_template = stratagus::map_template::get(value);
		} else if (key == "core") {
			value = FindAndReplaceString(value, "_", "-");
			
			CFaction *faction = PlayerRaces.GetFaction(value);
			if (faction != nullptr) {
				this->Cores.push_back(faction);
				faction->Cores.push_back(this);
				faction->sites.push_back(this);
				if (faction->civilization) {
					faction->civilization->sites.push_back(this);
				}
			} else {
				fprintf(stderr, "Invalid faction: \"%s\".\n", value.c_str());
			}
		} else if (key == "region") {
			value = FindAndReplaceString(value, "_", "-");
			
			CRegion *region = GetRegion(value);
			if (region != nullptr) {
				this->Regions.push_back(region);
				region->sites.push_back(this);
			} else {
				fprintf(stderr, "Invalid region: \"%s\".\n", value.c_str());
			}
		} else {
			fprintf(stderr, "Invalid site property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "cultural_names") {
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				key = FindAndReplaceString(key, "_", "-");
				
				const stratagus::civilization *civilization = stratagus::civilization::get(key);
				this->CulturalNames[civilization] = value;
			}
		} else if (child_config_data->Tag == "historical_owner") {
			CDate date;
			CFaction *owner_faction = nullptr;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "date") {
					value = FindAndReplaceString(value, "_", "-");
					date = CDate::FromString(value);
				} else if (key == "faction") {
					value = FindAndReplaceString(value, "_", "-");
					owner_faction = PlayerRaces.GetFaction(value);
					if (!owner_faction) {
						fprintf(stderr, "Invalid faction: \"%s\".\n", value.c_str());
					}
				} else {
					fprintf(stderr, "Invalid historical owner property: \"%s\".\n", key.c_str());
				}
			}
			
			this->HistoricalOwners[date] = owner_faction;
		} else if (child_config_data->Tag == "historical_building") {
			CDate start_date;
			CDate end_date;
			const stratagus::unit_class *building_class = nullptr;
			CUniqueItem *unique = nullptr;
			CFaction *building_owner_faction = nullptr;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "start_date") {
					value = FindAndReplaceString(value, "_", "-");
					start_date = CDate::FromString(value);
				} else if (key == "end_date") {
					value = FindAndReplaceString(value, "_", "-");
					end_date = CDate::FromString(value);
				} else if (key == "building_class") {
					building_class = stratagus::unit_class::get(value);
				} else if (key == "unique") {
					value = FindAndReplaceString(value, "_", "-");
					unique = GetUniqueItem(value);
					if (!unique) {
						fprintf(stderr, "Invalid unique: \"%s\".\n", value.c_str());
					}
				} else if (key == "faction") {
					value = FindAndReplaceString(value, "_", "-");
					building_owner_faction = PlayerRaces.GetFaction(value);
					if (!building_owner_faction) {
						fprintf(stderr, "Invalid faction: \"%s\".\n", value.c_str());
					}
				} else {
					fprintf(stderr, "Invalid historical building property: \"%s\".\n", key.c_str());
				}
			}
			
			if (building_class == nullptr) {
				fprintf(stderr, "Historical building has no building class.\n");
				continue;
			}
			
			this->HistoricalBuildings.push_back(std::tuple<CDate, CDate, const stratagus::unit_class *, CUniqueItem *, CFaction *>(start_date, end_date, building_class, unique, building_owner_faction));
		} else {
			fprintf(stderr, "Invalid site property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	if (!this->Major && !this->Cores.empty()) { //if the site is a minor one, but has faction cores, remove them
		for (CFaction *core_faction : this->Cores) {
			core_faction->Cores.erase(std::remove(core_faction->Cores.begin(), core_faction->Cores.end(), this), core_faction->Cores.end());
		}
		this->Cores.clear();
	}
	
	if (this->map_template) {
		if (this->Position.x != -1 && this->Position.y != -1) {
			if (this->map_template->sites_by_position.find(this->Position) == this->map_template->sites_by_position.end()) {
				this->map_template->sites_by_position[this->Position] = this;
			} else {
				fprintf(stderr, "Position (%d, %d) of map template \"%s\" already has a site.", this->Position.x, this->Position.y, this->map_template->Ident.c_str());
			}
		}
		
		this->map_template->sites.push_back(this);
	}
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
std::string site::GetCulturalName(const stratagus::civilization *civilization) const
{
	if (civilization != nullptr) {
		std::map<const stratagus::civilization *, std::string>::const_iterator find_iterator = this->CulturalNames.find(civilization);
		if (find_iterator != this->CulturalNames.end()) {
			return find_iterator->second;
		}
	}
	
	return this->get_name();
}

}
