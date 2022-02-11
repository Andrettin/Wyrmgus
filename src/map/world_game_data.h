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
//      (c) Copyright 2021-2022 by Andrettin
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

class CMapLayer;

namespace wyrmgus {

class gsml_data;
class gsml_property;
class scheduled_season;
class scheduled_time_of_day;
class season;
class season_schedule;
class time_of_day;
class time_of_day_schedule;
class world;

class world_game_data final
{
public:
	explicit world_game_data(const world *world) : world(world)
	{
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	bool is_on_map() const
	{
		return this->map_layer != nullptr && this->map_rect.isValid();
	}

	const QRect &get_map_rect() const
	{
		return this->map_rect;
	}

	void set_map_rect(const QRect &map_rect, const CMapLayer *map_layer)
	{
		this->map_rect = map_rect;
		this->map_layer = map_layer;
	}

	const std::string &get_current_cultural_name() const;

	void do_per_in_game_hour_loop();

	void set_remaining_time_of_day_hours(const int hours)
	{
		this->remaining_time_of_day_hours = hours;
	}

private:
	void decrement_remaining_time_of_day_hours();
	void increment_time_of_day();

public:
	void set_time_of_day_by_hours(const unsigned long long hours);

	const scheduled_time_of_day *get_scheduled_time_of_day() const
	{
		return this->time_of_day;
	}

	void set_time_of_day(const scheduled_time_of_day *time_of_day);
	const wyrmgus::time_of_day *get_time_of_day() const;

	void set_remaining_season_hours(const int hours)
	{
		this->remaining_season_hours = hours;
	}

private:
	void decrement_remaining_season_hours();
	void increment_season();

public:
	void set_season_by_hours(const unsigned long long hours);

	const scheduled_season *get_scheduled_season() const
	{
		return this->season;
	}

	void set_season(const scheduled_season *season);
	const wyrmgus::season *get_season() const;

private:
	const wyrmgus::world *world = nullptr;
	QRect map_rect; //the map rectangle containing this world
	const CMapLayer *map_layer = nullptr; //the map layer containing this world
	const scheduled_time_of_day *time_of_day = nullptr;	/// the time of day for the map layer
	int remaining_time_of_day_hours = 0;		/// the quantity of hours remaining for the current time of day to end
	const scheduled_season *season = nullptr;			/// the current season for the map layer
	int remaining_season_hours = 0;				/// the quantity of hours remaining for the current season to end
};

}
