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

#include <QAbstractListModel> 

class CMapLayer;
class CUnit;

namespace wyrmgus {

class player_color;
class unit_type;
class unit_type_variation;

class unit_list_model : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(int map_layer READ get_map_layer WRITE set_map_layer NOTIFY map_layer_changed)

public:
	enum class role {
		image_source = Qt::UserRole,
		mirrored_image,
		tile_pos,
		tile_size
	};

	struct unit_data {
		QString image_source;
		bool mirrored_image = false;
		QPoint tile_pos = QPoint(0, 0);
		QSize tile_size = QSize(0, 0);
	};

	static QString build_image_source(const unit_type *unit_type, const unit_type_variation *variation, const int frame, const player_color *player_color);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;

	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final
	{
		Q_UNUSED(parent)

		return 1;
	}

	virtual QVariant data(const QModelIndex &index, int role) const override final;

	virtual QModelIndex parent(const QModelIndex &index) const override final
	{
		Q_UNUSED(index)
		
		return QModelIndex();
	}

	virtual QHash<int, QByteArray> roleNames() const override final
	{
		QHash<int, QByteArray> role_names;

		role_names.insert(static_cast<int>(role::image_source), "image_source");
		role_names.insert(static_cast<int>(role::mirrored_image), "mirrored_image");
		role_names.insert(static_cast<int>(role::tile_pos), "tile_pos");
		role_names.insert(static_cast<int>(role::tile_size), "tile_size");

		return role_names;
	}

	int get_map_layer() const;
	void set_map_layer(const int z);

	unit_data *get_unit_data(const int index)
	{
		const auto find_iterator = this->unit_data_map.find(index);
		if (find_iterator != this->unit_data_map.end()) {
			return &find_iterator->second;
		}

		return nullptr;
	}

	const unit_data *get_unit_data(const int index) const
	{
		const auto find_iterator = this->unit_data_map.find(index);
		if (find_iterator != this->unit_data_map.end()) {
			return &find_iterator->second;
		}

		return nullptr;
	}

	void add_unit_data(const int unit_index, const unit_type *unit_type, const unit_type_variation *variation, const int frame, const player_color *player_color, const QPoint &tile_pos);

	void remove_unit_data(const int unit_index)
	{
		this->unit_data_map.erase(unit_index);

		const QModelIndex index = this->index(unit_index);
		emit dataChanged(index, index);
	}

	void update_unit_image(const int unit_index, const unit_type *unit_type, const unit_type_variation *variation, const int frame, const player_color *player_color)
	{
		unit_data *unit_data = this->get_unit_data(unit_index);
		unit_data->image_source = unit_list_model::build_image_source(unit_type, variation, frame, player_color);
		unit_data->mirrored_image = frame < 0;

		const QModelIndex index = this->index(unit_index);
		emit dataChanged(index, index, { static_cast<int>(role::image_source), static_cast<int>(role::mirrored_image) });
	}

	void update_unit_tile_pos(const int unit_index, const QPoint &tile_pos)
	{
		unit_data *unit_data = this->get_unit_data(unit_index);
		unit_data->tile_pos = tile_pos;

		const QModelIndex index = this->index(unit_index);
		emit dataChanged(index, index, { static_cast<int>(role::tile_pos) });
	}

	void update_unit_tile_size(const int unit_index, const QSize &tile_size)
	{
		unit_data *unit_data = this->get_unit_data(unit_index);
		unit_data->tile_size = tile_size;

		const QModelIndex index = this->index(unit_index);
		emit dataChanged(index, index, { static_cast<int>(role::tile_size) });
	}

signals:
	void map_layer_changed();

private:
	const CMapLayer *map_layer = nullptr;
	std::map<int, unit_data> unit_data_map; //unit data mapped to the corresponding unit slot
};

}
