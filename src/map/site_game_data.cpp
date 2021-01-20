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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "map/site_game_data.h"

#include "ai/ai_local.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/tile.h"
#include "player.h"
#include "unit/unit.h"
#include "util/vector_util.h"

namespace wyrmgus {

const std::string &site_game_data::get_current_cultural_name() const
{
	if (this->get_site_unit() != nullptr) {
		return this->site->get_cultural_name(this->get_site_unit()->get_civilization());
	}

	return this->site->get_name();
}

void site_game_data::set_site_unit(CUnit *unit)
{
	if (unit == this->get_site_unit()) {
		return;
	}

	this->site_unit = unit;

	if (this->site_unit != nullptr && this->site_unit->Player != nullptr && this->site_unit->Player->Index != PlayerNumNeutral && !this->site_unit->UnderConstruction) {
		this->set_owner(this->site_unit->Player);
	} else {
		this->set_owner(nullptr);
	}
}

void site_game_data::set_owner(CPlayer *player)
{
	if (player == this->get_owner()) {
		return;
	}

	this->owner = player;

	if (this->site->is_settlement() && GameCycle > 0) {
		this->update_border_tiles();
		this->update_minimap_territory();
	}
}

CPlayer *site_game_data::get_realm_owner() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_realm_player();
	}

	return nullptr;
}

void site_game_data::process_territory_tile(const tile *tile, const QPoint &tile_pos, const int z)
{
	if (CMap::get()->tile_borders_other_settlement_territory(tile_pos, z)) {
		this->add_border_tile(tile_pos);
	}

	if (tile->is_on_trade_route()) {
		this->trade_route_tiles.push_back(tile_pos);
	}

	if (!this->coastal && tile->is_coastal_water() && !tile->is_river()) {
		this->coastal = true;
	}

	const resource *tile_resource = tile->get_resource();
	if (tile_resource != nullptr) {
		this->increment_resource_tile_count(tile_resource);
	}
}

void site_game_data::update_border_tiles()
{
	if (this->get_site_unit() == nullptr) {
		return;
	}

	const int z = this->get_site_unit()->MapLayer->ID;
	for (const QPoint &tile_pos : this->border_tiles) {
		CMap::get()->CalculateTileOwnershipTransition(tile_pos, z);

		//update adjacent tiles with different settlements as well
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset == 0 && y_offset == 0) {
					continue;
				}

				const QPoint adjacent_pos(tile_pos.x() + x_offset, tile_pos.y() + y_offset);
				if (!CMap::get()->Info.IsPointOnMap(adjacent_pos, z)) {
					continue;
				}

				tile *adjacent_tile = CMap::get()->Field(adjacent_pos, z);
				if (adjacent_tile->get_settlement() != nullptr && adjacent_tile->get_settlement() != this->site) {
					CMap::get()->CalculateTileOwnershipTransition(adjacent_pos, z);
				}
			}
		}
	}
}

void site_game_data::update_minimap_territory()
{
	if (this->get_site_unit() == nullptr) {
		return;
	}

	const int z = this->get_site_unit()->MapLayer->ID;

	for (int x = this->territory_rect.x(); x <= this->territory_rect.right(); ++x) {
		for (int y = this->territory_rect.y(); y <= this->territory_rect.bottom(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = CMap::get()->Field(tile_pos, z);

			if (tile->get_settlement() == this->site) {
				UI.get_minimap()->update_territory_xy(tile_pos, z);
			}
		}
	}
}

bool site_game_data::has_resource_source(const resource *resource) const
{
	if (this->get_resource_tile_count(resource) > 0 || !this->get_resource_units(resource).empty()) {
		return true;
	}

	for (const wyrmgus::resource *child_resource : resource->ChildResources) {
		if (this->get_resource_tile_count(child_resource) > 0 || !this->get_resource_units(child_resource).empty()) {
			return true;
		}
	}

	return false;
}

void site_game_data::add_resource_unit(CUnit *unit)
{
	const resource *unit_resource = unit->Type->get_given_resource();
	this->add_resource_unit(unit, unit_resource);

	for (const resource *produced_resource : AiHelpers.get_produced_resources(unit->Type)) {
		if (produced_resource == unit_resource) {
			continue;
		}

		this->add_resource_unit(unit, produced_resource);
	}
}

void site_game_data::remove_resource_unit(CUnit *unit, const resource *resource)
{
	std::vector<CUnit *> &resource_units = this->resource_units[resource];
	vector::remove(resource_units, unit);

	if (resource_units.empty()) {
		this->resource_units.erase(resource);
	}
}

void site_game_data::remove_resource_unit(CUnit *unit)
{
	const resource *unit_resource = unit->Type->get_given_resource();
	this->remove_resource_unit(unit, unit_resource);

	for (const resource *produced_resource : AiHelpers.get_produced_resources(unit->Type)) {
		if (produced_resource == unit_resource) {
			continue;
		}

		this->remove_resource_unit(unit, produced_resource);
	}
}

}
