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

#include "map/world_game_data.h"

#include "database/sml_data.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/world.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "unit/unit.h"
#include "unit/unit_find.h"

namespace wyrmgus {

void world_game_data::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "map_layer") {
		const size_t map_layer_index = std::stoul(value);

		if (map_layer_index >= CMap::get()->MapLayers.size()) {
			throw std::runtime_error("Invalid map layer index for world game data: \"" + std::to_string(map_layer_index) + "\".");
		}

		this->map_layer = CMap::get()->MapLayers[map_layer_index].get();
	} else if (key == "time_of_day") {
		this->time_of_day = this->world->get_time_of_day_schedule()->get_scheduled_times_of_day()[std::stoi(value)].get();
	} else if (key == "remaining_time_of_day_hours") {
		this->remaining_time_of_day_hours = std::stoi(value);
	} else if (key == "season") {
		this->season = this->world->get_season_schedule()->get_scheduled_seasons()[std::stoi(value)].get();
	} else if (key == "remaining_season_hours") {
		this->remaining_season_hours = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid site game data property: \"" + key + "\".");
	}
}

void world_game_data::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "map_rect") {
		this->map_rect = scope.to_rect();
	} else {
		throw std::runtime_error("Invalid site game data scope: \"" + scope.get_tag() + "\".");
	}
}

sml_data world_game_data::to_sml_data() const
{
	sml_data data(this->world->get_identifier());

	if (this->get_map_rect().isValid()) {
		data.add_child(sml_data::from_rect(this->get_map_rect(), "map_rect"));
	}

	if (this->map_layer != nullptr) {
		data.add_property("map_layer", std::to_string(this->map_layer->ID));
	}

	data.add_property("time_of_day", std::to_string(this->time_of_day->get_index()));
	data.add_property("remaining_time_of_day_hours", std::to_string(this->remaining_time_of_day_hours));
	data.add_property("season", std::to_string(this->season->get_index()));
	data.add_property("remaining_season_hours", std::to_string(this->remaining_season_hours));

	return data;
}

const std::string &world_game_data::get_current_cultural_name() const
{
	/*
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
	*/

	return this->world->get_name();
}

void world_game_data::do_per_in_game_hour_loop()
{
	this->decrement_remaining_season_hours();
	this->decrement_remaining_time_of_day_hours();
}

void world_game_data::decrement_remaining_time_of_day_hours()
{
	if (this->world->get_time_of_day_schedule() == nullptr) {
		return;
	}

	this->remaining_time_of_day_hours -= this->world->get_time_of_day_schedule()->HourMultiplier;

	if (this->remaining_time_of_day_hours <= 0) {
		this->increment_time_of_day();
	}
}

void world_game_data::increment_time_of_day()
{
	size_t current_time_of_day_id = this->time_of_day->get_index();
	current_time_of_day_id++;
	if (current_time_of_day_id >= this->world->get_time_of_day_schedule()->get_scheduled_times_of_day().size()) {
		current_time_of_day_id = 0;
	}

	this->set_time_of_day(this->world->get_time_of_day_schedule()->get_scheduled_times_of_day()[current_time_of_day_id].get());
	this->remaining_time_of_day_hours += this->time_of_day->get_hours(this->get_season());
}

/**
**	@brief	Set the time of day corresponding to an amount of hours
*/
void world_game_data::set_time_of_day_by_hours(const unsigned long long hours)
{
	if (!this->world->get_time_of_day_schedule()) {
		return;
	}

	int remaining_hours = hours % this->world->get_time_of_day_schedule()->get_total_hours();
	this->set_time_of_day(this->world->get_time_of_day_schedule()->get_scheduled_times_of_day().front().get());
	this->remaining_time_of_day_hours = this->time_of_day->get_hours(this->get_season());
	this->remaining_time_of_day_hours -= remaining_hours;

	while (this->remaining_time_of_day_hours <= 0) {
		this->increment_time_of_day();
	}
}

