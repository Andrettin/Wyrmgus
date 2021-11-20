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

#include "map/map_template.h"

#include "character.h"
#include "character_history.h"
#include "config.h"
#include "database/defines.h"
#include "editor.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "item/unique_item.h"
#include "map/character_substitution.h"
#include "map/character_unit.h"
#include "map/generated_terrain.h"
#include "map/historical_location.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/map_projection.h"
#include "map/map_template_history.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/world.h"
#include "map/world_game_data.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/faction_history.h"
#include "player/faction_type.h"
#include "player/player.h"
#include "player/player_type.h"
#include "quest/campaign.h"
#include "settings.h"
#include "time/calendar.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "translate.h"
#include "unit/historical_unit.h"
#include "unit/historical_unit_history.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/container_util.h"
#include "util/geoshape_util.h"
#include "util/image_util.h"
#include "util/number_util.h"
#include "util/path_util.h"
#include "util/point_util.h"
#include "util/size_util.h"
#include "util/string_util.h"
#include "util/string_conversion_util.h"
#include "util/util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"
#include "video/video.h"

namespace wyrmgus {

//map templates must be initialized after sites, as sites add themselves to the map template site list in their initialization function, and during map template initialization the sites are then sorted
const std::set<std::string> map_template::database_dependencies = { site::class_identifier };

map_template::map_template(const std::string &identifier)
	: named_data_entry(identifier), CDataType(identifier),
	georectangle(geocoordinate(geocoordinate::min_longitude, geocoordinate::min_latitude), geocoordinate(geocoordinate::max_longitude, geocoordinate::max_latitude))
{
}

map_template::~map_template()
{
}

void map_template::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "adjacent_template") {
		map_template *adjacent_template = map_template::get(value);
		this->AdjacentTemplates.push_back(adjacent_template);
		this->add_dependency_template(adjacent_template);
		adjacent_template->dependent_adjacent_templates.push_back(this);
	} else if (key == "north_of") {
		const map_template *north_of_template = map_template::get(value);
		this->NorthOfTemplates.push_back(north_of_template);
		this->add_dependency_template(north_of_template);
	} else if (key == "south_of") {
		const map_template *south_of_template = map_template::get(value);
		this->SouthOfTemplates.push_back(south_of_template);
		this->add_dependency_template(south_of_template);
	} else if (key == "west_of") {
		const map_template *west_of_template = map_template::get(value);
		this->WestOfTemplates.push_back(west_of_template);
		this->add_dependency_template(west_of_template);
	} else if (key == "east_of") {
		const map_template *east_of_template = map_template::get(value);
		this->EastOfTemplates.push_back(east_of_template);
		this->add_dependency_template(east_of_template);
	} else if (key == "min_subtemplate_x") {
		this->min_subtemplate_pos.setX(std::stoi(value));
	} else if (key == "min_subtemplate_y") {
		this->min_subtemplate_pos.setY(std::stoi(value));
	} else if (key == "max_subtemplate_x") {
		this->max_subtemplate_pos.setX(std::stoi(value));
	} else if (key == "max_subtemplate_y") {
		this->max_subtemplate_pos.setY(std::stoi(value));
	} else {
		data_entry::process_sml_property(property);
	}
}

