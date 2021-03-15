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

#include "unit/unit_list_model.h"

#include "map/map.h"
#include "map/map_layer.h"
#include "player_color.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "util/exception_util.h"
#include "util/log_util.h"

namespace wyrmgus {

QString unit_list_model::build_image_source(const unit_type *unit_type, const unit_type_variation *variation, const int frame, const player_color *player_color)
{
	QString image_source = unit_type->get_identifier_qstring() + "/";

	if (variation != nullptr && variation->Sprite != nullptr) {
		image_source += QString::fromStdString(variation->get_identifier()) + "/";
	}

	image_source += player_color->get_identifier_qstring() + "/";

	image_source += QString::number(std::abs(frame));

	return image_source;
}

int unit_list_model::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)

	return unit_manager::get()->GetUsedSlotCount();
}

QVariant unit_list_model::data(const QModelIndex &index, const int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	try {
		const unit_list_model::role model_role = static_cast<unit_list_model::role>(role);
		const int unit_slot = index.row();
		const unit_data *unit_data = this->get_unit_data(unit_slot);

		switch (model_role) {
			case role::image_source:
				if (unit_data != nullptr) {
					return unit_data->image_source;
				} else {
					return QString();
				}
			case role::mirrored_image:
				if (unit_data != nullptr) {
					return unit_data->mirrored_image;
				} else {
					return false;
				}
			case role::tile_pos:
				if (unit_data != nullptr) {
					return unit_data->tile_pos;
				} else {
					return QPoint(0, 0);
				}
			case role::tile_size:
				if (unit_data != nullptr) {
					return unit_data->tile_size;
				} else {
					return QSize(0, 0);
				}
			default:
				throw std::runtime_error("Invalid unit list model role: " + std::to_string(role) + ".");
		}
	} catch (const std::exception &exception) {
		exception::report(exception);
	}

	return QVariant();
}

int unit_list_model::get_map_layer() const
{
	if (this->map_layer == nullptr) {
		return -1;
	}

	return this->map_layer->ID;
}

void unit_list_model::set_map_layer(const int z)
{
	if (z == this->get_map_layer()) {
		return;
	}

	this->unit_data_map.clear();

	if (z != -1) {
		this->map_layer = CMap::get()->MapLayers[z].get();
	} else {
		this->map_layer = nullptr;
	}

	if (this->map_layer != nullptr) {
		for (CUnit *unit : unit_manager::get()->get_units()) {
			if (unit->Destroyed) {
				continue;
			}

			if (unit->Removed) {
				continue;
			}

			if (unit->MapLayer != this->map_layer) {
				continue;
			}

			unit_data unit_data;

			unit_data.image_source = unit_list_model::build_image_source(unit->Type, unit->GetVariation(), unit->Frame, unit->get_player_color());
			unit_data.mirrored_image = unit->Frame < 0;
			unit_data.tile_pos = unit->tilePos;
			unit_data.tile_size = unit->get_tile_size();

			this->unit_data_map[UnitNumber(*unit)] = std::move(unit_data);
		}

		connect(this->map_layer, &CMapLayer::unit_added, this, &unit_list_model::add_unit_data);
		connect(this->map_layer, &CMapLayer::unit_removed, this, &unit_list_model::remove_unit_data);
		connect(this->map_layer, &CMapLayer::unit_image_changed, this, &unit_list_model::update_unit_image);
		connect(this->map_layer, &CMapLayer::unit_tile_pos_changed, this, &unit_list_model::update_unit_tile_pos);
		connect(this->map_layer, &CMapLayer::unit_tile_size_changed, this, &unit_list_model::update_unit_tile_size);
	}

	emit map_layer_changed();
}

void unit_list_model::add_unit_data(const int unit_index, const unit_type *unit_type, const unit_type_variation *variation, const int frame, const player_color *player_color, const QPoint &tile_pos)
{
	unit_data unit_data;

	unit_data.image_source = unit_list_model::build_image_source(unit_type, variation, frame, player_color);
	unit_data.mirrored_image = frame < 0;
	unit_data.tile_pos = tile_pos;
	unit_data.tile_size = unit_type->get_tile_size();

	this->unit_data_map[unit_index] = std::move(unit_data);

	const QModelIndex index = this->index(unit_index);
	emit dataChanged(index, index);
}

void unit_list_model::update_unit_image(const int unit_index, const unit_type *unit_type, const unit_type_variation *variation, const int frame, const player_color *player_color)
{
	unit_data *unit_data = this->get_unit_data(unit_index);

	if (unit_data == nullptr) {
		log::log_error("Tried to update the image for unit " + std::to_string(unit_index) + ", of type \"" + unit_type->get_identifier() + "\", but it is not a part of the unit list model.");
		return;
	}

	unit_data->image_source = unit_list_model::build_image_source(unit_type, variation, frame, player_color);
	unit_data->mirrored_image = frame < 0;

	const QModelIndex index = this->index(unit_index);
	emit dataChanged(index, index, { static_cast<int>(role::image_source), static_cast<int>(role::mirrored_image) });
}

void unit_list_model::update_unit_tile_pos(const int unit_index, const QPoint &tile_pos)
{
	unit_data *unit_data = this->get_unit_data(unit_index);

	if (unit_data == nullptr) {
		log::log_error("Tried to update the tile position for unit " + std::to_string(unit_index) + ", but it is not a part of the unit list model.");
		return;
	}

	unit_data->tile_pos = tile_pos;

	const QModelIndex index = this->index(unit_index);
	emit dataChanged(index, index, { static_cast<int>(role::tile_pos) });
}

void unit_list_model::update_unit_tile_size(const int unit_index, const QSize &tile_size)
{
	unit_data *unit_data = this->get_unit_data(unit_index);

	if (unit_data == nullptr) {
		log::log_error("Tried to update the tile size for unit " + std::to_string(unit_index) + ", but it is not a part of the unit list model.");
		return;
	}

	unit_data->tile_size = tile_size;

	const QModelIndex index = this->index(unit_index);
	emit dataChanged(index, index, { static_cast<int>(role::tile_size) });
}

}
