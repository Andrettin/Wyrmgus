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
//

#include "stratagus.h"

#include "map/terrain_type.h"

#include "config.h"
#include "database/defines.h"
#include "iolib.h"
#include "map/terrain_feature.h"
#include "map/tileset.h"
#include "player_color.h"
#include "time/season.h"
#include "upgrade/upgrade_structs.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/string_conversion_util.h"
#include "util/util.h"
#include "util/vector_util.h"
#include "video/video.h"

namespace wyrmgus {

void terrain_type::LoadTerrainTypeGraphics()
{
	for (terrain_type *terrain_type : terrain_type::get_all()) {
		if (terrain_type->graphics != nullptr) {
			terrain_type->graphics->Load(false, defines::get()->get_scale_factor());

			if (!terrain_type->minimap_color.isValid()) {
				terrain_type->calculate_minimap_color();
			}
		}

		if (terrain_type->transition_graphics != nullptr) {
			terrain_type->transition_graphics->Load(false, defines::get()->get_scale_factor());
		}

		for (const auto &kv_pair : terrain_type->season_graphics) {
			const season *season = kv_pair.first;
			kv_pair.second->Load(false, defines::get()->get_scale_factor());

			if (!terrain_type->season_minimap_colors[season].isValid()) {
				terrain_type->calculate_minimap_color(season);
			}
		}

		if (terrain_type->elevation_graphics != nullptr) {
			terrain_type->elevation_graphics->Load(false, defines::get()->get_scale_factor());
		}
	}
}

/**
**	@brief	Get a terrain flag by name
**
**	@param	flag_name	The name of the terrain flag
**
**	@return	The terrain flag if it exists, or 0 otherwise
*/
unsigned long terrain_type::GetTerrainFlagByName(const std::string &flag_name)
{
	if (flag_name == "land") {
		return MapFieldLandAllowed;
	} else if (flag_name == "coast") {
		return MapFieldCoastAllowed;
	} else if (flag_name == "water") {
		return MapFieldWaterAllowed;
	} else if (flag_name == "no-building" || flag_name == "no_building") {
		return MapFieldNoBuilding;
	} else if (flag_name == "unpassable") {
		return MapFieldUnpassable;
	} else if (flag_name == "wall") {
		return MapFieldWall;
	} else if (flag_name == "rock") {
		return MapFieldRocks;
	} else if (flag_name == "forest") {
		return MapFieldForest;
	} else if (flag_name == "air-unpassable" || flag_name == "air_unpassable") {
		return MapFieldAirUnpassable;
	} else if (flag_name == "desert") {
		return MapFieldDesert;
	} else if (flag_name == "dirt") {
		return MapFieldDirt;
	} else if (flag_name == "grass") {
		return MapFieldGrass;
	} else if (flag_name == "gravel") {
		return MapFieldGravel;
	} else if (flag_name == "ice") {
		return MapFieldIce;
	} else if (flag_name == "mud") {
		return MapFieldMud;
	} else if (flag_name == "railroad") {
		return MapFieldRailroad;
	} else if (flag_name == "road") {
		return MapFieldRoad;
	} else if (flag_name == "no-rail") {
		return MapFieldNoRail;
	} else if (flag_name == "snow") {
		return MapFieldSnow;
	} else if (flag_name == "stone_floor") {
		return MapFieldStoneFloor;
	} else if (flag_name == "stumps") {
		return MapFieldStumps;
	} else if (flag_name == "underground") {
		return MapFieldUnderground;
	} else if (flag_name == "space") {
		return MapFieldSpace;
	} else if (flag_name == "cliff") {
		return MapFieldCliff;
	} else {
		exception::throw_with_trace(std::runtime_error("Flag \"" + flag_name + "\" doesn't exist."));
	}
}

terrain_type::~terrain_type()
{
}

void terrain_type::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "character") {
		this->set_character(value.front());
	} else {
		data_entry::process_sml_property(property);
	}
}