void map_template::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "generated_terrains") {
		scope.for_each_child([&](const sml_data &child_scope) {
			terrain_type *terrain_type = terrain_type::get(child_scope.get_tag());

			auto generated_terrain = std::make_unique<wyrmgus::generated_terrain>(terrain_type);
			database::process_sml_data(generated_terrain, child_scope);

			this->generated_terrains.push_back(std::move(generated_terrain));
		});
	} else if (tag == "generated_neutral_units" || tag == "player_location_generated_neutral_units") {
		scope.for_each_property([&](const sml_property &property) {
			unit_type *unit_type = unit_type::get(property.get_key());
			const int quantity = std::stoi(property.get_value());

			if (tag == "generated_neutral_units") {
				this->GeneratedNeutralUnits.push_back(std::pair<wyrmgus::unit_type *, int>(unit_type, quantity));
			} else if (tag == "player_location_generated_neutral_units") {
				this->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<wyrmgus::unit_type *, int>(unit_type, quantity));
			}
		});
	} else if (tag == "tile_terrains" || tag == "overlay_tile_terrains") {
		point_map<const terrain_type *> &tile_terrains = (tag == "overlay_tile_terrains") ? this->overlay_tile_terrains : this->tile_terrains;

		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const int x = std::stoi(tag);

			child_scope.for_each_property([&](const sml_property &property) {
				const std::string &key = property.get_key();
				const std::string &value = property.get_value();

				const int y = std::stoi(key);
				const terrain_type *terrain = terrain_type::get(value);

				tile_terrains[QPoint(x, y)] = terrain;
			});
		});
	} else if (tag == "terrain_substitutions") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const terrain_type *terrain = terrain_type::get(key);
			const terrain_type *other_terrain = terrain_type::get(value);

			this->terrain_substitutions[terrain] = other_terrain;
		});
	} else if (tag == "default_base_terrains") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const terrain_type *overlay_terrain = terrain_type::get(key);
			const terrain_type *base_terrain = terrain_type::get(value);

			this->default_base_terrains[overlay_terrain] = base_terrain;
		});
	} else if (tag == "character_units") {
		scope.for_each_element([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const char character = string::to_character(key);
			const unit_type *unit_type = unit_type::get(value);

			this->character_units[character] = std::make_unique<character_unit>(unit_type);
		}, [&](const sml_data &child_scope) {
			const char character = string::to_character(child_scope.get_tag());

			auto unit = std::make_unique<character_unit>();
			database::process_sml_data(unit, child_scope);

			this->character_units[character] = std::move(unit);
		});
	} else if (tag == "character_substitutions") {
		scope.for_each_child([&](const sml_data &child_scope) {
			auto substitution = std::make_unique<character_substitution>();
			database::process_sml_data(substitution, child_scope);
			this->character_substitutions.push_back(std::move(substitution));
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void map_template::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "circle") {
			this->circle = string::to_bool(value);
		} else if (key == "world") {
			wyrmgus::world *world = world::get(value);
			this->world = world;
		} else if (key == "terrain_file") {
			this->terrain_file = value;
		} else if (key == "overlay_terrain_file") {
			this->overlay_terrain_file = value;
		} else if (key == "width") {
			this->size.setWidth(std::stoi(value));
		} else if (key == "height") {
			this->size.setHeight(std::stoi(value));
		} else if (key == "priority") {
			this->Priority = std::stoi(value);
		} else if (key == "min_x") {
			this->min_subtemplate_pos.setX(std::stoi(value));
		} else if (key == "min_y") {
			this->min_subtemplate_pos.setY(std::stoi(value));
		} else if (key == "max_x") {
			this->max_subtemplate_pos.setX(std::stoi(value));
		} else if (key == "max_y") {
			this->max_subtemplate_pos.setY(std::stoi(value));
		} else if (key == "main_template") {
			map_template *main_template = map_template::get(value);
			this->set_main_template(main_template);
		} else if (key == "upper_template") {
			map_template *upper_template = map_template::get(value);
			if (upper_template != nullptr) {
				this->UpperTemplate = upper_template;
				upper_template->LowerTemplate = this;
			}
		} else if (key == "lower_template") {
			map_template *lower_template = map_template::get(value);
			if (lower_template != nullptr) {
				this->LowerTemplate = lower_template;
				lower_template->UpperTemplate = this;
			}
		} else if (key == "adjacent_template") {
			map_template *adjacent_template = map_template::get(value);
			this->AdjacentTemplates.push_back(adjacent_template);
			this->add_dependency_template(adjacent_template);
			adjacent_template->dependent_adjacent_templates.push_back(this);
		} else if (key == "north_of") {
			const map_template *north_of_template = map_template::get(value);
			this->NorthOfTemplates.push_back(north_of_template);
			this->add_dependency_template(north_of_template);
		} else if (key == "south_of") {
			const map_template *south_of_template = map_template::get(value);
			this->SouthOfTemplates.push_back(south_of_template);
			this->add_dependency_template(south_of_template);
		} else if (key == "west_of") {
			const map_template *west_of_template = map_template::get(value);
			this->WestOfTemplates.push_back(west_of_template);
			this->add_dependency_template(west_of_template);
		} else if (key == "east_of") {
			const map_template *east_of_template = map_template::get(value);
			this->EastOfTemplates.push_back(east_of_template);
			this->add_dependency_template(east_of_template);
		} else if (key == "base_terrain_type") {
			terrain_type *terrain_type = terrain_type::get(value);
			this->base_terrain_type = terrain_type;
		} else if (key == "base_overlay_terrain_type") {
			terrain_type *terrain_type = terrain_type::get(value);
			this->base_overlay_terrain_type = terrain_type;
		} else if (key == "unusable_area_terrain_type") {
			terrain_type *terrain_type = terrain_type::get(value);
			this->unusable_area_terrain_type = terrain_type;
		} else if (key == "unusable_area_overlay_terrain_type") {
			terrain_type *terrain_type = terrain_type::get(value);
			this->unusable_area_overlay_terrain_type = terrain_type;
		} else {
			fprintf(stderr, "Invalid map template property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "generated_neutral_unit" || child_config_data->Tag == "player_location_generated_neutral_unit") {
			unit_type *unit_type = nullptr;
			int quantity = 1;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "unit_type") {
					unit_type = unit_type::get(value);
				} else if (key == "quantity") {
					quantity = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid generated neutral unit property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!unit_type) {
				fprintf(stderr, "Generated neutral unit has no unit type.\n");
				continue;
			}
			
			if (child_config_data->Tag == "generated_neutral_unit") {
				this->GeneratedNeutralUnits.push_back(std::pair<wyrmgus::unit_type *, int>(unit_type, quantity));
			} else if (child_config_data->Tag == "player_location_generated_neutral_unit") {
				this->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<wyrmgus::unit_type *, int>(unit_type, quantity));
			}
		} else if (child_config_data->Tag == "generated_terrain") {
			auto generated_terrain = std::make_unique<wyrmgus::generated_terrain>();
			
			generated_terrain->ProcessConfigData(child_config_data);
				
			if (generated_terrain->get_terrain_type() == nullptr) {
				continue;
			}
			
			this->generated_terrains.push_back(std::move(generated_terrain));
		} else {
			fprintf(stderr, "Invalid map template property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void map_template::initialize()
{
	if (this->get_subtemplate_top_left_pos().x() == -1 && this->get_subtemplate_top_left_pos().y() == -1 && this->get_subtemplate_center_pos().x() == -1 && this->get_subtemplate_center_pos().y() == -1) {
		const QPoint subtemplate_offset = size::to_point(this->get_applied_size()) - QPoint(1, 1) / 2;
		this->subtemplate_top_left_pos = this->get_subtemplate_center_pos() - subtemplate_offset;
	}

	if (this->get_main_template() != nullptr) {
		if (!this->get_main_template()->is_initialized()) {
			this->get_main_template()->initialize();
		}

		if (this->get_main_template()->get_world() != nullptr) {
			this->world = this->get_main_template()->world;
		}

		if (!this->min_subtemplate_geocoordinate.is_null()) {
			this->min_subtemplate_pos = this->get_main_template()->get_geocoordinate_pos(this->min_subtemplate_geocoordinate);

			this->min_subtemplate_pos.setX(std::max(this->min_subtemplate_pos.x(), 0));
			this->min_subtemplate_pos.setY(std::max(this->min_subtemplate_pos.y(), 0));

			this->min_subtemplate_pos.setX(std::min(this->min_subtemplate_pos.x(), this->get_main_template()->get_width() - 1));
			this->min_subtemplate_pos.setY(std::min(this->min_subtemplate_pos.y(), this->get_main_template()->get_height() - 1));
		}

		if (!this->max_subtemplate_geocoordinate.is_null()) {
			this->max_subtemplate_pos = this->get_main_template()->get_geocoordinate_pos(this->max_subtemplate_geocoordinate);

			this->max_subtemplate_pos.setX(std::max(this->max_subtemplate_pos.x(), 0));
			this->max_subtemplate_pos.setY(std::max(this->max_subtemplate_pos.y(), 0));

			this->max_subtemplate_pos.setX(std::min(this->max_subtemplate_pos.x(), this->get_main_template()->get_width() - 1));
			this->max_subtemplate_pos.setY(std::min(this->max_subtemplate_pos.y(), this->get_main_template()->get_height() - 1));
		}
	}

	if (!this->subtemplates.empty()) { //if this template has subtemplates, sort them according to priority, and to size (the larger map templates should be applied first, to make it more likely that they appear at all
		std::sort(this->subtemplates.begin(), this->subtemplates.end(), [](const map_template *a, const map_template *b) {
			//give priority to the template if the other template's position depends on its own
			if (a->is_dependent_on(b)) {
				return false;
			} else if (b->is_dependent_on(a)) {
				return true;
			} else if (a->is_optional() != b->is_optional()) {
				return b->is_optional();
			} else if (a->Priority != b->Priority) {
				return a->Priority > b->Priority;
			}

			const bool a_has_subtemplate_min_max_pos = a->get_min_subtemplate_pos().x() != -1 || a->get_min_subtemplate_pos().y() != -1 || a->get_max_subtemplate_pos().x() != -1 || a->get_max_subtemplate_pos().y() != -1;
			const bool b_has_subtemplate_min_max_pos = b->get_min_subtemplate_pos().x() != -1 || b->get_min_subtemplate_pos().y() != -1 || b->get_max_subtemplate_pos().x() != -1 || b->get_max_subtemplate_pos().y() != -1;
			if (a_has_subtemplate_min_max_pos != b_has_subtemplate_min_max_pos) {
				return a_has_subtemplate_min_max_pos;
			}
			
			if (a->get_applied_area_with_dependent_template_offsets() != b->get_applied_area_with_dependent_template_offsets()) {
				return a->get_applied_area_with_dependent_template_offsets() > b->get_applied_area_with_dependent_template_offsets();
			} else if (a->get_total_adjacent_template_count() != b->get_total_adjacent_template_count()) {
				return a->get_total_adjacent_template_count() < b->get_total_adjacent_template_count();
			} else {
				return a->get_identifier() < b->get_identifier();
			}
		});
	}

	if (!this->sites.empty()) {
		//sort sites to change their order of application
		std::sort(this->sites.begin(), this->sites.end(), [](const wyrmgus::site *site, const wyrmgus::site *other_site) {
			if (site->is_settlement() != other_site->is_settlement()) {
				//give priority to settlement sites
				return site->is_settlement();
			}

			if (site->get_satellites().size() != other_site->get_satellites().size()) {
				//give priority to sites with more satellites (e.g. stars with more planets in their systems)
				return site->get_satellites().size() > other_site->get_satellites().size();
			}

			return site->get_identifier() < other_site->get_identifier();
		});
	}

	if (this->outputs_terrain_image()) {
		this->save_terrain_images();
	}

	if (this->outputs_territory_image()) {
		site_map<std::vector<std::unique_ptr<QGeoShape>>> territory_data;

		if (this->get_world() != nullptr) {
			territory_data = this->get_world()->parse_territories_geojson_folder();
		}

		const std::string filename = this->get_identifier() + "_territories.png";

		this->save_territory_image(filename, territory_data);
	}

	data_entry::initialize();
}

void map_template::check() const
{
	for (const auto &kv_pair : this->character_units) {
		const std::unique_ptr<character_unit> &character_unit = kv_pair.second;
		character_unit->check();
	}

	for (const std::unique_ptr<character_substitution> &substitution : this->character_substitutions) {
		substitution->check();
	}

	if (this->clear_terrain && this->get_base_terrain_type() != nullptr && this->get_base_overlay_terrain_type() != nullptr) {
		throw std::runtime_error("Map template \"" + this->get_identifier() + "\" is set to clear terrain in its area, but also has a base terrain type and an overlay base terrain type.");
	}

	if (this->get_min_longitude() != 0 || this->get_min_latitude() != 0 || this->get_max_longitude() != 0 || this->get_max_latitude() != 0) {
		const wyrmgus::map_projection *map_projection = this->get_map_projection();
		bool area_changeable = false;

		if (this->IsSubtemplateArea()) {
			if (this->get_world() == this->get_main_template()->get_world()) {
				area_changeable = true;
			}
		}

		map_projection->validate_area(this->get_georectangle(), this->get_size());
	}

	if (this->get_main_template() != nullptr) {
		const QSize applied_size = this->get_applied_size();
		const QSize main_template_applied_size = this->get_main_template()->get_applied_size();

		if (applied_size.width() > main_template_applied_size.width() || applied_size.height() > main_template_applied_size.height()) {
			throw std::runtime_error("Map template \"" + this->get_identifier() + "\" is greater in size than its main template (\"" + this->get_main_template()->get_identifier() + "\").");
		}
	}

	/*
	if (this->get_default_astrocoordinate_reference_subtemplate() != nullptr && !this->get_default_astrocoordinate_reference_subtemplate()->is_any_subtemplate_of(this)) {
		throw std::runtime_error("Map template \"" + this->get_identifier() + "\" has \"" + this->get_default_astrocoordinate_reference_subtemplate()->get_identifier() + "\" as its default astrocoordinate reference subtemplate, but the latter is not a subtemplate of the former, even indirectly.");
	}
	*/
}

data_entry_history *map_template::get_history_base()
{
	return this->history.get();
}

void map_template::reset_history()
{
	this->history = std::make_unique<map_template_history>(this->is_active_by_default());
}

void map_template::reset_game_data()
{
	this->current_map_start_pos = QPoint(-1, -1);
	this->current_map_end_pos = QPoint(-1, -1);
	this->current_start_pos = QPoint(0, 0);
}

void map_template::apply_terrain(const QPoint &template_start_pos, const QPoint &map_start_pos, const int z)
{
	this->load_terrain(false);
	this->load_terrain(true);
	this->load_trade_route_image();

	this->apply_terrain(false, template_start_pos, map_start_pos, z);
	this->apply_terrain(true, template_start_pos, map_start_pos, z);

	this->apply_tile_terrains(false, template_start_pos, map_start_pos, z);
	this->apply_tile_terrains(true, template_start_pos, map_start_pos, z);
}

void map_template::apply_terrain(const bool overlay, const QPoint &template_start_pos, const QPoint &map_start_pos, const int z)
{
	const QImage &terrain_image = overlay ? this->overlay_terrain_image : this->terrain_image;
	if (!terrain_image.isNull()) {
		this->apply_terrain_image(overlay, template_start_pos, map_start_pos, z);
	}
}

void map_template::apply_terrain_image(const bool overlay, const QPoint &template_start_pos, const QPoint &map_start_pos, const int z)
{
	const QImage &terrain_image = overlay ? this->overlay_terrain_image : this->terrain_image;

	if (terrain_image.size() != this->get_size()) {
		throw std::runtime_error("The terrain data for map template \"" + this->get_identifier() + "\" has a different size " + size::to_string(terrain_image.size()) + " than that of the map template itself " + size::to_string(this->get_size()) + ".");
	}

	for (int y = 0; y < terrain_image.height(); ++y) {
		if (y < template_start_pos.y() || y >= (template_start_pos.y() + CMap::get()->Info->MapHeights[z])) {
			continue;
		}
		
		if (this->get_end_pos().y() != -1 && y > this->get_end_pos().y()) {
			break;
		}

		for (int x = 0; x < terrain_image.width(); ++x) {
			if (x < template_start_pos.x() || x >= (template_start_pos.x() + CMap::get()->Info->MapWidths[z])) {
				continue;
			}

			if (this->get_end_pos().x() != -1 && x > this->get_end_pos().x()) {
				break;
			}

			const QColor color = terrain_image.pixelColor(x, y);
			
			if (color.alpha() == 0) { //transparent pixels mean leaving the area as it is (e.g. if it is a subtemplate use the main template's terrain for this tile instead)
				continue;
			}

			const terrain_type *terrain = nullptr;
			const terrain_feature *terrain_feature = terrain_feature::try_get_by_color(color);
			if (terrain_feature != nullptr) {
				terrain = terrain_feature->get_terrain_type();
			} else {
				terrain = terrain_type::try_get_by_color(color);

				if (!this->terrain_substitutions.empty()) {
					const auto find_iterator = this->terrain_substitutions.find(terrain);
					if (find_iterator != this->terrain_substitutions.end()) {
						terrain = find_iterator->second;
					}
				}
			}

			const Vec2i real_pos(map_start_pos.x() + (x - template_start_pos.x()), map_start_pos.y() + (y - template_start_pos.y()));

			if (!CMap::get()->Info->IsPointOnMap(real_pos, z)) {
				continue;
			}

			if (!this->is_map_pos_usable(real_pos)) {
				continue;
			}

			tile *tile = CMap::get()->Field(real_pos, z);

			if (terrain != nullptr) {
				//if we are setting base terrain and the tile already has an overlay terrain set to it, remove it, so that when subtemplates are applied on top of base terrain they remove overlays (e.g. space terrain)
				if (!overlay && tile->get_overlay_terrain() != nullptr) {
					tile->RemoveOverlayTerrain();
				}

				if (terrain->is_overlay() && tile->get_terrain() == nullptr) {
					//if we are applying an overlay terrain type, and the tile has no base terrain, check if we have a default base terrain type defined for it for this map template
					const terrain_type *base_terrain = this->get_default_base_terrain(terrain);
					if (base_terrain != nullptr) {
						tile->SetTerrain(base_terrain);
					}
				}

				tile->SetTerrain(terrain);

				if (terrain_feature != nullptr) {
					tile->set_terrain_feature(terrain_feature);
				}
			} else {
				if (terrain_feature == nullptr && (color != terrain_type::none_color || !overlay)) {
					//fully black pixels represent areas in overlay terrain files that don't have any overlays
					throw std::runtime_error("Invalid map terrain: (" + std::to_string(x) + ", " + std::to_string(y) + ") (RGB: " + std::to_string(color.red()) + "/" + std::to_string(color.green()) + "/" + std::to_string(color.blue()) + ").");
				} else if (overlay && tile->get_overlay_terrain() != nullptr) { //fully black pixel or trade route on overlay terrain map = no overlay
					tile->RemoveOverlayTerrain();
				}
			}
		}
	}
}

void map_template::apply_territory_image(const QPoint &template_start_pos, const QPoint &map_start_pos, const int z) const
{
	const std::filesystem::path &territory_filepath = this->get_territory_file();
	
	if (territory_filepath.empty()) {
		return;
	}
	
	if (!std::filesystem::exists(territory_filepath)) {
		throw std::runtime_error("File \"" + territory_filepath.string() + "\" not found.");
	}

	const QImage territory_image(path::to_qstring(territory_filepath));
	
	for (int y = 0; y < territory_image.height(); ++y) {
		if (y < template_start_pos.y()) {
			continue;
		}
		
		if (y >= (template_start_pos.y() + CMap::get()->Info->MapHeights[z])) {
			break;
		}

		if (this->get_end_pos().y() != -1 && y > this->get_end_pos().y()) {
			break;
		}

		for (int x = 0; x < territory_image.width(); ++x) {
			if (x < template_start_pos.x()) {
				continue;
			}

			if (x >= (template_start_pos.x() + CMap::get()->Info->MapWidths[z])) {
				break;
			}

			if (this->get_end_pos().x() != -1 && x > this->get_end_pos().x()) {
				break;
			}

			const QColor color = territory_image.pixelColor(x, y);
			
			if (color.alpha() == 0) { //transparent pixels mean leaving the tile as it is
				continue;
			}

			site *settlement = site::get_by_color(color);
			const QPoint real_pos(map_start_pos.x() + (x - template_start_pos.x()), map_start_pos.y() + (y - template_start_pos.y()));

			if (!CMap::get()->Info->IsPointOnMap(real_pos, z)) {
				continue;
			}

			if (!this->is_map_pos_usable(real_pos)) {
				continue;
			}

			CMap::get()->Field(real_pos, z)->set_settlement(settlement);
		}
	}
}

void map_template::apply(const QPoint &template_start_pos, const QPoint &map_start_pos, const int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	if (template_start_pos.x() < 0 || template_start_pos.x() >= this->get_width() || template_start_pos.y() < 0 || template_start_pos.y() >= this->get_height()) {
		throw std::runtime_error("Invalid map coordinate for map template \"" + this->get_identifier() + "\": " + point::to_string(template_start_pos) + ".");
	}

	const QSize applied_size = this->get_applied_size();

	this->current_map_start_pos = map_start_pos;
	this->current_map_end_pos = map_start_pos + size::to_point(applied_size) - QPoint(1, 1);
	this->current_start_pos = template_start_pos;

	const campaign *current_campaign = game::get()->get_current_campaign();
	
	if (z >= (int) CMap::get()->MapLayers.size()) {
		int width = std::min(applied_size.width(), CMap::get()->Info->MapWidth);
		int height = std::min(applied_size.height(), CMap::get()->Info->MapHeight);
		if (current_campaign != nullptr && z < static_cast<int>(current_campaign->MapSizes.size())) {
			//applies the map size set for the campaign for this map layer; for the first map layer that is already Map.Info.Width/Height, so it isn't necessary here
			width = current_campaign->MapSizes[z].x;
			height = current_campaign->MapSizes[z].y;
		}
	
		auto map_layer = std::make_unique<CMapLayer>(width, height);

		if (QApplication::instance()->thread() != QThread::currentThread()) {
			map_layer->moveToThread(QApplication::instance()->thread());
		}

		map_layer->ID = CMap::get()->MapLayers.size();
		CMap::get()->Info->MapWidths.push_back(map_layer->get_width());
		CMap::get()->Info->MapHeights.push_back(map_layer->get_height());
		map_layer->world = this->get_world();
		CMap::get()->MapLayers.push_back(std::move(map_layer));
	} else {
		if (!this->IsSubtemplateArea()) {
			CMap::get()->MapLayers[z]->world = this->get_world();
		}
	}

	if (!this->IsSubtemplateArea()) {
		if (!CEditor::get()->is_running()) {
			if (this->get_world() != nullptr && this->get_world()->get_season_schedule() != nullptr) {
				CMap::get()->MapLayers[z]->set_season_schedule(this->get_world()->get_season_schedule());
			} else {
				CMap::get()->MapLayers[z]->set_season_schedule(defines::get()->get_default_season_schedule());
			}
			
			CMap::get()->MapLayers[z]->SetSeasonByHours(game::get()->get_current_total_hours());
			
			CMap::get()->MapLayers[z]->set_time_of_day_schedule(nullptr);
			CMap::get()->MapLayers[z]->SetTimeOfDay(nullptr);
			
			if (!GameSettings.Inside) {
				if (this->get_world() != nullptr && this->get_world()->get_time_of_day_schedule() != nullptr) {
					CMap::get()->MapLayers[z]->set_time_of_day_schedule(this->get_world()->get_time_of_day_schedule());
				} else {
					CMap::get()->MapLayers[z]->set_time_of_day_schedule(defines::get()->get_default_time_of_day_schedule());
				}
				
				CMap::get()->MapLayers[z]->SetTimeOfDayByHours(game::get()->get_current_total_hours());
			}
		}
	}
	
	const QPoint map_end(std::min(CMap::get()->Info->MapWidths[z], map_start_pos.x() + applied_size.width()), std::min(CMap::get()->Info->MapHeights[z], map_start_pos.y() + applied_size.height()));
	if (!CMap::get()->Info->IsPointOnMap(map_start_pos, z)) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), map_start_pos.x(), map_start_pos.y());
		return;
	}
	
	const bool has_base_map = !this->get_terrain_file().empty();
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Terrain..."), this->get_name().c_str());
	
	if (this->get_base_terrain_type() != nullptr || this->get_border_terrain_type() != nullptr || this->clear_terrain) {
		for (int x = map_start_pos.x(); x < map_end.x(); ++x) {
			for (int y = map_start_pos.y(); y < map_end.y(); ++y) {
				terrain_type *terrain = nullptr;
				terrain_type *overlay_terrain = nullptr;

				if (this->get_border_terrain_type() != nullptr && (x == map_start_pos.x() || y == map_start_pos.y() || x == (map_end.x() - 1) || y == (map_end.y() - 1))) {
					terrain = this->get_border_terrain_type();
					overlay_terrain = this->get_border_overlay_terrain_type();
				} else if (this->get_base_terrain_type() != nullptr) {
					terrain = this->get_base_terrain_type();
					overlay_terrain = this->get_base_overlay_terrain_type();
				} else if (!this->clear_terrain) {
					continue;
				}

				Vec2i tile_pos(x, y);

				if (!this->is_map_pos_usable(tile_pos)) {
					continue;
				}

				CMap::get()->Field(tile_pos, z)->SetTerrain(terrain);
				
				if (overlay_terrain != nullptr) {
					CMap::get()->Field(tile_pos, z)->SetTerrain(overlay_terrain);
				} else {
					CMap::get()->Field(tile_pos, z)->RemoveOverlayTerrain();
				}
			}
		}
	}
	
	if (this->get_unusable_area_terrain_type() != nullptr) {
		for (int x = map_start_pos.x(); x < map_end.x(); ++x) {
			for (int y = map_start_pos.y(); y < map_end.y(); ++y) {
				const QPoint tile_pos(x, y);
				if (this->is_map_pos_usable(tile_pos)) {
					continue;
				}

				if (CMap::get()->is_point_in_a_subtemplate_area(tile_pos, z)) {
					//already a part of a different subtemplate area, so don't change it
					continue;
				}

				CMap::get()->Field(tile_pos, z)->SetTerrain(this->get_unusable_area_terrain_type());
				
				if (this->get_unusable_area_overlay_terrain_type() != nullptr) {
					CMap::get()->Field(tile_pos, z)->SetTerrain(this->get_unusable_area_overlay_terrain_type());
				} else {
					CMap::get()->Field(tile_pos, z)->RemoveOverlayTerrain();
				}
			}
		}
	}
	
	try {
		this->apply_terrain(template_start_pos, map_start_pos, z);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to apply terrain for map template \"" + this->get_identifier() + "\"."));
	}
	
	if (current_campaign) {
		for (size_t i = 0; i < HistoricalTerrains.size(); ++i) {
			Vec2i history_pos = std::get<0>(HistoricalTerrains[i]);
			if (history_pos.x < template_start_pos.x() || history_pos.x >= (template_start_pos.x() + CMap::get()->Info->MapWidths[z]) || history_pos.y < template_start_pos.y() || history_pos.y >= (template_start_pos.y() + CMap::get()->Info->MapHeights[z])) {
				continue;
			}
			if (current_campaign->get_start_date() >= std::get<2>(HistoricalTerrains[i]) || std::get<2>(HistoricalTerrains[i]).Year == 0) {
				terrain_type *historical_terrain = std::get<1>(HistoricalTerrains[i]);
				
				Vec2i real_pos(map_start_pos.x() + history_pos.x - template_start_pos.x(), map_start_pos.y() + history_pos.y - template_start_pos.y());

				if (!CMap::get()->Info->IsPointOnMap(real_pos, z)) {
					continue;
				}

				if (historical_terrain) {
					if (historical_terrain->is_overlay() && historical_terrain->is_pathway() && !CMap::get()->Field(real_pos, z)->has_flag(tile_flag::land_allowed)) {
						continue;
					}
					CMap::get()->Field(real_pos, z)->SetTerrain(historical_terrain);
				} else { //if the terrain type is null, then that means a previously set overlay terrain should be removed
					CMap::get()->Field(real_pos, z)->RemoveOverlayTerrain();
				}
			}
		}
	}

	if (this->IsSubtemplateArea() && this->get_surrounding_terrain_type() != nullptr) {
		const QPoint surrounding_start_pos(map_start_pos - QPoint(1, 1));
		const QPoint surrounding_end(map_end + QPoint(1, 1));
		for (int x = surrounding_start_pos.x(); x < surrounding_end.x(); ++x) {
			for (int y = surrounding_start_pos.y(); y < surrounding_end.y(); y += (surrounding_end.y() - surrounding_start_pos.y() - 1)) {
				const QPoint surrounding_pos(x, y);
				if (!CMap::get()->Info->IsPointOnMap(surrounding_pos, z) || CMap::get()->is_point_in_a_subtemplate_area(surrounding_pos, z)) {
					continue;
				}
				CMap::get()->Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_terrain_type());
				if (this->get_surrounding_overlay_terrain_type() != nullptr) {
					CMap::get()->Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_overlay_terrain_type());
				}
			}
		}
		for (int x = surrounding_start_pos.x(); x < surrounding_end.x(); x += (surrounding_end.x() - surrounding_start_pos.x() - 1)) {
			for (int y = surrounding_start_pos.y(); y < surrounding_end.y(); ++y) {
				const QPoint surrounding_pos(x, y);
				if (!CMap::get()->Info->IsPointOnMap(surrounding_pos, z) || CMap::get()->is_point_in_a_subtemplate_area(surrounding_pos, z)) {
					continue;
				}
				CMap::get()->Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_terrain_type());
				if (this->get_surrounding_overlay_terrain_type() != nullptr) {
					CMap::get()->Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_overlay_terrain_type());
				}
			}
		}
	}
	
	if (current_campaign) {
		const faction *current_faction = current_campaign->get_faction();
		if (current_faction != nullptr && !this->IsSubtemplateArea() && CPlayer::GetThisPlayer()->get_faction() != current_faction) {
			CPlayer::GetThisPlayer()->set_civilization(current_faction->get_civilization());
			CPlayer::GetThisPlayer()->SetFaction(current_faction);
			CPlayer::GetThisPlayer()->set_resource(defines::get()->get_wealth_resource(), 2500); // give the player enough resources to start up
			CPlayer::GetThisPlayer()->set_resource(resource::get_all()[WoodCost], 2500);
			CPlayer::GetThisPlayer()->set_resource(resource::get_all()[StoneCost], 2500);
		}
	}
	
	if (!this->get_subtemplates().empty()) {
		ShowLoadProgress(_("Applying \"%s\" Subtemplates..."), this->get_name().c_str());
		this->apply_subtemplates(template_start_pos, map_start_pos, map_end, z, false, false);
	}
	
	if (!this->IsSubtemplateArea()) {
		ShowLoadProgress(_("Adjusting \"%s\" Map Template Terrain..."), this->get_name().c_str());
		CMap::get()->AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::get()->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		CMap::get()->AdjustTileMapTransitions(map_start_pos, map_end, z);
		CMap::get()->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Units..."), this->get_name().c_str());

	for (std::map<std::pair<int, int>, std::tuple<unit_type *, int, unique_item *>>::const_iterator iterator = this->Resources.begin(); iterator != this->Resources.end(); ++iterator) {
		Vec2i unit_raw_pos(iterator->first.first, iterator->first.second);
		Vec2i unit_pos(map_start_pos.x() + unit_raw_pos.x - template_start_pos.x(), map_start_pos.y() + unit_raw_pos.y - template_start_pos.y());
		if (!CMap::get()->Info->IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		const unit_type *type = std::get<0>(iterator->second);
		
		Vec2i unit_offset((type->get_tile_size() - QSize(1, 1)) / 2);
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::get()->Info->IsPointOnMap(unit_pos - unit_offset, z) && CMap::get()->Info->IsPointOnMap(unit_pos - unit_offset + Vec2i(type->get_tile_size() - QSize(1, 1)), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
		}

		CUnit *unit = CreateResourceUnit(unit_pos - unit_offset, *type, z);
		
		if (std::get<1>(iterator->second)) {
			unit->SetResourcesHeld(std::get<1>(iterator->second));
			unit->Variable[GIVERESOURCE_INDEX].Value = std::get<1>(iterator->second);
			unit->Variable[GIVERESOURCE_INDEX].Max = std::get<1>(iterator->second);
			unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}
		
		if (std::get<2>(iterator->second)) {
			unit->set_unique(std::get<2>(iterator->second));
		}
	}

	if (!this->get_territory_file().empty()) {
		this->apply_territory_image(template_start_pos, map_start_pos, z);
	}

	if (current_campaign != nullptr) {
		this->ApplyConnectors(template_start_pos, map_start_pos, map_end, z);
	}
	this->apply_sites(template_start_pos, map_start_pos, map_end, z);
	this->ApplyUnits(template_start_pos, map_start_pos, map_end, z);

	if (!this->get_subtemplates().empty()) {
		ShowLoadProgress(_("Applying \"%s\" Random Subtemplates..."), this->get_name().c_str());
		this->apply_subtemplates(template_start_pos, map_start_pos, map_end, z, true, false);
	}

	if (!this->IsSubtemplateArea()) {
		CMap::get()->generate_missing_terrain(QRect(map_start_pos, map_end - QPoint(1, 1)), z);
	}

	ShowLoadProgress(_("Generating \"%s\" Map Template Random Terrain..."), this->get_name().c_str());
	for (const auto &generated_terrain : this->generated_terrains) {
		CMap::get()->generate_terrain(generated_terrain.get(), map_start_pos, map_end - Vec2i(1, 1), has_base_map, z);
	}

	if (!this->IsSubtemplateArea()) {
		ShowLoadProgress(_("Readjusting \"%s\" Map Template Terrain..."), this->get_name().c_str());
		CMap::get()->AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::get()->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		CMap::get()->AdjustTileMapTransitions(map_start_pos, map_end, z);
		CMap::get()->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}

	if (!this->get_subtemplates().empty()) {
		ShowLoadProgress(_("Applying \"%s\" Constructed Subtemplates..."), this->get_name().c_str());
		this->apply_subtemplates(template_start_pos, map_start_pos, map_end, z, false, true);
		this->apply_subtemplates(template_start_pos, map_start_pos, map_end, z, true, true);
	}

	ShowLoadProgress(_("Generating \"%s\" Map Template Random Units..."), this->get_name().c_str());

	// now, generate the units and heroes that were set to be generated at a random position (by having their position set to {-1, -1})
	if (current_campaign != nullptr) {
		this->ApplyConnectors(template_start_pos, map_start_pos, map_end, z, true);
	}
	this->apply_sites(template_start_pos, map_start_pos, map_end, z, true);
	this->ApplyUnits(template_start_pos, map_start_pos, map_end, z, true);

	for (int i = 0; i < PlayerMax; ++i) {
		CPlayer *player = CPlayer::Players[i].get();

		if (player->get_type() != player_type::person && player->get_type() != player_type::computer && player->get_type() != player_type::rescue_active) {
			continue;
		}
		if (CMap::get()->is_point_in_a_subtemplate_area(player->StartPos, z)) {
			continue;
		}
		if (player->StartPos.x < map_start_pos.x() || player->StartPos.y < map_start_pos.y() || player->StartPos.x >= map_end.x() || player->StartPos.y >= map_end.y() || player->StartMapLayer != z) {
			continue;
		}
		if (player->StartPos.x == 0 && player->StartPos.y == 0) {
			continue;
		}

		// add five workers at the player's starting location
		if (player->NumTownHalls > 0) {
			const unit_type *worker_type = player->get_faction()->get_class_unit_type(unit_class::get("worker"));
			if (worker_type != nullptr && player->GetUnitTypeCount(worker_type) == 0) { //only create if the player doesn't have any workers created in another manner
				const Vec2i worker_unit_offset((worker_type->get_tile_size() - QSize(1, 1)) / 2);
				
				Vec2i worker_pos(player->StartPos);

				const CUnit *worker_town_hall = nullptr;
				std::vector<CUnit *> table;
				Select(worker_pos - Vec2i(4, 4), worker_pos + Vec2i(4, 4), table, z, HasSamePlayerAs(*player));
				for (size_t j = 0; j < table.size(); ++j) {
					if (table[j]->Type->BoolFlag[TOWNHALL_INDEX].value) {
						worker_town_hall = table[j];
						break;
					}
				}
				
				if (worker_town_hall == nullptr) {
					//if the start pos doesn't have a town hall, create the workers in the position of a town hall the player has
					for (int j = 0; j < player->GetUnitCount(); ++j) {
						CUnit *town_hall_unit = &player->GetUnit(j);

						if (!town_hall_unit->Type->BoolFlag[TOWNHALL_INDEX].value) {
							continue;
						}

						if (town_hall_unit->MapLayer->ID != z) {
							continue;
						}

						worker_pos = town_hall_unit->tilePos;
						worker_town_hall = town_hall_unit;
					}
				}
				
				for (int j = 0; j < 5; ++j) {
					CreateUnit(worker_pos - worker_unit_offset, *worker_type, player, player->StartMapLayer, false, worker_town_hall->settlement);
				}
			}
		}
		
		if (player->NumTownHalls > 0 || player == CPlayer::GetThisPlayer()) {
			for (size_t j = 0; j < this->PlayerLocationGeneratedNeutralUnits.size(); ++j) {
				CMap::get()->generate_neutral_units(this->PlayerLocationGeneratedNeutralUnits[j].first, this->PlayerLocationGeneratedNeutralUnits[j].second, player->StartPos - QPoint(8, 8), player->StartPos + QPoint(8, 8), true, z);
			}
		}
	}
	
	for (size_t i = 0; i < this->GeneratedNeutralUnits.size(); ++i) {
		const bool grouped = this->GeneratedNeutralUnits[i].first->get_given_resource() != nullptr && this->GeneratedNeutralUnits[i].first->get_tile_width() == 1 && this->GeneratedNeutralUnits[i].first->get_tile_height() == 1; // group small resources
		CMap::get()->generate_neutral_units(this->GeneratedNeutralUnits[i].first, this->GeneratedNeutralUnits[i].second, map_start_pos, map_end - Vec2i(1, 1), grouped, z);
	}

	if (!this->IsSubtemplateArea()) {
		CMap::get()->adjust_territory_irregularities(map_start_pos, map_end - QPoint(1, 1), z);
	}

	//this has to be done at the end, so that it doesn't prevent the application from working properly, due to the map template code thinking that its own area belongs to another map template
	const QRect map_rect(map_start_pos, map_end - Vec2i(1, 1));

	if (this->IsSubtemplateArea()) {
		CMap::get()->MapLayers[z]->subtemplate_areas[this] = map_rect;

		//if this is the top subtemplate for a given world, set the world's map rect to this map template's map rect
		if (this->get_world() != nullptr && this->get_world() != this->get_main_template()->get_world()) {
			this->get_world()->get_game_data()->set_map_rect(map_rect, CMap::get()->MapLayers[z].get());
		}
	} else {
		if (this->get_world() != nullptr) {
			this->get_world()->get_game_data()->set_map_rect(map_rect, CMap::get()->MapLayers[z].get());
		}
	}

	this->clear_application_data();
}

/**
**	@brief	Apply the subtemplates to the map
**
**	@param	template_start_pos	The start position of the map relative to the map template
**	@param	map_start_pos		The start position of the map template relative to the map
**	@param	z					The map layer
**	@param	random				Whether it is subtemplates with a random position that should be applied, or ones with a fixed one
*/
void map_template::apply_subtemplates(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random, bool constructed) const
{
	std::vector<map_template *> subtemplates;
	std::vector<map_template *> optional_subtemplates;

	for (map_template *subtemplate : this->get_subtemplates()) {
		if (!subtemplate->history->is_active()) {
			continue;
		}

		if (constructed != subtemplate->is_constructed_only()) {
			continue;
		}

		if (CMap::get()->is_subtemplate_on_map(subtemplate)) {
			continue;
		}

		const bool optional = subtemplate->is_optional_for_campaign(game::get()->get_current_campaign());
		if (optional) {
			optional_subtemplates.push_back(subtemplate);
		} else {
			subtemplates.push_back(subtemplate);
		}
	}

	//make it so optional subtemplates are processed in random order, so that we get different ones applied on different playthroughs
	vector::shuffle(optional_subtemplates);
	vector::merge(subtemplates, std::move(optional_subtemplates));

	for (map_template *subtemplate : subtemplates) {
		this->apply_subtemplate(subtemplate, template_start_pos, map_start_pos, map_end, z, random);
	}
}

void map_template::apply_subtemplate(map_template *subtemplate, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	if (CMap::get()->get_subtemplate_pos(subtemplate) != QPoint(-1, -1)) {
		//already applied
		return;
	}

	QPoint subtemplate_pos = subtemplate->get_subtemplate_top_left_pos();
	bool found_location = false;

	if (subtemplate->UpperTemplate != nullptr && (subtemplate_pos.x() < 0 || subtemplate_pos.y() < 0)) { //if has no given position, but has an upper template, use its coordinates instead
		subtemplate_pos = CMap::get()->get_subtemplate_pos(subtemplate->UpperTemplate);
		if (subtemplate_pos.x() >= 0 && subtemplate_pos.y() >= 0) {
			found_location = true;
		}
	}

	if (subtemplate->LowerTemplate != nullptr && (subtemplate_pos.x() < 0 || subtemplate_pos.y() < 0)) { //if has no given position, but has a lower template, use its coordinates instead
		subtemplate_pos = CMap::get()->get_subtemplate_pos(subtemplate->LowerTemplate);
		if (subtemplate_pos.x() >= 0 && subtemplate_pos.y() >= 0) {
			found_location = true;
		}
	}

	const bool optional = subtemplate->is_optional_for_campaign(game::get()->get_current_campaign());

	if (!found_location) {
		if (subtemplate_pos.x() < 0 || subtemplate_pos.y() < 0) {
			if (!random) {
				return;
			}

			QPoint max_adjacent_template_distance = map_template::max_adjacent_template_distance;
			while (subtemplate_pos.x() < 0 || subtemplate_pos.y() < 0) {
				bool adjacency_restriction_occurred = false;
				subtemplate_pos = this->generate_subtemplate_position(subtemplate, template_start_pos, map_start_pos, map_end, z, max_adjacent_template_distance, adjacency_restriction_occurred);

				if (adjacency_restriction_occurred) {
					//double the maximum adjacent template distance for the next try
					max_adjacent_template_distance.setX(max_adjacent_template_distance.x() * 2);
					max_adjacent_template_distance.setY(max_adjacent_template_distance.y() * 2);
				} else {
					break;
				}
			}

			if (subtemplate_pos.x() < 0 || subtemplate_pos.y() < 0) {
				if (!optional) {
					throw std::runtime_error("No location available for map template \"" + subtemplate->get_identifier() + "\" to be applied to.");
				}
			} else {
				found_location = true;
			}
		} else {
			if (random) {
				return;
			}
			subtemplate_pos = map_start_pos + subtemplate_pos - template_start_pos;
			found_location = true;
		}
	} else {
		if (random) {
			return;
		}
	}

	if (found_location) {
		if (subtemplate_pos.x() >= 0 && subtemplate_pos.y() >= 0 && subtemplate_pos.x() < CMap::get()->Info->MapWidths[z] && subtemplate_pos.y() < CMap::get()->Info->MapHeights[z]) {
			try {
				subtemplate->apply(subtemplate->get_start_pos(), subtemplate_pos, z);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to apply map subtemplate \"" + subtemplate->get_identifier() + "\"."));
			}

			//also apply all dependent adjacent templates, if the other templates they depend on have also been applied, so that they are more likely to be close to this subtemplate
			for (map_template *dependent_subtemplate : subtemplate->dependent_adjacent_templates) {
				if (CMap::get()->is_subtemplate_on_map(dependent_subtemplate)) {
					continue;
				}

				bool other_dependencies_fulfilled = true;
				for (const map_template *dependency_template : dependent_subtemplate->dependency_templates) {
					if (!CMap::get()->is_subtemplate_on_map(dependency_template)) {
						other_dependencies_fulfilled = false;
						break;
					}
				}

				if (!other_dependencies_fulfilled) {
					continue;
				}

				this->apply_subtemplate(dependent_subtemplate, template_start_pos, map_start_pos, map_end, z, random);
			}
		}
	} else {
		//data could have been loaded for the purpose of finding an appropriate location for the subtemplate, e.g. terrain image data
		subtemplate->clear_application_data();

		if (!optional) {
			throw std::runtime_error("Failed to find a location for map subtemplate \"" + subtemplate->get_identifier() + "\".");
		}
	}
}

