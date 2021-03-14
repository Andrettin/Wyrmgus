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
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "util/exception_util.h"

namespace wyrmgus {

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
		CUnit *unit = this->get_unit(unit_slot);

		switch (model_role) {
			case role::unit:
				return QVariant::fromValue(unit);
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

	this->units.clear();

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

			this->units[UnitNumber(*unit)] = unit;
		}
	}

	emit map_layer_changed();
}

void unit_list_model::add_unit(CUnit *unit)
{
	const int slot = UnitNumber(*unit);
	this->units[slot] = unit;

	const QModelIndex index = this->index(slot);
	emit dataChanged(index, index, { static_cast<int>(role::unit) });
}

void unit_list_model::remove_unit(CUnit *unit)
{
	const int slot = UnitNumber(*unit);
	this->units.erase(slot);

	const QModelIndex index = this->index(slot);
	emit dataChanged(index, index, { static_cast<int>(role::unit) });
}

}
