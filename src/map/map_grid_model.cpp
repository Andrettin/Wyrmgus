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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "map/map_grid_model.h"

#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "player/player_color.h"
#include "time/time_of_day.h"
#include "util/exception_util.h"
#include "util/point_util.h"
#include "video/video.h"

namespace wyrmgus {

QString map_grid_model::build_image_source(const terrain_type *terrain, const short tile_frame, const player_color *player_color, const bool elevation)
{
	QString image_source = terrain->get_identifier_qstring();

	if (elevation) {
		image_source += "/elevation";
	}
	
	if (player_color != nullptr) {
		image_source += "/" + player_color->get_identifier_qstring();
	}
	
	image_source += "/" + QString::number(tile_frame);

	return image_source;
}

int map_grid_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)

	return CMap::get()->MapLayers[this->get_map_layer()]->get_height();
}

int map_grid_model::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)

	return CMap::get()->MapLayers[this->get_map_layer()]->get_width();
}

QVariant map_grid_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const map_grid_model::role model_role = static_cast<map_grid_model::role>(role);
		const QPoint tile_pos(index.column(), index.row());

		if (!CMap::get()->Info->IsPointOnMap(tile_pos, this->get_map_layer())) {
			throw std::runtime_error("Invalid tile position: " + point::to_string(tile_pos) + ", map layer " + std::to_string(this->get_map_layer()) + ".");
		}

		const int tile_index = point::to_index(tile_pos, this->columnCount());

		const tile_data &tile_data = this->tile_data_list.at(tile_index);

		switch (model_role) {
			case role::image_source:
				return tile_data.image_source;
			case role::overlay_image_source:
				return tile_data.overlay_image_source;
			case role::transition_image_sources:
				return tile_data.transition_image_sources;
			case role::overlay_transition_image_sources:
				return tile_data.overlay_transition_image_sources;
			case role::overlay_transition_elevation_image_sources:
				return tile_data.overlay_transition_elevation_image_sources;
			case role::red_change: {
				const time_of_day *time_of_day = this->map_layer->get_tile_time_of_day(tile_index);
				if (time_of_day != nullptr) {
					return time_of_day->ColorModification.R;
				} else {
					return 0;
				}
			}
			case role::green_change: {
				const time_of_day *time_of_day = this->map_layer->get_tile_time_of_day(tile_index);
				if (time_of_day != nullptr) {
					return time_of_day->ColorModification.G;
				} else {
					return 0;
				}
			}
			case role::blue_change: {
				const time_of_day *time_of_day = this->map_layer->get_tile_time_of_day(tile_index);
				if (time_of_day != nullptr) {
					return time_of_day->ColorModification.B;
				} else {
					return 0;
				}
			}
			case role::transition_red_changes: {
				QVariantList red_changes;

				for (const auto &[terrain_type, tile_frame] : this->map_layer->Field(tile_index)->TransitionTiles) {
					const time_of_day *time_of_day = this->map_layer->get_tile_time_of_day(tile_index, terrain_type->Flags);

					short change = 0;

					if (time_of_day != nullptr) {
						change = time_of_day->ColorModification.R;
					}

					red_changes.push_back(change);
				}

				return red_changes;
			}
			case role::transition_green_changes: {
				QVariantList green_changes;

				for (const auto &[terrain_type, tile_frame] : this->map_layer->Field(tile_index)->TransitionTiles) {
					const time_of_day *time_of_day = this->map_layer->get_tile_time_of_day(tile_index, terrain_type->Flags);

					short change = 0;

					if (time_of_day != nullptr) {
						change = time_of_day->ColorModification.G;
					}

					green_changes.push_back(change);
				}

				return green_changes;
			}
			case role::transition_blue_changes: {
				QVariantList blue_changes;

				for (const auto &[terrain_type, tile_frame] : this->map_layer->Field(tile_index)->TransitionTiles) {
					const time_of_day *time_of_day = this->map_layer->get_tile_time_of_day(tile_index, terrain_type->Flags);

					short change = 0;

					if (time_of_day != nullptr) {
						change = time_of_day->ColorModification.B;
					}

					blue_changes.push_back(change);
				}

				return blue_changes;
			}
			default:
				throw std::runtime_error("Invalid map grid model role: " + std::to_string(role) + ".");
		}
	} catch (...) {
		exception::report(std::current_exception());
	}

	return QVariant();
}

int map_grid_model::get_map_layer() const
{
	if (this->map_layer == nullptr) {
		return -1;
	}

	return this->map_layer->ID;
}