void map_template::apply_sites(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	const campaign *current_campaign = game::get()->get_current_campaign();
	CDate start_date;
	if (current_campaign != nullptr) {
		start_date = current_campaign->get_start_date();
	}

	for (const site *site : this->sites) {
		const QPoint site_raw_pos = site->get_pos();
		QPoint site_pos(map_start_pos + site_raw_pos - template_start_pos);

		QPoint unit_offset(0, 0);

		const unit_type *base_unit_type = site->get_base_unit_type();

		if (base_unit_type != nullptr) {
			unit_offset = base_unit_type->get_tile_center_pos_offset();
		}

		if (random) {
			if (site_raw_pos.x() != -1 || site_raw_pos.y() != -1) {
				continue;
			}

			if (base_unit_type != nullptr) {
				try {
					if (site->orbits_map_template()) {
						site_pos = this->generate_celestial_site_position(site, z);
					} else {
						site_pos = CMap::get()->generate_unit_location(base_unit_type, nullptr, map_start_pos, map_end - QPoint(1, 1), z);
						site_pos += unit_offset;
					}
				} catch (...) {
					std::throw_with_nested(std::runtime_error("Failed to generate position for site \"" + site->get_identifier() + "\"."));
				}
			}
		} else {
			if (site_raw_pos.x() == -1 && site_raw_pos.y() == -1) {
				continue;
			}

			//as a special case for fixed space sites (e.g. stars), don't apply them if they would be applied on a pre-existing subtemplate area, so that e.g. if stars would be applied where the Nidavellir terrain circle is, they don't get applied, instead of being applied on the nearest available space (which would leave Nidavellir encircled with stars)
			if (base_unit_type != nullptr && base_unit_type->get_domain() == unit_domain::space) {
				const QPoint top_left_pos = site_pos - unit_offset;
				const QPoint bottom_right_pos = top_left_pos + size::to_point(base_unit_type->get_tile_size()) - QPoint(1, 1);
				if (CMap::get()->is_rect_in_a_subtemplate_area(QRect(top_left_pos, bottom_right_pos), z)) {
					continue;
				}
			}
		}

		if (!CMap::get()->Info->IsPointOnMap(site_pos, z) || site_pos.x() < map_start_pos.x() || site_pos.y() < map_start_pos.y()) {
			continue;
		}

		if (!this->is_map_pos_usable(site_pos)) {
			continue;
		}

		try {
			this->apply_site(site, site_pos, z);
		} catch (...) {
			std::throw_with_nested(std::runtime_error("Failed to apply site \"" + site->get_identifier() + "\"."));
		}
	}
}