void world_game_data::set_time_of_day(const scheduled_time_of_day *time_of_day)
{
	if (this->time_of_day == time_of_day) {
		return;
	}

	const scheduled_time_of_day *old_time_of_day = this->time_of_day;
	this->time_of_day = time_of_day;

	const bool is_day_changed = (this->time_of_day && this->time_of_day->get_time_of_day()->is_day()) != (old_time_of_day && old_time_of_day->get_time_of_day()->is_day());
	const bool is_night_changed = (this->time_of_day && this->time_of_day->get_time_of_day()->is_night()) != (old_time_of_day && old_time_of_day->get_time_of_day()->is_night());

	//update the sight of all units
	if (is_day_changed || is_night_changed) {
		std::vector<CUnit *> units;
		Select(this->map_rect.topLeft(), this->map_rect.bottomRight(), units, map_layer->ID);

		for (CUnit *unit : units) {
			const tile *center_tile = unit->get_center_tile();
			if (center_tile != nullptr && center_tile->get_world() != this->world) {
				continue;
			}

			if (
				unit->IsAlive()
				&& (
					//if has day sight bonus and is entering or exiting day
					(is_day_changed && unit->Variable[DAYSIGHTRANGEBONUS_INDEX].Value != 0)
					//if has night sight bonus and is entering or exiting night
					|| (is_night_changed && unit->Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value != 0)
					)
				) {
				MapUnmarkUnitSight(*unit);
				UpdateUnitSightRange(*unit);
				MapMarkUnitSight(*unit);
			}
		}
	}
}

const time_of_day *world_game_data::get_time_of_day() const
{
	if (this->time_of_day == nullptr) {
		return nullptr;
	}

	return this->time_of_day->get_time_of_day();
}

void world_game_data::decrement_remaining_season_hours()
{
	if (!this->world->get_season_schedule()) {
		return;
	}

	this->remaining_season_hours -= this->world->get_season_schedule()->HourMultiplier;

	if (this->remaining_season_hours <= 0) {
		this->increment_season();
	}
}

void world_game_data::increment_season()
{
	size_t current_season_id = this->season->get_index();
	current_season_id++;
	if (current_season_id >= this->world->get_season_schedule()->get_scheduled_seasons().size()) {
		current_season_id = 0;
	}

	this->set_season(this->world->get_season_schedule()->get_scheduled_seasons()[current_season_id].get());
	this->remaining_season_hours += this->season->get_hours();
}

/**
**	@brief	Set the season corresponding to an amount of hours
**
**	@param	hours	The quantity of hours
*/
void world_game_data::set_season_by_hours(const unsigned long long hours)
{
	if (!this->world->get_season_schedule()) {
		return;
	}

	int remaining_hours = hours % this->world->get_season_schedule()->get_total_hours();
	this->set_season(this->world->get_season_schedule()->get_scheduled_seasons().front().get());
	this->remaining_season_hours = this->season->get_hours();
	this->remaining_season_hours -= remaining_hours;

	while (this->remaining_season_hours <= 0) {
		this->increment_season();
	}
}

void world_game_data::set_season(const scheduled_season *season)
{
	if (season == this->season) {
		return;
	}

	const wyrmgus::season *old_season = this->season ? this->season->get_season() : nullptr;
	const wyrmgus::season *new_season = season ? season->get_season() : nullptr;

	this->season = season;

	//update world tiles affected by the season change
	for (int x = this->map_rect.x(); x <= this->map_rect.right(); ++x) {
		for (int y = this->map_rect.y(); y <= this->map_rect.bottom(); ++y) {
			const QPoint tile_pos(x, y);
			const tile *tile = this->map_layer->Field(tile_pos);

			if (tile->get_world() != this->world) {
				continue;
			}

			//check if the tile's terrain graphics have changed due to the new season and if so, update the minimap
			if (
				(tile->player_info->SeenTerrain && tile->player_info->SeenTerrain->get_graphics(old_season) != tile->player_info->SeenTerrain->get_graphics(new_season))
				|| (tile->player_info->SeenOverlayTerrain && tile->player_info->SeenOverlayTerrain->get_graphics(old_season) != tile->player_info->SeenOverlayTerrain->get_graphics(new_season))
			) {
				UI.get_minimap()->UpdateXY(tile_pos, this->map_layer->ID);
			}
		}
	}

	//update units which may have had their variation become invalid due to the season change
	std::vector<CUnit *> units;
	Select(this->map_rect.topLeft(), this->map_rect.bottomRight(), units, map_layer->ID);

	for (CUnit *unit : units) {
		const tile *center_tile = unit->get_center_tile();
		if (center_tile != nullptr && center_tile->get_world() != this->world) {
			continue;
		}

		if (unit->IsAlive()) {
			const wyrmgus::unit_type_variation *variation = unit->GetVariation();
			if (variation != nullptr && !unit->can_have_variation(variation)) {
				unit->ChooseVariation(); //choose a new variation, as the old one has become invalid due to the season change
			}
		}
	}
}

const wyrmgus::season *world_game_data::get_season() const
{
	if (this->season == nullptr) {
		return nullptr;
	}

	return this->season->get_season();
}

}
