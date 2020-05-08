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

#include "map/map_template.h"

#include "campaign.h"
#include "civilization.h"
#include "config.h"
#include "editor.h"
#include "faction.h"
#include "game.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/historical_location.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "plane.h"
#include "player.h"
#include "quest.h"
#include "settings.h"
#include "time/calendar.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "translate.h"
#include "unit/historical_unit.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/point_util.h"
#include "util/size_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"
#include "video.h"
#include "world.h"

namespace stratagus {

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

			auto generated_terrain = std::make_unique<stratagus::generated_terrain>(terrain_type);
			database::process_sml_data(generated_terrain, child_scope);

			this->generated_terrains.push_back(std::move(generated_terrain));
		});
	} else if (tag == "generated_neutral_units" || tag == "player_location_generated_neutral_units") {
		scope.for_each_property([&](const sml_property &property) {
			CUnitType *unit_type = CUnitType::get(property.get_key());
			const int quantity = std::stoi(property.get_value());

			if (tag == "generated_neutral_units") {
				this->GeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			} else if (tag == "player_location_generated_neutral_units") {
				this->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			}
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void map_template::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "circle") {
			this->circle = string::to_bool(value);
		} else if (key == "plane") {
			stratagus::plane *plane = plane::get(value);
			this->plane = plane;
		} else if (key == "world") {
			stratagus::world *world = world::get(value);
			this->set_world(world);
		} else if (key == "terrain_file") {
			this->terrain_file = value;
		} else if (key == "overlay_terrain_file") {
			this->overlay_terrain_file = value;
		} else if (key == "terrain_image") {
			this->terrain_image = value;
		} else if (key == "overlay_terrain_image") {
			this->overlay_terrain_image = value;
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
		} else if (key == "output_terrain_image") {
			this->output_terrain_image = string::to_bool(value);
		} else {
			fprintf(stderr, "Invalid map template property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "generated_neutral_unit" || child_config_data->Tag == "player_location_generated_neutral_unit") {
			CUnitType *unit_type = nullptr;
			int quantity = 1;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "unit_type") {
					unit_type = CUnitType::get(value);
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
				this->GeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			} else if (child_config_data->Tag == "player_location_generated_neutral_unit") {
				this->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<CUnitType *, int>(unit_type, quantity));
			}
		} else if (child_config_data->Tag == "generated_terrain") {
			auto generated_terrain = std::make_unique<stratagus::generated_terrain>();
			
			generated_terrain->ProcessConfigData(child_config_data);
				
			if (!generated_terrain->TerrainType) {
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

		if (this->get_main_template()->get_plane() != nullptr) {
			this->plane = this->get_main_template()->get_plane();
		}
		if (this->get_main_template()->get_world() != nullptr) {
			this->world = this->get_main_template()->get_world();
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

	data_entry::initialize();
}

void map_template::ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const
{
	std::filesystem::path terrain_file;
	if (overlay) {
		terrain_file = this->get_overlay_terrain_file();
	} else {
		terrain_file = this->get_terrain_file();
	}
	
	if (terrain_file.empty()) {
		return;
	}
	
	const std::string terrain_filename = LibraryFileName(terrain_file.string().c_str());
		
	if (!CanAccessFile(terrain_filename.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", terrain_filename.c_str());
	}
	
	std::ifstream is_map(terrain_filename);
	
	std::string line_str;
	int y = 0;
	while (std::getline(is_map, line_str))
	{
		if (y < template_start_pos.y || y >= (template_start_pos.y + CMap::Map.Info.MapHeights[z])) {
			y += 1;
			continue;
		}

		if (this->get_end_pos().y() != -1 && y > this->get_end_pos().y()) {
			break;
		}

		int x = 0;
		
		for (unsigned int i = 0; i < line_str.length(); ++i) {
			try {
				if (x < template_start_pos.x || x >= (template_start_pos.x + CMap::Map.Info.MapWidths[z])) {
					x++;
					continue;
				}

				if (this->get_end_pos().x() != -1 && x > this->get_end_pos().x()) {
					break;
				}

				const char terrain_character = line_str.at(i);
				terrain_type *terrain = nullptr;

				const QPoint real_pos(map_start_pos.x + x - template_start_pos.x, map_start_pos.y + y - template_start_pos.y);
				CMapField *tile = CMap::Map.Field(real_pos, z);

				if (terrain_character != '0' && terrain_character != '=') {
					terrain = terrain_type::get_by_character(terrain_character);
					tile->SetTerrain(terrain);
				} else if (terrain_character == '0') {
					if (overlay) { //"0" in an overlay terrain file means no overlay, while "=" means no change
						if (tile->OverlayTerrain) {
							tile->RemoveOverlayTerrain();
						}
					} else {
						throw std::runtime_error("\"0\" cannot be used for non-overlay terrain files.");
					}
				}

				x += 1;
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to parse character " + std::to_string(i) + " of line for terrain file \"" + terrain_filename + "\": \"" + line_str + "\"."));
			}
		}
		
		y += 1;
	}

//	std::string filename = this->Ident;
//	if (overlay) {
//		filename += "-overlay";
//	}
//	filename += ".png";
//	save_map_template_png(filename.c_str(), this, overlay);
}

void map_template::ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const
{
	std::filesystem::path terrain_file;
	if (overlay) {
		terrain_file = this->get_overlay_terrain_image();
	} else {
		terrain_file = this->get_terrain_image();
	}
	
	if (terrain_file.empty()) {
		ApplyTerrainFile(overlay, template_start_pos, map_start_pos, z);
		return;
	}
	
	const std::string terrain_filename = LibraryFileName(terrain_file.string().c_str());
		
	if (!CanAccessFile(terrain_filename.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", terrain_filename.c_str());
	}
	
	const QImage terrain_image(terrain_filename.c_str());
	
	const int bpp = terrain_image.depth() / 8;

	for (int y = 0; y < terrain_image.height(); ++y) {
		if (y < template_start_pos.y || y >= (template_start_pos.y + CMap::Map.Info.MapHeights[z])) {
			continue;
		}
		
		if (this->get_end_pos().y() != -1 && y > this->get_end_pos().y()) {
			break;
		}

		for (int x = 0; x < terrain_image.width(); ++x) {
			if (x < template_start_pos.x || x >= (template_start_pos.x + CMap::Map.Info.MapWidths[z])) {
				continue;
			}

			if (this->get_end_pos().x() != -1 && x > this->get_end_pos().x()) {
				break;
			}

			const QColor color = terrain_image.pixelColor(x, y);
			
			if (color.alpha() == 0) { //transparent pixels means leaving the area as it is (e.g. if it is a subtemplate use the main template's terrain for this tile instead)
				continue;
			}

			terrain_type *terrain = nullptr;
			short terrain_feature_id = -1;
			if (TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(color.red(), color.green(), color.blue())) != TerrainFeatureColorToIndex.end()) {
				terrain_feature_id = TerrainFeatureColorToIndex.find(std::tuple<int, int, int>(color.red(), color.green(), color.blue()))->second;
				terrain = TerrainFeatures[terrain_feature_id]->TerrainType;
			} else {
				terrain = terrain_type::try_get_by_color(color);
			}
			Vec2i real_pos(map_start_pos.x + (x - template_start_pos.x), map_start_pos.y + (y - template_start_pos.y));

			if (!CMap::Map.Info.IsPointOnMap(real_pos, z)) {
				continue;
			}

			if (terrain) {
				CMap::Map.Field(real_pos, z)->SetTerrain(terrain);

				if (terrain_feature_id != -1) {
					CMap::Map.Field(real_pos, z)->TerrainFeature = TerrainFeatures[terrain_feature_id];
				}
			} else {
				if (color.red() != 0 || color.green() != 0 || color.blue() != 0 || !overlay) { //fully black pixels represent areas in overlay terrain files that don't have any overlays
					throw std::runtime_error("Invalid map terrain: (" + std::to_string(x) + ", " + std::to_string(y) + ") (RGB: " + std::to_string(color.red()) + "/" + std::to_string(color.green()) + "/" + std::to_string(color.blue()) + ").");
				} else if (overlay && CMap::Map.Field(real_pos, z)->OverlayTerrain) { //fully black pixel in overlay terrain map = no overlay
					CMap::Map.Field(real_pos, z)->RemoveOverlayTerrain();
				}
			}
		}
	}
}

void map_template::Apply(const QPoint &template_start_pos, const QPoint &map_start_pos, const int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	if (template_start_pos.x() < 0 || template_start_pos.x() >= this->get_width() || template_start_pos.y() < 0 || template_start_pos.y() >= this->get_height()) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), template_start_pos.x(), template_start_pos.y());
		return;
	}

	this->current_map_start_pos = map_start_pos;
	this->current_start_pos = template_start_pos;

	const campaign *current_campaign = game::get()->get_current_campaign();
	
	if (z >= (int)CMap::Map.MapLayers.size()) {
		int width = std::min(this->get_applied_width(), CMap::Map.Info.MapWidth);
		int height = std::min(this->get_applied_height(), CMap::Map.Info.MapHeight);
		if (current_campaign != nullptr && z < static_cast<int>(current_campaign->MapSizes.size())) {
			//applies the map size set for the campaign for this map layer; for the first map layer that is already Map.Info.Width/Height, so it isn't necessary here
			width = current_campaign->MapSizes[z].x;
			height = current_campaign->MapSizes[z].y;
		}
	
		CMapLayer *map_layer = new CMapLayer(width, height);
		map_layer->ID = CMap::Map.MapLayers.size();
		CMap::Map.Info.MapWidths.push_back(map_layer->get_width());
		CMap::Map.Info.MapHeights.push_back(map_layer->get_height());
		map_layer->plane = this->get_plane();
		map_layer->world = this->get_world();
		CMap::Map.MapLayers.push_back(map_layer);
	} else {
		if (!this->IsSubtemplateArea()) {
			CMap::Map.MapLayers[z]->plane = this->get_plane();
			CMap::Map.MapLayers[z]->world = this->get_world();
		}
	}

	if (!this->IsSubtemplateArea()) {
		if (Editor.Running == EditorNotRunning) {
			if (this->get_world() != nullptr && this->get_world()->SeasonSchedule) {
				CMap::Map.MapLayers[z]->SeasonSchedule = this->get_world()->SeasonSchedule;
			} else if (!this->get_world() && this->get_plane() && this->get_plane()->SeasonSchedule) {
				CMap::Map.MapLayers[z]->SeasonSchedule = this->get_plane()->SeasonSchedule;
			} else {
				CMap::Map.MapLayers[z]->SeasonSchedule = CSeasonSchedule::DefaultSeasonSchedule;
			}
			
			CMap::Map.MapLayers[z]->SetSeasonByHours(CDate::CurrentTotalHours);
			
			CMap::Map.MapLayers[z]->TimeOfDaySchedule = nullptr;
			CMap::Map.MapLayers[z]->SetTimeOfDay(nullptr);
			
			if (!GameSettings.Inside && !GameSettings.NoTimeOfDay) {
				if (this->get_world() && this->get_world()->TimeOfDaySchedule) {
					CMap::Map.MapLayers[z]->TimeOfDaySchedule = this->get_world()->TimeOfDaySchedule;
				} else if (!this->get_world() && this->get_plane() && this->get_plane()->TimeOfDaySchedule) {
					CMap::Map.MapLayers[z]->TimeOfDaySchedule = this->get_plane()->TimeOfDaySchedule;
				} else {
					CMap::Map.MapLayers[z]->TimeOfDaySchedule = CTimeOfDaySchedule::DefaultTimeOfDaySchedule;
				}
				
				CMap::Map.MapLayers[z]->SetTimeOfDayByHours(CDate::CurrentTotalHours);
			}
		}
	}
	
	const QPoint map_end(std::min(CMap::Map.Info.MapWidths[z], map_start_pos.x() + this->get_applied_width()), std::min(CMap::Map.Info.MapHeights[z], map_start_pos.y() + this->get_applied_height()));
	if (!CMap::Map.Info.IsPointOnMap(map_start_pos, z)) {
		fprintf(stderr, "Invalid map coordinate for map template \"%s\": (%d, %d)\n", this->Ident.c_str(), map_start_pos.x(), map_start_pos.y());
		return;
	}
	
	bool has_base_map = !this->get_terrain_file().empty() || !this->get_terrain_image().empty();
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Terrain"), this->get_name().c_str());
	
	if (this->get_base_terrain_type() != nullptr || this->get_border_terrain_type() != nullptr) {
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
				} else {
					continue;
				}

				Vec2i tile_pos(x, y);
				CMap::Map.Field(tile_pos, z)->SetTerrain(terrain);
				
				if (overlay_terrain != nullptr) {
					CMap::Map.Field(tile_pos, z)->SetTerrain(overlay_terrain);
				} else {
					CMap::Map.Field(tile_pos, z)->RemoveOverlayTerrain();
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

				CMap::Map.Field(tile_pos, z)->SetTerrain(this->get_unusable_area_terrain_type());
				
				if (this->get_unusable_area_overlay_terrain_type()) {
					CMap::Map.Field(tile_pos, z)->SetTerrain(this->get_unusable_area_overlay_terrain_type());
				} else {
					CMap::Map.Field(tile_pos, z)->RemoveOverlayTerrain();
				}
			}
		}
	}
	
	try {
		this->ApplyTerrainImage(false, template_start_pos, map_start_pos, z);
		this->ApplyTerrainImage(true, template_start_pos, map_start_pos, z);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to apply terrain file for map template \"" + this->get_identifier() + "\"."));
	}
	
	if (this->outputs_terrain_image()) {
		const std::string filename = this->get_identifier() + ".png";
		const std::string overlay_filename = this->get_identifier() + "_overlay.png";
		
		save_map_template_png(filename.c_str(), this, false);
		save_map_template_png(overlay_filename.c_str(), this, true);
	}

	for (const auto &kv_pair : this->get_tile_terrains()) {
		const QPoint &tile_pos = kv_pair.first;

		if (!this->contains_pos(tile_pos)) {
			continue;
		}

		terrain_type *terrain = kv_pair.second;
		const QPoint tile_map_pos = map_start_pos + tile_pos - template_start_pos;

		if (!this->contains_map_pos(tile_map_pos)) {
			continue;
		}

		CMap::Map.Field(tile_map_pos, z)->SetTerrain(terrain);
	}

	if (current_campaign) {
		for (size_t i = 0; i < HistoricalTerrains.size(); ++i) {
			Vec2i history_pos = std::get<0>(HistoricalTerrains[i]);
			if (history_pos.x < template_start_pos.x() || history_pos.x >= (template_start_pos.x() + CMap::Map.Info.MapWidths[z]) || history_pos.y < template_start_pos.y() || history_pos.y >= (template_start_pos.y() + CMap::Map.Info.MapHeights[z])) {
				continue;
			}
			if (current_campaign->get_start_date() >= std::get<2>(HistoricalTerrains[i]) || std::get<2>(HistoricalTerrains[i]).Year == 0) {
				terrain_type *historical_terrain = std::get<1>(HistoricalTerrains[i]);
				
				Vec2i real_pos(map_start_pos.x() + history_pos.x - template_start_pos.x(), map_start_pos.y() + history_pos.y - template_start_pos.y());

				if (!CMap::Map.Info.IsPointOnMap(real_pos, z)) {
					continue;
				}

				if (historical_terrain) {
					if (historical_terrain->is_overlay() && ((historical_terrain->Flags & MapFieldRoad) || (historical_terrain->Flags & MapFieldRailroad)) && !(CMap::Map.Field(real_pos, z)->Flags & MapFieldLandAllowed)) {
						continue;
					}
					CMap::Map.Field(real_pos, z)->SetTerrain(historical_terrain);
				} else { //if the terrain type is null, then that means a previously set overlay terrain should be removed
					CMap::Map.Field(real_pos, z)->RemoveOverlayTerrain();
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
				if (!CMap::Map.Info.IsPointOnMap(surrounding_pos, z) || CMap::Map.is_point_in_a_subtemplate_area(surrounding_pos, z)) {
					continue;
				}
				CMap::Map.Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_terrain_type());
				if (this->get_surrounding_overlay_terrain_type() != nullptr) {
					CMap::Map.Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_overlay_terrain_type());
				}
			}
		}
		for (int x = surrounding_start_pos.x(); x < surrounding_end.x(); x += (surrounding_end.x() - surrounding_start_pos.x() - 1)) {
			for (int y = surrounding_start_pos.y(); y < surrounding_end.y(); ++y) {
				const QPoint surrounding_pos(x, y);
				if (!CMap::Map.Info.IsPointOnMap(surrounding_pos, z) || CMap::Map.is_point_in_a_subtemplate_area(surrounding_pos, z)) {
					continue;
				}
				CMap::Map.Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_terrain_type());
				if (this->get_surrounding_overlay_terrain_type() != nullptr) {
					CMap::Map.Field(surrounding_pos, z)->SetTerrain(this->get_surrounding_overlay_terrain_type());
				}
			}
		}
	}
	
	if (current_campaign) {
		faction *current_faction = current_campaign->get_faction();
		if (current_faction != nullptr && !this->IsSubtemplateArea() && CPlayer::GetThisPlayer()->Faction != current_faction->ID) {
			CPlayer::GetThisPlayer()->set_civilization(current_faction->get_civilization()->ID);
			CPlayer::GetThisPlayer()->SetFaction(current_faction);
			CPlayer::GetThisPlayer()->Resources[CopperCost] = 2500; // give the player enough resources to start up
			CPlayer::GetThisPlayer()->Resources[WoodCost] = 2500;
			CPlayer::GetThisPlayer()->Resources[StoneCost] = 2500;
		}
	}
	
	if (!this->get_subtemplates().empty()) {
		ShowLoadProgress(_("Applying \"%s\" Subtemplates"), this->get_name().c_str());
		this->apply_subtemplates(template_start_pos, map_start_pos, map_end, z, false);
		this->apply_subtemplates(template_start_pos, map_start_pos, map_end, z, true);
	}

	CMap::Map.GenerateMissingTerrain(map_start_pos, map_end - Vec2i(1, 1), z, this);
	
	if (!has_base_map) {
		ShowLoadProgress(_("Generating \"%s\" Map Template Random Terrain"), this->get_name().c_str());
		
		for (const auto &generated_terrain : this->generated_terrains) {
			int map_width = (map_end.x() - map_start_pos.x());
			int map_height = (map_end.y() - map_start_pos.y());
			
			CMap::Map.GenerateTerrain(generated_terrain, map_start_pos, map_end - Vec2i(1, 1), has_base_map, z);
		}
	}
	
	if (!this->IsSubtemplateArea()) {
		ShowLoadProgress(_("Adjusting \"%s\" Map Template Terrain"), this->get_name().c_str());
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	ShowLoadProgress(_("Applying \"%s\" Map Template Units"), this->get_name().c_str());

	for (std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>>::const_iterator iterator = this->Resources.begin(); iterator != this->Resources.end(); ++iterator) {
		Vec2i unit_raw_pos(iterator->first.first, iterator->first.second);
		Vec2i unit_pos(map_start_pos.x() + unit_raw_pos.x - template_start_pos.x(), map_start_pos.y() + unit_raw_pos.y - template_start_pos.y());
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		
		const CUnitType *type = std::get<0>(iterator->second);
		
		Vec2i unit_offset((type->TileSize - 1) / 2);
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
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
			unit->SetUnique(std::get<2>(iterator->second));
		}
	}

	if (current_campaign != nullptr) {
		this->ApplyConnectors(template_start_pos, map_start_pos, map_end, z);
	}
	this->apply_sites(template_start_pos, map_start_pos, map_end, z);
	this->ApplyUnits(template_start_pos, map_start_pos, map_end, z);
	
	bool generated_random_terrain = false;
	if (has_base_map) {
		ShowLoadProgress(_("Generating \"%s\" Map Template Random Terrain"), this->get_name().c_str());
		
		for (const auto &generated_terrain : this->generated_terrains) {
			int map_width = (map_end.x() - map_start_pos.x());
			int map_height = (map_end.y() - map_start_pos.y());
			
			CMap::Map.GenerateTerrain(generated_terrain, map_start_pos, map_end - Vec2i(1, 1), has_base_map, z);
			generated_random_terrain = true;
		}
	}
	
	if (!this->IsSubtemplateArea() && generated_random_terrain) {
		ShowLoadProgress(_("Readjusting \"%s\" Map Template Terrain"), this->get_name().c_str());
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapTransitions(map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		CMap::Map.AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
	
	ShowLoadProgress(_("Generating \"%s\" Map Template Random Units"), this->get_name().c_str());

	// now, generate the units and heroes that were set to be generated at a random position (by having their position set to {-1, -1})
	if (current_campaign != nullptr) {
		this->ApplyConnectors(template_start_pos, map_start_pos, map_end, z, true);
	}
	this->apply_sites(template_start_pos, map_start_pos, map_end, z, true);
	this->ApplyUnits(template_start_pos, map_start_pos, map_end, z, true);

	for (int i = 0; i < PlayerMax; ++i) {
		if (CPlayer::Players[i]->Type != PlayerPerson && CPlayer::Players[i]->Type != PlayerComputer && CPlayer::Players[i]->Type != PlayerRescueActive) {
			continue;
		}
		if (CMap::Map.is_point_in_a_subtemplate_area(CPlayer::Players[i]->StartPos, z)) {
			continue;
		}
		if (CPlayer::Players[i]->StartPos.x < map_start_pos.x() || CPlayer::Players[i]->StartPos.y < map_start_pos.y() || CPlayer::Players[i]->StartPos.x >= map_end.x() || CPlayer::Players[i]->StartPos.y >= map_end.y() || CPlayer::Players[i]->StartMapLayer != z) {
			continue;
		}
		if (CPlayer::Players[i]->StartPos.x == 0 && CPlayer::Players[i]->StartPos.y == 0) {
			continue;
		}
		// add five workers at the player's starting location
		if (CPlayer::Players[i]->NumTownHalls > 0) {
			CUnitType *worker_type = faction::get_all()[CPlayer::Players[i]->Faction]->get_class_unit_type(unit_class::get("worker"));
			if (worker_type != nullptr && CPlayer::Players[i]->GetUnitTypeCount(worker_type) == 0) { //only create if the player doesn't have any workers created in another manner
				Vec2i worker_unit_offset((worker_type->TileSize - 1) / 2);
				
				Vec2i worker_pos(CPlayer::Players[i]->StartPos);

				bool start_pos_has_town_hall = false;
				std::vector<CUnit *> table;
				Select(worker_pos - Vec2i(4, 4), worker_pos + Vec2i(4, 4), table, z, HasSamePlayerAs(*CPlayer::Players[i]));
				for (size_t j = 0; j < table.size(); ++j) {
					if (table[j]->Type->BoolFlag[TOWNHALL_INDEX].value) {
						start_pos_has_town_hall = true;
						break;
					}
				}
				
				if (!start_pos_has_town_hall) { //if the start pos doesn't have a town hall, create the workers in the position of a town hall the player has
					for (int j = 0; j < CPlayer::Players[i]->GetUnitCount(); ++j) {
						CUnit *town_hall_unit = &CPlayer::Players[i]->GetUnit(j);
						if (!town_hall_unit->Type->BoolFlag[TOWNHALL_INDEX].value) {
							continue;
						}
						if (town_hall_unit->MapLayer->ID != z) {
							continue;
						}
						worker_pos = town_hall_unit->tilePos;
					}
				}
				
				for (int j = 0; j < 5; ++j) {
					CUnit *worker_unit = CreateUnit(worker_pos, *worker_type, CPlayer::Players[i], CPlayer::Players[i]->StartMapLayer);
				}
			}
		}
		
		if (CPlayer::Players[i]->NumTownHalls > 0 || CPlayer::Players[i]->Index == CPlayer::GetThisPlayer()->Index) {
			for (size_t j = 0; j < this->PlayerLocationGeneratedNeutralUnits.size(); ++j) {
				CMap::Map.GenerateNeutralUnits(this->PlayerLocationGeneratedNeutralUnits[j].first, this->PlayerLocationGeneratedNeutralUnits[j].second, CPlayer::Players[i]->StartPos - Vec2i(8, 8), CPlayer::Players[i]->StartPos + Vec2i(8, 8), true, z);
			}
		}
	}
	
	for (size_t i = 0; i < this->GeneratedNeutralUnits.size(); ++i) {
		bool grouped = this->GeneratedNeutralUnits[i].first->GivesResource && this->GeneratedNeutralUnits[i].first->TileSize.x == 1 && this->GeneratedNeutralUnits[i].first->TileSize.y == 1; // group small resources
		CMap::Map.GenerateNeutralUnits(this->GeneratedNeutralUnits[i].first, this->GeneratedNeutralUnits[i].second, map_start_pos, map_end - Vec2i(1, 1), grouped, z);
	}

	//this has to be done at the end, so that it doesn't prevent the application from working properly, due to the map template code thinking that its own area belongs to another map template
	if (this->IsSubtemplateArea()) {
		CMap::Map.MapLayers[z]->subtemplate_areas.push_back(std::tuple<Vec2i, Vec2i, map_template *>(map_start_pos, map_end - Vec2i(1, 1), this));
	}
}

/**
**	@brief	Apply the subtemplates to the map
**
**	@param	template_start_pos	The start position of the map relative to the map template
**	@param	map_start_pos		The start position of the map template relative to the map
**	@param	z					The map layer
**	@param	random				Whether it is subtemplates with a random position that should be applied, or ones with a fixed one
*/
void map_template::apply_subtemplates(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	for (map_template *subtemplate : this->get_subtemplates()) {
		if (CMap::Map.is_subtemplate_on_map(subtemplate)) {
			continue;
		}

		this->apply_subtemplate(subtemplate, template_start_pos, map_start_pos, map_end, z, random);
	}
}

void map_template::apply_subtemplate(map_template *subtemplate, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	QPoint subtemplate_pos = subtemplate->get_subtemplate_top_left_pos();
	bool found_location = false;

	if (subtemplate->UpperTemplate && (subtemplate_pos.x() < 0 || subtemplate_pos.y() < 0)) { //if has no given position, but has an upper template, use its coordinates instead
		subtemplate_pos = CMap::Map.get_subtemplate_pos(subtemplate->UpperTemplate);
		if (subtemplate_pos.x() >= 0 && subtemplate_pos.y() >= 0) {
			found_location = true;
		}
	}

	if (subtemplate->LowerTemplate && (subtemplate_pos.x() < 0 || subtemplate_pos.y() < 0)) { //if has no given position, but has a lower template, use its coordinates instead
		subtemplate_pos = CMap::Map.get_subtemplate_pos(subtemplate->LowerTemplate);
		if (subtemplate_pos.x() >= 0 && subtemplate_pos.y() >= 0) {
			found_location = true;
		}
	}

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
				if (!subtemplate->is_optional()) {
					fprintf(stderr, "No location available for map template \"%s\" to be applied to.\n", subtemplate->get_identifier().c_str());
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
		if (subtemplate_pos.x() >= 0 && subtemplate_pos.y() >= 0 && subtemplate_pos.x() < CMap::Map.Info.MapWidths[z] && subtemplate_pos.y() < CMap::Map.Info.MapHeights[z]) {
			subtemplate->Apply(subtemplate->get_start_pos(), subtemplate_pos, z);

			//also apply all dependent adjacent templates, if the other templates they depend on have also been applied, so that they are more likely to be close to this subtemplate
			for (map_template *dependent_subtemplate : subtemplate->dependent_adjacent_templates) {
				if (CMap::Map.is_subtemplate_on_map(dependent_subtemplate)) {
					continue;
				}

				bool other_dependencies_fulfilled = true;
				for (const map_template *dependency_template : dependent_subtemplate->dependency_templates) {
					if (!CMap::Map.is_subtemplate_on_map(dependency_template)) {
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
	} else if (!subtemplate->is_optional()) {
		throw std::runtime_error("Failed to apply subtemplate \"" + subtemplate->get_identifier() + "\".");
	}
}

/**
**	@brief	Apply sites to the map
**
**	@param	template_start_pos	The start position of the map relative to the map template
**	@param	map_start_pos		The start position of the map template relative to the map
**	@param	z					The map layer
**	@param	random				Whether it is sites with a random position that should be applied, or ones with a fixed one
*/
void map_template::apply_sites(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	const campaign *current_campaign = game::get()->get_current_campaign();
	CDate start_date;
	if (current_campaign != nullptr) {
		start_date = current_campaign->get_start_date();
	}

	for (site *site : this->sites) {
		const QPoint site_raw_pos = site->get_pos();
		Vec2i site_pos(map_start_pos + site_raw_pos - template_start_pos);

		Vec2i unit_offset((settlement_site_unit_type->TileSize - 1) / 2);
			
		if (random) {
			if (site_raw_pos.x() != -1 || site_raw_pos.y() != -1) {
				continue;
			}
			if (settlement_site_unit_type) {
				site_pos = CMap::Map.GenerateUnitLocation(settlement_site_unit_type, nullptr, map_start_pos, map_end - Vec2i(1, 1), z);
				site_pos += unit_offset;
			}
		} else {
			if (site_raw_pos.x() == -1 && site_raw_pos.y() == -1) {
				continue;
			}
		}

		if (!CMap::Map.Info.IsPointOnMap(site_pos, z) || site_pos.x < map_start_pos.x() || site_pos.y < map_start_pos.y()) {
			continue;
		}

		site->load_history();

		if (site->is_major() && settlement_site_unit_type != nullptr) { //add a settlement site for major sites
			if (!UnitTypeCanBeAt(*settlement_site_unit_type, site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset + Vec2i(settlement_site_unit_type->TileSize - 1), z)) {
				fprintf(stderr, "The settlement site for \"%s\" should be placed on (%d, %d), but it cannot be there.\n", site->Ident.c_str(), site_raw_pos.x(), site_raw_pos.y());
			}
			CUnit *unit = CreateUnit(site_pos - unit_offset, *settlement_site_unit_type, CPlayer::Players[PlayerNumNeutral], z, true);
			unit->settlement = site;
			unit->settlement->set_site_unit(unit);
			CMap::Map.site_units.push_back(unit);
		}
		
		for (size_t j = 0; j < site->HistoricalResources.size(); ++j) {
			if (
				(!current_campaign && std::get<1>(site->HistoricalResources[j]).Year == 0 && std::get<1>(site->HistoricalResources[j]).Year == 0)
				|| (
					current_campaign && start_date.ContainsDate(std::get<0>(site->HistoricalResources[j]))
					&& (!start_date.ContainsDate(std::get<1>(site->HistoricalResources[j])) || std::get<1>(site->HistoricalResources[j]).Year == 0)
				)
			) {
				const CUnitType *type = std::get<2>(site->HistoricalResources[j]);
				if (!type) {
					fprintf(stderr, "Error in CMap::apply_sites (site ident \"%s\"): historical resource type is null.\n", site->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((type->TileSize - 1) / 2);
				CUnit *unit = CreateResourceUnit(site_pos - unit_offset, *type, z, false); // don't generate unique resources when setting special properties, since for map templates unique resources are supposed to be explicitly indicated
				if (std::get<3>(site->HistoricalResources[j])) {
					unit->SetUnique(std::get<3>(site->HistoricalResources[j]));
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
		
		if (current_campaign == nullptr) {
			continue;
		}
		
		const faction *site_owner = site->get_owner_faction();
		if (site_owner == nullptr) {
			//fall back to the old historical owners functionality
			for (std::map<CDate, const faction *>::reverse_iterator owner_iterator = site->HistoricalOwners.rbegin(); owner_iterator != site->HistoricalOwners.rend(); ++owner_iterator) {
				if (start_date.ContainsDate(owner_iterator->first)) { // set the owner to the latest historical owner given the scenario's start date
					site_owner = owner_iterator->second;
					break;
				}
			}
		}
		
		if (!site_owner) {
			continue;
		}
		
		CPlayer *player = GetOrAddFactionPlayer(site_owner);
		
		if (!player) {
			continue;
		}
		
		bool is_capital = false;
		for (int i = ((int) site_owner->HistoricalCapitals.size() - 1); i >= 0; --i) {
			if (start_date.ContainsDate(site_owner->HistoricalCapitals[i].first) || site_owner->HistoricalCapitals[i].first.Year == 0) {
				if (site_owner->HistoricalCapitals[i].second == site->Ident) {
					is_capital = true;
				}
				break;
			}
		}
		
		if ((player->StartPos.x == 0 && player->StartPos.y == 0) || is_capital) {
			player->SetStartView(site_pos, z);
		}
		
		CUnitType *pathway_type = nullptr;
		for (size_t j = 0; j < site->HistoricalBuildings.size(); ++j) {
			if (
				start_date.ContainsDate(std::get<0>(site->HistoricalBuildings[j]))
				&& (!start_date.ContainsDate(std::get<1>(site->HistoricalBuildings[j])) || std::get<1>(site->HistoricalBuildings[j]).Year == 0)
			) {
				CUnitType *unit_type = site_owner->get_class_unit_type(std::get<2>(site->HistoricalBuildings[j]));
				if (unit_type == nullptr) {
					continue;
				}
				if (unit_type->TerrainType != nullptr) {
					if ((unit_type->TerrainType->Flags & MapFieldRoad) || (unit_type->TerrainType->Flags & MapFieldRailroad)) {
						pathway_type = unit_type;
					}
				}
			}
		}
		
		bool first_building = true;
		for (const unit_class *building_class : site->get_building_classes()) {
			const CUnitType *unit_type = site_owner->get_class_unit_type(building_class);

			if (unit_type == nullptr) {
				continue;
			}

			if (unit_type->TerrainType) {
				throw std::runtime_error("A terrain type building (e.g. a wall) cannot be applied from the list of historical building classes of a site.");
			}

			if (unit_type->BoolFlag[TOWNHALL_INDEX].value && !site->is_major()) {
				throw std::runtime_error("Site \"" + site->get_identifier() + "\" has a town hall, but isn't set as a major one.");
			}

			const QPoint unit_offset = unit_type->get_tile_center_pos_offset();
			if (first_building) {
				if (!OnTopDetails(*unit_type, nullptr) && !UnitTypeCanBeAt(*unit_type, site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset + QPoint(unit_type->TileSize - 1), z)) {
					throw std::runtime_error("The \"" + unit_type->get_identifier() + "\" representing the minor site of \"" + site->get_identifier() + "\" should be placed on (" + std::to_string(site_raw_pos.x()) + ", " + std::to_string(site_raw_pos.y()) + "), but it cannot be there.");
				}
			}

			CUnit *unit = CreateUnit(site_pos - unit_offset, *unit_type, player, z, true);

			if (first_building) {
				if (!unit_type->BoolFlag[TOWNHALL_INDEX].value) { //if one building is representing a minor site, make it have the site's name
					unit->Name = site->GetCulturalName(site_owner->get_civilization());
				}
				first_building = false;
			}

			if (unit_type->BoolFlag[TOWNHALL_INDEX].value) {
				unit->UpdateBuildingSettlementAssignment();
			}

			if (pathway_type) {
				for (int x = unit->tilePos.x - 1; x < unit->tilePos.x + unit->Type->TileSize.x + 1; ++x) {
					for (int y = unit->tilePos.y - 1; y < unit->tilePos.y + unit->Type->TileSize.y + 1; ++y) {
						if (!CMap::Map.Info.IsPointOnMap(x, y, unit->MapLayer)) {
							continue;
						}
						CMapField &mf = *unit->MapLayer->Field(x, y);
						if (mf.Flags & MapFieldBuilding) { //this is a tile where the building itself is located, continue
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
				const CUnitType *unit_type = nullptr;
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
				if (unit_type->BoolFlag[TOWNHALL_INDEX].value && !site->is_major()) {
					fprintf(stderr, "Error in CMap::apply_sites (site ident \"%s\"): site has a town hall, but isn't set as a major one.\n", site->Ident.c_str());
					continue;
				}
				Vec2i unit_offset((unit_type->TileSize - 1) / 2);
				if (first_building) {
					if (!OnTopDetails(*unit_type, nullptr) && !UnitTypeCanBeAt(*unit_type, site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(site_pos - unit_offset + Vec2i(unit_type->TileSize - 1), z)) {
						fprintf(stderr, "The \"%s\" representing the minor site of \"%s\" should be placed on (%d, %d), but it cannot be there.\n", unit_type->Ident.c_str(), site->Ident.c_str(), site_raw_pos.x(), site_raw_pos.y());
					}
				}
				CUnit *unit = nullptr;
				if (building_owner) {
					CPlayer *building_player = GetOrAddFactionPlayer(building_owner);
					if (!building_player) {
						continue;
					}
					if (building_player->StartPos.x == 0 && building_player->StartPos.y == 0) {
						building_player->SetStartView(site_pos - unit_offset, z);
					}
					unit = CreateUnit(site_pos - unit_offset, *unit_type, building_player, z, true);
				} else {
					unit = CreateUnit(site_pos - unit_offset, *unit_type, player, z, true);
				}
				if (std::get<3>(site->HistoricalBuildings[j])) {
					unit->SetUnique(std::get<3>(site->HistoricalBuildings[j]));
				}
				if (first_building) {
					if (!unit_type->BoolFlag[TOWNHALL_INDEX].value && !unit->Unique && (!building_owner || building_owner == site_owner)) { //if one building is representing a minor site, make it have the site's name
						unit->Name = site->GetCulturalName(site_owner->get_civilization());
					}
					first_building = false;
				}
				if (unit_type->BoolFlag[TOWNHALL_INDEX].value && (!building_owner || building_owner == site_owner)) {
					unit->UpdateBuildingSettlementAssignment();
				}
				if (pathway_type) {
					for (int x = unit->tilePos.x - 1; x < unit->tilePos.x + unit->Type->TileSize.x + 1; ++x) {
						for (int y = unit->tilePos.y - 1; y < unit->tilePos.y + unit->Type->TileSize.y + 1; ++y) {
							if (!CMap::Map.Info.IsPointOnMap(x, y, unit->MapLayer)) {
								continue;
							}
							CMapField &mf = *unit->MapLayer->Field(x, y);
							if (mf.Flags & MapFieldBuilding) { //this is a tile where the building itself is located, continue
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
		
		for (size_t j = 0; j < site->HistoricalUnits.size(); ++j) {
			if (
				start_date.ContainsDate(std::get<0>(site->HistoricalUnits[j]))
				&& (!start_date.ContainsDate(std::get<1>(site->HistoricalUnits[j])) || std::get<1>(site->HistoricalUnits[j]).Year == 0)
			) {
				int unit_quantity = std::get<3>(site->HistoricalUnits[j]);
						
				if (unit_quantity > 0) {
					const CUnitType *type = std::get<2>(site->HistoricalUnits[j]);
					if (type->BoolFlag[ORGANIC_INDEX].value) {
						unit_quantity = std::max(1, unit_quantity / PopulationPerUnit); //each organic unit represents 1,000 people
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
					Vec2i unit_offset((type->TileSize - 1) / 2);

					for (int j = 0; j < unit_quantity; ++j) {
						CUnit *unit = CreateUnit(site_pos - unit_offset, *type, unit_player, z);
						if (!type->BoolFlag[HARVESTER_INDEX].value) { // make non-worker units not have an active AI
							unit->Active = 0;
							unit_player->ChangeUnitTypeAiActiveCount(type, -1);
						}
					}
				}
			}
		}
	}
}

void map_template::ApplyConnectors(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const
{
	for (size_t i = 0; i < this->PlaneConnectors.size(); ++i) {
		const CUnitType *type = std::get<1>(this->PlaneConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->PlaneConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		Vec2i unit_offset((type->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(type, nullptr, map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x() || unit_pos.y < map_start_pos.y()) {
			continue;
		}
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, CPlayer::Players[PlayerNumNeutral], z, true);
		if (std::get<3>(this->PlaneConnectors[i])) {
			unit->SetUnique(std::get<3>(this->PlaneConnectors[i]));
		}
		CMap::Map.MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < CMap::Map.MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (CMap::Map.MapLayers[second_z]->plane == std::get<2>(this->PlaneConnectors[i])) {
				for (size_t j = 0; j < CMap::Map.MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Unique == unit->Unique && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == nullptr) {
						CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = CMap::Map.MapLayers[second_z]->LayerConnectors[j];
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
	
	for (size_t i = 0; i < this->WorldConnectors.size(); ++i) {
		const CUnitType *type = std::get<1>(this->WorldConnectors[i]);
		Vec2i unit_raw_pos(std::get<0>(this->WorldConnectors[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		Vec2i unit_offset((type->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(type, nullptr, map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x() || unit_pos.y < map_start_pos.y()) {
			continue;
		}
		
		if (!OnTopDetails(*type, nullptr) && !UnitTypeCanBeAt(*type, unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset, z) && CMap::Map.Info.IsPointOnMap(unit_pos - unit_offset + Vec2i(type->TileSize - 1), z)) {
			fprintf(stderr, "Unit \"%s\" should be placed on (%d, %d) for map template \"%s\", but it cannot be there.\n", type->Ident.c_str(), unit_raw_pos.x, unit_raw_pos.y, this->Ident.c_str());
		}

		CUnit *unit = CreateUnit(unit_pos - unit_offset, *type, CPlayer::Players[PlayerNumNeutral], z, true);
		if (std::get<3>(this->WorldConnectors[i])) {
			unit->SetUnique(std::get<3>(this->WorldConnectors[i]));
		}
		CMap::Map.MapLayers[z]->LayerConnectors.push_back(unit);
		for (size_t second_z = 0; second_z < CMap::Map.MapLayers.size(); ++second_z) {
			bool found_other_connector = false;
			if (CMap::Map.MapLayers[second_z]->world == std::get<2>(this->WorldConnectors[i])) {
				for (size_t j = 0; j < CMap::Map.MapLayers[second_z]->LayerConnectors.size(); ++j) {
					if (CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Type == unit->Type && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->Unique == unit->Unique && CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination == nullptr) {
						CMap::Map.MapLayers[second_z]->LayerConnectors[j]->ConnectingDestination = unit;
						unit->ConnectingDestination = CMap::Map.MapLayers[second_z]->LayerConnectors[j];
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
	const campaign *current_campaign = game::get()->get_current_campaign();
	CDate start_date;
	if (current_campaign) {
		start_date = current_campaign->get_start_date();
	}

	for (size_t i = 0; i < this->Units.size(); ++i) {
		Vec2i unit_raw_pos(std::get<0>(this->Units[i]));
		Vec2i unit_pos(map_start_pos + unit_raw_pos - template_start_pos);
		const CUnitType *type = std::get<1>(this->Units[i]);
		Vec2i unit_offset((type->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(type, std::get<2>(this->Units[i]), map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x() || unit_pos.y < map_start_pos.y()) {
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
				player = CPlayer::Players[PlayerNumNeutral];
			}

			CUnit *unit = CreateUnit(unit_pos - unit_offset, *std::get<1>(this->Units[i]), player, z, type->BoolFlag[BUILDING_INDEX].value && type->TileSize.x > 1 && type->TileSize.y > 1);
			if (!type->BoolFlag[BUILDING_INDEX].value && !type->BoolFlag[HARVESTER_INDEX].value) { // make non-building, non-harvester units not have an active AI
				unit->Active = 0;
				player->ChangeUnitTypeAiActiveCount(type, -1);
			}
			if (std::get<5>(this->Units[i])) {
				unit->SetUnique(std::get<5>(this->Units[i]));
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
		Vec2i unit_offset((hero->get_unit_type()->TileSize - 1) / 2);
		if (random) {
			if (unit_raw_pos.x != -1 || unit_raw_pos.y != -1) {
				continue;
			}
			unit_pos = CMap::Map.GenerateUnitLocation(hero->get_unit_type(), std::get<2>(this->Heroes[i]), map_start_pos, map_end - Vec2i(1, 1), z);
			unit_pos += unit_offset;
		}
		if (!CMap::Map.Info.IsPointOnMap(unit_pos, z) || unit_pos.x < map_start_pos.x() || unit_pos.y < map_start_pos.y()) {
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
				player = CPlayer::Players[PlayerNumNeutral];
			}
			CUnit *unit = CreateUnit(unit_pos - unit_offset, *hero->get_unit_type(), player, z);
			unit->SetCharacter(hero->Ident);
			if (!unit->Type->BoolFlag[BUILDING_INDEX].value && !unit->Type->BoolFlag[HARVESTER_INDEX].value) { // make non-building, non-harvester units not have an active AI
				unit->Active = 0;
				player->ChangeUnitTypeAiActiveCount(hero->get_unit_type(), -1);
			}
		}
	}
	
	for (const historical_unit *historical_unit : historical_unit::get_all()) {
		if (!historical_unit->is_active()) {
			continue;
		}
		
		faction *unit_faction = historical_unit->get_faction();
		CPlayer *unit_player = unit_faction ? GetFactionPlayer(unit_faction) : nullptr;
		CUnitType *unit_type = historical_unit->get_unit_type();
		if (unit_type == nullptr && historical_unit->get_unit_class() != nullptr) {
			unit_type = unit_faction->get_class_unit_type(historical_unit->get_unit_class());
		}

		if (unit_type == nullptr) {
			continue;
		}
		
		const bool in_another_map_template = historical_unit->get_location()->map_template != this;
		if (in_another_map_template) {
			continue;
		}

		if (historical_unit->get_location()->Position.x != -1 && historical_unit->get_location()->Position.y != -1 && !this->contains_pos(historical_unit->get_location()->Position)) {
			continue;
		}

		QPoint unit_pos = this->get_location_map_position(historical_unit->get_location(), template_start_pos, map_start_pos, false);
		
		if (unit_pos.x() == -1 && unit_pos.y() == -1) {
			if (!random) { //apply units whose position is that of a randomly-placed site, or that of their player's start position, together with randomly-placed units
				continue;
			}
			
			unit_pos = this->get_location_map_position(historical_unit->get_location(), template_start_pos, map_start_pos, true);
			
			if (unit_pos.x() == -1 && unit_pos.y() == -1) {
				unit_pos = CMap::Map.GenerateUnitLocation(unit_type, unit_faction, map_start_pos, map_end - Vec2i(1, 1), z);
				if (unit_pos.x() != -1 && unit_pos.y() != -1) {
					unit_pos += unit_type->get_tile_center_pos_offset();
				}
			}
		} else {
			if (random) {
				continue;
			}
		}
		
		const QPoint unit_top_left_pos = unit_pos - unit_type->get_tile_center_pos_offset();
		const QPoint unit_bottom_right_pos = unit_top_left_pos + size::to_point(unit_type->GetTileSize()) - QPoint(1, 1);
		if (!CMap::Map.Info.IsPointOnMap(unit_top_left_pos, z) || !CMap::Map.Info.IsPointOnMap(unit_bottom_right_pos, z) || !this->contains_map_pos(unit_top_left_pos) || !this->contains_map_pos(unit_bottom_right_pos)) { //units whose faction hasn't been created already and who don't have a valid historical location set won't be created
			continue;
		}
		
		if (unit_faction != nullptr) {
			unit_player = GetOrAddFactionPlayer(unit_faction);
			if (!unit_player) {
				continue;
			}
			if (!unit_type->BoolFlag[ITEM_INDEX].value && unit_player->StartPos.x == 0 && unit_player->StartPos.y == 0) {
				unit_player->SetStartView(unit_pos, z);
			}
		} else {
			unit_player = CPlayer::Players[PlayerNumNeutral];
		}
		for (int i = 0; i < historical_unit->get_quantity(); ++i) {
			//item units only use factions to generate special properties for them
			CUnit *unit = CreateUnit(unit_top_left_pos, *unit_type, unit_type->BoolFlag[ITEM_INDEX].value ? CPlayer::Players[PlayerNumNeutral] : unit_player, z);
			if (unit_type->BoolFlag[ITEM_INDEX].value) {
				unit->GenerateSpecialProperties(nullptr, unit_player, false);
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
	
	for (character *character : character::get_all()) {
		if (!character->CanAppear()) {
			continue;
		}
		
		if (character->get_faction() == nullptr && !character->get_unit_type()->BoolFlag[FAUNA_INDEX].value) { //only fauna "heroes" may have no faction
			continue;
		}
		
		if (character->StartDate.Year == 0 || !start_date.ContainsDate(character->StartDate) || start_date.ContainsDate(character->DeathDate)) { //contrary to other elements, heroes aren't implemented if their date isn't set
			continue;
		}
		
		const faction *hero_faction = character->get_faction();
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
		
		if (!CMap::Map.Info.IsPointOnMap(hero_pos, z) || hero_pos.x < map_start_pos.x() || hero_pos.y < map_start_pos.y()) { //heroes whose faction hasn't been created already and who don't have a valid historical location set won't be created
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
			hero_player = CPlayer::Players[PlayerNumNeutral];
		}
		CUnit *unit = CreateUnit(hero_pos - character->get_unit_type()->get_tile_center_pos_offset(), *character->get_unit_type(), hero_player, z);
		unit->SetCharacter(character->Ident);
		unit->Active = 0;
		hero_player->ChangeUnitTypeAiActiveCount(character->get_unit_type(), -1);
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

void map_template::set_world(stratagus::world *world)
{
	if (world == this->get_world()) {
		return;
	}

	this->world = world;
	this->plane = this->world->get_plane();
}

void map_template::set_terrain_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_terrain_file()) {
		return;
	}

	this->terrain_file = database::get_maps_path(this->get_module()) / filepath;
}

void map_template::set_overlay_terrain_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_overlay_terrain_file()) {
		return;
	}

	this->overlay_terrain_file = database::get_maps_path(this->get_module()) / filepath;
}

void map_template::set_terrain_image(const std::filesystem::path &filepath)
{
	if (filepath == this->get_terrain_image()) {
		return;
	}

	this->terrain_image = database::get_maps_path(this->get_module()) / filepath;
}

void map_template::set_overlay_terrain_image(const std::filesystem::path &filepath)
{
	if (filepath == this->get_overlay_terrain_image()) {
		return;
	}

	this->overlay_terrain_image = database::get_maps_path(this->get_module()) / filepath;
}

bool map_template::is_dependent_on(const map_template *other_template) const
{
	//get whether this map template is dependent on another (i.e. needs it to establish its position)

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

QPoint map_template::generate_subtemplate_position(const map_template *subtemplate, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const QPoint &max_adjacent_template_distance, bool &adjacency_restriction_occurred) const
{
	QPoint min_pos(map_start_pos);
	QPoint max_pos(map_end.x() - subtemplate->get_applied_width(), map_end.y() - subtemplate->get_applied_height());

	if (subtemplate->get_min_subtemplate_pos().x() != -1) {
		min_pos.setX(min_pos.x() + subtemplate->get_min_subtemplate_pos().x() - template_start_pos.x());
	}
	if (subtemplate->get_min_subtemplate_pos().y() != -1) {
		min_pos.setY(min_pos.y() + subtemplate->get_min_subtemplate_pos().y() - template_start_pos.y());
	}

	if (subtemplate->get_max_subtemplate_pos().x() != -1) {
		max_pos.setX(max_pos.x() + subtemplate->get_max_subtemplate_pos().x() - this->get_applied_width());
	}
	if (subtemplate->get_max_subtemplate_pos().y() != -1) {
		max_pos.setY(max_pos.y() + subtemplate->get_max_subtemplate_pos().y() - this->get_applied_height());
	}

	//bound the minimum and maximum positions depending on which other templates should be adjacent to this one (if they have already been applied to the map)
	for (const map_template *adjacent_template : subtemplate->AdjacentTemplates) {
		const QPoint adjacent_template_pos = CMap::Map.get_subtemplate_pos(adjacent_template);

		if (!CMap::Map.Info.IsPointOnMap(adjacent_template_pos, z)) {
			fprintf(stderr, "Could not apply adjacency restriction for map template \"%s\", as the other template (\"%s\") has not been applied (yet).\n", subtemplate->Ident.c_str(), adjacent_template->Ident.c_str());
			continue;
		}

		const QPoint min_adjacency_pos = adjacent_template_pos - max_adjacent_template_distance - size::to_point(subtemplate->get_applied_size());
		const QPoint max_adjacency_pos = adjacent_template_pos + size::to_point(subtemplate->get_applied_size()) + max_adjacent_template_distance;
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

	//bound the minimum and maximum positions depending on whether this template should be to the north, south, west or east of other ones
	for (const map_template *other_template : subtemplate->NorthOfTemplates) {
		const QPoint other_template_pos = CMap::Map.get_subtemplate_pos(other_template);
		if (!CMap::Map.Info.IsPointOnMap(other_template_pos, z)) {
			fprintf(stderr, "Could not apply \"north of\" restriction for map template \"%s\", as the other template (\"%s\") has not been applied (yet).\n", subtemplate->Ident.c_str(), other_template->Ident.c_str());
			continue;
		}
		max_pos.setY(std::min<short>(max_pos.y(), other_template_pos.y() - (subtemplate->get_applied_height() / 2)));
	}
	for (const map_template *other_template : subtemplate->SouthOfTemplates) {
		const QPoint other_template_pos = CMap::Map.get_subtemplate_pos(other_template);
		if (!CMap::Map.Info.IsPointOnMap(other_template_pos, z)) {
			fprintf(stderr, "Could not apply \"south of\" restriction for map template \"%s\", as the other template (\"%s\") has not been applied (yet).\n", subtemplate->Ident.c_str(), other_template->Ident.c_str());
			continue;
		}
		min_pos.setY(std::max<short>(min_pos.y(), other_template_pos.y() + other_template->get_applied_height() - (subtemplate->get_applied_height() / 2)));
	}
	for (const map_template *other_template : subtemplate->WestOfTemplates) {
		const QPoint other_template_pos = CMap::Map.get_subtemplate_pos(other_template);
		if (!CMap::Map.Info.IsPointOnMap(other_template_pos, z)) {
			fprintf(stderr, "Could not apply \"west of\" restriction for map template \"%s\", as the other template (\"%s\") has not been applied (yet).\n", subtemplate->Ident.c_str(), other_template->Ident.c_str());
			continue;
		}
		max_pos.setX(std::min<short>(max_pos.x(), other_template_pos.x() - (subtemplate->get_applied_width() / 2)));
	}
	for (const map_template *other_template : subtemplate->EastOfTemplates) {
		const QPoint other_template_pos = CMap::Map.get_subtemplate_pos(other_template);
		if (!CMap::Map.Info.IsPointOnMap(other_template_pos, z)) {
			fprintf(stderr, "Could not apply \"east of\" restriction for map template \"%s\", as the other template (\"%s\") has not been applied (yet).\n", subtemplate->Ident.c_str(), other_template->Ident.c_str());
			continue;
		}
		min_pos.setX(std::max<short>(min_pos.x(), other_template_pos.x() + other_template->get_applied_width() - (subtemplate->get_applied_width() / 2)));
	}

	std::vector<QPoint> potential_positions;
	for (int x = min_pos.x(); x <= max_pos.x(); ++x) {
		for (int y = min_pos.y(); y <= max_pos.y(); ++y) {
			potential_positions.push_back(QPoint(x, y));
		}
	}

	while (!potential_positions.empty()) {
		const QPoint subtemplate_pos = potential_positions[SyncRand(potential_positions.size())];
		vector::remove(potential_positions, subtemplate_pos);

		//include the offsets relevant for the templates dependent on this one's position (e.g. templates that have to be to the north of this one), so that there is enough space for them to be generated there
		const int north_offset = subtemplate->GetDependentTemplatesNorthOffset();
		const int south_offset = subtemplate->GetDependentTemplatesSouthOffset();
		const int west_offset = subtemplate->GetDependentTemplatesWestOffset();
		const int east_offset = subtemplate->GetDependentTemplatesEastOffset();
		const bool top_left_on_map = this->contains_map_pos(subtemplate_pos - QPoint(west_offset, north_offset));
		const bool bottom_right_on_map = this->contains_map_pos(QPoint(subtemplate_pos.x() + subtemplate->get_applied_width() + east_offset - 1, subtemplate_pos.y() + subtemplate->get_applied_height() + south_offset - 1));
		const bool on_map = top_left_on_map && bottom_right_on_map;

		if (!on_map) {
			continue;
		}

		bool on_usable_area = true;
		for (int x = (map_template::min_adjacent_template_distance.x() * -1) - west_offset; x < (subtemplate->get_applied_width() + map_template::min_adjacent_template_distance.x() + east_offset); ++x) {
			for (int y = (map_template::min_adjacent_template_distance.y() * -1) - north_offset; y < (subtemplate->get_applied_height() + map_template::min_adjacent_template_distance.y() + south_offset); ++y) {
				if (CMap::Map.is_point_in_a_subtemplate_area(subtemplate_pos + Vec2i(x, y), z)) {
					on_usable_area = false;
					break;
				}
			}
			if (!on_usable_area) {
				break;
			}
		}

		if (on_usable_area) {
			for (int x = -west_offset; x < (subtemplate->get_applied_width() + east_offset); ++x) {
				for (int y = -north_offset; y < (subtemplate->get_applied_height() + south_offset); ++y) {
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

		if (!on_usable_area) {
			continue;
		}

		return subtemplate_pos;
	}

	return QPoint(-1, -1);
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
			if (historical_location->map_template == this) {
				if (historical_location->Position.x != -1 && historical_location->Position.y != -1) { //historical unit position, could also have been inherited from a site with a fixed position
					pos = map_start_pos + historical_location->Position - template_start_pos;
				} else if (random) {
					if (historical_location->site != nullptr && historical_location->site->get_site_unit() != nullptr) { //sites with random positions will have no valid stored fixed position, but will have had a site unit randomly placed; use that site unit's position instead for this unit then
						pos = historical_location->site->get_site_unit()->get_center_tile_pos();
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

QPoint map_template::get_location_map_position(const std::unique_ptr<historical_location> &historical_location, const QPoint &template_start_pos, const QPoint &map_start_pos, const bool random) const
{
	//get a map position for a historical location
	QPoint pos(-1, -1);
	
	if (historical_location->map_template == this) {
		if (historical_location->Position.x != -1 && historical_location->Position.y != -1) { //historical unit position, could also have been inherited from a site with a fixed position
			return map_start_pos + historical_location->Position - template_start_pos;
		} else if (random) {
			if (historical_location->site != nullptr && historical_location->site->get_site_unit() != nullptr) { //sites with random positions will have no valid stored fixed position, but will have had a site unit randomly placed; use that site unit's position instead for this unit then
				return historical_location->site->get_site_unit()->get_center_tile_pos();
			}
		}
	}

	return QPoint(-1, -1);
}

void generated_terrain::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "seed_count") {
		this->SeedCount = std::stoi(value);
	} else if (key == "expansion_chance") {
		this->ExpansionChance = std::stoi(value);
	} else if (key == "max_percent") {
		this->MaxPercent = std::stoi(value);
	} else if (key == "use_existing_as_seeds") {
		this->UseExistingAsSeeds = string::to_bool(value);
	} else if (key == "use_subtemplate_borders_as_seeds") {
		this->UseSubtemplateBordersAsSeeds = string::to_bool(value);
	} else if (key == "target_terrain_type") {
		const terrain_type *target_terrain_type = terrain_type::get(value);
		this->TargetTerrainTypes.push_back(target_terrain_type);
	} else {
		throw std::runtime_error("Invalid generated terrain property: \"" + key + "\".");
	}
}

void generated_terrain::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid generated terrain scope: \"" + scope.get_tag() + "\".");
}

void generated_terrain::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "terrain_type") {
			this->TerrainType = terrain_type::get(value);
		} else if (key == "seed_count") {
			this->SeedCount = std::stoi(value);
		} else if (key == "expansion_chance") {
			this->ExpansionChance = std::stoi(value);
		} else if (key == "max_percent") {
			this->MaxPercent = std::stoi(value);
		} else if (key == "use_existing_as_seeds") {
			this->UseExistingAsSeeds = string::to_bool(value);
		} else if (key == "use_subtemplate_borders_as_seeds") {
			this->UseSubtemplateBordersAsSeeds = string::to_bool(value);
		} else if (key == "target_terrain_type") {
			const terrain_type *target_terrain_type = terrain_type::get(value);
			this->TargetTerrainTypes.push_back(target_terrain_type);
		} else {
			fprintf(stderr, "Invalid generated terrain property: \"%s\".\n", key.c_str());
		}
	}
	
	if (!this->TerrainType) {
		fprintf(stderr, "Generated terrain has no terrain type.\n");
	}
}

/**
**	@brief	Get whether the terrain generation can use the given tile as a seed
**
**	@param	tile	The tile
**
**	@return	True if the tile can be used as a seed, or false otherwise
*/
bool generated_terrain::CanUseTileAsSeed(const CMapField *tile) const
{
	const terrain_type *top_terrain = tile->GetTopTerrain();
	
	if (top_terrain == this->TerrainType) { //top terrain is the same as the one for the generation, so the tile can be used as a seed
		return true;
	}
	
	if (this->TerrainType == tile->Terrain && std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), top_terrain) == this->TargetTerrainTypes.end()) { //the tile's base terrain is the same as the one for the generation, and its overlay terrain is not a target for the generation
		return true;
	}
	
	return false;
}

/**
**	@brief	Get whether the terrain can be generated on the given tile
**
**	@param	tile	The tile
**
**	@return	True if the terrain can be generated on the tile, or false otherwise
*/
bool generated_terrain::CanGenerateOnTile(const CMapField *tile) const
{
	if (this->TerrainType->is_overlay()) {
		if (std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->GetTopTerrain()) == this->TargetTerrainTypes.end()) { //disallow generating over terrains that aren't a target for the generation
			return false;
		}
	} else {
		if (
			std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->GetTopTerrain()) == this->TargetTerrainTypes.end()
			&& std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->Terrain) == this->TargetTerrainTypes.end()
		) {
			return false;
		}
		
		if ( //don't allow generating the terrain on the tile if it is a base terrain, and putting it there would destroy an overlay terrain that isn't a target of the generation
			tile->OverlayTerrain
			&& !this->CanRemoveTileOverlayTerrain(tile)
			&& !vector::contains(tile->OverlayTerrain->get_base_terrain_types(), this->TerrainType)
		) {
			return false;
		}
		
		if (std::find(this->TerrainType->BorderTerrains.begin(), this->TerrainType->BorderTerrains.end(), tile->Terrain) == this->TerrainType->BorderTerrains.end()) { //don't allow generating on the tile if it can't be a border terrain to the terrain we want to generate
			return false;
		}
	}
	
	return true;
}

/**
**	@brief	Get whether the tile can be a part of an expansion
**
**	@param	tile	The tile
**
**	@return	True if the tile can be part of an expansion, or false otherwise
*/
bool generated_terrain::CanTileBePartOfExpansion(const CMapField *tile) const
{
	if (this->CanGenerateOnTile(tile)) {
		return true;
	}
	
	if (this->TerrainType == tile->GetTopTerrain()) {
		return true;
	}
	
	if (!this->TerrainType->is_overlay()) {
		if (this->TerrainType == tile->Terrain) {
			return true;
		}
	}
	
	return false;
}

/**
**	@brief	Get whether the terrain generation can remove the tile's overlay terrain
**
**	@param	tile	The tile
**
**	@return	True if the terrain generation can remove the tile's overlay terrain, or false otherwise
*/
bool generated_terrain::CanRemoveTileOverlayTerrain(const CMapField *tile) const
{
	if (std::find(this->TargetTerrainTypes.begin(), this->TargetTerrainTypes.end(), tile->OverlayTerrain) == this->TargetTerrainTypes.end()) {
		return false;
	}
	
	return true;
}

}
