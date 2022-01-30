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
//      (c) Copyright 2020-2022 by Andrettin
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
#include "database/defines.h"
#include "database/sml_data.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/tile.h"
#include "player/player.h"
#include "population/employment_type.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/population_unit_key.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/set_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

site_game_data::site_game_data(const wyrmgus::site *site) : site(site)
{
}

site_game_data::~site_game_data()
{
}

void site_game_data::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "map_layer") {
		this->map_layer = CMap::get()->MapLayers[std::stoi(value)].get();
	} else if (key == "population") {
		this->set_population(std::stoll(value));
	} else {
		throw std::runtime_error("Invalid site game data property: \"" + key + "\".");
	}
}

void site_game_data::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "map_pos") {
		this->map_pos = scope.to_point();
	} else if (tag == "population_units") {
		scope.for_each_child([&](const sml_data &child_scope) {
			auto population_unit = make_qunique<wyrmgus::population_unit>();
			population_unit->moveToThread(QApplication::instance()->thread());
			database::process_sml_data(population_unit, child_scope);
			this->population_units.push_back(std::move(population_unit));
		});
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

	if (this->get_population() != 0) {
		data.add_property("population", std::to_string(this->get_population()));
	}

	if (!this->population_units.empty()) {
		sml_data population_units_data("population_units");

		for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
			population_units_data.add_child(population_unit->to_sml_data());
		}

		data.add_child(std::move(population_units_data));
	}

	return data;
}

void site_game_data::do_per_minute_loop()
{
	if (defines::get()->is_population_enabled() && this->site->is_settlement()) {
		this->do_population_growth();
	}
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

	if (this->site_unit != nullptr && this->site_unit->Player != nullptr && this->site_unit->Player->get_index() != PlayerNumNeutral && !this->site_unit->is_under_construction()) {
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

	CPlayer *old_owner = this->get_owner();

	this->owner = player;

	if (this->site->is_settlement()) {
		if (defines::get()->is_population_enabled()) {
			if (old_owner != nullptr) {
				old_owner->change_population(-this->get_population());

				old_owner->check_unit_home_settlements();
			}

			if (this->owner != nullptr) {
				if (this->get_population() != 0) {
					this->owner->change_population(this->get_population());
				} else {
					this->ensure_minimum_population();
				}

				this->owner->check_unit_home_settlements();
			} else {
				this->clear_population_units();
			}
		}

		if (GameCycle > 0) {
			this->update_border_tiles();
			this->update_territory_tiles();
		}

		for (const auto &[resource, tile_count] : this->resource_tile_counts) {
			const wyrmgus::resource *final_resource = resource->get_final_resource();

			if (!final_resource->is_special()) {
				continue;
			}

			if (old_owner != nullptr) {
				old_owner->check_special_resource(final_resource);
			}

			if (this->get_owner() != nullptr) {
				this->get_owner()->check_special_resource(final_resource);
			}
		}

		for (const auto &[resource, units] : this->resource_units) {
			const wyrmgus::resource *final_resource = resource->get_final_resource();

			if (!final_resource->is_special()) {
				continue;
			}

			if (old_owner != nullptr) {
				old_owner->check_special_resource(final_resource);
			}

			if (this->get_owner() != nullptr) {
				this->get_owner()->check_special_resource(final_resource);
			}
		}
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

	for (const wyrmgus::resource *child_resource : resource->get_child_resources()) {
		if (this->get_resource_tile_count(child_resource) > 0 || !this->get_resource_units(child_resource).empty()) {
			return true;
		}
	}

	return false;
}

void site_game_data::add_resource_unit(CUnit *unit, const resource *resource)
{
	assert_throw(resource != nullptr);

	std::vector<CUnit *> &resource_units = this->resource_units[resource];

	const bool was_empty = resource_units.empty();

	resource_units.push_back(unit);

	if (was_empty && resource->is_special() && this->get_owner() != nullptr) {
		this->get_owner()->check_special_resource(resource);
	}
}

void site_game_data::add_resource_unit(CUnit *unit)
{
	const resource *unit_resource = unit->Type->get_given_resource();
	if (unit_resource != nullptr) {
		this->add_resource_unit(unit, unit_resource);
	}

	for (const resource *produced_resource : AiHelpers.get_produced_resources(unit->Type)) {
		if (produced_resource == unit_resource) {
			continue;
		}

		this->add_resource_unit(unit, produced_resource);
	}
}

void site_game_data::remove_resource_unit(CUnit *unit, const resource *resource)
{
	assert_throw(resource != nullptr);

	std::vector<CUnit *> &resource_units = this->resource_units[resource];
	vector::remove(resource_units, unit);

	if (resource_units.empty()) {
		this->resource_units.erase(resource);

		if (resource->is_special() && this->get_owner() != nullptr) {
			this->get_owner()->check_special_resource(resource);
		}
	}
}

void site_game_data::remove_resource_unit(CUnit *unit)
{
	const resource *unit_resource = unit->Type->get_given_resource();
	if (unit_resource != nullptr) {
		this->remove_resource_unit(unit, unit_resource);
	}

	for (const resource *produced_resource : AiHelpers.get_produced_resources(unit->Type)) {
		if (produced_resource == unit_resource) {
			continue;
		}

		this->remove_resource_unit(unit, produced_resource);
	}
}

void site_game_data::set_population(const int64_t population)
{
	if (population == this->get_population()) {
		return;
	}

	const int64_t old_population = this->get_population();

	this->population = population;

	if (this->get_owner() != nullptr) {
		this->get_owner()->change_population(this->get_population() - old_population);
	}
}

void site_game_data::ensure_minimum_population()
{
	if (this->get_population() > 0) {
		return;
	}

	this->set_default_population_type_population(site_game_data::min_population);
}

QVariantList site_game_data::get_population_units_qvariant_list() const
{
	QVariantList list;

	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		list.append(QVariant::fromValue(population_unit.get()));
	}

	return list;
}

population_unit *site_game_data::get_population_unit(const population_unit_key &key) const
{
	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		if (population_unit->get_key() == key) {
			return population_unit.get();
		}
	}

	return nullptr;
}

