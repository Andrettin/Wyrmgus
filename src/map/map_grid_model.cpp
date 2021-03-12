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
//      (c) Copyright 2021 by Andrettin
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

#include "engine_interface.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "util/exception_util.h"
#include "util/point_util.h"
#include "video/video.h"

namespace wyrmgus {

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

		if (!CMap::get()->Info.IsPointOnMap(tile_pos, this->get_map_layer())) {
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
			default:
				throw std::runtime_error("Invalid map grid model role: " + std::to_string(role) + ".");
		}
	} catch (const std::exception &exception) {
		exception::report(exception);
	}

	return QVariant();
}

void map_grid_model::set_map_layer(const size_t z)
{
	if (z == this->get_map_layer()) {
		return;
	}

	this->map_layer = z;

	this->tile_data_list.clear();

	//the game loop thread will be waiting while the map view is created, so it is safe to access map data here
	const CMapLayer *map_layer = CMap::get()->MapLayers[this->get_map_layer()].get();
	for (int y = 0; y < map_layer->get_height(); ++y) {
		for (int x = 0; x < map_layer->get_width(); ++x) {
			const tile *tile = map_layer->Field(x, y);

			tile_data tile_data;

			tile_data.image_source = QString::fromStdString(tile->get_terrain()->get_identifier()) + "/" + QString::number(tile->SolidTile);
			if (tile->get_overlay_terrain() != nullptr) {
				tile_data.overlay_image_source = QString::fromStdString(tile->get_overlay_terrain()->get_identifier()) + "/" + QString::number(tile->OverlaySolidTile);
			}

			for (const auto &[terrain_type, tile_frame] : tile->TransitionTiles) {
				tile_data.transition_image_sources.push_back(QString::fromStdString(terrain_type->get_identifier()) + "/" + QString::number(tile_frame));
			}

			for (const auto &[terrain_type, tile_frame] : tile->OverlayTransitionTiles) {
				tile_data.overlay_transition_image_sources.push_back(QString::fromStdString(terrain_type->get_identifier()) + "/" + QString::number(tile_frame));

				if (terrain_type->get_elevation_graphics() != nullptr) {
					tile_data.overlay_transition_elevation_image_sources.push_back(QString::fromStdString(terrain_type->get_identifier()) + "/elevation/" + QString::number(tile_frame));
				}
			}

			this->tile_data_list.push_back(std::move(tile_data));
		}
	}

	emit map_layer_changed();
}

}
