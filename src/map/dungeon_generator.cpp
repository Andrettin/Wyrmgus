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
//      (c) Copyright 2022 by Andrettin and Mike Anderson
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

#include "map/dungeon_generator.h"

#include "character.h"
#include "direction.h"
#include "map/dungeon_generation_settings.h"
#include "map/map.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "player/player.h"
#include "player/player_type.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/container_random_util.h"
#include "util/point_util.h"
#include "util/random.h"
#include "util/rect_util.h"

namespace wyrmgus {

//largely based on the dungeon generation code written by Mike Anderson for the rogue-like game Tyrant, which is licensed under the GPL 2.0

void dungeon_generator::generate() const
{
	assert_throw(this->settings != nullptr);
	assert_throw(this->settings->get_floor_terrain() != nullptr);
	assert_throw(this->settings->get_wall_terrain() != nullptr);
	assert_throw(this->settings->get_deep_wall_terrain() != nullptr);
	assert_throw(this->settings->get_water_terrain() != nullptr);
	assert_throw(!this->settings->get_unit_types().empty());
	assert_throw(!this->settings->get_item_unit_types().empty());
	assert_throw(!this->settings->get_trap_unit_types().empty());
	assert_throw(!this->settings->get_heroes().empty());

	//set the edges to be walls
	rect::for_each_edge_point(this->map_rect, [&](const QPoint &tile_pos) {
		this->set_tile_terrain(tile_pos, this->get_wall_terrain());
	});

	this->generate_central_room();

	const int dungeon_density_percent = 10;
	const int build_loop_max = this->map_rect.width() * this->map_rect.height() * dungeon_density_percent / 100;

	static constexpr std::array cardinal_directions = {direction::north, direction::east, direction::south, direction::west};

	for (int i = 0; i < build_loop_max; ++i) {
		const direction dir = container::get_random(cardinal_directions);
		const QPoint dir_offset = direction_to_offset(dir);

		const QPoint edge_tile_pos = this->find_edge_tile_pos(dir_offset);

		if (edge_tile_pos == QPoint(-1, -1)) {
			continue;
		}

		this->extend_dungeon(edge_tile_pos, dir_offset);
	}

	//set tiles which have no terrain so far to be walls
	rect::for_each_point(this->map_rect, [&](const QPoint &tile_pos) {
		tile *tile = CMap::get()->Field(tile_pos, this->z);

		if (tile->get_terrain() == nullptr) {
			tile->SetTerrain(this->get_floor_terrain());
			tile->SetTerrain(this->get_wall_terrain());
		}
	});

	const int generated_unit_count = this->map_rect.width() * this->map_rect.height() / 400;
	for (int i = 0; i < generated_unit_count; ++i) {
		this->generate_creep();
	}

	if (this->settings->get_glyph_unit_type() != nullptr) {
		CMap::get()->generate_neutral_units(this->settings->get_glyph_unit_type(), 1, this->map_rect.topLeft(), this->map_rect.bottomRight(), false, this->z);
	}

	//create the hero for each player
	for (CPlayer *player : CPlayer::get_non_neutral_players()) {
		if (player->get_type() != player_type::person) {
			continue;
		}

		this->generate_hero(player);
	}

	//make wall tiles which only border other wall tiles into deep wall tiles
	rect::for_each_point(this->map_rect, [&](const QPoint &tile_pos) {
		tile *tile = CMap::get()->Field(tile_pos, this->z);

		if (tile->get_overlay_terrain() != this->get_wall_terrain()) {
			return;
		}

		bool adjacent_to_non_wall = false;
		point::for_each_adjacent_until(tile_pos, [&](const QPoint &adjacent_pos) {
			if (!this->map_rect.contains(adjacent_pos)) {
				return false;
			}

			const wyrmgus::tile *adjacent_tile = CMap::get()->Field(adjacent_pos, this->z);

			if (adjacent_tile->get_overlay_terrain() == this->get_wall_terrain()) {
				return false;
			}

			if (adjacent_tile->get_overlay_terrain() == this->get_deep_wall_terrain()) {
				return false;
			}

			adjacent_to_non_wall = true;
			return true;
		});

		if (adjacent_to_non_wall) {
			return;
		}

		tile->SetTerrain(this->get_deep_wall_terrain());
	});
}

void dungeon_generator::generate_central_room() const
{
	const QPoint map_top_left = this->map_rect.topLeft();
	const QPoint map_bottom_right = this->map_rect.bottomRight();

	const int map_middle_x = (map_top_left.x() + map_bottom_right.x()) / 2;
	const int map_middle_y = (map_top_left.y() + map_bottom_right.y()) / 2;

	const int start_x = map_middle_x - random::get()->dice(3);
	const int end_x = map_middle_x + random::get()->dice(3);

	const int start_y = map_middle_y - random::get()->dice(3);
	const int end_y = map_middle_y + random::get()->dice(3);

	for (int x = start_x; x <= end_x; ++x) {
		for (int y = start_y; y <= end_y; ++y) {
			const QPoint tile_pos(x, y);
			this->set_tile_terrain(tile_pos, this->get_floor_terrain());
		}
	}
}

void dungeon_generator::generate_oval_room(const QPoint &edge_tile_pos, const QPoint &dir_offset) const
{
	const int width = random::get()->dice(2, 3);
	const int height = random::get()->dice(2, 3);

	const QPoint start_pos = edge_tile_pos + (dir_offset - QPoint(1, 1)) * QPoint(width, height);
	const QPoint end_pos = edge_tile_pos + (dir_offset + QPoint(1, 1)) * QPoint(width, height);

	const QRect room_rect = dungeon_generator::create_rect(start_pos, end_pos);

	if (!this->is_area_clear(room_rect)) {
		return;
	}

	const QPoint cp = (start_pos + end_pos) / 2;

	for (int x = start_pos.x(); x <= (start_pos.x() + width * 2); ++x) {
		for (int y = start_pos.y(); y < (start_pos.y() + height * 2); ++y) {
			if ((((x - cp.x()) * (x - cp.x()) * 100) / (width * width) + ((y - cp.y()) * (y - cp.y()) * 100) / (height * height)) < 100) {
				this->set_tile_terrain(QPoint(x, y), this->get_floor_terrain());
			}
		}
	}

	const QRect inner_rect = dungeon_generator::create_rect(cp, edge_tile_pos);
	this->set_area_terrain(inner_rect, this->get_floor_terrain());
}

bool dungeon_generator::generate_room(const QPoint &edge_tile_pos, const QPoint &dir_offset) const
{
	const QPoint min_pos_offset(random::get()->dice(std::abs(dir_offset.x() - 1), 5), random::get()->dice(std::abs(dir_offset.y() - 1), 5));
	const QPoint min_pos = edge_tile_pos - min_pos_offset;

	const QPoint max_pos_offset(random::get()->dice(std::abs(dir_offset.x() + 1), 5), random::get()->dice(std::abs(dir_offset.y() + 1), 5));
	const QPoint max_pos = edge_tile_pos + max_pos_offset;

	const QRect room_rect = dungeon_generator::create_rect(min_pos, max_pos);

	if (room_rect.width() < 4 || room_rect.height() < 4) {
		return false;
	}

	if (!this->is_area_clear(room_rect)) {
		return false;
	}

	//set the floor
	const QRect room_floor_rect(room_rect.topLeft() + QPoint(1, 1), room_rect.bottomRight() - QPoint(1, 1));
	this->set_area_terrain(room_floor_rect, this->get_floor_terrain());

	//create a passage
	this->set_tile_terrain(edge_tile_pos, this->get_floor_terrain());

	if (random::get()->dice(2) == 1) {
		this->complete_area_terrain(room_rect, this->get_wall_terrain());
		this->generate_ending_room_features(room_floor_rect);
	} else {
		QPoint top_left = edge_tile_pos;
		QPoint bottom_right = edge_tile_pos;

		if (dir_offset.x() == 0) {
			top_left.setX(min_pos.x());
			bottom_right.setX(max_pos.x());
		}

		if (dir_offset.y() == 0) {
			top_left.setY(min_pos.y());
			bottom_right.setY(max_pos.y());
		}

		const QRect rect = dungeon_generator::create_rect(top_left, bottom_right);
		this->complete_area_terrain(rect, this->get_wall_terrain());

		this->generate_room_features(room_floor_rect);
	}

	return true;
}

void dungeon_generator::generate_corridor_to_room(const QPoint &edge_tile_pos, const QPoint &dir_offset) const
{
	const int corridor_length = random::get()->dice(2, 10);

	//ensure that the corridor is clear (3-tiles-wide)
	const QPoint corridor_area_top_left(edge_tile_pos.x() - dir_offset.y(), edge_tile_pos.y() - dir_offset.x());
	const QPoint corridor_area_bottom_right(edge_tile_pos.x() + corridor_length * dir_offset.x() + dir_offset.y(), edge_tile_pos.y() + corridor_length * dir_offset.y() + dir_offset.x());
	const QRect corridor_area = dungeon_generator::create_rect(corridor_area_top_left, corridor_area_bottom_right);
	if (!this->is_area_clear(corridor_area)) {
		return;
	}

	if (!this->generate_room(edge_tile_pos + (corridor_length * dir_offset), dir_offset)) {
		return;
	}

	const QRect corridor_floor_rect = dungeon_generator::create_rect(edge_tile_pos, edge_tile_pos + (corridor_length * dir_offset));
	this->set_area_terrain(corridor_floor_rect, this->get_floor_terrain());

	this->set_tile_terrain(QPoint(edge_tile_pos.x() + dir_offset.y(), edge_tile_pos.y() - dir_offset.x()), this->get_wall_terrain());
	this->set_tile_terrain(QPoint(edge_tile_pos.x() - dir_offset.y(), edge_tile_pos.y() + dir_offset.x()), this->get_wall_terrain());

	const int j1 = random::get()->generate_in_range(1, corridor_length - 1);
	this->generate_room(edge_tile_pos + (j1 * dir_offset), QPoint(dir_offset.y(), -dir_offset.x()));

	const int j2 = random::get()->generate_in_range(1, corridor_length - 1);
	this->generate_room(edge_tile_pos + (j2 * dir_offset), QPoint(-dir_offset.y(), dir_offset.x()));
}

void dungeon_generator::generate_room_features(const QRect &room_floor_rect) const
{
	const int width = room_floor_rect.width();
	const int height = room_floor_rect.height();

	switch (random::get()->dice(50)) {
		case 10:
		case 11:
			//central room
			if (width > 5 && height > 5) {
				const QRect inner_rect(room_floor_rect.topLeft() + QPoint(1, 1), room_floor_rect.bottomRight() - QPoint(1, 1));
				this->set_area_terrain(inner_rect, this->get_wall_terrain());

				const QRect innermost_rect(room_floor_rect.topLeft() + QPoint(2, 2), room_floor_rect.bottomRight() - QPoint(2, 2));
				this->set_area_terrain(innermost_rect, this->get_floor_terrain());
				
				this->generate_room_features(innermost_rect);

				switch (random::get()->dice(4)) {
					case 1:
						this->set_tile_terrain(inner_rect.topLeft() + QPoint(0, random::get()->dice(height - 4)), this->get_floor_terrain());
						break;
					case 2:
						this->set_tile_terrain(inner_rect.topRight() + QPoint(0, random::get()->dice(height - 4)), this->get_floor_terrain());
						break;
					case 3:
						this->set_tile_terrain(inner_rect.topLeft() + QPoint(random::get()->dice(width - 4), 0), this->get_floor_terrain());
						break;
					case 4:
						this->set_tile_terrain(inner_rect.bottomLeft() + QPoint(random::get()->dice(width - 4), 0), this->get_floor_terrain());
						break;
				}
			}
			break;
		default:
			this->generate_internal_room_features(room_floor_rect);
			break;
	}
}

void dungeon_generator::generate_ending_room_features(const QRect &room_floor_rect) const
{
	switch (random::get()->dice(50)) {
		case 1:
			//dungeon shop
			break;
		case 2:
			//side dungeon
			break;
		default:
			//as for normal room
			this->generate_internal_room_features(room_floor_rect);
			break;
	}
}

void dungeon_generator::generate_internal_room_features(const QRect &room_floor_rect) const
{
	const int width = room_floor_rect.width();
	const int height = room_floor_rect.height();

	switch (random::get()->dice(50)) {
		case 1:
		case 2:
			if ((width * height) < 66) {
				rect::for_each_point(room_floor_rect, [&](const QPoint &tile_pos) {
					if (random::get()->dice(5) == 1) {
						this->generate_guard(tile_pos);
					}
				});
			}
			break;
		case 3:
		case 4:
			//pillars
			if (width >= 5 && height >= 5) {
				if ((width % 2) == 1) {
					for (int x = room_floor_rect.x() + 1; x <= room_floor_rect.right(); x += 2) {
						this->set_tile_terrain(QPoint(x, room_floor_rect.top() + 1), this->get_wall_terrain());
						this->set_tile_terrain(QPoint(x, room_floor_rect.bottom() - 1), this->get_wall_terrain());
					}
				}

				if ((height % 2) == 1) {
					for (int y = room_floor_rect.y() + 1; y <= room_floor_rect.bottom(); y += 2) {
						this->set_tile_terrain(QPoint(room_floor_rect.left() + 1, y), this->get_wall_terrain());
						this->set_tile_terrain(QPoint(room_floor_rect.right() - 1, y), this->get_wall_terrain());
					}
				}
			}
			break;
		case 5:
		case 6:
			//plants
			break;
		case 7:
		case 8:
			//item
			this->generate_item(room_floor_rect.topLeft() + QPoint(random::get()->generate(width), random::get()->generate(height)));
			break;
		case 9:
			//unfilled central area
			break;
		case 12:
			//room with runetraps
			rect::for_each_point(room_floor_rect, [&](const QPoint &tile_pos) {
				if (random::get()->dice(4) == 1) {
					this->generate_trap(tile_pos);
				}
			});
			break;
		case 13:
			//item and trap
			this->generate_item(room_floor_rect.topLeft() + QPoint(random::get()->generate(width), random::get()->generate(height)));
			this->generate_trap(room_floor_rect.topLeft() + QPoint(random::get()->generate(width), random::get()->generate(height)));
			break;
		case 14:
		case 15:
		case 16:
			//vertically partitioned room
			if (width > 3) {
				const int x = room_floor_rect.left() + random::get()->dice(width - 2);

				if (this->is_tile_clear(QPoint(x, room_floor_rect.top() - 1)) && this->is_tile_clear(QPoint(x, room_floor_rect.bottom() + 1))) {
					this->set_area_terrain(QRect(QPoint(x, room_floor_rect.top()), QPoint(x, room_floor_rect.bottom())), this->get_wall_terrain());

					//"secret" passage
					const int y = room_floor_rect.top() + random::get()->generate(height);
					this->set_tile_terrain(QPoint(x, y), this->get_floor_terrain());

					this->generate_guard(QPoint(x + 1, y));
					this->generate_guard(QPoint(x - 1, y));
				}
			}
			break;
		case 17:
		case 18:
		case 19:
			//horizontally partitioned room
			if (height > 3) {
				const int y = room_floor_rect.top() + random::get()->dice(height - 2);

				if (this->is_tile_clear(QPoint(room_floor_rect.left() - 1, y)) && this->is_tile_clear(QPoint(room_floor_rect.right() + 1, y))) {
					this->set_area_terrain(QRect(QPoint(room_floor_rect.left(), y), QPoint(room_floor_rect.right(), y)), this->get_wall_terrain());

					//"secret" passage
					const int x = room_floor_rect.left() + random::get()->generate(width);
					this->set_tile_terrain(QPoint(x, y), this->get_floor_terrain());

					this->generate_guard(QPoint(x, y + 1));
					this->generate_guard(QPoint(x, y - 1));
				}
			}
			break;
		case 20:
			//monster and spellbook
			this->generate_guard(room_floor_rect.topLeft() + QPoint(random::get()->generate(width), random::get()->generate(height)));
			break;
		case 21: {
			const QRect inner_rect(room_floor_rect.topLeft() + QPoint(1, 1), room_floor_rect.bottomRight() - QPoint(1, 1));
			this->set_area_terrain(inner_rect, this->get_water_terrain());
			break;
		}
		case 22:
		case 23:
			//food
			break;
		case 24:
			//traps and bones
			rect::for_each_point(room_floor_rect, [&](const QPoint &tile_pos) {
				if (random::get()->dice(4) == 1) {
					this->generate_trap(tile_pos);
				}
			});
			break;
		default:
			break;
	}
}

void dungeon_generator::extend_dungeon(const QPoint &edge_tile_pos, const QPoint &dir_offset) const
{
	static constexpr std::array dungeon_dna = { 't', 't', 't', 'k', 'r', 'r', 'r', 'o', 'o', 'z', 'h', 's'};

	//choose a new feature to add
	const char c = container::get_random(dungeon_dna);
	
	switch (c) {
		case 'o':
			this->generate_oval_room(edge_tile_pos, dir_offset);
			break;
		case 'r':
			this->generate_room(edge_tile_pos, dir_offset);
			break;
		case 't':
			this->generate_corridor_to_room(edge_tile_pos, dir_offset);
			break;
		default:
			break;
	}
}

QPoint dungeon_generator::find_edge_tile_pos(const QPoint &dir_offset) const
{
	const int map_width = this->map_rect.width();
	const int map_height = this->map_rect.height();
	const int loop_max = 10 * map_width * map_height;

	for (int i = 0; i < loop_max; ++i) {
		const int x = random::get()->generate(map_width - 2) + 1;
		const int y = random::get()->generate(map_height - 2) + 1;
		const QPoint tile_pos(x, y);

		const tile *tile = CMap::get()->Field(tile_pos, this->z);

		if (tile->get_terrain() == nullptr) {
			continue;
		}

		if (tile->get_overlay_terrain() != nullptr) {
			continue;
		}

		const QPoint adjacent_tile_pos = tile_pos + dir_offset;

		if (!this->map_rect.contains(adjacent_tile_pos)) {
			continue;
		}

		const wyrmgus::tile *adjacent_tile = CMap::get()->Field(adjacent_tile_pos, this->z);

		if (adjacent_tile->get_terrain() != nullptr) {
			continue;
		}

		return adjacent_tile_pos;
	}

	return QPoint(-1, -1);
}

bool dungeon_generator::is_tile_clear(const QPoint &tile_pos) const
{
	if (!this->map_rect.contains(tile_pos)) {
		return false;
	}

	const tile *tile = CMap::get()->Field(tile_pos, this->z);

	if (tile->get_terrain() != nullptr) {
		return false;
	}

	return true;
}

bool dungeon_generator::is_area_clear(const QRect &rect) const
{
	for (int x = rect.x(); x <= rect.right(); ++x) {
		for (int y = rect.y(); y <= rect.bottom(); ++y) {
			const QPoint tile_pos(x, y);

			if (!this->is_tile_clear(tile_pos)) {
				return false;
			}
		}
	}

	return true;
}

void dungeon_generator::set_tile_terrain(const QPoint &tile_pos, const terrain_type *terrain) const
{
	assert_throw(this->map_rect.contains(tile_pos));

	tile *tile = CMap::get()->Field(tile_pos, this->z);

	if (terrain->is_overlay()) {
		tile->SetTerrain(this->get_floor_terrain());
	} else {
		tile->RemoveOverlayTerrain();
	}

	tile->SetTerrain(terrain);
}

void dungeon_generator::set_area_terrain(const QRect &rect, const terrain_type *terrain) const
{
	rect::for_each_point(rect, [&](const QPoint &tile_pos) {
		this->set_tile_terrain(tile_pos, terrain);
	});
}

void dungeon_generator::complete_area_terrain(const QRect &rect, const terrain_type *terrain) const
{
	rect::for_each_point(rect, [&](const QPoint &tile_pos) {
		assert_throw(this->map_rect.contains(tile_pos));

		const tile *tile = CMap::get()->Field(tile_pos, this->z);

		if (tile->get_terrain() != nullptr) {
			return;
		}

		this->set_tile_terrain(tile_pos, terrain);
	});
}

void dungeon_generator::generate_guard(const QPoint &tile_pos) const
{
	const unit_type *unit_type = this->get_random_unit_type();

	if (!UnitTypeCanBeAt(*unit_type, tile_pos, this->z)) {
		return;
	}

	CreateUnit(tile_pos, *unit_type, CPlayer::get_neutral_player(), this->z);
}

void dungeon_generator::generate_creep() const
{
	const unit_type *unit_type = this->get_random_unit_type();
	const QPoint tile_pos = CMap::get()->generate_unit_location(unit_type, CPlayer::get_neutral_player(), this->map_rect.topLeft(), this->map_rect.bottomRight(), this->z, nullptr, true);

	if (!this->map_rect.contains(tile_pos)) {
		assert_log(false);
		return;
	}

	CreateUnit(tile_pos, *unit_type, CPlayer::get_neutral_player(), this->z);

	if (random::get()->dice(10) == 1) {
		this->generate_item(tile_pos);
	}
}

void dungeon_generator::generate_item(const QPoint &tile_pos) const
{
	const unit_type *unit_type = this->get_random_item_unit_type();

	assert_throw(unit_type->BoolFlag[ITEM_INDEX].value);

	CUnit *item_unit = CreateUnit(tile_pos, *unit_type, CPlayer::get_neutral_player(), this->z);

	item_unit->generate_special_properties(nullptr, CPlayer::get_neutral_player(), true, false, false);
}

void dungeon_generator::generate_trap(const QPoint &tile_pos) const
{
	const unit_type *unit_type = this->get_random_trap_unit_type();

	if (!UnitTypeCanBeAt(*unit_type, tile_pos, this->z)) {
		return;
	}

	CreateUnit(tile_pos, *unit_type, CPlayer::get_neutral_player(), this->z);
}

void dungeon_generator::generate_hero(CPlayer *player) const
{
	character *hero = this->get_random_hero();
	const unit_type *unit_type = hero->get_unit_type();
	const QPoint unit_pos = CMap::get()->generate_unit_location(unit_type, player, this->map_rect.topLeft(), this->map_rect.bottomRight(), this->z, nullptr);

	assert_throw(this->map_rect.contains(unit_pos));

	CUnit *hero_unit = CreateUnit(unit_pos, *unit_type, player, this->z);
	hero_unit->set_character(hero);

	player->SetStartView(unit_pos, this->z);
}

const terrain_type *dungeon_generator::get_floor_terrain() const
{
	return this->settings->get_floor_terrain();
}

const terrain_type *dungeon_generator::get_wall_terrain() const
{
	return this->settings->get_wall_terrain();
}

const terrain_type *dungeon_generator::get_deep_wall_terrain() const
{
	return this->settings->get_deep_wall_terrain();
}

const terrain_type *dungeon_generator::get_water_terrain() const
{
	return this->settings->get_water_terrain();
}

const unit_type *dungeon_generator::get_random_unit_type() const
{
	return container::get_random(this->settings->get_unit_types());
}

const unit_type *dungeon_generator::get_random_item_unit_type() const
{
	return container::get_random(this->settings->get_item_unit_types());
}

const unit_type *dungeon_generator::get_random_trap_unit_type() const
{
	return container::get_random(this->settings->get_trap_unit_types());
}

character *dungeon_generator::get_random_hero() const
{
	//get a random hero out of the lowest-level ones
	const std::vector<character *> &heroes = this->settings->get_heroes();

	std::vector<character *> potential_heroes;
	int lowest_level = std::numeric_limits<int>::max();

	for (character *hero : heroes) {
		if (!hero->CanAppear()) {
			continue;
		}

		if (hero->get_level() <= lowest_level) {
			if (hero->get_level() < lowest_level) {
				lowest_level = hero->get_level();
				potential_heroes.clear();
			}

			potential_heroes.push_back(hero);
		}
	}

	assert_throw(!potential_heroes.empty());

	return container::get_random(potential_heroes);
}

}
