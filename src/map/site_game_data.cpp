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

#include "stratagus.h"

#include "map/site_game_data.h"

#include "ai/ai_local.h"
#include "database/sml_data.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/tile.h"
#include "player/player.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "util/assert_util.h"
#include "util/set_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void site_game_data::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "map_layer") {
		this->map_layer = CMap::get()->MapLayers[std::stoi(value)].get();
	} else {
		throw std::runtime_error("Invalid site game data property: \"" + key + "\".");
	}
}

void site_game_data::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "map_pos") {
		this->map_pos = scope.to_point();
	} else {
		throw std::runtime_error("Invalid site game data scope: \"" + scope.get_tag() + "\".");
	}
}

sml_data site_game_data::to_sml_data() const
{
	sml_data data(this->site->get_identifier());

	if (this->get_map_pos() != QPoint(-1, -1)) {
		data.add_child(sml_data::from_point(this->get_map_pos(), "map_pos"));
	}

	if (this->get_map_layer() != nullptr) {
		data.add_property("map_layer", std::to_string(this->get_map_layer()->ID));
	}

	return data;
}

const std::string &site_game_data::get_current_cultural_name() const
{
	const CUnit *unit = this->get_site_unit();

	if (unit != nullptr) {
		const civilization *name_civilization = unit->get_civilization();

		if (name_civilization == nullptr && unit->Player->get_index() == PlayerNumNeutral) {
			const CPlayer *unit_tile_owner = unit->get_center_tile_owner();
			if (unit_tile_owner != nullptr) {
				name_civilization = unit_tile_owner->get_civilization();
			}
		}

		return this->site->get_cultural_name(name_civilization);
	}

	return this->site->get_name();
}

void site_game_data::set_site_unit(CUnit *unit)
{
	if (unit == this->get_site_unit()) {
		return;
	}

	this->site_unit = unit;

	if (this->site_unit != nullptr && this->site_unit->Player != nullptr && this->site_unit->Player->get_index() != PlayerNumNeutral && !this->site_unit->UnderConstruction) {
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
		this->update_territory_tiles();
	}
}

CPlayer *site_game_data::get_realm_owner() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_realm_player();
	}

	return nullptr;
}

const landmass *site_game_data::get_landmass() const
{
	assert_throw(this->is_on_map());

	if (this->get_site_unit() != nullptr) {
		return CMap::get()->get_tile_landmass(this->get_site_unit()->tilePos, this->get_site_unit()->MapLayer->ID);
	}

	return CMap::get()->get_tile_landmass(this->get_map_pos(), this->get_map_layer()->ID);
}

void site_game_data::process_territory_tile(const tile *tile, const QPoint &tile_pos, const int z)
{
	if (CMap::get()->tile_borders_other_settlement_territory(tile_pos, z)) {
		this->add_border_tile(tile_pos);

		const site_set tile_border_settlements = CMap::get()->get_tile_border_settlements(tile_pos, z);
		set::merge(this->border_settlements, tile_border_settlements);
	} else if (CMap::get()->is_tile_on_map_borders(tile_pos, z)) {
		this->add_tile_pos_to_territory_rect(tile_pos);
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
				if (!CMap::get()->Info->IsPointOnMap(adjacent_pos, z)) {
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

void site_game_data::update_territory_tiles()
{
	if (this->get_site_unit() == nullptr || !this->site->is_settlement()) {
		return;
	}

	const CMapLayer *map_layer = this->get_site_unit()->MapLayer;
	const int z = map_layer->ID;
	const player_color *player_color = this->get_site_unit()->get_player_color();

	for (int x = this->territory_rect.x(); x <= this->territory_rect.right(); ++x) {
		for (int y = this->territory_rect.y(); y <= this->territory_rect.bottom(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = map_layer->Field(tile_pos);

			if (tile->get_settlement() != this->site) {
				continue;
			}

			UI.get_minimap()->update_territory_xy(tile_pos, z);

			emit map_layer->tile_image_changed(tile_pos, tile->get_terrain(), tile->SolidTile, player_color);
			emit map_layer->tile_overlay_image_changed(tile_pos, tile->get_overlay_terrain(), tile->OverlaySolidTile, player_color);
			emit map_layer->tile_transition_images_changed(tile_pos, tile->TransitionTiles, player_color);
			emit map_layer->tile_overlay_transition_images_changed(tile_pos, tile->OverlayTransitionTiles, player_color);
		}
	}

	this->update_border_territory_tiles();
}

void site_game_data::update_border_territory_tiles()
{
	//update the minimap territories of border tiles for adjacent settlements, in case a change in ownership of this one affects the realm outline of the other settlement owner's territory

	const int z = this->get_site_unit()->MapLayer->ID;
	for (const QPoint &tile_pos : this->border_tiles) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset == 0 && y_offset == 0) {
					continue;
				}

				const QPoint adjacent_pos(tile_pos.x() + x_offset, tile_pos.y() + y_offset);
				if (!CMap::get()->Info->IsPointOnMap(adjacent_pos, z)) {
					continue;
				}

				tile *adjacent_tile = CMap::get()->Field(adjacent_pos, z);
				if (adjacent_tile->get_settlement() != nullptr && adjacent_tile->get_settlement() != this->site) {
					UI.get_minimap()->update_territory_xy(adjacent_pos, z);
				}
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
