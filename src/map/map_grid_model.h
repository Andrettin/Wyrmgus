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

class map_grid_model : public QAbstractItemModel
{
	Q_OBJECT

	Q_PROPERTY(size_t map_layer READ get_map_layer WRITE set_map_layer NOTIFY map_layer_changed)

public:
	map_grid_model();

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override final;

	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override final;

	virtual QVariant data(const QModelIndex &index, int role) const override final
	{
		Q_UNUSED(index)
		Q_UNUSED(role)

		return QVariant();
	}

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

	size_t get_map_layer() const
	{
		return this->map_layer;
	}

	void set_map_layer(const size_t z)
	{
		if (z == this->get_map_layer()) {
			return;
		}

		this->map_layer = z;
		emit map_layer_changed();
	}

signals:
	void map_layer_changed();

private:
	size_t map_layer = 0;
};

}