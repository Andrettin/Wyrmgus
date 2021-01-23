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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#pragma once

#include "color.h"
#include "vec2i.h"

class CUnit;
class CViewport;

typedef unsigned int GLuint;

namespace wyrmgus {

enum class minimap_mode;

class minimap final
{
public:
	minimap();

private:
	void UpdateTerrain(int z);
	void update_territories(const int z);

public:
	void UpdateXY(const Vec2i &pos, const int z);
	void UpdateSeenXY(const Vec2i &) {}
	void update_territory_xy(const QPoint &pos, const int z);
	void update_territory_pixel(const int mx, const int my, const int z);
	void Update();
	void Create();
	void create_textures(const int z);
	void create_texture(GLuint &texture, const unsigned char *texture_data, const int z);
#if defined(USE_OPENGL) || defined(USE_GLES)
	void FreeOpenGL();
	void Reload();
#endif
	void Destroy();
	void Draw() const;
	void draw_texture(const GLuint &texture, const unsigned char *texture_data, const int z) const;
	void DrawViewportArea(const CViewport &viewport) const;
	void DrawUnitOn(const CUnit *unit, const bool red_phase);
	void AddEvent(const Vec2i &pos, int z, IntColor color);
	void draw_events() const;

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
	std::vector<GLuint> terrain_textures;
	std::vector<GLuint> overlay_textures;

	// Minimap surface with terrain only (for OpenGL)
	std::vector<std::vector<unsigned char>> terrain_texture_data;

	std::map<minimap_mode, std::vector<std::vector<unsigned char>>> mode_overlay_texture_data;

	//texture data for the overlay with units and unexplored terrain
	std::vector<std::vector<unsigned char>> overlay_texture_data;
};

}