void map_template::apply_site(const site *site, const QPoint &site_pos, const int z) const
{
	const unit_type *base_unit_type = site->get_base_unit_type();

	Vec2i unit_offset(0, 0);
	if (base_unit_type != nullptr) {
		unit_offset = (base_unit_type->get_tile_size() - QSize(1, 1)) / 2;
	}

	site_game_data *site_game_data = site->get_game_data();

	site_game_data->set_map_pos(site_pos);
	site_game_data->set_map_layer(CMap::get()->MapLayers[z].get());

	const wyrmgus::site *settlement = site->is_settlement() ? site : CMap::get()->Field(site_pos, z)->get_settlement();

	//it is acceptable sites with geocoordinate to have their positions shifted, e.g. if it was coastal to shift it enough inland to give space for the building to be placed
	const bool is_position_shift_acceptable = !site->get_geocoordinate().is_null() || !site->get_astrocoordinate().is_null();

	bool first_building = true;

	if (base_unit_type != nullptr) {
		if (!is_position_shift_acceptable && !UnitTypeCanBeAt(*base_unit_type, site_pos - unit_offset, z) && CMap::get()->Info->IsPointOnMap(site_pos - unit_offset, z) && CMap::get()->Info->IsPointOnMap(site_pos - unit_offset + Vec2i(base_unit_type->get_tile_size() - QSize(1, 1)), z)) {
			fprintf(stderr, "The site for \"%s\" should be placed on (%d, %d), but it cannot be there.\n", site->Ident.c_str(), site->get_pos().x(), site->get_pos().y());
		}

		const bool no_bordering_impassable = is_position_shift_acceptable;

		CUnit *unit = CreateUnit(site_pos - unit_offset, *base_unit_type, CPlayer::get_neutral_player(), z, no_bordering_impassable, settlement);
		unit->set_site(site);

		if (site->is_settlement()) {
			unit->settlement = site;
		} else {
			first_building = false;
		}

		if (site->is_settlement()) {
			CMap::get()->add_settlement_unit(unit);
			for (int x = unit->tilePos.x; x < (unit->tilePos.x + unit->Type->get_tile_width()); ++x) {
				for (int y = unit->tilePos.y; y < (unit->tilePos.y + unit->Type->get_tile_height()); ++y) {
					const QPoint tile_pos(x, y);
					CMap::get()->Field(tile_pos, z)->set_settlement(unit->settlement);
				}
			}
		}

		if (site->get_mass() != 0 && base_unit_type->get_default_mass() != 0 && site->get_mass() != base_unit_type->get_default_mass() && unit->ResourcesHeld != 0) {
			//change the unit's resources held depending on the difference between its mass and the default mass of its base unit type
			const int mass_multiplier = resource::get_mass_multiplier(site->get_mass(), base_unit_type->get_default_mass());

			const int resource_quantity = unit->ResourcesHeld * mass_multiplier / 100;
			unit->SetResourcesHeld(resource_quantity);
			unit->Variable[GIVERESOURCE_INDEX].Value = resource_quantity;
			unit->Variable[GIVERESOURCE_INDEX].Max = resource_quantity;
			unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}

		//if the site is a connector, and the destination site has already been applied, establish the connection
		if (site->get_connection_destination() != nullptr) {
			CMap::get()->MapLayers[z]->LayerConnectors.push_back(unit);

			wyrmgus::site_game_data *destination_site_game_data = site->get_connection_destination()->get_game_data();

			if (destination_site_game_data->get_site_unit() != nullptr) {
				destination_site_game_data->get_site_unit()->ConnectingDestination = unit;
				unit->ConnectingDestination = destination_site_game_data->get_site_unit();
			}
		}
	}

	const campaign *current_campaign = game::get()->get_current_campaign();
	CDate start_date;
	if (current_campaign != nullptr) {
		start_date = current_campaign->get_start_date();
	}

	for (size_t j = 0; j < site->HistoricalResources.size(); ++j) {
		if (
			(!current_campaign && std::get<1>(site->HistoricalResources[j]).Year == 0 && std::get<1>(site->HistoricalResources[j]).Year == 0)
			|| (
				current_campaign && start_date.ContainsDate(std::get<0>(site->HistoricalResources[j]))
				&& (!start_date.ContainsDate(std::get<1>(site->HistoricalResources[j])) || std::get<1>(site->HistoricalResources[j]).Year == 0)
				)
			) {
			const unit_type *type = std::get<2>(site->HistoricalResources[j]);
			if (!type) {
				fprintf(stderr, "Error in CMap::apply_sites (site ident \"%s\"): historical resource type is null.\n", site->Ident.c_str());
				continue;
			}
			const Vec2i resource_unit_offset((type->get_tile_size() - QSize(1, 1)) / 2);
			CUnit *unit = CreateResourceUnit(site_pos - resource_unit_offset, *type, z, false); // don't generate unique resources when setting special properties, since for map templates unique resources are supposed to be explicitly indicated
			if (std::get<3>(site->HistoricalResources[j])) {
				unit->set_unique(std::get<3>(site->HistoricalResources[j]));
			}
			int resource_quantity = std::get<4>(site->HistoricalResources[j]);
			if (resource_quantity) { //set the resource_quantity after setting the unique unit, so that unique resources can be decreased in quantity over time
				unit->SetResourcesHeld(resource_quantity);
				unit->Variable[GIVERESOURCE_INDEX].Value = resource_quantity;
				unit->Variable[GIVERESOURCE_INDEX].Max = resource_quantity;
				unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
			}
		}
	}

	if (!site->get_satellites().empty()) {
		int64_t orbit_distance = 0;
		if (site->get_base_unit_type() != nullptr) {
			orbit_distance += site->get_base_unit_type()->get_tile_width() / 2;
		}
		orbit_distance += site::base_orbit_distance;

		for (const wyrmgus::site *satellite : site->get_satellites()) {
			const QSize satellite_size = satellite->get_size_with_satellites();

			int64_t satellite_orbit_distance = orbit_distance;
			satellite_orbit_distance += satellite_size.width() / 2;
			satellite_orbit_distance += number::cbrt(satellite->get_distance_from_orbit_center()) / site::distance_from_orbit_center_divider;

			const QPoint satellite_pos = this->generate_site_orbit_position(satellite, z, satellite_orbit_distance);

			try {
				this->apply_site(satellite, satellite_pos, z);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to apply satellite site \"" + satellite->get_identifier() + "\"."));
			}
		}
	}

	if (current_campaign == nullptr) {
		return;
	}

	const site_history *site_history = site->get_history();

	const faction *site_owner = site_history->get_owner();
	if (site_owner == nullptr) {
		//fall back to the old historical owners functionality
		for (std::map<CDate, const faction *>::const_reverse_iterator owner_iterator = site->HistoricalOwners.rbegin(); owner_iterator != site->HistoricalOwners.rend(); ++owner_iterator) {
			if (start_date.ContainsDate(owner_iterator->first)) { // set the owner to the latest historical owner given the scenario's start date
				site_owner = owner_iterator->second;
				break;
			}
		}
	}

	if (site != settlement && settlement != nullptr) {
		const faction *settlement_owner = settlement->get_history()->get_owner();
		if (settlement_owner != nullptr && site_owner != settlement_owner && (site_owner == nullptr || !is_faction_type_neutral(site_owner->get_type()))) {
			//if the site owner is different from the settlement owner, the latter is non-null, and the site owner is not a neutral faction, then override the site owner with the settlement owner
			site_owner = settlement_owner;
		}
	}

	if (site_owner == nullptr) {
		return;
	}

	CPlayer *player = GetOrAddFactionPlayer(site_owner);

	if (player == nullptr) {
		return;
	}

	bool is_capital = site_owner->get_history()->get_capital() == site;
	if (!is_capital) {
		for (int i = ((int) site_owner->HistoricalCapitals.size() - 1); i >= 0; --i) {
			if (start_date.ContainsDate(site_owner->HistoricalCapitals[i].first) || site_owner->HistoricalCapitals[i].first.Year == 0) {
				if (site_owner->HistoricalCapitals[i].second == site->Ident) {
					is_capital = true;
				}
				break;
			}
		}
	}

	if ((player->StartPos.x == 0 && player->StartPos.y == 0) || is_capital) {
		player->SetStartView(site_pos, z);
	}

	const unit_type *pathway_type = nullptr;
	if (site_history->get_pathway_class() != nullptr) {
		pathway_type = site_owner->get_class_unit_type(site_history->get_pathway_class());
	}

	for (size_t j = 0; j < site->HistoricalBuildings.size(); ++j) {
		if (
			start_date.ContainsDate(std::get<0>(site->HistoricalBuildings[j]))
			&& (!start_date.ContainsDate(std::get<1>(site->HistoricalBuildings[j])) || std::get<1>(site->HistoricalBuildings[j]).Year == 0)
			) {
			unit_type *unit_type = site_owner->get_class_unit_type(std::get<2>(site->HistoricalBuildings[j]));
			if (unit_type == nullptr) {
				continue;
			}
			if (unit_type->TerrainType != nullptr && unit_type->TerrainType->is_pathway()) {
				pathway_type = unit_type;
			}
		}
	}

	bool built_ontop = false;

	for (const unit_class *building_class : site_history->get_building_classes()) {
		const unit_type *unit_type = site_owner->get_class_unit_type(building_class);

		if (unit_type == nullptr) {
			continue;
		}

		if (unit_type->TerrainType != nullptr) {
			throw std::runtime_error("A terrain type building (e.g. a wall) cannot be applied from the list of historical building classes of a site.");
		}

		if (unit_type->BoolFlag[TOWNHALL_INDEX].value && !site->is_settlement()) {
			throw std::runtime_error("Site \"" + site->get_identifier() + "\" has a town hall, but isn't set as a settlement one.");
		}

		const CBuildRestrictionOnTop *ontop = OnTopDetails(*unit_type, nullptr);
		if (ontop != nullptr && ontop->Parent != nullptr) {
			if (ontop->Parent != base_unit_type) {
				throw std::runtime_error("Site \"" + site->get_identifier() + "\" has a building (\"" + unit_type->get_identifier() + "\") which requires being built over a \"" + ontop->Parent->get_identifier() + "\", but the site does not have that as its base unit type.");
			}

			if (built_ontop) {
				throw std::runtime_error("Site \"" + site->get_identifier() + "\" has a building (\"" + unit_type->get_identifier() + "\") which requires being built over a \"" + ontop->Parent->get_identifier() + "\", but the site has already been built upon.");
			}

			built_ontop = true;
		}

		const QPoint building_unit_offset = unit_type->get_tile_center_pos_offset();
		if (!is_position_shift_acceptable && first_building) {
			if (ontop == nullptr && !UnitTypeCanBeAt(*unit_type, site_pos - building_unit_offset, z) && CMap::get()->Info->IsPointOnMap(site_pos - building_unit_offset, z) && CMap::get()->Info->IsPointOnMap(site_pos - building_unit_offset + size::to_point(unit_type->get_tile_size() - QSize(1, 1)), z)) {
				throw std::runtime_error("The \"" + unit_type->get_identifier() + "\" representing the minor site of \"" + site->get_identifier() + "\" should be placed on " + point::to_string(site->get_pos()) + ", but it cannot be there.");
			}
		}

		CUnit *unit = CreateUnit(site_pos - building_unit_offset, *unit_type, player, z, true, settlement);

		if (first_building) {
			if (base_unit_type == nullptr && !site->get_name().empty()) {
				//if one building is representing a site with no base unit type, make it have the site's name
				unit->Name = site->get_cultural_name(site_owner->get_civilization());
			}
			first_building = false;
		}

		if (unit_type->BoolFlag[TOWNHALL_INDEX].value) {
			unit->UpdateBuildingSettlementAssignment();
		}

		if (pathway_type != nullptr) {
			for (int x = unit->tilePos.x - 1; x < unit->tilePos.x + unit->Type->get_tile_width() + 1; ++x) {
				for (int y = unit->tilePos.y - 1; y < unit->tilePos.y + unit->Type->get_tile_height() + 1; ++y) {
					if (!CMap::get()->Info->IsPointOnMap(x, y, unit->MapLayer)) {
						continue;
					}
					tile &mf = *unit->MapLayer->Field(x, y);
					if (mf.has_flag(tile_flag::building)) {
						//this is a tile where the building itself is located, continue
						continue;
					}
					const QPoint pathway_pos(x, y);
					if (!UnitTypeCanBeAt(*pathway_type, pathway_pos, unit->MapLayer->ID)) {
						continue;
					}

					mf.SetTerrain(pathway_type->TerrainType);
				}
			}
		}
	}

	for (size_t j = 0; j < site->HistoricalBuildings.size(); ++j) {
		if (
			start_date.ContainsDate(std::get<0>(site->HistoricalBuildings[j]))
			&& (!start_date.ContainsDate(std::get<1>(site->HistoricalBuildings[j])) || std::get<1>(site->HistoricalBuildings[j]).Year == 0)
			) {
			const faction *building_owner = std::get<4>(site->HistoricalBuildings[j]);
			const unit_type *unit_type = nullptr;
			if (building_owner) {
				unit_type = building_owner->get_class_unit_type(std::get<2>(site->HistoricalBuildings[j]));
			} else {
				unit_type = site_owner->get_class_unit_type(std::get<2>(site->HistoricalBuildings[j]));
			}
			if (unit_type == nullptr) {
				continue;
			}
			if (unit_type->TerrainType) {
				continue;
			}
			if (unit_type->BoolFlag[TOWNHALL_INDEX].value && !site->is_settlement()) {
				fprintf(stderr, "Error in CMap::apply_sites (site ident \"%s\"): site has a town hall, but isn't set as a settlement one.\n", site->Ident.c_str());
				continue;
			}
			const Vec2i building_unit_offset((unit_type->get_tile_size() - QSize(1, 1)) / 2);
			if (!is_position_shift_acceptable && first_building) {
				if (!OnTopDetails(*unit_type, nullptr) && !UnitTypeCanBeAt(*unit_type, site_pos - building_unit_offset, z) && CMap::get()->Info->IsPointOnMap(site_pos - building_unit_offset, z) && CMap::get()->Info->IsPointOnMap(site_pos - building_unit_offset + Vec2i(unit_type->get_tile_size() - QSize(1, 1)), z)) {
					fprintf(stderr, "The \"%s\" representing the minor site of \"%s\" should be placed on (%d, %d), but it cannot be there.\n", unit_type->Ident.c_str(), site->Ident.c_str(), site->get_pos().x(), site->get_pos().y());
				}
			}
			CUnit *unit = nullptr;
			if (building_owner) {
				CPlayer *building_player = GetOrAddFactionPlayer(building_owner);
				if (!building_player) {
					continue;
				}
				if (building_player->StartPos.x == 0 && building_player->StartPos.y == 0) {
					building_player->SetStartView(site_pos - building_unit_offset, z);
				}
				unit = CreateUnit(site_pos - building_unit_offset, *unit_type, building_player, z, true, settlement);
			} else {
				unit = CreateUnit(site_pos - building_unit_offset, *unit_type, player, z, true, settlement);
			}
			if (std::get<3>(site->HistoricalBuildings[j])) {
				unit->set_unique(std::get<3>(site->HistoricalBuildings[j]));
			}
			if (first_building) {
				if (!unit_type->BoolFlag[TOWNHALL_INDEX].value && unit->get_unique() == nullptr && (!building_owner || building_owner == site_owner)) {
					//if one building is representing a minor site, make it have the site's name
					unit->Name = site->get_cultural_name(site_owner->get_civilization());
				}
				first_building = false;
			}
			if (unit_type->BoolFlag[TOWNHALL_INDEX].value && (!building_owner || building_owner == site_owner)) {
				unit->UpdateBuildingSettlementAssignment();
			}
			if (pathway_type != nullptr) {
				for (int x = unit->tilePos.x - 1; x < unit->tilePos.x + unit->Type->get_tile_width() + 1; ++x) {
					for (int y = unit->tilePos.y - 1; y < unit->tilePos.y + unit->Type->get_tile_height() + 1; ++y) {
						if (!CMap::get()->Info->IsPointOnMap(x, y, unit->MapLayer)) {
							continue;
						}
						tile &mf = *unit->MapLayer->Field(x, y);
						if (mf.has_flag(tile_flag::building)) {
							//this is a tile where the building itself is located, continue
							continue;
						}
						Vec2i pathway_pos(x, y);
						if (!UnitTypeCanBeAt(*pathway_type, pathway_pos, unit->MapLayer->ID)) {
							continue;
						}

						mf.SetTerrain(pathway_type->TerrainType);
					}
				}
			}
		}
	}

	int population = site_history->get_population();

	for (const auto &kv_pair : site_history->get_population_groups()) {
		const unit_class *unit_class = kv_pair.first;
		const int group_population = kv_pair.second;

		this->apply_population_unit(unit_class, group_population, site_pos, z, player, settlement);
		population -= group_population;
	}

	if (population != 0) { //remaining population after subtracting the amount of population specified to belong to particular groups
		const unit_class *population_class = defines::get()->get_default_population_class();
		if (base_unit_type != nullptr) {
			if (base_unit_type->get_domain() == unit_domain::water && player->get_class_unit_type(defines::get()->get_default_water_population_class()) != nullptr) {
				population_class = defines::get()->get_default_water_population_class();
			} else if (base_unit_type->get_domain() == unit_domain::space && player->get_class_unit_type(defines::get()->get_default_space_population_class()) != nullptr) {
				population_class = defines::get()->get_default_space_population_class();
			}
		}

		this->apply_population_unit(population_class, population, site_pos, z, player, settlement);
	}

	for (size_t j = 0; j < site->HistoricalUnits.size(); ++j) {
		if (
			start_date.ContainsDate(std::get<0>(site->HistoricalUnits[j]))
			&& (!start_date.ContainsDate(std::get<1>(site->HistoricalUnits[j])) || std::get<1>(site->HistoricalUnits[j]).Year == 0)
			) {
			int unit_quantity = std::get<3>(site->HistoricalUnits[j]);

			if (unit_quantity > 0) {
				const unit_type *type = std::get<2>(site->HistoricalUnits[j]);
				if (type->BoolFlag[ORGANIC_INDEX].value) {
					//each organic unit represents a certain amount of people
					unit_quantity = unit_quantity / defines::get()->get_population_per_unit();
				}

				CPlayer *unit_player = nullptr;
				const faction *unit_owner = std::get<4>(site->HistoricalUnits[j]);
				if (unit_owner) {
					unit_player = GetOrAddFactionPlayer(unit_owner);
					if (!unit_player) {
						continue;
					}
					if (unit_player->StartPos.x == 0 && unit_player->StartPos.y == 0) {
						unit_player->SetStartView(site_pos, z);
					}
				} else {
					unit_player = player;
				}
				const Vec2i historical_unit_offset((type->get_tile_size() - QSize(1, 1)) / 2);

				for (int k = 0; k < unit_quantity; ++k) {
					CUnit *unit = CreateUnit(site_pos - historical_unit_offset, *type, unit_player, z, false, settlement);
					if (!type->BoolFlag[HARVESTER_INDEX].value) { // make non-worker units not have an active AI
						unit->Active = 0;
						unit_player->ChangeUnitTypeAiActiveCount(type, -1);
					}
				}
			}
		}
	}
}

