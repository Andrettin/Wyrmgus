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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "color.h"
#include "vec2i.h"

class CUnit;
class CViewport;

typedef unsigned int GLuint;

namespace wyrmgus {

class unit_type;
enum class minimap_mode;

class minimap final
{
public:
	static inline const QColor unexplored_color = QColor(0, 0, 0);
	static inline const QColor fog_of_war_color = QColor(0, 0, 0, 128);

	minimap();

private:
	void UpdateTerrain(int z);
	void update_territories(const int z);
public:
	void update_exploration(const int z);

	void UpdateXY(const Vec2i &pos, const int z);
	void UpdateSeenXY(const Vec2i &) {}
	void update_territory_xy(const QPoint &pos, const int z);
	void update_territory_pixel(const int mx, const int my, const int z);
	void update_exploration_index(const int index, const int z);
	void update_exploration_xy(const QPoint &pos, const int z);
	void update_exploration_pixel(const int mx, const int my, const int z, const unsigned short visibility_state);
	void Update();
	void Create();
	void Destroy();
	void Draw(std::vector<std::function<void(renderer *)>> &render_commands) const;
	void draw_image(const QImage &image, const int z, std::vector<std::function<void(renderer *)>> &render_commands) const;
	void DrawViewportArea(const CViewport &viewport, std::vector<std::function<void(renderer *)>> &render_commands) const;

private:
	const unit_type *get_unit_minimap_type(const CUnit *unit) const;
	uint32_t get_unit_minimap_color(const CUnit *unit, const unit_type *type, const bool red_phase) const;
	uint32_t get_terrain_unit_minimap_color(const CUnit *unit, const unit_type *type, const bool red_phase) const;

	void draw_unit_on(const CUnit *unit, const bool red_phase);
	void draw_terrain_unit_on(const CUnit *unit, const bool red_phase);

public:
	void AddEvent(const Vec2i &pos, int z, IntColor color);
	void draw_events(std::vector<std::function<void(renderer *)>> &render_commands) const;

	QPoint texture_to_tile_pos(const QPoint &texture_pos) const;
	QPoint texture_to_screen_pos(const QPoint &texture_pos) const;
	QPoint screen_to_tile_pos(const QPoint &screen_pos) const;
	QPoint tile_to_texture_pos(const QPoint &tile_pos) const;
	QPoint tile_to_screen_pos(const QPoint &tile_pos) const;

	int get_width() const
	{
		return this->W;
	}

	int get_height() const
	{
		return this->H;
	}

	bool Contains(const PixelPos &screenPos) const;

	int get_texture_width(const size_t z) const;
	int get_texture_height(const size_t z) const;

	minimap_mode get_mode() const
	{
		return this->mode;
	}

	void set_mode(const minimap_mode mode)
	{
		if (mode == this->get_mode()) {
			return;
		}

		this->mode = mode;
		this->UpdateCache = true;
	}

	bool is_zoomed() const
	{
		return this->zoomed;
	}

	void set_zoomed(const bool zoomed)
	{
		this->zoomed = zoomed;
	}

	bool can_zoom(const int z) const
	{
		return this->get_texture_width(z) != this->get_width() || this->get_texture_height(z) != this->get_height();
	}

	minimap_mode get_next_mode(const minimap_mode mode) const;
	bool is_mode_valid(const minimap_mode mode) const;
	void toggle_mode();
	bool is_terrain_visible() const;
	bool are_units_visible() const;
	bool is_fog_of_war_visible() const;

	QRect get_texture_draw_rect(const int z) const;

public:
	int X = 0;
	int Y = 0;
	int W = 0;
	int H = 0;
	std::vector<int> XOffset;
	std::vector<int> YOffset;
	bool ShowSelected = false;
	bool Transparent = false;
	bool UpdateCache = false;

private:
	minimap_mode mode;
	bool zoomed = false; //whether the minimap texture is being shown at full resolution
	std::vector<QImage> terrain_images;

	//image for unexplored tiles
	std::vector<QImage> unexplored_images;

	//image for tiles under fog of war
	std::vector<QImage> fog_of_war_images;

	//image for the overlay with units
	std::vector<QImage> overlay_images;

	std::map<minimap_mode, std::vector<QImage>> mode_overlay_images;
};

}
