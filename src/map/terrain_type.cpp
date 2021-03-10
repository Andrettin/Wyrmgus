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

#include "map/terrain_type.h"

#include "config.h"
#include "database/defines.h"
#include "economy/resource.h"
#include "iolib.h"
#include "map/terrain_feature.h"
#include "map/tile_flag.h"
#include "map/tileset.h"
#include "player_color.h"
#include "time/season.h"
#include "upgrade/upgrade_structs.h"
#include "util/container_util.h"
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

terrain_type::terrain_type(const std::string &identifier)
	: named_data_entry(identifier), Flags(tile_flag::none)
{
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
			const tile_flag flag = string_to_tile_flag(value);
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

void terrain_type::initialize()
{
	if (!this->get_image_file().empty() && this->graphics == nullptr) {
		this->graphics = CPlayerColorGraphic::New(this->get_image_file().string(), defines::get()->get_tile_size(), nullptr);
		this->graphics->set_store_scaled_image(true);
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
		throw std::runtime_error("The movement bonus for terrain type \"" + this->get_identifier() + "\" is greater than or equal to the default tile movement cost.");
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
		throw std::runtime_error("Character \"" + std::string(1, character) + "\" is already used by another terrain type.");
	}

	terrain_type::terrain_types_by_character[character] = this;
}

void terrain_type::set_color(const QColor &color)
{
	if (color == this->get_color()) {
		return;
	}

	if (terrain_type::try_get_by_color(color) != nullptr) {
		throw std::runtime_error("Color is already used by another terrain type.");
	} else if (terrain_feature::try_get_by_color(color) != nullptr) {
		throw std::runtime_error("Color is already used by a terrain feature.");
	}

	this->color = color;
	terrain_type::terrain_types_by_color[color] = this;
}

const QColor &terrain_type::get_minimap_color(const season *season) const
{
	if (season != nullptr) {
		const auto find_iterator = this->season_minimap_colors.find(season);

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
		throw std::runtime_error("No valid pixels for calculating the minimap color for terrain type \"" + this->get_identifier() + "\".");
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
		throw std::runtime_error("Tile number \"" + std::to_string(tile_number) + "\" is already used by another terrain type.");
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
		const auto find_iterator = this->season_graphics.find(season);

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

bool terrain_type::has_flag(const tile_flag flag) const
{
	return (this->Flags & flag) != tile_flag::none;
}

bool terrain_type::is_water() const
{
	return this->has_flag(tile_flag::water_allowed);
}

bool terrain_type::is_wall() const
{
	return this->has_flag(tile_flag::wall);
}

bool terrain_type::is_constructed() const
{
	return this->is_overlay() && (this->is_wall() || this->has_flag(tile_flag::road) || this->has_flag(tile_flag::railroad));
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