void map_template::apply_population_unit(const unit_class *unit_class, const int population, const QPoint &unit_pos, const int z, CPlayer *player, const site *settlement) const
{
	const unit_type *unit_type = player->get_class_unit_type(unit_class);

	if (unit_type == nullptr) {
		return;
	}

	const QPoint unit_offset = unit_type->get_tile_center_pos_offset();
	const QPoint unit_top_left_pos = unit_pos - unit_offset;

	const int unit_quantity = population / defines::get()->get_population_per_unit();
	for (int i = 0; i < unit_quantity; ++i) {
		CreateUnit(unit_top_left_pos, *unit_type, player, z, false, settlement);
	}
}

void map_template::ApplyConnectors(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	for (size_t i = 0; i < this->WorldConnectors.size(); ++i) {
		const unit_type *type = std::get<1>(this->WorldConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->WorldConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		Vec2i unit_offset((type->get_tile_size() - QSize(1, 1)) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::get()->generate_unit_location(type, nullptr, map_start_pos, map_end - QPoint(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::get()->Info->IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x() || unit_pos.y < map_start_pos.y()) {
			continue;
		}
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::get()->Info->IsPointOnMap(unit_pos - unit_offset, z) && CMap::get()->Info->IsPointOnMap(unit_pos - unit_offset + Vec2i(type->get_tile_size() - QSize(1, 1)), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, CPlayer::get_neutral_player(), z, true);
		if (std::get<3>(this->WorldConnectors[i])) {
			unit->set_unique(std::get<3>(this->WorldConnectors[i]));
		}
		CMap::get()->MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < CMap::get()->MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (CMap::get()->MapLayers[second_z]->world == std::get<2>(this->WorldConnectors[i])) {
				for (size_t j = 0; j < CMap::get()->MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (CMap::get()->MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && CMap::get()->MapLayers[second_z]->LayerConnectors[j]->get_unique() == unit->get_unique() && CMap::get()->MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == nullptr) {
						CMap::get()->MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = CMap::get()->MapLayers[second_z]->LayerConnectors[j];
						found_other_connector = true;
						break;
					}
				}
			}
			if (found_other_connector) {
				break;
			}
		}
	}
}

void map_template::ApplyUnits(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	if (!random) {
		this->apply_character_map_units(template_start_pos, map_start_pos, z);
	}

	const campaign *current_campaign = game::get()->get_current_campaign();
	CDate start_date;
	if (current_campaign) {
		start_date = current_campaign->get_start_date();
	}

	for (size_t i = 0; i < this->Units.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Units[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		const unit_type *type = std::get<1>(this->Units[i]);
		Vec2i unit_offset((type->get_tile_size() - QSize(1, 1)) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::get()->generate_unit_location(type, std::get<2>(this->Units[i]), map_start_pos, map_end - QPoint(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::get()->Info->IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x() || unit_pos.y < map_start_pos.y()) {
			continue;
		}
		
		if (
			(!current_campaign && std::get<3>(this->Units[i]).Year == 0 && std::get<4>(this->Units[i]).Year == 0)
			|| (
				current_campaign && (std::get<3>(this->Units[i]).Year == 0 || start_date.ContainsDate(std::get<3>(this->Units[i])))
				&& (std::get<4>(this->Units[i]).Year == 0 || (!start_date.ContainsDate(std::get<4>(this->Units[i]))))
			)
		) {
			CPlayer *player = nullptr;
			if (std::get<2>(this->Units[i])) {
				if (!current_campaign) { //only apply neutral units for when applying map templates for non-campaign/scenario maps
					continue;
				}
				player = GetOrAddFactionPlayer(std::get<2>(this->Units[i]));
				if (!player) {
					continue;
				}
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					player->SetStartView(unit_pos, z);
				}
			} else {
				player = CPlayer::get_neutral_player();
			}

			CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->Units[i]), player, z, type->BoolFlag[BUILDING_INDEX].value && type->get_tile_width() > 1 && type->get_tile_height() > 1);
			if (!type->BoolFlag[BUILDING_INDEX].value && !type->BoolFlag[HARVESTER_INDEX].value) { // make non-building, non-harvester units not have an active AI
				unit->Active = 0;
				player->ChangeUnitTypeAiActiveCount(type, -1);
			}
			if (std::get<5>(this->Units[i])) {
				unit->set_unique(std::get<5>(this->Units[i]));
			}
		}
	}

	if (!current_campaign) {
		return;
	}

	for (size_t i = 0; i < this->Heroes.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Heroes[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		character *hero = std::get<1>(this->Heroes[i]);
		Vec2i unit_offset((hero->get_unit_type()->get_tile_size() - QSize(1, 1)) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::get()->generate_unit_location(hero->get_unit_type(), std::get<2>(this->Heroes[i]), map_start_pos, map_end - QPoint(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::get()->Info->IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x() || unit_pos.y < map_start_pos.y()) {
			continue;
		}
		
		if ((!current_campaign || std::get<3>(this->Heroes[i]).Year == 0 || start_date.ContainsDate(std::get<3>(this->Heroes[i]))) && (std::get<4>(this->Heroes[i]).Year == 0 || !start_date.ContainsDate(std::get<4>(this->Heroes[i])))) {
			CPlayer *player = nullptr;
			if (std::get<2>(this->Heroes[i])) {
				player = GetOrAddFactionPlayer(std::get<2>(this->Heroes[i]));
				if (!player) {
					continue;
				}
				if (player->StartPos.x == 0 && player->StartPos.y == 0) {
					player->SetStartView(unit_pos, z);
				}
			} else {
				player = CPlayer::get_neutral_player();
			}
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *hero->get_unit_type(), player, z);
			unit->set_character(hero);
			if (!unit->Type->BoolFlag[BUILDING_INDEX].value && !unit->Type->BoolFlag[HARVESTER_INDEX].value) { // make non-building, non-harvester units not have an active AI
				unit->Active = 0;
				player->ChangeUnitTypeAiActiveCount(hero->get_unit_type(), -1);
			}
		}
	}
	
	for (const historical_unit *historical_unit : historical_unit::get_all()) {
		const historical_unit_history *unit_history = historical_unit->get_history();

		if (!unit_history->is_active()) {
			continue;
		}
		
		for (int i = 0; i < historical_unit->get_repeat_count(); ++i) {
			this->apply_historical_unit(historical_unit, template_start_pos, map_start_pos, map_end, z, random);
		}
	}

	for (character *character : character::get_all()) {
		if (!character->get_history()->is_active()) {
			continue;
		}

		this->apply_character(character, template_start_pos, map_start_pos, map_end, z, random);
	}
	
	for (character *character : character::get_all()) {
		if (!character->CanAppear()) {
			continue;
		}
		
		if (character->get_default_faction() == nullptr && !character->get_unit_type()->BoolFlag[FAUNA_INDEX].value) { //only fauna "heroes" may have no faction
			continue;
		}
		
		if (character->StartDate.Year == 0 || !start_date.ContainsDate(character->StartDate) || start_date.ContainsDate(character->DeathDate)) { //contrary to other elements, heroes aren't implemented if their date isn't set
			continue;
		}
		
		const faction *hero_faction = character->get_default_faction();
		for (int i = ((int) character->HistoricalFactions.size() - 1); i >= 0; --i) {
			if (start_date.ContainsDate(character->HistoricalFactions[i].first)) {
				hero_faction = character->HistoricalFactions[i].second;
				break;
			}
		}

		CPlayer *hero_player = hero_faction ? GetFactionPlayer(hero_faction) : nullptr;
		
		bool in_another_map_template = false;
		Vec2i hero_pos = this->get_best_location_map_position(character->HistoricalLocations, in_another_map_template, template_start_pos, map_start_pos, false);
		
		if (in_another_map_template) {
			continue;
		}
		
		if (hero_pos.x == -1 || hero_pos.y == -1) {
			if (!random) { //apply heroes whose position is that of a randomly-placed site, or that of their player's start position, together with randomly-placed units
				continue;
			}
			
			hero_pos = this->get_best_location_map_position(character->HistoricalLocations, in_another_map_template, template_start_pos, map_start_pos, true);
			
			if ((hero_pos.x == -1 || hero_pos.y == -1) && hero_player && hero_player->StartMapLayer == z) {
				hero_pos = hero_player->StartPos;
			}
		} else {
			if (random) {
				continue;
			}
		}
		
		if (!CMap::get()->Info->IsPointOnMap(hero_pos, z) || hero_pos.x < map_start_pos.x() || hero_pos.y < map_start_pos.y()) { //heroes whose faction hasn't been created already and who don't have a valid historical location set won't be created
			continue;
		}
		
		if (hero_faction) {
			hero_player = GetOrAddFactionPlayer(hero_faction);
			if (!hero_player) {
				continue;
			}
			if (hero_player->StartPos.x == 0 && hero_player->StartPos.y == 0) {
				hero_player->SetStartView(hero_pos, z);
			}
		} else {
			hero_player = CPlayer::get_neutral_player();
		}
		CUnit *unit = CreateUnit(hero_pos - character->get_unit_type()->get_tile_center_pos_offset(), *character->get_unit_type(), hero_player, z);
		unit->set_character(character);
		unit->Active = 0;
		hero_player->ChangeUnitTypeAiActiveCount(character->get_unit_type(), -1);
	}
}

void map_template::apply_character_map_units(const QPoint &template_start_pos, const QPoint &map_start_pos, const int z) const
{
	const terrain_character_map_type &terrain_character_map = this->overlay_terrain_character_map;

	if (terrain_character_map.empty()) {
		return;
	}

	for (int y = template_start_pos.y(); y < static_cast<int>(terrain_character_map.size()); ++y) {
		if (y >= (template_start_pos.y() + CMap::get()->Info->MapHeights[z])) {
			break;
		}

		if (this->get_end_pos().y() != -1 && y > this->get_end_pos().y()) {
			break;
		}

		const std::vector<char> &row = terrain_character_map[y];

		for (int x = template_start_pos.x(); x < static_cast<int>(row.size()); ++x) {
			try {
				if (x >= (template_start_pos.x() + CMap::get()->Info->MapWidths[z])) {
					break;
				}

				if (this->get_end_pos().x() != -1 && x > this->get_end_pos().x()) {
					break;
				}

				const char terrain_character = row.at(x);

				const QPoint real_pos(map_start_pos.x() + x - template_start_pos.x(), map_start_pos.y() + y - template_start_pos.y());

				if (!this->is_map_pos_usable(real_pos)) {
					continue;
				}

				const character_unit *character_unit = this->get_character_unit(terrain_character);
				if (character_unit != nullptr) {
					character_unit->create_at(real_pos, z);
				}
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to process character " + std::to_string(x + 1) + " of line " + std::to_string(y + 1) + " when applying character map units."));
			}
		}
	}
}

void map_template::apply_historical_unit(const historical_unit *historical_unit, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	const historical_unit_history *unit_history = historical_unit->get_history();

	const faction *unit_faction = unit_history->get_faction();
	CPlayer *unit_player = unit_faction ? GetFactionPlayer(unit_faction) : nullptr;

	const unit_type *unit_type = nullptr;
	if (!historical_unit->get_unit_types().empty()) {
		unit_type = historical_unit->get_unit_types()[SyncRand(historical_unit->get_unit_types().size())];
	} else if (!historical_unit->get_unit_classes().empty()) {
		const unit_class *unit_class = historical_unit->get_unit_classes()[SyncRand(historical_unit->get_unit_classes().size())];

		if (unit_faction == nullptr) {
			throw std::runtime_error("Historical unit \"" + historical_unit->get_identifier() + "\" is being applied via unit class, but it has no faction.");
		}

		unit_type = unit_faction->get_class_unit_type(unit_class);
	} else if (historical_unit->get_unique() != nullptr) {
		unit_type = historical_unit->get_unique()->get_unit_type();
	}

	if (unit_type == nullptr) {
		return;
	}

	const historical_location *unit_location = unit_history->get_location();

	if (unit_location == nullptr) {
		throw std::runtime_error("Historical unit \"" + historical_unit->get_identifier() + "\" has no historical location.");
	}

	const bool in_another_map_template = unit_location->get_map_template() != this;
	if (in_another_map_template) {
		return;
	}

	if (unit_location->get_pos().x() != -1 && unit_location->get_pos().y() != -1 && !this->contains_pos(unit_location->get_pos())) {
		return;
	}

	QPoint unit_pos = this->get_location_map_position(unit_location, template_start_pos, map_start_pos, false);

	if (unit_pos.x() == -1 && unit_pos.y() == -1) {
		if (!random) { //apply units whose position is that of a randomly-placed site, or that of their player's start position, together with randomly-placed units
			return;
		}

		unit_pos = this->get_location_map_position(unit_location, template_start_pos, map_start_pos, true);

		if (unit_pos.x() == -1 && unit_pos.y() == -1) {
			unit_pos = CMap::get()->generate_unit_location(unit_type, unit_faction, map_start_pos, map_end - QPoint(1, 1), z);
			if (unit_pos.x() != -1 && unit_pos.y() != -1) {
				unit_pos += unit_type->get_tile_center_pos_offset();
			}
		}
	} else {
		if (random) {
			return;
		}
	}

	const QPoint unit_top_left_pos = unit_pos - unit_type->get_tile_center_pos_offset();
	const QPoint unit_bottom_right_pos = unit_top_left_pos + size::to_point(unit_type->get_tile_size()) - QPoint(1, 1);
	if (!CMap::get()->Info->IsPointOnMap(unit_top_left_pos, z) || !CMap::get()->Info->IsPointOnMap(unit_bottom_right_pos, z) || !this->contains_map_pos(unit_top_left_pos) || !this->contains_map_pos(unit_bottom_right_pos)) { //units whose faction hasn't been created already and who don't have a valid historical location set won't be created
		return;
	}

	if (!this->is_map_pos_usable(unit_pos)) {
		return;
	}

	if (unit_faction != nullptr) {
		unit_player = GetOrAddFactionPlayer(unit_faction);
		if (!unit_player) {
			return;
		}
		if (!unit_type->BoolFlag[ITEM_INDEX].value && unit_player->StartPos.x == 0 && unit_player->StartPos.y == 0) {
			unit_player->SetStartView(unit_pos, z);
		}
	} else {
		unit_player = CPlayer::get_neutral_player();
	}

	for (int i = 0; i < historical_unit->get_quantity(); ++i) {
		const site *settlement = nullptr;
		if (unit_location != nullptr && unit_location->get_site() != nullptr && unit_location->get_site()->is_settlement()) {
			settlement = unit_location->get_site();
		}

		//item units only use factions to generate special properties for them
		CUnit *unit = CreateUnit(unit_top_left_pos, *unit_type, unit_type->BoolFlag[ITEM_INDEX].value ? CPlayer::get_neutral_player() : unit_player, z, true, settlement);

		if (historical_unit->get_unique() != nullptr) {
			unit->set_unique(historical_unit->get_unique());
		} else if (unit_type->BoolFlag[ITEM_INDEX].value) {
			unit->generate_special_properties(nullptr, unit_player, false, false, false);
		}

		if (historical_unit->get_resources_held() != 0) {
			unit->SetResourcesHeld(historical_unit->get_resources_held());
			unit->Variable[GIVERESOURCE_INDEX].Value = historical_unit->get_resources_held();
			unit->Variable[GIVERESOURCE_INDEX].Max = historical_unit->get_resources_held();
			unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}

		if (!historical_unit->is_ai_active()) {
			unit->Active = 0;
			unit_player->ChangeUnitTypeAiActiveCount(unit_type, -1);
		}

		if (historical_unit->get_ttl() != 0) {
			unit->TTL = historical_unit->get_ttl();
		}
	}
}

void map_template::apply_character(character *character, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	const character_history *character_history = character->get_history();
	const faction *unit_faction = character_history->get_faction();
	CPlayer *unit_player = unit_faction ? GetFactionPlayer(unit_faction) : nullptr;
	const unit_type *unit_type = character->get_unit_type();

	if (unit_type == nullptr) {
		return;
	}

	const historical_location *character_location = character_history->get_location();

	if (character_location == nullptr) {
		throw std::runtime_error("Character \"" + character->get_identifier() + "\" has no historical location.");
	}

	const bool in_another_map_template = character_location->get_map_template() != this;
	if (in_another_map_template) {
		return;
	}

	if (character_location->get_pos().x() != -1 && character_location->get_pos().y() != -1 && !this->contains_pos(character_location->get_pos())) {
		return;
	}

	QPoint unit_pos = this->get_location_map_position(character_location, template_start_pos, map_start_pos, false);

	if (unit_pos.x() == -1 && unit_pos.y() == -1) {
		if (!random) { //apply units whose position is that of a randomly-placed site, or that of their player's start position, together with randomly-placed units
			return;
		}

		unit_pos = this->get_location_map_position(character_location, template_start_pos, map_start_pos, true);

		if (unit_pos.x() == -1 && unit_pos.y() == -1) {
			unit_pos = CMap::get()->generate_unit_location(unit_type, unit_faction, map_start_pos, map_end - QPoint(1, 1), z);
			if (unit_pos.x() != -1 && unit_pos.y() != -1) {
				unit_pos += unit_type->get_tile_center_pos_offset();
			}
		}
	} else {
		if (random) {
			return;
		}
	}

	const QPoint unit_top_left_pos = unit_pos - unit_type->get_tile_center_pos_offset();
	const QPoint unit_bottom_right_pos = unit_top_left_pos + size::to_point(unit_type->get_tile_size()) - QPoint(1, 1);
	if (!CMap::get()->Info->IsPointOnMap(unit_top_left_pos, z) || !CMap::get()->Info->IsPointOnMap(unit_bottom_right_pos, z) || !this->contains_map_pos(unit_top_left_pos) || !this->contains_map_pos(unit_bottom_right_pos)) { //units whose faction hasn't been created already and who don't have a valid historical location set won't be created
		return;
	}

	if (unit_faction != nullptr) {
		unit_player = GetOrAddFactionPlayer(unit_faction);
		if (!unit_player) {
			return;
		}
		if (!unit_type->BoolFlag[ITEM_INDEX].value && unit_player->StartPos.x == 0 && unit_player->StartPos.y == 0) {
			unit_player->SetStartView(unit_pos, z);
		}
	} else {
		unit_player = CPlayer::get_neutral_player();
	}

	CUnit *unit = CreateUnit(unit_top_left_pos, *unit_type, unit_type->BoolFlag[ITEM_INDEX].value ? CPlayer::get_neutral_player() : unit_player, z);
	unit->set_character(character);
	if (!character->is_ai_active()) {
		unit->Active = 0;
		unit_player->ChangeUnitTypeAiActiveCount(unit_type, -1);
	}
}

/**
**	@brief	Get whether this map template is a subtemplate area of another one
**
**	@return	True if it is a subtemplate area, or false otherwise
*/
bool map_template::IsSubtemplateArea() const
{
	return this->get_main_template() != nullptr;
}

/**
**	@brief	Get the top map template for this one
**
**	@return	The topmost map template for this one (which can be itself if it isn't a subtemplate area)
*/
const map_template *map_template::GetTopMapTemplate() const
{
	if (this->get_main_template() != nullptr) {
		return this->get_main_template()->GetTopMapTemplate();
	} else {
		return this;
	}
}

QSize map_template::get_applied_size() const
{
	QSize applied_size = this->get_size();

	if (this->get_end_pos().x() != -1) {
		applied_size.setWidth(std::min(applied_size.width(), this->get_end_pos().x() + 1));
	}

	if (this->get_end_pos().y() != -1) {
		applied_size.setHeight(std::min(applied_size.height(), this->get_end_pos().y() + 1));
	}

	if (this->get_start_pos().x() != 0 || this->get_start_pos().y() != 0) {
		applied_size -= point::to_size(this->get_start_pos());
	}

	return applied_size;
}

bool map_template::is_optional_for_campaign(const campaign *campaign) const
{
	return this->is_optional() && !campaign->is_required_map_template(this);
}

bool map_template::is_pos_usable(const QPoint &pos) const
{
	if (this->is_circle()) {
		const QPoint start_pos(0, 0);
		const QPoint end_pos = QPoint(this->get_applied_width() - 1, this->get_applied_height() - 1);

		const decimillesimal_int middle_x = decimillesimal_int(end_pos.x() + start_pos.x()) / 2;
		const decimillesimal_int middle_y = decimillesimal_int(end_pos.y() + start_pos.y()) / 2;
		const decimillesimal_int radius = ((middle_x - start_pos.x()) + (middle_y - start_pos.y())) / 2;

		const decimillesimal_int rel_x = pos.x() - middle_x;
		const decimillesimal_int rel_y = pos.y() - middle_y;
		const decimillesimal_int my = radius * radius - rel_x * rel_x;
		if ((rel_y * rel_y) > my) {
			return false;
		}
	}

	return true;
}

void map_template::load_terrain_character_map(const bool overlay)
{
	using namespace std::string_literals;

	terrain_character_map_type &terrain_character_map = overlay ? this->overlay_terrain_character_map : this->terrain_character_map;
	if (!terrain_character_map.empty()) {
		//already loaded
		return;
	}

	std::filesystem::path terrain_filepath;
	if (overlay) {
		terrain_filepath = this->get_overlay_terrain_file();
	} else {
		terrain_filepath = this->get_terrain_file();
	}

	if (!std::filesystem::exists(terrain_filepath)) {
		throw std::runtime_error("File \"" + terrain_filepath.string() + "\" not found.");
	}

	std::ifstream is_map(terrain_filepath);

	std::string line_str;
	while (std::getline(is_map, line_str)) {
		std::vector<char> line_chars;

		for (unsigned int i = 0; i < line_str.length(); ++i) {
			try {
				const char terrain_character = line_str.at(i);
				line_chars.push_back(terrain_character);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to parse character " + std::to_string(i + 1) + " of line for terrain file \"" + terrain_filepath.string() + "\": \"" + line_str + "\"."));
			}
		}

		terrain_character_map.push_back(std::move(line_chars));
	}

	const QSize character_map_size(terrain_character_map.front().size(), terrain_character_map.size());
	if (character_map_size != this->get_size()) {
		throw std::runtime_error("The "s + (overlay ? "overlay " : "") + "terrain file for map template \"" + this->get_identifier() + "\" has a different size " + size::to_string(character_map_size) + " than that of the map template itself " + size::to_string(this->get_size()) + ".");
	}

	this->do_character_substitutions(overlay);
}

void map_template::do_character_substitutions(const bool overlay)
{
	terrain_character_map_type &terrain_character_map = overlay ? this->overlay_terrain_character_map : this->terrain_character_map;

	for (const std::unique_ptr<character_substitution> &substitution : this->character_substitutions) {
		substitution->apply_to_map(terrain_character_map);
	}
}

void map_template::set_terrain_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_terrain_file()) {
		return;
	}

	this->terrain_file = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::set_overlay_terrain_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_overlay_terrain_file()) {
		return;
	}

	this->overlay_terrain_file = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::set_trade_route_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_trade_route_file()) {
		return;
	}

	this->trade_route_file = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::load_terrain(const bool overlay)
{
	const std::filesystem::path &terrain_filepath = overlay ? this->get_overlay_terrain_file() : this->get_terrain_file();

	if (terrain_filepath.empty()) {
		return;
	}

	if (!std::filesystem::exists(terrain_filepath)) {
		throw std::runtime_error("File \"" + terrain_filepath.string() + "\" not found.");
	}

	if (terrain_filepath.extension() == ".png") {
		this->load_terrain_image(overlay);
	} else if (terrain_filepath.extension() == ".map") {
		this->load_terrain_file(overlay);
	} else if (terrain_filepath.extension() == ".wes" && !overlay) {
		this->load_wesnoth_terrain_file();
	} else {
		throw std::runtime_error("Invalid terrain file extension: \"" + terrain_filepath.extension().string() + "\".");
	}
}