void site_game_data::create_population_unit(const population_unit_key &key, const int64_t population)
{
	auto population_unit = make_qunique<wyrmgus::population_unit>(key, population);
	population_unit->moveToThread(QApplication::instance()->thread());
	this->population_units.push_back(std::move(population_unit));

	this->change_population(population);

	emit population_units_changed(this->get_population_units_qvariant_list());
}

void site_game_data::remove_population_unit(const population_unit_key &key)
{
	for (size_t i = 0; i < this->population_units.size(); ++i) {
		const qunique_ptr<population_unit> &population_unit = this->population_units[i];

		if (population_unit->get_key() == key) {
			this->change_population(-population_unit->get_population());
			this->population_units.erase(this->population_units.begin() + i);

			emit population_units_changed(this->get_population_units_qvariant_list());

			return;
		}
	}
}

void site_game_data::clear_population_units()
{
	this->population_units.clear();
	this->set_population(0);

	emit population_units_changed(this->get_population_units_qvariant_list());
}

void site_game_data::set_population_unit_population(const population_unit_key &key, const int64_t population)
{
	if (population == 0) {
		this->remove_population_unit(key);
		return;
	}

	population_unit *population_unit = this->get_population_unit(key);
	if (population_unit != nullptr) {
		const int64_t old_population = population_unit->get_population();
		population_unit->set_population(population);
		this->change_population(population - old_population);
		return;
	}

	//create a population unit for the key if no existing population unit was found
	this->create_population_unit(key, population);
}

void site_game_data::change_population_unit_population(const population_unit_key &key, const int64_t change)
{
	population_unit *population_unit = this->get_population_unit(key);
	if (population_unit != nullptr) {

		population_unit->change_population(change);
		this->change_population(change);

		if (population_unit->get_population() <= 0) {
			this->remove_population_unit(key);
		}

		return;
	}

	if (change > 0) {
		this->create_population_unit(key, change);
	}
}

int64_t site_game_data::get_population_type_population(const population_type *population_type) const
{
	int64_t population = 0;

	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		if (population_unit->get_type() == population_type) {
			population += population_unit->get_population();
		}
	}

	return population;
}

const population_type *site_game_data::get_default_population_type() const
{
	return this->get_class_population_type(defines::get()->get_default_population_class());
}

void site_game_data::set_default_population_type_population(const int64_t population)
{
	const population_type *population_type = this->get_default_population_type();

	if (population_type == nullptr) {
		return;
	}

	const population_unit_key population_unit_key(population_type);
	this->set_population_unit_population(population_unit_key, population);
}

