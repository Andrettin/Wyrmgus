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
//      (c) Copyright 2022 by Andrettin
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

namespace wyrmgus {

class dungeon_generation_settings;
class terrain_type;
class unit_type;

class dungeon_generator final
{
public:
	static QRect create_rect(const QPoint &base_top_left, const QPoint &base_bottom_right)
	{
		QPoint top_left = base_top_left;
		QPoint bottom_right = base_bottom_right;

		if (base_top_left.x() > base_bottom_right.x()) {
			top_left.setX(base_bottom_right.x());
			bottom_right.setX(base_top_left.x());
		}

		if (base_top_left.y() > base_bottom_right.y()) {
			top_left.setY(base_bottom_right.y());
			bottom_right.setY(base_top_left.y());
		}

		return QRect(top_left, bottom_right);
	}

	explicit dungeon_generator(const QRect &map_rect, const int z, const dungeon_generation_settings *settings)
		: map_rect(map_rect), z(z), settings(settings)
	{
	}

	void generate() const;

private:
	void generate_central_room() const;
	bool generate_room(const QPoint &edge_tile_pos, const QPoint &dir_offset) const;
	void generate_corridor_to_room(const QPoint &edge_tile_pos, const QPoint &dir_offset) const;

	void generate_room_features(const QRect &room_floor_rect) const;
	void generate_ending_room_features(const QRect &room_floor_rect) const;
	void generate_internal_room_features(const QRect &room_floor_rect) const;

	void extend_dungeon(const QPoint &edge_tile_pos, const QPoint &dir_offset) const;

	QPoint find_edge_tile_pos(const QPoint &dir_offset) const;
	bool is_tile_clear(const QPoint &tile_pos) const;
	bool is_area_clear(const QRect &rect) const;

	void set_tile_terrain(const QPoint &tile_pos, const terrain_type *terrain) const;
	void set_area_terrain(const QRect &rect, const terrain_type *terrain) const;
	void complete_area_terrain(const QRect &rect, const terrain_type *terrain) const;

	void generate_guard(const QPoint &tile_pos) const;

	const terrain_type *get_floor_terrain() const;
	const terrain_type *get_wall_terrain() const;
	const terrain_type *get_deep_wall_terrain() const;
	const terrain_type *get_water_terrain() const;

	const unit_type *get_random_unit_type() const;
	const unit_type *get_random_trap_unit_type() const;

private:
	QRect map_rect;
	int z = 0;
	const dungeon_generation_settings *settings = nullptr;
};

}