void map_template::load_terrain_file(const bool overlay)
{
	using namespace std::string_literals;

	std::filesystem::path terrain_file;
	if (overlay) {
		terrain_file = this->get_overlay_terrain_file();
	} else {
		terrain_file = this->get_terrain_file();
	}

	this->load_terrain_character_map(overlay);

	const terrain_character_map_type &terrain_character_map = overlay ? this->overlay_terrain_character_map : this->terrain_character_map;

	const size_t height = terrain_character_map.size();

	if (height == 0) {
		throw std::runtime_error("Terrain character map is empty.");
	}

	const size_t width = terrain_character_map.front().size();

	QImage &terrain_image = overlay ? this->overlay_terrain_image : this->terrain_image;
	terrain_image = QImage(width, height, QImage::Format_RGBA8888);
	terrain_image.fill(Qt::transparent);

	for (size_t y = 0; y < terrain_character_map.size(); ++y) {
		const std::vector<char> &row = terrain_character_map.at(y);

		for (size_t x = 0; x < row.size(); ++x) {
			try {
				const char terrain_character = row.at(x);

				if (terrain_character == '=') {
					continue;
				}

				if (terrain_character == '0') {
					if (overlay) { //"0" in an overlay terrain file means no overlay, while "=" means no change
						terrain_image.setPixelColor(x, y, terrain_type::none_color);
					} else {
						throw std::runtime_error("\"0\" cannot be used for non-overlay terrain files.");
					}

					continue;
				}

				const character_unit *character_unit = this->get_character_unit(terrain_character);
				if (character_unit != nullptr) {
					if (!overlay) {
						throw std::runtime_error("Tried to use a character unit (character \""s + terrain_character + "\") in a non-overlay terrain map.");
					}

					continue;
				}

				const terrain_type *terrain = terrain_type::get_by_character(terrain_character);

				if (this->is_constructed_only() && !terrain->is_constructed()) {
					throw std::runtime_error("A non-constructed terrain is present in constructed-only map template \"" + this->get_identifier() + "\", as character \"" + terrain_character + "\".");
				}

				const QColor &color = terrain->get_color();

				if (!color.isValid()) {
					throw std::runtime_error("Terrain \"" + terrain->get_identifier() + "\" has no color.");
				}

				terrain_image.setPixelColor(x, y, color);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to process character " + std::to_string(x + 1) + " of line " + std::to_string(y + 1) + " for terrain file \"" + terrain_file.string() + "\"."));
			}
		}
	}
}

void map_template::load_wesnoth_terrain_file()
{
	const std::filesystem::path &terrain_filepath = this->get_terrain_file();

	std::vector<std::vector<std::string>> terrain_strings;

	std::ifstream is_map(terrain_filepath);

	std::string line_str;
	while (std::getline(is_map, line_str)) {
		string::replace(line_str, " ", "");
		string::replace(line_str, "\r", "");
		string::replace(line_str, "1", "");
		string::replace(line_str, "2", "");
		string::replace(line_str, "3", "");

		std::vector<std::string> line_terrain_strings = string::split(line_str, ',');
		terrain_strings.push_back(std::move(line_terrain_strings));
	}

	const size_t height = terrain_strings.size();
	if (height == 0) {
		throw std::runtime_error("Terrain data is empty.");
	}

	const size_t width = terrain_strings.front().size();

	this->terrain_image = QImage(width, height, QImage::Format_RGBA8888);
	this->terrain_image.fill(Qt::transparent);

	this->overlay_terrain_image = QImage(width, height, QImage::Format_RGBA8888);
	this->overlay_terrain_image.fill(Qt::transparent);

	for (size_t y = 0; y < terrain_strings.size(); ++y) {
		const std::vector<std::string> &row = terrain_strings.at(y);

		for (size_t x = 0; x < row.size(); ++x) {
			try {
				const std::string &terrain_string = row.at(x);

				const terrain_type *terrain = nullptr;
				const terrain_type *overlay_terrain = nullptr;

				const size_t circumflex_pos = terrain_string.find('^');
				if (circumflex_pos != std::string::npos) {
					//the terrain string contains an overlay terrain
					const std::string base_terrain_string = terrain_string.substr(0, circumflex_pos);
					terrain = terrain_type::get_by_wesnoth_string(base_terrain_string);

					const std::string overlay_terrain_string = terrain_string.substr(circumflex_pos + 1, terrain_string.size() - circumflex_pos - 1);
					overlay_terrain = terrain_type::get_by_wesnoth_string(overlay_terrain_string);
				} else {
					terrain = terrain_type::get_by_wesnoth_string(terrain_string);
				}

				if (terrain != nullptr) {
					const QColor &color = terrain->get_color();

					if (!color.isValid()) {
						throw std::runtime_error("Terrain \"" + terrain->get_identifier() + "\" has no color.");
					}

					this->terrain_image.setPixelColor(x, y, color);
				}

				if (overlay_terrain != nullptr) {
					const QColor &overlay_color = overlay_terrain->get_color();

					if (!overlay_color.isValid()) {
						throw std::runtime_error("Terrain \"" + overlay_terrain->get_identifier() + "\" has no color.");
					}

					this->overlay_terrain_image.setPixelColor(x, y, overlay_color);
				}
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to process terrain string " + std::to_string(x + 1) + " of line " + std::to_string(y + 1) + " for terrain file \"" + terrain_filepath.string() + "\"."));
			}
		}
	}
}

QImage map_template::load_terrain_image_file(const std::filesystem::path &filepath)
{
	if (!std::filesystem::exists(filepath)) {
		throw std::runtime_error("The terrain image file \"" + filepath.string() + "\" for map template \"" + this->get_identifier() + "\" does not exist.");
	}

	QImage terrain_image(path::to_qstring(filepath));

	if (terrain_image.size() != this->get_size()) {
		throw std::runtime_error("The terrain image for map template \"" + this->get_identifier() + "\" has a different size " + size::to_string(terrain_image.size()) + " than that of the map template itself " + size::to_string(this->get_size()) + ".");
	}

	return terrain_image;
}

void map_template::load_terrain_image(const bool overlay)
{
	QImage &terrain_image = overlay ? this->overlay_terrain_image : this->terrain_image;
	if (!terrain_image.isNull()) {
		//already loaded
		return;
	}

	std::filesystem::path terrain_file;
	if (overlay) {
		terrain_file = this->get_overlay_terrain_file();
	} else {
		terrain_file = this->get_terrain_file();
	}

	terrain_image = this->load_terrain_image_file(terrain_file);
}