void terrain_type::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "character_aliases") {
		for (const std::string &value : values) {
			this->map_to_character(value.front());
		}
	} else if (tag == "tile_numbers") {
		for (const std::string &value : values) {
			this->map_to_tile_number(std::stoi(value));
		}
	} else if (tag == "flags") {
		for (const std::string &value : values) {
			const unsigned long flag = terrain_type::GetTerrainFlagByName(value);
			this->Flags |= flag;
		}
	} else if (tag == "solid_tiles") {
		for (const std::string &value : values) {
			this->solid_tiles.push_back(std::stoi(value));
		}

		scope.for_each_property([&](const sml_property &property) {
			const int tile = std::stoi(property.get_key());
			const int weight = std::stoi(property.get_value());

			for (int i = 0; i < weight; ++i) {
				this->solid_tiles.push_back(tile);
			}
		});
	} else if (tag == "damaged_tiles") {
		for (const std::string &value : values) {
			this->damaged_tiles.push_back(std::stoi(value));
		}
	} else if (tag == "destroyed_tiles") {
		for (const std::string &value : values) {
			this->destroyed_tiles.push_back(std::stoi(value));
		}
	} else if (tag == "season_image_files") {
		scope.for_each_property([&](const sml_property &property) {
			const season *season = season::get(property.get_key());
			const std::filesystem::path filepath = property.get_value();

			this->season_image_files[season] = filepath;
		});
	} else if (tag == "season_minimap_colors") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const season *season = season::get(child_scope.get_tag());
			const QColor color = child_scope.to_color();

			this->season_minimap_colors[season] = color;
		});
	} else if (tag == "transition_tiles" || tag == "adjacent_transition_tiles") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const terrain_type *transition_terrain = nullptr;

			if (child_tag != "any") {
				transition_terrain = terrain_type::get(child_tag);
			}

			child_scope.for_each_child([&](const sml_data &grandchild_scope) {
				const std::string &grandchild_tag = grandchild_scope.get_tag();
				const tile_transition_type transition_type = GetTransitionTypeIdByName(FindAndReplaceString(grandchild_tag, "_", "-"));
				std::vector<int> tiles;

				for (const std::string &value : grandchild_scope.get_values()) {
					const int tile = std::stoi(value);

					if (tag == "transition_tiles") {
						this->transition_tiles[transition_terrain][transition_type].push_back(tile);
					} else if (tag == "adjacent_transition_tiles") {
						this->adjacent_transition_tiles[transition_terrain][transition_type].push_back(tile);
					}
				}
			});
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void terrain_type::ProcessConfigData(const CConfigData *config_data)
{
	std::string graphics_file;
	std::string elevation_graphics_file;
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "character") {
			this->set_character(value.front());
		} else if (key == "character_alias") {
			const char c = value.front();
			this->map_to_character(c);
		} else if (key == "color") {
			const CColor color = CColor::FromString(value);
			this->set_color(QColor(color.R, color.G, color.B));
		} else if (key == "overlay") {
			this->overlay = string::to_bool(value);
		} else if (key == "buildable") {
			this->buildable = string::to_bool(value);
		} else if (key == "allow_single") {
			this->allow_single = string::to_bool(value);
		} else if (key == "hidden") {
			this->hidden = string::to_bool(value);
		} else if (key == "resource") {
			this->resource = resource::get(value);
		} else if (key == "flag") {
			value = FindAndReplaceString(value, "_", "-");
			const unsigned long flag = terrain_type::GetTerrainFlagByName(value);
			this->Flags |= flag;
		} else if (key == "graphics") {
			graphics_file = value;
			if (!CanAccessFile(graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "elevation_graphics") {
			elevation_graphics_file = value;
			if (!CanAccessFile(elevation_graphics_file.c_str())) {
				fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "base_terrain_type") {
			terrain_type *base_terrain_type = terrain_type::get(value);
			this->add_base_terrain_type(base_terrain_type);
		} else if (key == "inner_border_terrain_type") {
			terrain_type *border_terrain_type = terrain_type::get(value);
			this->add_inner_border_terrain_type(border_terrain_type);
		} else if (key == "outer_border_terrain_type") {
			terrain_type *border_terrain_type = terrain_type::get(value);
			this->add_outer_border_terrain_type(border_terrain_type);
		} else if (key == "solid_tile") {
			this->solid_tiles.push_back(std::stoi(value));
		} else if (key == "damaged_tile") {
			this->damaged_tiles.push_back(std::stoi(value));
		} else if (key == "destroyed_tile") {
			this->destroyed_tiles.push_back(std::stoi(value));
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "season_graphics") {
			std::string season_graphics_file;
			season *season = nullptr;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "season") {
					season = season::get(value);
				} else if (key == "graphics") {
					season_graphics_file = value;
					if (!CanAccessFile(season_graphics_file.c_str())) {
						fprintf(stderr, "File \"%s\" doesn't exist.\n", value.c_str());
					}
				} else {
					fprintf(stderr, "Invalid season graphics property: \"%s\".\n", key.c_str());
				}
			}
			
			if (season_graphics_file.empty()) {
				fprintf(stderr, "Season graphics have no file.\n");
				continue;
			}
			
			if (!season) {
				fprintf(stderr, "Season graphics have no season.\n");
				continue;
			}
			
			this->season_graphics[season] = CPlayerColorGraphic::New(season_graphics_file, defines::get()->get_tile_size(), nullptr);
		} else if (child_config_data->Tag == "transition_tile" || child_config_data->Tag == "adjacent_transition_tile") {
			const terrain_type *transition_terrain = nullptr; //any terrain, by default
			tile_transition_type transition_type = tile_transition_type::none;
			std::vector<int> tiles;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "terrain_type") {
					transition_terrain = terrain_type::get(value);
				} else if (key == "transition_type") {
					value = FindAndReplaceString(value, "_", "-");
					transition_type = GetTransitionTypeIdByName(value);
				} else if (key == "tile") {
					tiles.push_back(std::stoi(value));
				} else {
					fprintf(stderr, "Invalid transition tile property: \"%s\".\n", key.c_str());
				}
			}
			
			if (transition_type == tile_transition_type::none) {
				fprintf(stderr, "Transition tile has no transition type.\n");
				continue;
			}

			for (size_t j = 0; j < tiles.size(); ++j) {
				if (child_config_data->Tag == "transition_tile") {
					this->transition_tiles[transition_terrain][transition_type].push_back(tiles[j]);
				} else if (child_config_data->Tag == "adjacent_transition_tile") {
					this->adjacent_transition_tiles[transition_terrain][transition_type].push_back(tiles[j]);
				}
			}
		} else {
			fprintf(stderr, "Invalid terrain type property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	if (!graphics_file.empty()) {
		this->graphics = CPlayerColorGraphic::New(graphics_file, defines::get()->get_tile_size(), nullptr);
	}
	if (!elevation_graphics_file.empty()) {
		this->elevation_graphics = CGraphic::New(elevation_graphics_file, defines::get()->get_tile_size());
	}
}