void map_grid_model::set_map_layer(const int z)
{
	if (z == this->get_map_layer()) {
		return;
	}

	this->tile_data_list.clear();

	if (z != -1) {
		this->map_layer = CMap::get()->MapLayers[z].get();
	} else {
		this->map_layer = nullptr;
	}

	if (this->map_layer != nullptr) {
		for (int y = 0; y < this->map_layer->get_height(); ++y) {
			for (int x = 0; x < this->map_layer->get_width(); ++x) {
				const tile *tile = this->map_layer->Field(x, y);
				const player_color *player_color = tile->get_player_color();

				tile_data tile_data;

				tile_data.image_source = map_grid_model::build_image_source(tile->get_terrain(), tile->SolidTile, player_color);
				if (tile->get_overlay_terrain() != nullptr) {
					tile_data.overlay_image_source = map_grid_model::build_image_source(tile->get_overlay_terrain(), tile->OverlaySolidTile, player_color);
				}

				for (const auto &[terrain_type, tile_frame] : tile->TransitionTiles) {
					tile_data.transition_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame, player_color));
				}

				for (const auto &[terrain_type, tile_frame] : tile->OverlayTransitionTiles) {
					tile_data.overlay_transition_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame, player_color));

					if (terrain_type->get_elevation_graphics() != nullptr) {
						tile_data.overlay_transition_elevation_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame, player_color, true));
					}
				}

				this->tile_data_list.push_back(std::move(tile_data));
			}
		}

		connect(this->map_layer, &CMapLayer::tile_image_changed, this, &map_grid_model::update_tile_image_source);
		connect(this->map_layer, &CMapLayer::tile_overlay_image_changed, this, &map_grid_model::update_tile_overlay_image_source);
		connect(this->map_layer, &CMapLayer::tile_transition_images_changed, this, &map_grid_model::update_tile_transition_image_sources);
		connect(this->map_layer, &CMapLayer::tile_overlay_transition_images_changed, this, &map_grid_model::update_tile_overlay_transition_image_sources);
		connect(this->map_layer, &CMapLayer::tile_rect_color_change_changed, this, &map_grid_model::update_tile_rect_color_change);
	}

	emit map_layer_changed();
}

void map_grid_model::update_tile_image_source(const QPoint &tile_pos, const terrain_type *terrain, const short tile_frame, const player_color *player_color)
{
	const int tile_index = point::to_index(tile_pos, this->map_layer->get_width());
	this->tile_data_list.at(tile_index).image_source = map_grid_model::build_image_source(terrain, tile_frame, player_color);

	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, { static_cast<int>(role::image_source) });
}

void map_grid_model::update_tile_overlay_image_source(const QPoint &tile_pos, const terrain_type *terrain, const short tile_frame, const player_color *player_color)
{
	const int tile_index = point::to_index(tile_pos, this->map_layer->get_width());
	tile_data &tile_data = this->tile_data_list.at(tile_index);

	if (terrain != nullptr) {
		tile_data.overlay_image_source = map_grid_model::build_image_source(terrain, tile_frame, player_color);
	} else {
		tile_data.overlay_image_source.clear();
	}

	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, { static_cast<int>(role::overlay_image_source) });
}

void map_grid_model::update_tile_transition_image_sources(const QPoint &tile_pos, const std::vector<tile_transition> &tile_transitions, const player_color *player_color)
{
	const int tile_index = point::to_index(tile_pos, this->map_layer->get_width());
	tile_data &tile_data = this->tile_data_list.at(tile_index);

	tile_data.transition_image_sources.clear();
	for (const auto &[terrain_type, tile_frame] : tile_transitions) {
		tile_data.transition_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame, player_color));
	}

	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, { static_cast<int>(role::transition_image_sources) });
}

void map_grid_model::update_tile_overlay_transition_image_sources(const QPoint &tile_pos, const std::vector<tile_transition> &tile_transitions, const player_color *player_color)
{
	const int tile_index = point::to_index(tile_pos, this->map_layer->get_width());
	tile_data &tile_data = this->tile_data_list.at(tile_index);

	tile_data.overlay_transition_image_sources.clear();
	tile_data.overlay_transition_elevation_image_sources.clear();

	for (const auto &[terrain_type, tile_frame] : tile_transitions) {
		tile_data.overlay_transition_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame, player_color));

		if (terrain_type->get_elevation_graphics() != nullptr) {
			tile_data.overlay_transition_elevation_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame, player_color, true));
		}
	}

	const QModelIndex index = this->index(tile_pos.y(), tile_pos.x());
	emit dataChanged(index, index, { static_cast<int>(role::overlay_transition_image_sources), static_cast<int>(role::overlay_transition_elevation_image_sources) });
}

void map_grid_model::update_tile_rect_color_change(const QRect &tile_rect)
{
	const QPoint top_left = tile_rect.topLeft();
	const QPoint bottom_right = tile_rect.bottomRight();

	const QModelIndex min_index = this->index(top_left.y(), top_left.x());
	const QModelIndex max_index = this->index(bottom_right.y(), bottom_right.x());

	emit dataChanged(min_index, max_index, {
		static_cast<int>(role::red_change),
		static_cast<int>(role::green_change),
		static_cast<int>(role::blue_change),
		static_cast<int>(role::transition_red_changes),
		static_cast<int>(role::transition_green_changes),
		static_cast<int>(role::transition_blue_changes)
	});
}

}