void site_game_data::change_default_population_type_population(const int64_t population)
{
	const population_type *population_type = this->get_default_population_type();

	if (population_type == nullptr) {
		return;
	}

	const population_unit_key population_unit_key(population_type);
	this->change_population_unit_population(population_unit_key, population);
}

std::vector<std::pair<population_unit *, int64_t>> site_game_data::get_population_units_permyriad() const
{
	//get the settlement's population proportion for each population unit, in permyriad
	std::vector<std::pair<population_unit *, int64_t>> population_units_permyriad;

	const int64_t total_population = this->get_population();

	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		const int64_t permyriad = population_unit->get_population() * 10000 / total_population;

		population_units_permyriad.emplace_back(std::make_pair(population_unit.get(), permyriad));
	}

	return population_units_permyriad;
}

void site_game_data::do_population_growth()
{
	const int food_surplus = this->get_food_supply() - this->get_food_demand();

	const int64_t available_capacity = this->get_population_capacity() - this->get_population();
	int64_t population_growth = static_cast<int64_t>(food_surplus) * defines::get()->get_population_growth_per_food();

	if (available_capacity > 0) {
		//the population cannot grow beyond capacity
		population_growth = std::min<int64_t>(available_capacity, population_growth);
	} else if (available_capacity < 0) {
		//must decrease by something
		population_growth = std::min<int64_t>(-defines::get()->get_population_growth_per_food(), population_growth);

		//the population cannot decrease by more than the overpopulation
		const int64_t overpopulation = this->get_population() - this->get_population_capacity();
		population_growth = std::max<int64_t>(-overpopulation, population_growth);
	}

	if (population_growth != 0) {
		if ((this->get_population() + population_growth) < site_game_data::min_population) {
			population_growth = site_game_data::min_population - this->get_population();
		}

		this->apply_population_growth(population_growth);
	}
}

void site_game_data::apply_population_growth(const int64_t population_growth)
{
	const std::vector<std::pair<population_unit *, int64_t>> population_units_permyriad = this->get_population_units_permyriad();

	int64_t remaining_population_growth = population_growth;

	for (const auto &[population_unit, permyriad] : population_units_permyriad) {
		const int64_t population_unit_growth = population_growth * permyriad / 10000;

		if (population_unit_growth == 0) {
			continue;
		}

		population_unit_key key = population_unit->get_key();

		if (!population_unit->get_type()->is_growable()) {
			key.type = this->get_default_population_type();
		}

		this->change_population_unit_population(key, population_unit_growth);

		remaining_population_growth -= population_unit_growth;
	}

	//if there is any remaining population growth, apply it to the default population class if positive, or subtract from existing population units if negative
	if (remaining_population_growth != 0) {
		if (remaining_population_growth > 0) {
			this->change_default_population_type_population(remaining_population_growth);
		} else {
			for (size_t i = 0; i < this->population_units.size();) {
				const qunique_ptr<population_unit> &population_unit = this->population_units[i];

				int64_t change = remaining_population_growth;
				bool removed_pop = false;

				if (std::abs(remaining_population_growth) >= population_unit->get_population()) {
					change = -population_unit->get_population();
					removed_pop = true;
				}

				this->change_population_unit_population(population_unit->get_key(), change);

				remaining_population_growth -= change;

				if (!removed_pop) {
					++i;
				}

				if (remaining_population_growth == 0) {
					break;
				}
			}
		}
	}
}

const population_type *site_game_data::get_class_population_type(const population_class *population_class) const
{
	if (this->owner == nullptr) {
		return nullptr;
	}

	return this->owner->get_class_population_type(population_class);
}

void site_game_data::on_settlement_building_added(const CUnit *building)
{
	if (building->Variable[SUPPLY_INDEX].Value != 0) {
		this->change_food_supply(building->Variable[SUPPLY_INDEX].Value);
	}

	if (building->Type->get_employment_type() != nullptr) {
		this->change_employment_capacity(building->Type->get_employment_type(), building->Type->get_employment_capacity());
	}
}

void site_game_data::on_settlement_building_removed(const CUnit *building)
{
	if (building->Variable[SUPPLY_INDEX].Value != 0) {
		this->change_food_supply(-building->Variable[SUPPLY_INDEX].Value);
	}

	if (building->Type->get_employment_type() != nullptr) {
		this->change_employment_capacity(building->Type->get_employment_type(), -building->Type->get_employment_capacity());
	}
}

}
