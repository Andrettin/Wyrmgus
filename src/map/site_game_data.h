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

#pragma once

#include "economy/resource_container.h"
#include "map/site_container.h"

class CMapLayer;
class CPlayer;
class CUnit;

namespace wyrmgus {

class landmass;
class resource;
class site;
class sml_data;
class sml_property;
class tile;

class site_game_data final
{
public:
	static constexpr int min_population = 100;
	static constexpr int population_per_food = 10000;

	explicit site_game_data(const site *site) : site(site)
	{
	}

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	sml_data to_sml_data() const;

	const std::string &get_current_cultural_name() const;

	CUnit *get_site_unit() const
	{
		return this->site_unit;
	}

	void set_site_unit(CUnit *unit);

	CPlayer *get_owner() const
	{
		return this->owner;
	}

	void set_owner(CPlayer *player);

	CPlayer *get_realm_owner() const;

	const QPoint &get_map_pos() const
	{
		return this->map_pos;
	}

	void set_map_pos(const QPoint &pos)
	{
		this->map_pos = pos;
	}

	const CMapLayer *get_map_layer() const
	{
		return this->map_layer;
	}

	void set_map_layer(const CMapLayer *map_layer)
	{
		this->map_layer = map_layer;
	}

	bool is_on_map() const
	{
		if (this->get_site_unit() != nullptr) {
			return true;
		}

		return this->get_map_layer() != nullptr && this->get_map_pos() != QPoint(-1, -1);
	}

	const landmass *get_landmass() const;

	const QRect &get_territory_rect() const
	{
		return this->territory_rect;
	}

	void process_territory_tile(const tile *tile, const QPoint &tile_pos, const int z);

	void add_border_tile(const QPoint &tile_pos)
	{
		this->border_tiles.push_back(tile_pos);

		this->add_tile_pos_to_territory_rect(tile_pos);
	}

	void add_tile_pos_to_territory_rect(const QPoint &tile_pos)
	{
		if (this->territory_rect.isNull()) {
			this->territory_rect = QRect(tile_pos, QSize(1, 1));
		} else {
			if (tile_pos.x() < this->territory_rect.x()) {
				this->territory_rect.setX(tile_pos.x());
			} else if (tile_pos.x() > this->territory_rect.right()) {
				this->territory_rect.setRight(tile_pos.x());
			}
			if (tile_pos.y() < this->territory_rect.y()) {
				this->territory_rect.setY(tile_pos.y());
			} else if (tile_pos.y() > this->territory_rect.bottom()) {
				this->territory_rect.setBottom(tile_pos.y());
			}
		}
	}

	void update_border_tiles();
	void update_territory_tiles();
	void update_border_territory_tiles();

	const std::vector<QPoint> &get_trade_route_tiles() const
	{
		return this->trade_route_tiles;
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	bool has_resource_source(const resource *resource) const;

	int get_resource_tile_count(const resource *resource) const
	{
		const auto find_iterator = this->resource_tile_counts.find(resource);
		if (find_iterator != this->resource_tile_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void increment_resource_tile_count(const resource *resource)
	{
		++this->resource_tile_counts[resource];
	}

	void decrement_resource_tile_count(const resource *resource)
	{
		const int quantity = --this->resource_tile_counts[resource];

		if (quantity <= 0) {
			this->resource_tile_counts.erase(resource);
		}
	}

	const std::vector<CUnit *> &get_resource_units(const resource *resource) const
	{
		static std::vector<CUnit *> empty_vector;

		const auto find_iterator = this->resource_units.find(resource);
		if (find_iterator != this->resource_units.end()) {
			return find_iterator->second;
		}

		return empty_vector;
	}

	void add_resource_unit(CUnit *unit, const resource *resource);
	void add_resource_unit(CUnit *unit);
	void remove_resource_unit(CUnit *unit, const resource *resource);
	void remove_resource_unit(CUnit *unit);

	void clear_resource_units()
	{
		this->resource_units.clear();
	}

	const site_set &get_border_settlements() const
	{
		return this->border_settlements;
	}

	int get_population() const
	{
		return this->population;
	}

	void set_population(const int population)
	{
		if (population == this->get_population()) {
			return;
		}

		this->population = population;
	}

	void change_population(const int change)
	{
		this->set_population(this->get_population() + change);
	}

	void do_population_growth();

	int get_food_supply() const
	{
		return this->food_supply;
	}

	void set_food_supply(const int food_supply)
	{
		if (food_supply == this->get_food_supply()) {
			return;
		}

		this->food_supply = food_supply;
	}

	void change_food_supply(const int change)
	{
		this->set_food_supply(this->get_food_supply() + change);
	}

	int get_food_demand() const
	{
		return this->get_population() / site_game_data::population_per_food;
	}

private:
	const wyrmgus::site *site = nullptr;
	CUnit *site_unit = nullptr; //unit which represents the site
	CPlayer *owner = nullptr;
	QPoint map_pos = QPoint(-1, -1);
	const CMapLayer *map_layer = nullptr;
	std::vector<QPoint> border_tiles; //the tiles for the settlement which border the territory of another settlement
	QRect territory_rect; //the territory rectangle of the site
	std::vector<QPoint> trade_route_tiles; //the tiles containing a trade route in the settlement's territory
	bool coastal = false;
	resource_map<int> resource_tile_counts; //resource tile counts in the settlement's territory
	resource_map<std::vector<CUnit *>> resource_units; //resource units in the settlement's territory
	site_set border_settlements; //other settlements bordering this one
	int population = 0;
	int food_supply = 0;
};

}
