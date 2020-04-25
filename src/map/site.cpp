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
#include "faction.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "player.h" //for factions
#include "province.h" //for regions
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace stratagus {

void site::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "major") {
			this->major = string::to_bool(value);
		} else if (key == "position_x") {
			this->pos.setX(std::stoi(value));
		} else if (key == "position_y") {
			this->pos.setY(std::stoi(value));
		} else if (key == "map_template") {
			this->map_template = map_template::get(value);
		} else if (key == "core") {
			faction *faction = faction::get(value);
			this->Cores.push_back(faction);
			faction->Cores.push_back(this);
			faction->sites.push_back(this);
			if (faction->civilization) {
				faction->civilization->sites.push_back(this);
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
				
				const civilization *civilization = civilization::get(key);
				this->CulturalNames[civilization] = value;
			}
		} else if (child_config_data->Tag == "historical_owner") {
			CDate date;
			faction *owner_faction = nullptr;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "date") {
					value = FindAndReplaceString(value, "_", "-");
					date = CDate::FromString(value);
				} else if (key == "faction") {
					owner_faction = faction::get(value);
				} else {
					fprintf(stderr, "Invalid historical owner property: \"%s\".\n", key.c_str());
				}
			}
			
			this->HistoricalOwners[date] = owner_faction;
		} else if (child_config_data->Tag == "historical_building") {
			CDate start_date;
			CDate end_date;
			const unit_class *building_class = nullptr;
			CUniqueItem *unique = nullptr;
			faction *building_owner_faction = nullptr;
				
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
					building_class = unit_class::get(value);
				} else if (key == "unique") {
					value = FindAndReplaceString(value, "_", "-");
					unique = GetUniqueItem(value);
					if (!unique) {
						fprintf(stderr, "Invalid unique: \"%s\".\n", value.c_str());
					}
				} else if (key == "faction") {
					building_owner_faction = faction::get(value);
				} else {
					fprintf(stderr, "Invalid historical building property: \"%s\".\n", key.c_str());
				}
			}
			
			if (building_class == nullptr) {
				fprintf(stderr, "Historical building has no building class.\n");
				continue;
			}
			
			this->HistoricalBuildings.push_back(std::tuple<CDate, CDate, const unit_class *, CUniqueItem *, faction *>(start_date, end_date, building_class, unique, building_owner_faction));
		} else {
			fprintf(stderr, "Invalid site property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void site::initialize()
{
	if (!this->is_major() && !this->Cores.empty()) { //if the site is a minor one, but has faction cores, remove them
		for (faction *core_faction : this->Cores) {
			vector::remove(core_faction->Cores, this);
		}
		this->Cores.clear();
	}

	if (this->get_map_template() != nullptr) {
		if (this->get_pos().x() != -1 && this->get_pos().y() != -1) {
			if (this->map_template->sites_by_position.find(this->get_pos()) == this->map_template->sites_by_position.end()) {
				this->map_template->sites_by_position[this->get_pos()] = this;
			} else {
				throw std::runtime_error("Position (" + std::to_string(this->get_pos().x()) + ", " + std::to_string(this->get_pos().y()) + ") of map template \"" + this->map_template->get_identifier() + "\" already has a site.");
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
std::string site::GetCulturalName(const civilization *civilization) const
{
	if (civilization != nullptr) {
		const auto find_iterator = this->CulturalNames.find(civilization);
		if (find_iterator != this->CulturalNames.end()) {
			return find_iterator->second;
		}
	}
	
	return this->get_name();
}

void site::set_site_unit(CUnit *unit)
{
	if (unit == this->get_site_unit()) {
		return;
	}

	this->site_unit = unit;

	if (this->site_unit != nullptr && this->site_unit->Player != nullptr && this->site_unit->Player->Index != PlayerNumNeutral) {
		this->set_owner(this->site_unit->Player);
	} else {
		this->set_owner(nullptr);
	}
}

void site::set_owner(CPlayer *player)
{
	if (player == this->get_owner()) {
		return;
	}

	this->owner = player;
	this->update_border_tile_graphics();
}

void site::update_border_tile_graphics()
{
	if (this->get_site_unit() != nullptr) {
		const int z = this->get_site_unit()->MapLayer->ID;
		const int minimap_territory_tile_range = UI.Minimap.get_territory_tile_range(z);
		for (const QPoint &tile_pos : this->border_tiles) {
			CMap::Map.CalculateTileOwnershipTransition(tile_pos, z);

			for (int minimap_offset_x = -minimap_territory_tile_range; minimap_offset_x <= minimap_territory_tile_range; ++minimap_offset_x) {
				for (int minimap_offset_y = -minimap_territory_tile_range; minimap_offset_y <= minimap_territory_tile_range; ++minimap_offset_y) {
					const QPoint minimap_tile_pos(tile_pos.x() + minimap_offset_x, tile_pos.y() + minimap_offset_y);

					if (!CMap::Map.Info.IsPointOnMap(minimap_tile_pos, z)) {
						continue;
					}

					UI.Minimap.UpdateXY(minimap_tile_pos, z);
				}
			}

			//update adjacent tiles with different settlements as well
			for (int x_offset = -1; x_offset <= 1; ++x_offset) {
				for (int y_offset = -1; y_offset <= 1; ++y_offset) {
					if (x_offset == 0 && y_offset == 0) {
						continue;
					}

					const QPoint adjacent_pos(tile_pos.x() + x_offset, tile_pos.y() + y_offset);
					if (!CMap::Map.Info.IsPointOnMap(adjacent_pos, z)) {
						continue;
					}

					CMapField *adjacent_tile = CMap::Map.Field(adjacent_pos, z);
					if (adjacent_tile->get_settlement() != nullptr && adjacent_tile->get_settlement() != this) {
						CMap::Map.CalculateTileOwnershipTransition(adjacent_pos, z);

						for (int minimap_offset_x = -minimap_territory_tile_range; minimap_offset_x <= minimap_territory_tile_range; ++minimap_offset_x) {
							for (int minimap_offset_y = -minimap_territory_tile_range; minimap_offset_y <= minimap_territory_tile_range; ++minimap_offset_y) {
								const QPoint minimap_tile_pos(adjacent_pos.x() + minimap_offset_x, adjacent_pos.y() + minimap_offset_y);

								if (!CMap::Map.Info.IsPointOnMap(minimap_tile_pos, z)) {
									continue;
								}

								UI.Minimap.UpdateXY(minimap_tile_pos, z);
							}
						}
					}
				}
			}
		}
	}
}

}