void terrain_type::initialize()
{
	if (!this->get_image_file().empty() && this->graphics == nullptr) {
		this->graphics = CPlayerColorGraphic::New(this->get_image_file().string(), defines::get()->get_tile_size(), nullptr);
	}

	if (!this->get_transition_image_file().empty() && this->transition_graphics == nullptr) {
		this->transition_graphics = CPlayerColorGraphic::New(this->get_transition_image_file().string(), defines::get()->get_tile_size(), nullptr);
	}

	if (!this->get_elevation_image_file().empty() && this->elevation_graphics == nullptr) {
		this->elevation_graphics = CGraphic::New(this->get_elevation_image_file().string(), defines::get()->get_tile_size());
	}

	for (const auto &kv_pair : this->season_image_files) {
		const season *season = kv_pair.first;
		const std::filesystem::path &filepath = kv_pair.second;

		if (!this->season_graphics.contains(season)) {
			this->season_graphics[season] = CPlayerColorGraphic::New(filepath.string(), defines::get()->get_tile_size(), nullptr);
		}
	}

	data_entry::initialize();
}

void terrain_type::check() const
{
	if (this->movement_bonus >= DefaultTileMovementCost) {
		exception::throw_with_trace(std::runtime_error("The movement bonus for terrain type \"" + this->get_identifier() + "\" is greater than or equal to the default tile movement cost."));
	}
}

void terrain_type::set_character(const char character)
{
	if (character == this->get_character()) {
		return;
	}

	this->character = character;
	this->map_to_character(character);
}

void terrain_type::map_to_character(const char character)
{
	if (terrain_type::try_get_by_character(character) != nullptr) {
		exception::throw_with_trace(std::runtime_error("Character \"" + std::string(1, character) + "\" is already used by another terrain type."));
	}

	terrain_type::terrain_types_by_character[character] = this;
}

void terrain_type::set_color(const QColor &color)
{
	if (color == this->get_color()) {
		return;
	}

	if (terrain_type::try_get_by_color(color) != nullptr) {
		exception::throw_with_trace(std::runtime_error("Color is already used by another terrain type."));
	} else if (terrain_feature::try_get_by_color(color) != nullptr) {
		exception::throw_with_trace(std::runtime_error("Color is already used by a terrain feature."));
	}

	this->color = color;
	terrain_type::terrain_types_by_color[color] = this;
}

