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
#include "database/defines.h"
#include "faction.h"
#include "item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/region.h"
#include "player.h" //for factions
#include "player_color.h"
#include "province.h" //for regions
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/geocoordinate_util.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace stratagus {

void site::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "geocoordinate_scale") {
		const double scale = std::stod(value);
		this->longitude_scale = scale;
		this->latitude_scale = scale;
	} else {
		data_entry::process_sml_property(property);
	}
}

void site::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "cultural_names") {
		scope.for_each_property([&](const sml_property &property) {
			const civilization *civilization = civilization::get(property.get_key());
			this->cultural_names[civilization] = property.get_value();
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void site::process_sml_dated_scope(const sml_data &scope, const QDateTime &date)
{
	const std::string &tag = scope.get_tag();

	if (tag == "population_groups") {
		scope.for_each_property([&](const sml_property &property) {
			const unit_class *unit_class = unit_class::get(property.get_key());
			this->population_groups[unit_class->get_index()] = std::stoi(property.get_value());
		});
	} else {
		data_entry::process_sml_dated_scope(scope, date);
	}
}

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
			this->add_core(faction);
		} else if (key == "region") {
			region *region = region::get(value);
			this->regions.push_back(region);
			region->add_site(this);
		} else {
			fprintf(stderr, "Invalid site property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "cultural_names") {
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				const civilization *civilization = civilization::get(key);
				this->cultural_names[civilization] = value;
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
	if (this->get_geocoordinate().isValid()) {
		if (this->geocoordinate_reference_site != nullptr) {
			if (!this->geocoordinate_reference_site->is_initialized()) {
				this->geocoordinate_reference_site->initialize();
			}

			double lon = this->get_geocoordinate().longitude();
			double lat = this->get_geocoordinate().latitude();

			if (this->longitude_scale != 100) {
				lon *= this->longitude_scale;
				lon /= 100;
			}

			if (this->latitude_scale != 100) {
				lat *= this->latitude_scale;
				lat /= 100;
			}

			lon += this->geocoordinate_reference_site->get_geocoordinate().longitude();
			lat += this->geocoordinate_reference_site->get_geocoordinate().latitude();

			this->geocoordinate = QGeoCoordinate(lat, lon);
		}

		this->pos = geocoordinate::to_point(this->get_geocoordinate(), this->get_map_template()->get_georectangle(), this->get_map_template()->get_size());
	}

	//if a settlement has no color assigned to it, assign a random one instead
	if (this->is_major() && !this->get_color().isValid()) {
		this->color = QColor(random::get()->generate(256), random::get()->generate(256), random::get()->generate(256));
	}

	for (faction *core_faction : this->get_cores()) {
		core_faction->get_civilization()->sites.push_back(this);
	}

	if (!this->is_major() && !this->get_cores().empty()) { //if the site is a minor one, but has faction cores, remove them
		for (faction *core_faction : this->get_cores()) {
			vector::remove(core_faction->Cores, this);
		}
		this->cores.clear();
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

	data_entry::initialize();
}

const std::string &site::get_cultural_name(const civilization *civilization) const
{
	if (civilization != nullptr) {
		const auto find_iterator = this->cultural_names.find(civilization);
		if (find_iterator != this->cultural_names.end()) {
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

	if (this->site_unit != nullptr && this->site_unit->Player != nullptr && this->site_unit->Player->Index != PlayerNumNeutral && !this->site_unit->UnderConstruction) {
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
	if (GameCycle > 0) {
		this->update_border_tiles();
		this->update_minimap_territory();
	}
}

CPlayer *site::get_realm_owner() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_realm_player();
	}

	return nullptr;
}

QVariantList site::get_building_classes_qvariant_list() const
{
	return container::to_qvariant_list(this->get_building_classes());
}

void site::remove_building_class(unit_class *building_class)
{
	vector::remove_one(this->building_classes, building_class);
}

void site::update_border_tiles()
{
	if (this->get_site_unit() == nullptr) {
		return;
	}

	const int z = this->get_site_unit()->MapLayer->ID;
	for (const QPoint &tile_pos : this->border_tiles) {
		CMap::Map.CalculateTileOwnershipTransition(tile_pos, z);

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
				}
			}
		}
	}
}

void site::update_minimap_territory()
{
	if (this->get_site_unit() == nullptr) {
		return;
	}

	const int z = this->get_site_unit()->MapLayer->ID;

	for (int x = this->territory_rect.x(); x <= this->territory_rect.right(); ++x) {
		for (int y = this->territory_rect.y(); y <= this->territory_rect.bottom(); ++y) {
			const QPoint tile_pos(x, y);
			const CMapField *tile = CMap::Map.Field(tile_pos, z);

			if (tile->get_settlement() == this) {
				UI.Minimap.update_territory_xy(tile_pos, z);
			}
		}
	}
}

QVariantList site::get_cores_qvariant_list() const
{
	return container::to_qvariant_list(this->get_cores());
}

void site::add_core(faction *faction)
{
	this->cores.push_back(faction);
	faction->sites.push_back(this);
}


void site::remove_core(faction *faction)
{
	vector::remove(this->cores, faction);
	vector::remove(faction->sites, this);
}

QVariantList site::get_regions_qvariant_list() const
{
	return container::to_qvariant_list(this->get_regions());
}

void site::add_region(region *region)
{
	this->regions.push_back(region);
	region->add_site(this);
}


void site::remove_region(region *region)
{
	vector::remove(this->regions, region);
	region->remove_site(this);
}

}
