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

#pragma once

#include <QAbstractItemModel> 

namespace wyrmgus {

class terrain_type;

class map_grid_model : public QAbstractItemModel
{
	Q_OBJECT

	Q_PROPERTY(int map_layer READ get_map_layer WRITE set_map_layer NOTIFY map_layer_changed)

public:
	enum class role {
		image_source = Qt::UserRole,
		overlay_image_source,
		transition_image_sources,
		overlay_transition_image_sources,
		overlay_transition_elevation_image_sources
	};

	struct tile_data {
		QString image_source;
		QString overlay_image_source;
		QStringList transition_image_sources;
		QStringList overlay_transition_image_sources;
		QStringList overlay_transition_elevation_image_sources;
	};

	static QString build_image_source(const terrain_type *terrain, const short tile_frame, const bool elevation = false);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
	virtual QVariant data(const QModelIndex &index, int role) const override final;

	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override final
	{
		Q_UNUSED(parent)

		return this->createIndex(row, column);
	}

	virtual QModelIndex parent(const QModelIndex &index) const override final
	{
		Q_UNUSED(index)
		
		return QModelIndex();
	}

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names;

		role_names.insert(static_cast<int>(role::image_source), "image_source");
		role_names.insert(static_cast<int>(role::overlay_image_source), "overlay_image_source");
		role_names.insert(static_cast<int>(role::transition_image_sources), "transition_image_sources");
		role_names.insert(static_cast<int>(role::overlay_transition_image_sources), "overlay_transition_image_sources");
		role_names.insert(static_cast<int>(role::overlay_transition_elevation_image_sources), "overlay_transition_elevation_image_sources");

		return role_names;
	}

	size_t get_map_layer() const
	{
		return this->map_layer;
	}

	void set_map_layer(const size_t z);

	void update_tile_image_source(const int tile_index, const terrain_type *terrain, const short tile_frame)
	{
		this->tile_data_list.at(tile_index).image_source = map_grid_model::build_image_source(terrain, tile_frame);
	}

	void update_tile_overlay_image_source(const int tile_index, const terrain_type *terrain, const short tile_frame)
	{
		tile_data &tile_data = this->tile_data_list.at(tile_index);

		if (terrain != nullptr) {
			tile_data.overlay_image_source = map_grid_model::build_image_source(terrain, tile_frame);
		} else {
			tile_data.overlay_image_source.clear();
		}
	}

	void update_tile_transition_image_sources(const int tile_index, const std::vector<std::pair<const terrain_type *, short>> &tile_transitions)
	{
		tile_data &tile_data = this->tile_data_list.at(tile_index);
		
		tile_data.transition_image_sources.clear();
		for (const auto &[terrain_type, tile_frame] : tile_transitions) {
			tile_data.transition_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame));
		}
	}

	void update_tile_overlay_transition_image_sources(const int tile_index, const std::vector<std::pair<const terrain_type *, short>> &tile_transitions)
	{
		tile_data &tile_data = this->tile_data_list.at(tile_index);
		
		tile_data.overlay_transition_image_sources.clear();
		for (const auto &[terrain_type, tile_frame] : tile_transitions) {
			tile_data.overlay_transition_image_sources.push_back(map_grid_model::build_image_source(terrain_type, tile_frame));
		}
	}

signals:
	void map_layer_changed();

private:
	int map_layer = -1;
	std::vector<tile_data> tile_data_list;
};

}