const QColor &terrain_type::get_minimap_color(const season *season) const
{
	if (season != nullptr) {
		auto find_iterator = this->season_minimap_colors.find(season);

		if (find_iterator != this->season_minimap_colors.end()) {
			return find_iterator->second;
		}
	}

	return this->minimap_color;
}

void terrain_type::calculate_minimap_color(const season *season)
{
	if (this == defines::get()->get_border_terrain_type()) {
		return;
	}

	const std::shared_ptr<CPlayerColorGraphic> &graphic = this->get_graphics(season);

	const QImage &image = graphic->get_image();
	const player_color *conversible_player_color = graphic->get_conversible_player_color();

	int pixel_count = 0;
	int red = 0;
	int green = 0;
	int blue = 0;

	for (int x = 0; x < image.width(); ++x) {
		for (int y = 0; y < image.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QColor pixel_color = image.pixelColor(pixel_pos);

			if (pixel_color.alpha() != 255) { //transparent pixel, ignore
				continue;
			}

			if (vector::contains(conversible_player_color->get_colors(), pixel_color)) {
				continue;
			}

			red += pixel_color.red();
			green += pixel_color.green();
			blue += pixel_color.blue();
			pixel_count++;
		}
	}

	if (pixel_count == 0) {
		exception::throw_with_trace(std::runtime_error("No valid pixels for calculating the minimap color for terrain type \"" + this->get_identifier() + "\"."));
	}

	red /= pixel_count;
	green /= pixel_count;
	blue /= pixel_count;

	const QColor minimap_color(red, green, blue);

	if (season != nullptr) {
		this->season_minimap_colors[season] = minimap_color;
	} else {
		this->minimap_color = minimap_color;
	}
}

void terrain_type::map_to_tile_number(const int tile_number)
{
	if (terrain_type::try_get_by_tile_number(tile_number) != nullptr) {
		exception::throw_with_trace(std::runtime_error("Tile number \"" + std::to_string(tile_number) + "\" is already used by another terrain type."));
	}

	terrain_type::terrain_types_by_tile_number[tile_number] = this;
}

void terrain_type::set_image_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_file()) {
		return;
	}

	this->image_file = database::get()->get_graphics_path(this->get_module()) / filepath;
}

const std::shared_ptr<CPlayerColorGraphic> &terrain_type::get_graphics(const season *season) const
{
	if (season != nullptr) {
		auto find_iterator = this->season_graphics.find(season);

		if (find_iterator != this->season_graphics.end()) {
			return find_iterator->second;
		}
	}

	return this->graphics;
}

void terrain_type::set_transition_image_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_transition_image_file()) {
		return;
	}

	this->transition_image_file = database::get()->get_graphics_path(this->get_module()) / filepath;
}

void terrain_type::set_elevation_image_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_elevation_image_file()) {
		return;
	}

	this->elevation_image_file = database::get()->get_graphics_path(this->get_module()) / filepath;
}

bool terrain_type::is_water() const
{
	return this->Flags & MapFieldWaterAllowed;
}

bool terrain_type::is_wall() const
{
	return this->Flags & MapFieldWall;
}

bool terrain_type::is_constructed() const
{
	return this->is_overlay() && (this->is_wall() || (this->Flags & MapFieldRoad) || (this->Flags & MapFieldRailroad));
}

QVariantList terrain_type::get_base_terrain_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_base_terrain_types());
}

void terrain_type::remove_base_terrain_type(terrain_type *terrain_type)
{
	vector::remove(this->base_terrain_types, terrain_type);
}

QVariantList terrain_type::get_outer_border_terrain_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_outer_border_terrain_types());
}

void terrain_type::remove_outer_border_terrain_type(terrain_type *terrain_type)
{
	vector::remove(this->outer_border_terrain_types, terrain_type);
}

QVariantList terrain_type::get_inner_border_terrain_types_qvariant_list() const
{
	return container::to_qvariant_list(this->get_inner_border_terrain_types());
}

void terrain_type::remove_inner_border_terrain_type(terrain_type *terrain_type)
{
	vector::remove(this->inner_border_terrain_types, terrain_type);
}

}