void map_template::load_trade_route_image()
{
	if (this->get_trade_route_file().empty()) {
		return;
	}

	QImage &terrain_image = this->terrain_image;
	QImage &overlay_terrain_image = this->overlay_terrain_image;

	const QImage trade_route_image = this->load_terrain_image_file(this->get_trade_route_file());

	if (terrain_image.isNull()) {
		terrain_image = trade_route_image;
		return;
	}

	image::for_each_pixel_pos(trade_route_image, [&trade_route_image, &terrain_image, &overlay_terrain_image](const int x, const int y) {
		const QColor color = trade_route_image.pixelColor(x, y);

		if (color.alpha() == 0) {
			//ignore fully transparent pixels
			return;
		}

		if (color == terrain_type::none_color) {
			//ignore black pixels in the trade route image, as they are there only to prevent trade route pixels from being generated in certain positions
			return;
		}

		const QColor old_color = overlay_terrain_image.pixelColor(x, y);

		if (old_color.alpha() != 0) {
			const terrain_type *terrain = nullptr;
			const terrain_feature *terrain_feature = terrain_feature::try_get_by_color(old_color);
			if (terrain_feature != nullptr) {
				terrain = terrain_feature->get_terrain_type();
			} else {
				terrain = terrain_type::try_get_by_color(old_color);
			}

			if (terrain != nullptr && terrain->is_water()) {
				//replace river water pixels with ford ones, and do nothing for non-river water pixels
				if (terrain_feature != nullptr && terrain_feature->is_river()) {
					/*
					for (int x_offset = -1; x_offset <= 1; ++x_offset) {
						for (int y_offset = -1; y_offset <= 1; ++y_offset) {
							const QPoint offset_pixel_pos(x + x_offset, y + y_offset);

							if (!terrain_image.rect().contains(offset_pixel_pos)) {
								continue;
							}

							const QColor offset_old_color = overlay_terrain_image.pixelColor(offset_pixel_pos);
							const wyrmgus::terrain_feature *offset_terrain_feature = terrain_feature::try_get_by_color(offset_old_color);

							if (offset_terrain_feature == nullptr || !offset_terrain_feature->is_river()) {
								continue;
							}

							terrain_image.setPixelColor(offset_pixel_pos, defines::get()->get_ford_terrain_type()->get_color());
						}
					}
					*/

					overlay_terrain_image.setPixelColor(x, y, color);
				}

				return;
			}
		}

		//apply the trade route pixel to the overlay image
		overlay_terrain_image.setPixelColor(x, y, color);
	});
}

void map_template::set_territory_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_territory_file()) {
		return;
	}

	this->territory_file = database::get()->get_maps_path(this->get_module()) / filepath;
}

bool map_template::is_dependent_on(const map_template *other_template) const
{
	//get whether this map template is dependent on another (i.e. needs it to establish its position)

	if (this->is_optional() || other_template->is_optional()) {
		return false;
	}

	for (const map_template *map_template : this->dependency_templates) {
		if (map_template == other_template || map_template->is_dependent_on(other_template)) {
			return true;
		}
	}

	return false;
}

void map_template::add_dependency_template(const map_template *other_template)
{
	if (!vector::contains(this->dependency_templates, other_template)) {
		this->dependency_templates.push_back(other_template);
	}
}

QPoint map_template::generate_subtemplate_position(map_template *subtemplate, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const QPoint &max_adjacent_template_distance, bool &adjacency_restriction_occurred) const
{
	const QSize applied_size = this->get_applied_size();
	const QSize subtemplate_applied_size = subtemplate->get_applied_size();

	QPoint min_pos(map_start_pos);
	QPoint max_pos(map_end.x() - subtemplate_applied_size.width(), map_end.y() - subtemplate_applied_size.height());

	if (subtemplate->get_min_subtemplate_pos().x() != -1) {
		min_pos.setX(min_pos.x() + subtemplate->get_min_subtemplate_pos().x() - template_start_pos.x());
	}
	if (subtemplate->get_min_subtemplate_pos().y() != -1) {
		min_pos.setY(min_pos.y() + subtemplate->get_min_subtemplate_pos().y() - template_start_pos.y());
	}

	if (subtemplate->get_max_subtemplate_pos().x() != -1) {
		max_pos.setX(max_pos.x() + subtemplate->get_max_subtemplate_pos().x() - applied_size.width());
	}
	if (subtemplate->get_max_subtemplate_pos().y() != -1) {
		max_pos.setY(max_pos.y() + subtemplate->get_max_subtemplate_pos().y() - applied_size.height());
	}

	//bound the minimum and maximum positions depending on which other templates should be adjacent to this one (if they have already been applied to the map)
	for (const map_template *adjacent_template : subtemplate->AdjacentTemplates) {
		const QPoint adjacent_template_pos = CMap::get()->get_subtemplate_pos(adjacent_template);

		if (!CMap::get()->Info->IsPointOnMap(adjacent_template_pos, z)) {
			fprintf(stderr, "Could not apply adjacency restriction for map template \"%s\", as the other template (\"%s\") has not been applied (yet).\n", subtemplate->Ident.c_str(), adjacent_template->Ident.c_str());
			continue;
		}

		const QPoint min_adjacency_pos = adjacent_template_pos - max_adjacent_template_distance - size::to_point(subtemplate_applied_size);
		const QPoint max_adjacency_pos = adjacent_template_pos + size::to_point(subtemplate_applied_size) + max_adjacent_template_distance;
		if (min_adjacency_pos.x() > min_pos.x()) {
			min_pos.setX(min_adjacency_pos.x());
			adjacency_restriction_occurred = true;
		}
		if (min_adjacency_pos.y() > min_pos.y()) {
			min_pos.setY(min_adjacency_pos.y());
			adjacency_restriction_occurred = true;
		}
		if (max_adjacency_pos.x() < max_pos.x()) {
			max_pos.setX(max_adjacency_pos.x());
			adjacency_restriction_occurred = true;
		}
		if (max_adjacency_pos.y() < max_pos.y()) {
			max_pos.setY(max_adjacency_pos.y());
			adjacency_restriction_occurred = true;
		}
	}

	const bool optional = subtemplate->is_optional_for_campaign(game::get()->get_current_campaign());

	//bound the minimum and maximum positions depending on whether this template should be to the north, south, west or east of other ones
	for (const map_template *other_template : subtemplate->NorthOfTemplates) {
		const QPoint other_template_pos = CMap::get()->get_subtemplate_pos(other_template);
		if (!CMap::get()->Info->IsPointOnMap(other_template_pos, z)) {
			if (!optional) {
				throw std::runtime_error("Could not apply \"north of\" restriction for map template \"" + subtemplate->get_identifier() + "\", as the other template (\"" + other_template->get_identifier() + "\") has not been applied (yet).");
			}
			continue;
		}
		max_pos.setY(std::min<short>(max_pos.y(), other_template_pos.y() - (subtemplate_applied_size.height() / 2)));
	}
	for (const map_template *other_template : subtemplate->SouthOfTemplates) {
		const QPoint other_template_pos = CMap::get()->get_subtemplate_pos(other_template);
		if (!CMap::get()->Info->IsPointOnMap(other_template_pos, z)) {
			if (!optional) {
				throw std::runtime_error("Could not apply \"south of\" restriction for map template \"" + subtemplate->get_identifier() + "\", as the other template (\"" + other_template->get_identifier() + "\") has not been applied (yet).");
			}
			continue;
		}
		min_pos.setY(std::max<short>(min_pos.y(), other_template_pos.y() + other_template->get_applied_height() - (subtemplate_applied_size.height() / 2)));
	}
	for (const map_template *other_template : subtemplate->WestOfTemplates) {
		const QPoint other_template_pos = CMap::get()->get_subtemplate_pos(other_template);
		if (!CMap::get()->Info->IsPointOnMap(other_template_pos, z)) {
			if (!optional) {
				throw std::runtime_error("Could not apply \"west of\" restriction for map template \"" + subtemplate->get_identifier() + "\", as the other template (\"" + other_template->get_identifier() + "\") has not been applied (yet).");
			}
			continue;
		}
		max_pos.setX(std::min<short>(max_pos.x(), other_template_pos.x() - (subtemplate_applied_size.width() / 2)));
	}
	for (const map_template *other_template : subtemplate->EastOfTemplates) {
		const QPoint other_template_pos = CMap::get()->get_subtemplate_pos(other_template);
		if (!CMap::get()->Info->IsPointOnMap(other_template_pos, z)) {
			if (!optional) {
				throw std::runtime_error("Could not apply \"east of\" restriction for map template \"" + subtemplate->get_identifier() + "\", as the other template (\"" + other_template->get_identifier() + "\") has not been applied (yet).");
			}
			continue;
		}
		min_pos.setX(std::max<short>(min_pos.x(), other_template_pos.x() + other_template->get_applied_width() - (subtemplate_applied_size.width() / 2)));
	}

	std::vector<QPoint> potential_positions;
	for (int x = min_pos.x(); x <= max_pos.x(); ++x) {
		for (int y = min_pos.y(); y <= max_pos.y(); ++y) {
			potential_positions.push_back(QPoint(x, y));
		}
	}

	//include the offsets relevant for the templates dependent on this one's position (e.g. templates that have to be to the north of this one), so that there is enough space for them to be generated there
	const int north_offset = optional ? 0 : subtemplate->GetDependentTemplatesNorthOffset();
	const int south_offset = optional ? 0 : subtemplate->GetDependentTemplatesSouthOffset();
	const int west_offset = optional ? 0 : subtemplate->GetDependentTemplatesWestOffset();
	const int east_offset = optional ? 0 : subtemplate->GetDependentTemplatesEastOffset();

	int try_count = 0;

	while (!potential_positions.empty()) {
		// for the sake of performance, put a limit on optional subtemplate placement tries, instead of checking all possibilities
		if (optional && try_count >= 1000) {
			break;
		}

		++try_count;

		const QPoint subtemplate_pos = vector::take_random(potential_positions);

		const bool top_left_on_map = this->contains_map_pos(subtemplate_pos - QPoint(west_offset, north_offset));
		const bool bottom_right_on_map = this->contains_map_pos(QPoint(subtemplate_pos.x() + subtemplate_applied_size.width() + east_offset - 1, subtemplate_pos.y() + subtemplate_applied_size.height() + south_offset - 1));
		const bool on_map = top_left_on_map && bottom_right_on_map;

		if (!on_map) {
			continue;
		}

		bool on_usable_area = true;
		for (int x = (subtemplate->get_min_adjacent_template_distance().x() * -1) - west_offset; x < (subtemplate_applied_size.width() + subtemplate->get_min_adjacent_template_distance().x() + east_offset); ++x) {
			for (int y = (subtemplate->get_min_adjacent_template_distance().y() * -1) - north_offset; y < (subtemplate_applied_size.height() + subtemplate->get_min_adjacent_template_distance().y() + south_offset); ++y) {
				if (CMap::get()->is_point_in_a_subtemplate_area(subtemplate_pos + Vec2i(x, y), z)) {
					on_usable_area = false;
					break;
				}
			}
			if (!on_usable_area) {
				break;
			}
		}

		if (on_usable_area) {
			for (int x = -west_offset; x < (subtemplate_applied_size.width() + east_offset); ++x) {
				for (int y = -north_offset; y < (subtemplate_applied_size.height() + south_offset); ++y) {
					if (!this->is_map_pos_usable(subtemplate_pos + Vec2i(x, y))) {
						on_usable_area = false;
						break;
					}
				}
				if (!on_usable_area) {
					break;
				}
			}
		}

		if (on_usable_area) {
			//there must be no units in the position where the subtemplate would be applied, and nearby tiles
			std::vector<CUnit *> table;
			const bool select_circle = subtemplate->is_circle();
			Select(subtemplate_pos - subtemplate->get_min_adjacent_template_distance(), subtemplate_pos + size::to_point(subtemplate_applied_size) - QPoint(1, 1) + subtemplate->get_min_adjacent_template_distance(), table, z, select_circle);
			if (!table.empty()) {
				on_usable_area = false;
			}
		}

		if (on_usable_area && subtemplate->is_constructed_only()) {
			if (!this->is_constructed_subtemplate_compatible_with_terrain(subtemplate, subtemplate_pos, z)) {
				on_usable_area = false;
			}
		}

		if (!on_usable_area) {
			continue;
		}

		return subtemplate_pos;
	}

	return QPoint(-1, -1);
}

bool map_template::is_constructed_subtemplate_compatible_with_terrain(map_template *subtemplate, const QPoint &map_start_pos, const int z) const
{
	if (!subtemplate->get_overlay_terrain_file().empty()) {
		return this->is_constructed_subtemplate_compatible_with_terrain_file(subtemplate, map_start_pos, z);
	} else if (!subtemplate->get_overlay_terrain_file().empty()) {
		return this->is_constructed_subtemplate_compatible_with_terrain_image(subtemplate, map_start_pos, z);
	}

	return true;
}

bool map_template::is_constructed_subtemplate_compatible_with_terrain_file(map_template *subtemplate, const QPoint &map_start_pos, const int z) const
{
	const QPoint &template_start_pos = subtemplate->get_start_pos();

	subtemplate->load_terrain_character_map(true);

	const terrain_character_map_type &terrain_character_map = subtemplate->overlay_terrain_character_map;

	for (int y = template_start_pos.y(); y < static_cast<int>(terrain_character_map.size()); ++y) {
		if (y >= (template_start_pos.y() + CMap::get()->Info->MapHeights[z])) {
			break;
		}

		if (subtemplate->get_end_pos().y() != -1 && y > subtemplate->get_end_pos().y()) {
			break;
		}

		const std::vector<char> &row = terrain_character_map[y];

		for (int x = template_start_pos.x(); x < static_cast<int>(row.size()); ++x) {
			try {
				if (x >= (template_start_pos.x() + CMap::get()->Info->MapWidths[z])) {
					break;
				}

				if (subtemplate->get_end_pos().x() != -1 && x > subtemplate->get_end_pos().x()) {
					break;
				}

				const char terrain_character = row.at(x);

				if (terrain_character == '=') {
					//the '=' character means the tile is allowed to stay the same for the check, i.e. it is ignored for it
					continue;
				}

				const QPoint map_pos(map_start_pos.x() + x - template_start_pos.x(), map_start_pos.y() + y - template_start_pos.y());

				const tile *tile = CMap::get()->Field(map_pos, z);

				if (terrain_character == '0' || subtemplate->get_character_unit(terrain_character) != nullptr) {
					//the '0' character means the tile must have no overlay
					if (tile->get_overlay_terrain() != nullptr) {
						return false;
					}

					continue;
				}

				const terrain_type *terrain = terrain_type::get_by_character(terrain_character);

				if (!terrain->is_constructed()) {
					throw std::runtime_error("A non-constructed terrain is present in constructed-only map template \"" + subtemplate->get_identifier() + "\", as character \"" + terrain_character + "\".");
				}

				//the tile's overlay terrain must either match that in the map template exactly, or not be set
				if (tile->get_overlay_terrain() != nullptr && tile->get_overlay_terrain() != terrain) {
					return false;
				}

				if (tile->get_overlay_terrain() != terrain && !vector::contains(terrain->get_base_terrain_types(), tile->get_terrain())) {
					//the tile's terrain must be a valid base terrain for the overlay terrain type
					return false;
				}
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to process character " + std::to_string(x + 1) + " of line " + std::to_string(y + 1) + " for terrain file \"" + this->overlay_terrain_file.string() + "\"."));
			}
		}
	}

	return true;
}

bool map_template::is_constructed_subtemplate_compatible_with_terrain_image(map_template *subtemplate, const QPoint &map_start_pos, const int z) const
{
	const QPoint &template_start_pos = subtemplate->get_start_pos();

	subtemplate->load_terrain_image(true);
	const QImage &terrain_image = subtemplate->overlay_terrain_image;

	const int applied_width = subtemplate->get_applied_width();
	const int applied_height = subtemplate->get_applied_height();

	for (int x_offset = 0; x_offset < applied_width; ++x_offset) {
		const int x = template_start_pos.x() + x_offset;
		const int map_x = map_start_pos.x() + x_offset;

		if (map_x >= CMap::get()->Info->MapWidths[z]) {
			break;
		}

		for (int y_offset = 0; y_offset < applied_height; ++y_offset) {
			const int y = template_start_pos.y() + y_offset;
			const int map_y = map_start_pos.y() + y_offset;

			if (map_y >= CMap::get()->Info->MapHeights[z]) {
				break;
			}

			const QColor color = terrain_image.pixelColor(x, y);

			if (color.alpha() == 0) {
				//transparent pixels mean ignoring the tile's overlay for the check
				continue;
			}

			const tile *tile = CMap::get()->Field(map_x, map_y, z);

			if (color.red() == 0 && color.green() == 0 && color.blue() == 0) {
				//a pure black pixel means the tile must have no overlay
				if (tile->get_overlay_terrain() != nullptr) {
					return false;
				}

				continue;
			}

			const terrain_type *terrain = terrain_type::get_by_color(color);

			//the tile's overlay terrain must either match that in the map template exactly, or not be set
			if (tile->get_overlay_terrain() != nullptr && tile->get_overlay_terrain() != terrain) {
				return false;
			}

			if (tile->get_overlay_terrain() != terrain && !vector::contains(terrain->get_base_terrain_types(), tile->get_terrain())) {
				//the tile's terrain must be a valid base terrain for the overlay terrain type
				return false;
			}
		}
	}

	return true;
}

