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

class terrain_type;
struct tile_transition;

class unit_list_model : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(int map_layer READ get_map_layer WRITE set_map_layer NOTIFY map_layer_changed)

public:
	enum class role {
		unit = Qt::UserRole
	};

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

		role_names.insert(static_cast<int>(role::unit), "unit");

		return role_names;
	}

	int get_map_layer() const;
	void set_map_layer(const int z);

	CUnit *get_unit(const int index) const
	{
		const auto find_iterator = this->units.find(index);
		if (find_iterator != this->units.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	void add_unit(CUnit *unit);
	void remove_unit(CUnit *unit);

signals:
	void map_layer_changed();

private:
	const CMapLayer *map_layer = nullptr;
	std::map<int, CUnit *> units; //units mapped to their slot
};

}
