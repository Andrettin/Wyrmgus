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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "stratagus.h"

#include "map/site.h"

#include "civilization.h"
#include "config.h"
#include "database/defines.h"
#include "faction.h"
#include "item/unique_item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/minimap.h"
#include "map/region.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/tile.h"
#include "player.h" //for factions
#include "player_color.h"
#include "province.h" //for regions
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "util/astronomy_util.h"
#include "util/container_util.h"
#include "util/geocoordinate.h"
#include "util/geocoordinate_util.h"
#include "util/point_util.h"
#include "util/random.h"
#include "util/string_conversion_util.h"
#include "util/time_util.h"
#include "util/util.h"
#include "util/vector_util.h"

namespace wyrmgus {

site::site(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
{
}

site::~site()
{
}

void site::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "geocoordinate_scale") {
		const int scale = std::stoi(value);
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

void site::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "base_unit_type") {
			this->base_unit_type = unit_type::get(value);
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
			unique_item *unique = nullptr;
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
					unique = unique_item::get(value);
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
			
			this->HistoricalBuildings.push_back(std::make_tuple(start_date, end_date, building_class, unique, building_owner_faction));
		} else {
			fprintf(stderr, "Invalid site property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void site::initialize()
{
	if (this->geocoordinate_reference_site != nullptr) {
		if (!this->geocoordinate_reference_site->is_initialized()) {
			this->geocoordinate_reference_site->initialize();
		}

		if (this->geocoordinate_offset.is_null()) {
			throw std::runtime_error("Site \"" + this->get_identifier() + "\" has a geocoordinate reference site, but no geocoordinate offset.");
		}

		longitude lon = this->geocoordinate_offset.get_longitude();
		latitude lat = this->geocoordinate_offset.get_latitude();

		if (this->longitude_scale != 100) {
			lon *= this->longitude_scale;
			lon /= 100;
		}

		if (this->latitude_scale != 100) {
			lat *= this->latitude_scale;
			lat /= 100;
		}

		lon += this->geocoordinate_reference_site->get_geocoordinate().get_longitude();
		lat += this->geocoordinate_reference_site->get_geocoordinate().get_latitude();

		this->geocoordinate = wyrmgus::geocoordinate(lon, lat);

		if (!this->geocoordinate.is_valid()) {
			throw std::runtime_error("The application of a geocoordinate offset to a reference site's geocoordinate resulted in no valid geocoordinate for site \"" + this->get_identifier() + "\".");
		}
	} else if (this->pos_reference_site != nullptr) {
		if (!this->pos_reference_site->is_initialized()) {
			this->pos_reference_site->initialize();
		}

		this->pos = this->pos_reference_site->get_pos() + this->pos;
	}

	if (!this->get_geocoordinate().is_null()) {
		this->pos = this->get_map_template()->get_geocoordinate_pos(this->get_geocoordinate());
	} else if (!this->get_astrocoordinate().is_null()) {
		this->pos = this->astrocoordinate_to_pos<false>(this->get_astrocoordinate());
	}

	if (!this->satellites.empty()) {
		std::sort(this->satellites.begin(), this->satellites.end(), [](const site *a, const site *b) {
			if (a->distance_from_orbit_center == 0 || b->distance_from_orbit_center == 0) {
				return a->distance_from_orbit_center != 0; //place satellites without a predefined distance from orbit center last
			}

			return a->distance_from_orbit_center < b->distance_from_orbit_center;
		});
	}

	//if a settlement has no color assigned to it, assign a random one instead
	if (this->is_settlement() && !this->get_color().isValid()) {
		this->color = random::get()->generate_color();
	}

	for (faction *core_faction : this->get_cores()) {
		core_faction->get_civilization()->sites.push_back(this);
	}

	if (this->is_settlement()) { 
		for (faction *core_faction : this->get_cores()) {
			core_faction->add_core_settlement(this);
		}
	} else {
		//if the site is a minor one, but has faction cores, remove them
		this->cores.clear();
	}

	if (this->get_map_template() != nullptr) {
		if (this->get_pos().x() != -1 && this->get_pos().y() != -1) {
			auto find_iterator = this->map_template->sites_by_position.find(this->get_pos());
			if (find_iterator == this->map_template->sites_by_position.end()) {
				this->map_template->sites_by_position[this->get_pos()] = this;
			} else {
				throw std::runtime_error("Position " + point::to_string(this->get_pos()) + " of map template \"" + this->map_template->get_identifier() + "\" already has a site (\"" + find_iterator->second->get_identifier() + "\").");
			}
		}

		this->map_template->sites.push_back(this);
	}

	//create the site's game data object
	this->reset_game_data();

	data_entry::initialize();
}

void site::check() const
{
	if (!this->get_geocoordinate().is_null() && !this->get_astrocoordinate().is_null()) {
		throw std::runtime_error("Site \"" + this->get_identifier() + "\" has both a geocoordinate and an astrocoordinate.");
	}

	if (!this->get_astrocoordinate().is_null() && this->get_astrodistance() == 0) {
		throw std::runtime_error("Site \"" + this->get_identifier() + "\" has an astrocoordinate, but its astrodistance is zero.");
	}

	if (!this->get_astrocoordinate().is_null() && this->has_random_astrocoordinate()) {
		throw std::runtime_error("Site \"" + this->get_identifier() + "\" has both a fixed astrocoordinate, and is set to have a random one.");
	}

	if (this->orbit_center != nullptr && this->get_map_template() != nullptr) {
		throw std::runtime_error("Site \"" + this->get_identifier() + "\" has an orbit center and is assigned to a map template. It should not be assigned to a map template if it is a satellite, as it will be applied in its orbit center's map template.");
	}
}

data_entry_history *site::get_history_base()
{
	return this->history.get();
}

void site::reset_history()
{
	this->history = std::make_unique<site_history>(this);
}

void site::reset_game_data()
{
	this->game_data = std::make_unique<site_game_data>(this);
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

bool site::is_settlement() const
{
	if (this->get_base_unit_type() == nullptr) {
		return false;
	}

	return this->get_base_unit_type() == settlement_site_unit_type;
}

QTime site::get_right_ascension() const
{
	const decimillesimal_int lon = this->get_astrocoordinate().get_longitude();
	const decimillesimal_int ra = astronomy::lon_to_ra(lon);
	return ra.to_time();
}

void site::set_right_ascension(const QTime &ra)
{
	const decimillesimal_int lon = astronomy::ra_to_lon(time::to_number(ra));
	this->astrocoordinate.set_longitude(lon);
}

centesimal_int site::get_astrodistance_pc() const
{
	return astronomy::ly_to_pc(this->get_astrodistance());
}

void site::set_astrodistance_pc(const centesimal_int &astrodistance_pc)
{
	this->astrodistance = astronomy::pc_to_ly(astrodistance_pc);
}

void site::set_orbit_center(site *orbit_center)
{
	if (orbit_center == this->orbit_center) {
		return;
	}

	if (this->orbit_center != nullptr) {
		vector::remove(this->orbit_center->satellites, this);
	}

	this->orbit_center = orbit_center;

	if (orbit_center != nullptr) {
		orbit_center->satellites.push_back(this);
	}
}

centesimal_int site::get_distance_from_orbit_center_au() const
{
	return astronomy::gm_to_au(this->distance_from_orbit_center);
}

void site::set_distance_from_orbit_center_au(const centesimal_int &distance_au)
{
	this->distance_from_orbit_center = astronomy::au_to_gm(distance_au);
}

QPoint site::astrocoordinate_to_relative_pos(const wyrmgus::geocoordinate &astrocoordinate, const wyrmgus::map_template *reference_subtemplate) const
{
	QPoint direction_pos = astrocoordinate.to_circle_edge_point();
	int64_t astrodistance_value = this->get_astrodistance().to_int();
	astrodistance_value = isqrt(astrodistance_value);
	astrodistance_value *= this->get_map_template()->get_astrodistance_multiplier();

	//the size of the reference subtemplate serves as a minimum distance in tiles
	astrodistance_value += std::max(reference_subtemplate->get_applied_width(), reference_subtemplate->get_applied_height()) / 2;

	astrodistance_value += this->get_map_template()->get_astrodistance_additive_modifier();
	astrodistance_value += this->get_astrodistance_additive_modifier();
	const int64_t x = direction_pos.x() * astrodistance_value / geocoordinate::number_type::divisor;
	const int64_t y = direction_pos.y() * astrodistance_value / geocoordinate::number_type::divisor;

	return QPoint(x, y);
}

template <bool use_map_pos>
QPoint site::astrocoordinate_to_pos(const wyrmgus::geocoordinate &astrocoordinate) const
{
	const wyrmgus::map_template *reference_subtemplate = this->get_map_template()->get_default_astrocoordinate_reference_subtemplate();
	if (reference_subtemplate == nullptr) {
		throw std::runtime_error("Could not convert astrocoordinate to pos for site \"" + this->get_identifier() + "\", as its map template has no default astrocoordinate reference subtemplate.");
	}

	const QPoint relative_pos = this->astrocoordinate_to_relative_pos(astrocoordinate, reference_subtemplate);

	QPoint base_pos;
	if constexpr (use_map_pos) {
		if (!CMap::get()->is_subtemplate_on_map(reference_subtemplate)) {
			throw std::runtime_error("Could not convert an astrocoordinate to a map pos for site \"" + this->get_identifier() + "\", as its reference subtemplate is not on the map.");
		}

		base_pos = reference_subtemplate->get_current_map_start_pos();
	} else {
		if (reference_subtemplate->get_subtemplate_top_left_pos() == QPoint(-1, -1)) {
			throw std::runtime_error("Could not convert an astrocoordinate to a pos for site \"" + this->get_identifier() + "\", as its reference subtemplate has no subtemplate position within the main template.");
		}
		base_pos = reference_subtemplate->get_subtemplate_top_left_pos();
	}

	base_pos += QPoint(reference_subtemplate->get_applied_width() / 2 - 1, reference_subtemplate->get_applied_height() / 2 - 1);

	//apply the relative position of the celestial body to the reference subtemplate's center
	return base_pos + relative_pos;
}

template QPoint site::astrocoordinate_to_pos<false>(const wyrmgus::geocoordinate &) const;
template QPoint site::astrocoordinate_to_pos<true>(const wyrmgus::geocoordinate &) const;

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
