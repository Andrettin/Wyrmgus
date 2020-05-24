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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#ifdef USE_OPENGL
#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES 1
#endif
#include "SDL_opengl.h"
#endif

class CUnit;
class CViewport;

namespace stratagus {

enum class minimap_mode;

class minimap
{
public:
	minimap();

private:
	void UpdateTerrain(int z);
	void update_territories(const int z);

public:
	void UpdateXY(const Vec2i &pos, int z);
	void UpdateSeenXY(const Vec2i &) {}
	void update_territory_xy(const QPoint &pos, const int z);
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
	void DrawUnitOn(CUnit &unit, int red_phase);
	void AddEvent(const Vec2i &pos, int z, IntColor color);

	Vec2i ScreenToTilePos(const PixelPos &screenPos) const;
	PixelPos TilePosToScreenPos(const Vec2i &tilePos) const;

	bool Contains(const PixelPos &screenPos) const;

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

	void toggle_mode();
	bool is_terrain_visible() const;
	bool are_units_visible() const;
	bool is_fog_of_war_visible() const;

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
	std::vector<GLuint> terrain_textures;
	std::vector<GLuint> textures;

	// Minimap surface with terrain only (for OpenGL)
	std::vector<unsigned char *> terrain_surface_gl;

	// Minimap surface with units (for OpenGL)
	std::vector<unsigned char *> surface_gl;

	//minimap surface with territories
	std::vector<unsigned char *> territories_surface_gl;

	//minimap surface with territories, including non-land tiles
	std::vector<unsigned char *> territories_with_non_land_surface_gl;
};

}