QPoint map_template::generate_celestial_site_position(const site *site, const int z) const
{
	const QSize celestial_site_size = site->get_size_with_satellites();

	//check each possible geocoordinate, going through them in a random manner
	QPoint pos(-1, -1);

	geocoordinate::for_each_random_until([&](const geocoordinate &astrocoordinate) {
		if (astrocoordinate.get_longitude() == 0 && astrocoordinate.get_latitude() == 0) {
			return false;
		}

		QPoint random_pos = site->astrocoordinate_to_pos<true>(astrocoordinate);

		if (!CMap::get()->Info->IsPointOnMap(random_pos, z)) {
			return false;
		}

		//ensure there are no units placed where the celestial site and its satellites would be
		std::vector<CUnit *> units;
		Select(random_pos - size::to_point((celestial_site_size + QSize(1, 1)) / 2), random_pos + size::to_point((celestial_site_size + QSize(1, 1)) / 2), units, z);
		if (!units.empty()) {
			return false;
		}

		const unit_type *unit_type = site->get_base_unit_type();
		if (unit_type != nullptr) {
			const QPoint top_left_pos = random_pos - unit_type->get_tile_center_pos_offset();

			if (!UnitTypeCanBeAt(*unit_type, top_left_pos, z) || (unit_type->BoolFlag[BUILDING_INDEX].value && !CanBuildUnitType(nullptr, *unit_type, top_left_pos, 0, true, z))) {
				return false;
			}
		}

		pos = random_pos;
		return true;
	});

	if (pos == QPoint(-1, -1)) {
		throw std::runtime_error("Failed to generate celestial site position for site \"" + site->get_identifier() + "\".");
	}

	return pos;
}

QPoint map_template::generate_site_orbit_position(const site *site, const int z, const int64_t orbit_distance) const
{
	std::vector<QPoint> potential_circle_points;
	potential_circle_points.reserve(360 * 360);

	for (int x = -180; x <= 180; ++x) {
		for (int y = -180; y <= 180; ++y) {
			if (x == 0 && y == 0) {
				continue;
			}

			potential_circle_points.emplace_back(x, y);
		}
	}

	const QSize site_size = site->get_size_with_satellites();

	const site_game_data *orbit_center_game_data = site->get_orbit_center()->get_game_data();

	QPoint orbit_center_pos = orbit_center_game_data->get_map_pos();
	if (orbit_center_game_data->get_site_unit() != nullptr) {
		orbit_center_pos = orbit_center_game_data->get_site_unit()->get_center_tile_pos();
	}

	while (!potential_circle_points.empty()) {
		const QPoint orbit_circle_pos = vector::take_random(potential_circle_points);
		const QPoint orbit_pos = point::get_nearest_circle_edge_point(orbit_circle_pos, orbit_distance);
		QPoint random_pos = orbit_center_pos + orbit_pos;

		if (!CMap::get()->Info->IsPointOnMap(random_pos, z)) {
			continue;
		}

		const unit_type *unit_type = site->get_base_unit_type();
		if (unit_type != nullptr) {
			//ensure there are no units placed where the celestial site and its satellites would be
			std::vector<CUnit *> units;
			const QPoint top_left_pos = random_pos - unit_type->get_tile_center_pos_offset();
			Select(top_left_pos - QPoint(1, 1), top_left_pos + size::to_point(unit_type->get_tile_size()), units, z);
			if (!units.empty()) {
				continue;
			}

			if (!UnitTypeCanBeAt(*unit_type, top_left_pos, z) || (unit_type->BoolFlag[BUILDING_INDEX].value && !CanBuildUnitType(nullptr, *unit_type, top_left_pos, 0, true, z))) {
				continue;
			}

			if (unit_type->get_domain() == unit_domain::space) {
				//ensure that the generated unit will not be created on space transition
				bool on_space_border = false;
				for (int x = top_left_pos.x() - 1; x < top_left_pos.x() + unit_type->get_tile_width() + 1; ++x) {
					for (int y = top_left_pos.y() - 1; y < top_left_pos.y() + unit_type->get_tile_height() + 1; ++y) {
						const QPoint tile_pos(x, y);
						if (!CMap::get()->Info->IsPointOnMap(tile_pos, z)) {
							continue;
						}

						if (!CMap::get()->Field(tile_pos, z)->has_flag(tile_flag::space)) {
							on_space_border = true;
							break;
						}
					}

					if (on_space_border) {
						break;
					}
				}
				if (on_space_border) {
					continue;
				}
			}
		}

		return random_pos;
	}

	throw std::runtime_error("Failed to generate site orbit position for site \"" + site->get_identifier() + "\".");
}

/**
**	@brief	Get the best map position from a list of historical locations
**
**	@param	historical_location_list	The list of historical locations
**	@param	in_another_map_template		This is set to true if there is a valid position, but it is in another map template
**
**	@return	The best position if found, or an invalid one otherwise
*/
Vec2i map_template::get_best_location_map_position(const std::vector<std::unique_ptr<historical_location>> &historical_location_list, bool &in_another_map_template, const Vec2i &template_start_pos, const Vec2i &map_start_pos, const bool random) const
{
	Vec2i pos(-1, -1);
	in_another_map_template = false;
	const campaign *current_campaign = game::get()->get_current_campaign();
	CDate start_date;
	if (current_campaign) {
		start_date = current_campaign->get_start_date();
	}
	
	for (int i = ((int) historical_location_list.size() - 1); i >= 0; --i) {
		const std::unique_ptr<historical_location> &historical_location = historical_location_list[i];
		if (historical_location->Date.Year == 0 || start_date.ContainsDate(historical_location->Date)) {
			if (historical_location->get_map_template() == this) {
				if (historical_location->get_pos().x() != -1 && historical_location->get_pos().y() != -1) { //historical unit position, could also have been inherited from a site with a fixed position
					pos = map_start_pos + historical_location->get_pos() - template_start_pos;
				} else if (random) {
					if (historical_location->get_site() != nullptr && historical_location->get_site()->get_game_data()->get_site_unit() != nullptr) { //sites with random positions will have no valid stored fixed position, but will have had a site unit randomly placed; use that site unit's position instead for this unit then
						pos = historical_location->get_site()->get_game_data()->get_site_unit()->get_center_tile_pos();
					}
				}
			} else {
				in_another_map_template = true;
			}
			break;
		}
	}
	
	return pos;
}

QPoint map_template::get_location_map_position(const historical_location *historical_location, const QPoint &template_start_pos, const QPoint &map_start_pos, const bool random) const
{
	//get a map position for a historical location
	QPoint pos(-1, -1);
	
	if (historical_location->get_map_template() == this) {
		if (historical_location->get_pos().x() != -1 && historical_location->get_pos().y() != -1) { //historical unit position, could also have been inherited from a site with a fixed position
			return map_start_pos + historical_location->get_pos() - template_start_pos;
		} else if (random) {
			if (historical_location->get_site() != nullptr && historical_location->get_site()->get_game_data()->get_site_unit() != nullptr) { //sites with random positions will have no valid stored fixed position, but will have had a site unit randomly placed; use that site unit's position instead for this unit then
				return historical_location->get_site()->get_game_data()->get_site_unit()->get_center_tile_pos();
			}
		}
	}

	return QPoint(-1, -1);
}

void map_template::set_tile_terrain(const QPoint &tile_pos, const terrain_type *terrain)
{
	if (terrain->is_overlay()) {
		this->overlay_tile_terrains[tile_pos] = terrain;
	} else {
		this->tile_terrains[tile_pos] = terrain;
	}
}

void map_template::apply_tile_terrains(const bool overlay, const QPoint &template_start_pos, const QPoint &map_start_pos, const int z)
{
	const point_map<const terrain_type *> &tile_terrains = overlay ? this->get_overlay_tile_terrains() : this->get_tile_terrains();

	for (const auto &kv_pair : tile_terrains) {
		const QPoint &tile_pos = kv_pair.first;

		if (!this->contains_pos(tile_pos)) {
			continue;
		}

		const terrain_type *terrain = kv_pair.second;
		const QPoint tile_map_pos = map_start_pos + tile_pos - template_start_pos;

		if (!this->contains_map_pos(tile_map_pos)) {
			continue;
		}

		tile *tile = CMap::get()->Field(tile_map_pos, z);

		if (terrain == nullptr && overlay) {
			tile->RemoveOverlayTerrain();
		} else {
			tile->SetTerrain(terrain);
		}
	}
}

const map_projection *map_template::get_map_projection() const
{
	if (this->map_projection != nullptr) {
		return this->map_projection;
	}

	return defines::get()->get_default_map_projection();
}

QPoint map_template::get_geocoordinate_pos(const geocoordinate &geocoordinate) const
{
	return this->get_map_projection()->geocoordinate_to_point(geocoordinate, this->get_georectangle(), this->get_size());
}

geocoordinate map_template::get_pos_geocoordinate(const QPoint &pos) const
{
	return this->get_map_projection()->point_to_geocoordinate(pos, this->get_georectangle(), this->get_size());
}

void map_template::save_terrain_images()
{
	this->load_terrain(false);
	this->load_terrain(true);

	terrain_geodata_map terrain_data;

	if (this->get_world() != nullptr) {
		terrain_data = this->get_world()->parse_terrain_geojson_folder();
	}

	terrain_geodata_ptr_map base_terrain_data;
	terrain_geodata_ptr_map overlay_terrain_data;
	terrain_geodata_ptr_map trade_route_terrain_data;

	for (const auto &kv_pair : terrain_data) {
		const terrain_type *terrain = nullptr;
		const terrain_feature *terrain_feature = nullptr;

		if (std::holds_alternative<const wyrmgus::terrain_feature *>(kv_pair.first)) {
			terrain_feature = std::get<const wyrmgus::terrain_feature *>(kv_pair.first);

			if (terrain_feature->is_hidden()) {
				continue;
			}

			terrain = terrain_feature->get_terrain_type();
		} else {
			terrain = std::get<const terrain_type *>(kv_pair.first);
		}

		terrain_geodata_ptr_map *terrain_data_ptr = nullptr;

		if (terrain_feature != nullptr && terrain_feature->is_trade_route()) {
			terrain_data_ptr = &trade_route_terrain_data;
		} else if (terrain->is_overlay()) {
			terrain_data_ptr = &overlay_terrain_data;
		} else {
			terrain_data_ptr = &base_terrain_data;
		}

		for (const auto &geoshape : kv_pair.second) {
			(*terrain_data_ptr)[kv_pair.first].push_back(geoshape.get());
		}
	}

	const std::string filename = this->get_identifier() + ".png";
	const std::string overlay_filename = this->get_identifier() + "_overlay.png";
	const std::string trade_route_filename = this->get_identifier() + "_trade_routes.png";

	this->save_terrain_image(filename, this->terrain_image, base_terrain_data, this->get_tile_terrains());
	this->save_terrain_image(overlay_filename, this->overlay_terrain_image, overlay_terrain_data, this->get_overlay_tile_terrains());

	QImage trade_route_image;

	if (!this->get_trade_route_file().empty()) {
		trade_route_image = QImage(path::to_qstring(this->get_trade_route_file()));
	}

	this->save_terrain_image(trade_route_filename, trade_route_image, trade_route_terrain_data, point_map<const terrain_type *>());

	this->clear_application_data();
}

void map_template::save_terrain_image(const std::string &filename, const QImage &loaded_terrain_image, const terrain_geodata_ptr_map &terrain_data, const point_map<const terrain_type *> &terrain_map) const
{
	QImage image;

	if (!loaded_terrain_image.isNull()) {
		image = loaded_terrain_image;

		if (image.size() != this->get_size()) {
			throw std::runtime_error("Invalid terrain image size for map template \"" + this->get_identifier() + "\".");
		}
	} else {
		image = QImage(this->get_size(), QImage::Format_RGBA8888);
		image.fill(Qt::transparent);
	}

	if (!terrain_data.empty()) {
		this->create_terrain_image_from_geodata(image, terrain_data, filename);
	} else if (!terrain_map.empty()) {
		this->create_terrain_image_from_map(image, terrain_map);
	} else if (loaded_terrain_image.isNull()) {
		//nothing to save
		return;
	}

	image.save(QString::fromStdString(filename));
}

void map_template::create_terrain_image_from_geodata(QImage &image, const terrain_geodata_ptr_map &terrain_data, const std::string &image_checkpoint_save_filename) const
{
	const wyrmgus::georectangle &georectangle = this->get_georectangle();

	for (const auto &kv_pair : terrain_data) {
		const terrain_type *terrain = nullptr;
		const terrain_feature *terrain_feature = nullptr;

		QColor color;
		if (std::holds_alternative<const wyrmgus::terrain_feature *>(kv_pair.first)) {
			terrain_feature = std::get<const wyrmgus::terrain_feature *>(kv_pair.first);
			terrain = terrain_feature->get_terrain_type();
			color = terrain_feature->get_color();
		} else {
			terrain = std::get<const terrain_type *>(kv_pair.first);
			color = terrain->get_color();
		}

		for (const auto &geoshape : kv_pair.second) {
			geoshape::write_to_image(*geoshape, image, color, georectangle, this->get_map_projection(), image_checkpoint_save_filename);
		}
	}
}

void map_template::create_terrain_image_from_map(QImage &image, const point_map<const terrain_type *> &terrain_map) const
{
	for (const auto &kv_pair : terrain_map) {
		const QPoint &tile_pos = kv_pair.first;
		const terrain_type *terrain = kv_pair.second;

		if (terrain != nullptr) {
			image.setPixelColor(tile_pos, terrain->get_color());
		} else {
			image.setPixelColor(tile_pos, terrain_type::none_color);
		}
	}
}

void map_template::save_territory_image(const std::string &filename, const site_map<std::vector<std::unique_ptr<QGeoShape>>> &territory_data) const
{
	const std::filesystem::path &territory_filepath = this->get_territory_file();

	QImage image;
	if (!territory_filepath.empty()) {
		image = QImage(path::to_qstring(territory_filepath));

		if (image.size() != this->get_size()) {
			throw std::runtime_error("Invalid territory image size for map template \"" + this->get_identifier() + "\".");
		}
	} else {
		image = QImage(this->get_size(), QImage::Format_RGBA8888);
		image.fill(Qt::transparent);
	}

	const wyrmgus::georectangle georectangle = this->get_georectangle();

	for (const auto &kv_pair : territory_data) {
		const site *settlement = kv_pair.first;
		const QColor color = settlement->get_color();
		if (!color.isValid()) {
			throw std::runtime_error("Settlement \"" + settlement->get_identifier() + "\" has no valid color.");
		}

		for (const auto &geoshape : kv_pair.second) {
			geoshape::write_to_image(*geoshape, image, color, georectangle, this->get_map_projection(), filename);
		}
	}

	image.save(QString::fromStdString(filename));
}

void map_template::add_site(const site *site)
{
	if (!site->get_geocoordinate().is_null()) {
		const auto find_iterator = this->sites_by_geocoordinate.find(site->get_geocoordinate());
		if (find_iterator == this->sites_by_geocoordinate.end()) {
			this->sites_by_geocoordinate[site->get_geocoordinate()] = site;
		} else {
			throw std::runtime_error("Geocoordinate " + site->get_geocoordinate().to_string() + " of map template \"" + this->get_identifier() + "\" already has a site (\"" + find_iterator->second->get_identifier() + "\").");
		}
	} else if (!site->get_astrocoordinate().is_null()) {
		const auto find_iterator = this->sites_by_astrocoordinate.find(site->get_geocoordinate());
		if (find_iterator == this->sites_by_astrocoordinate.end()) {
			this->sites_by_astrocoordinate[site->get_astrocoordinate()] = site;
		} else {
			throw std::runtime_error("Astrocoordinate " + site->get_astrocoordinate().to_string() + " of map template \"" + this->get_identifier() + "\" already has a site (\"" + find_iterator->second->get_identifier() + "\").");
		}
	} else if (site->get_pos().x() != -1 && site->get_pos().y() != -1) {
		const auto find_iterator = this->sites_by_position.find(site->get_pos());
		if (find_iterator == this->sites_by_position.end()) {
			this->sites_by_position[site->get_pos()] = site;
		} else {
			throw std::runtime_error("Position " + point::to_string(site->get_pos()) + " of map template \"" + this->get_identifier() + "\" already has a site (\"" + find_iterator->second->get_identifier() + "\").");
		}
	}

	this->sites.push_back(site);
}

}
