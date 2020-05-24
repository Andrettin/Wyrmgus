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
//      (c) Copyright 1998-2020 by Lutz Sammer and Jimmy Salmon, Pali Rohár and Andrettin
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

#include "stratagus.h"

#include "map/minimap.h"

#include "database/defines.h"
#include "editor.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap_mode.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "plane.h"
#include "player.h"
#include "player_color.h"
#include "province.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "video.h"
#include "world.h"

static constexpr int MINIMAP_FAC = 16 * 3;  /// integer scale factor

/// unit attacked are shown red for at least this amount of cycles
static constexpr int ATTACK_RED_DURATION = 1 * CYCLES_PER_SECOND;
/// unit attacked are shown blinking for this amount of cycles
static constexpr int ATTACK_BLINK_DURATION = 7 * CYCLES_PER_SECOND;

static constexpr int SCALE_PRECISION = 100;

#if defined(USE_OPENGL) || defined(USE_GLES)
//Wyrmgus start
//static int MinimapTextureWidth;
//static int MinimapTextureHeight;
static std::vector<int> MinimapTextureWidth;
static std::vector<int> MinimapTextureHeight;
//Wyrmgus end
#endif

//Wyrmgus start
//static int *Minimap2MapX;                  /// fast conversion table
//static int *Minimap2MapY;                  /// fast conversion table
//static int Map2MinimapX[MaxMapWidth];      /// fast conversion table
//static int Map2MinimapY[MaxMapHeight];     /// fast conversion table
static std::vector<int *> Minimap2MapX;                  /// fast conversion table
static std::vector<int *> Minimap2MapY;                  /// fast conversion table
static std::vector<int *> Map2MinimapX;      /// fast conversion table
static std::vector<int *> Map2MinimapY;     /// fast conversion table
//Wyrmgus end

// MinimapScale:
// 32x32 64x64 96x96 128x128 256x256 512x512 ...
// *4 *2 *4/3   *1 *1/2 *1/4
static std::vector<int> MinimapScaleX;                  /// Minimap scale to fit into window
static std::vector<int> MinimapScaleY;                  /// Minimap scale to fit into window

static constexpr int MAX_MINIMAP_EVENTS = 8;

struct MinimapEvent {
	PixelPos pos;
	int Size;
	Uint32 Color;
} MinimapEvents[MAX_MINIMAP_EVENTS];
int NumMinimapEvents;

namespace stratagus {

minimap::minimap() : mode(minimap_mode::terrain)
{
}

void minimap::create_textures(const int z)
{
	this->create_texture(this->terrain_textures[z], this->terrain_surface_gl[z], z);
	this->create_texture(this->textures[z], this->surface_gl[z], z);
}

void minimap::create_texture(GLuint &texture, const unsigned char *texture_data, const int z)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MinimapTextureWidth[z], MinimapTextureHeight[z], 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
}

/**
**  Create a mini-map from the tiles of the map.
**
**  @todo Scaling and scrolling the minmap is currently not supported.
*/
void minimap::Create()
{
	this->terrain_textures.resize(CMap::Map.MapLayers.size());
	this->textures.resize(CMap::Map.MapLayers.size());
	MinimapTextureWidth.resize(CMap::Map.MapLayers.size());
	MinimapTextureHeight.resize(CMap::Map.MapLayers.size());

	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		// Scale to biggest value.
		int n = std::max(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]);
		n = std::max(n, 32);

		MinimapScaleX.push_back((W * MINIMAP_FAC + n - 1) / n);
		MinimapScaleY.push_back((H * MINIMAP_FAC + n - 1) / n);

		XOffset.push_back((W - (CMap::Map.Info.MapWidths[z] * MinimapScaleX[z]) / MINIMAP_FAC + 1) / 2);
		YOffset.push_back((H - (CMap::Map.Info.MapHeights[z] * MinimapScaleY[z]) / MINIMAP_FAC + 1) / 2);

		DebugPrint("MinimapScale %d %d (%d %d), X off %d, Y off %d\n" _C_
				   MinimapScaleX[z] / MINIMAP_FAC _C_ MinimapScaleY[z] / MINIMAP_FAC _C_
				   MinimapScaleX[z] _C_ MinimapScaleY[z] _C_
				   XOffset[z] _C_ YOffset[z]);

		//
		// Calculate minimap fast lookup tables.
		//
		Minimap2MapX.push_back(new int[W * H]);
		memset(Minimap2MapX[z], 0, W * H * sizeof(int));
		Minimap2MapY.push_back(new int[W * H]);
		memset(Minimap2MapY[z], 0, W * H * sizeof(int));
		for (int i = XOffset[z]; i < W - XOffset[z]; ++i) {
			Minimap2MapX[z][i] = ((i - XOffset[z]) * MINIMAP_FAC) / MinimapScaleX[z];
		}
		for (int i = YOffset[z]; i < H - YOffset[z]; ++i) {
			Minimap2MapY[z][i] = (((i - YOffset[z]) * MINIMAP_FAC) / MinimapScaleY[z]) * CMap::Map.Info.MapWidths[z];
		}
		Map2MinimapX.push_back(new int[CMap::Map.Info.MapWidths[z]]);
		memset(Map2MinimapX[z], 0, CMap::Map.Info.MapWidths[z] * sizeof(int));
		Map2MinimapY.push_back(new int[CMap::Map.Info.MapHeights[z]]);
		memset(Map2MinimapY[z], 0, CMap::Map.Info.MapHeights[z] * sizeof(int));
		for (int i = 0; i < CMap::Map.Info.MapWidths[z]; ++i) {
			Map2MinimapX[z][i] = (i * MinimapScaleX[z]) / MINIMAP_FAC;
		}
		for (int i = 0; i < CMap::Map.Info.MapHeights[z]; ++i) {
			Map2MinimapY[z][i] = (i * MinimapScaleY[z]) / MINIMAP_FAC;
		}

		// Palette updated from UpdateMinimapTerrain()
		for (MinimapTextureWidth[z] = 1; MinimapTextureWidth[z] < W; MinimapTextureWidth[z] <<= 1) {
		}
		for (MinimapTextureHeight[z] = 1; MinimapTextureHeight[z] < H; MinimapTextureHeight[z] <<= 1) {
		}

		this->terrain_surface_gl.push_back(new unsigned char[MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4]);
		memset(this->terrain_surface_gl[z], 0, MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);

		this->territories_surface_gl.push_back(new unsigned char[MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4]);
		memset(this->territories_surface_gl[z], 0, MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);

		this->territories_with_non_land_surface_gl.push_back(new unsigned char[MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4]);
		memset(this->territories_with_non_land_surface_gl[z], 0, MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);

		this->settlements_surface_gl.push_back(new unsigned char[MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4]);
		memset(this->settlements_surface_gl[z], 0, MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);

		this->surface_gl.push_back(new unsigned char[MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4]);
		memset(this->surface_gl[z], 0, MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);

		this->create_textures(z);

		this->UpdateTerrain(z);
		this->update_territories(z);
	}

	NumMinimapEvents = 0;
}

/**
**  Free OpenGL minimap
*/
void minimap::FreeOpenGL()
{
	for (size_t z = 0; z < this->terrain_textures.size(); ++z) {
		glDeleteTextures(1, &this->terrain_textures[z]);
	}
	this->terrain_textures.clear();

	for (size_t z = 0; z < this->textures.size(); ++z) {
		glDeleteTextures(1, &this->textures[z]);
	}
	this->textures.clear();
}

/**
**  Reload OpenGL minimap
*/
void minimap::Reload()
{
	//Wyrmgus start
//	CreateMinimapTexture();
	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		this->create_textures(z);
	}
	//Wyrmgus end
}

/**
**  Calculate the tile graphic pixel
*/
static inline QColor GetTileGraphicPixel(int xofs, int yofs, int mx, int my, int scalex, int scaley, int z, stratagus::terrain_type *terrain, const stratagus::season *season)
{
	const int scale_factor = stratagus::defines::get()->get_scale_factor();
	int x = (xofs + (7 * scale_factor) + ((mx * SCALE_PRECISION) % scalex) / SCALE_PRECISION * 8);
	int y = (yofs + (6 * scale_factor) + ((my * SCALE_PRECISION) % scaley) / SCALE_PRECISION * 8);
	return terrain->get_graphics(season)->get_image().pixelColor(x / scale_factor, y / scale_factor);
}

/**
**  Update a mini-map from the tiles of the map.
*/
void minimap::UpdateTerrain(int z)
{
	int scalex = MinimapScaleX[z] * SCALE_PRECISION / MINIMAP_FAC;
	if (!scalex) {
		scalex = 1;
	}
	int scaley = MinimapScaleY[z] * SCALE_PRECISION / MINIMAP_FAC;
	if (!scaley) {
		scaley = 1;
	}
	
	const stratagus::season *season = CMap::Map.MapLayers[z]->GetSeason();

	//Wyrmgus start
//	const int tilepitch = Map.TileGraphic->Surface->w / stratagus::defines::get()->get_scaled_tile_width();
	//Wyrmgus end

	const CMapLayer *map_layer = CMap::Map.MapLayers[z];
	
	//
	//  Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
	//
	for (int my = YOffset[z]; my < H - YOffset[z]; ++my) {
		for (int mx = XOffset[z]; mx < W - XOffset[z]; ++mx) {
			//Wyrmgus start
//			const int tile = Map.Fields[Minimap2MapX[mx] + Minimap2MapY[my]].getGraphicTile();
			const CMapField &mf = *map_layer->Field(Minimap2MapX[z][mx] + Minimap2MapY[z][my]);
			stratagus::terrain_type *terrain = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlayTerrain : mf.playerInfo.SeenTerrain;
			int tile = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlaySolidTile : mf.playerInfo.SeenSolidTile;
			if (!terrain) {
				terrain = mf.OverlayTerrain ? mf.OverlayTerrain : mf.Terrain;
				tile = mf.OverlayTerrain ? mf.OverlaySolidTile : mf.SolidTile;
			}
			
			stratagus::terrain_type *base_terrain = mf.playerInfo.SeenTerrain;
			int base_tile = mf.playerInfo.SeenSolidTile;
			if (!base_terrain) {
				base_terrain = mf.Terrain;
				base_tile = mf.SolidTile;
			}
			//Wyrmgus end
			
			//Wyrmgus start
			int tilepitch = terrain->get_graphics(season)->get_width() / stratagus::defines::get()->get_scaled_tile_width();
			
			int base_tilepitch = base_terrain->get_graphics(season)->get_width() / stratagus::defines::get()->get_scaled_tile_width();
			//Wyrmgus end
	
			const int xofs = stratagus::defines::get()->get_scaled_tile_width() * (tile % tilepitch);
			const int yofs = stratagus::defines::get()->get_scaled_tile_height() * (tile / tilepitch);
			
			//Wyrmgus start
			const int base_xofs = stratagus::defines::get()->get_scaled_tile_width() * (base_tile % base_tilepitch);
			const int base_yofs = stratagus::defines::get()->get_scaled_tile_height() * (base_tile / base_tilepitch);
			//Wyrmgus end

			uint32_t c;

			QColor color = GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, z, terrain, season);

			if (color.alpha() == 0) { //transparent pixel, use base instead
				color = GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, z, base_terrain, season);
			}

			c = Video.MapRGB(0, color.red(), color.green(), color.blue());

			*(uint32_t *)&(this->terrain_surface_gl[z][(mx + my * MinimapTextureWidth[z]) * 4]) = c;
		}
	}
}

void minimap::update_territories(const int z)
{
	for (int my = YOffset[z]; my < H - YOffset[z]; ++my) {
		for (int mx = XOffset[z]; mx < W - XOffset[z]; ++mx) {
			this->update_territory_pixel(mx, my, z);
		}
	}
}

/**
**	@brief	Update a single minimap tile after a change
**
**	@param	pos	The map position to update in the minimap
**	@param	z	The map layer of the tile to update
*/
void minimap::UpdateXY(const Vec2i &pos, int z)
{
	if (z >= (int) this->terrain_surface_gl.size() || !this->terrain_surface_gl[z]) {
		return;
	}

	int scalex = MinimapScaleX[z] * SCALE_PRECISION / MINIMAP_FAC;
	if (scalex == 0) {
		scalex = 1;
	}
	int scaley = MinimapScaleY[z] * SCALE_PRECISION / MINIMAP_FAC;
	if (scaley == 0) {
		scaley = 1;
	}

	const stratagus::season *season = CMap::Map.MapLayers[z]->GetSeason();

	//
	//  Pixel 7,6 7,14, 15,6 15,14 are taken for the minimap picture.
	//

	const int ty = pos.y * CMap::Map.Info.MapWidths[z];
	const int tx = pos.x;

	for (int my = YOffset[z]; my < H - YOffset[z]; ++my) {
		const int y = Minimap2MapY[z][my];
		if (y < ty) {
			continue;
		}
		if (y > ty) {
			break;
		}

		for (int mx = XOffset[z]; mx < W - XOffset[z]; ++mx) {
			const int x = Minimap2MapX[z][mx];

			if (x < tx) {
				continue;
			}
			if (x > tx) {
				break;
			}

			//Wyrmgus start
			/*
			int tile = Map.Fields[x + y].playerInfo.SeenTile;
			if (!tile) {
				tile = Map.Fields[x + y].getGraphicTile();
			}
			*/
			const CMapField &mf = *CMap::Map.MapLayers[z]->Field(x + y);
			stratagus::terrain_type *terrain = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlayTerrain : mf.playerInfo.SeenTerrain;
			int tile = mf.playerInfo.SeenOverlayTerrain ? mf.playerInfo.SeenOverlaySolidTile : mf.playerInfo.SeenSolidTile;
			if (!terrain) {
				terrain = mf.OverlayTerrain ? mf.OverlayTerrain : mf.Terrain;
				tile = mf.OverlayTerrain ? mf.OverlaySolidTile : mf.SolidTile;
			}
			
			stratagus::terrain_type *base_terrain = mf.playerInfo.SeenTerrain;
			int base_tile = mf.playerInfo.SeenSolidTile;
			if (!base_terrain) {
				base_terrain = mf.Terrain;
				base_tile = mf.SolidTile;
			}
			//Wyrmgus end

			//Wyrmgus start
			int tilepitch = terrain->get_graphics(season)->get_width() / stratagus::defines::get()->get_scaled_tile_width();
			
			int base_tilepitch = base_terrain->get_graphics(season)->get_width() / stratagus::defines::get()->get_scaled_tile_width();
			//Wyrmgus end
	
			const int xofs = stratagus::defines::get()->get_scaled_tile_width() * (tile % tilepitch);
			const int yofs = stratagus::defines::get()->get_scaled_tile_height() * (tile / tilepitch);
			
			//Wyrmgus start
			const int base_xofs = stratagus::defines::get()->get_scaled_tile_width() * (base_tile % base_tilepitch);
			const int base_yofs = stratagus::defines::get()->get_scaled_tile_height() * (base_tile / base_tilepitch);
			//Wyrmgus end

			uint32_t c;

			QColor color = GetTileGraphicPixel(xofs, yofs, mx, my, scalex, scaley, z, terrain, season);

			if (color.alpha() == 0) { //transparent pixel, use base instead
				color = GetTileGraphicPixel(base_xofs, base_yofs, mx, my, scalex, scaley, z, base_terrain, season);
			}

			c = Video.MapRGB(0, color.red(), color.green(), color.blue());
			*(uint32_t *) &(this->terrain_surface_gl[z][(mx + my * MinimapTextureWidth[z]) * 4]) = c;
		}
	}
}

void minimap::update_territory_xy(const QPoint &pos, const int z)
{
	if (z >= (int) this->territories_surface_gl.size() || !this->territories_surface_gl[z]) {
		return;
	}

	const int ty = pos.y() * CMap::Map.Info.MapWidths[z];
	const int tx = pos.x();
	const int non_land_territory_alpha = defines::get()->get_minimap_non_land_territory_alpha();

	for (int my = YOffset[z]; my < H - YOffset[z]; ++my) {
		const int y = Minimap2MapY[z][my];
		if (y < ty) {
			continue;
		}
		if (y > ty) {
			break;
		}

		for (int mx = XOffset[z]; mx < W - XOffset[z]; ++mx) {
			const int x = Minimap2MapX[z][mx];

			if (x < tx) {
				continue;
			}
			if (x > tx) {
				break;
			}

			this->update_territory_pixel(mx, my, z);
		}
	}
}

void minimap::update_territory_pixel(const int mx, const int my, const int z)
{
	const CMapLayer *map_layer = CMap::Map.MapLayers[z];
	const int non_land_territory_alpha = defines::get()->get_minimap_non_land_territory_alpha();
	const int minimap_color_index = defines::get()->get_minimap_color_index();

	const int pixel_index = (mx + my * MinimapTextureWidth[z]) * 4;
	QColor color(Qt::transparent);
	QColor with_non_land_color = color;

	const CMapField &mf = *map_layer->Field(Minimap2MapX[z][mx] + Minimap2MapY[z][my]);
	if (mf.get_settlement() != nullptr) {
		const bool is_tile_water = mf.Flags & (MapFieldWaterAllowed | MapFieldCoastAllowed);
		const bool is_tile_space = mf.Flags & MapFieldSpace;

		if (!is_tile_water && !is_tile_space) {
			if (mf.get_owner() != nullptr) {
				color = mf.get_owner()->get_minimap_color();
			} else {
				color = CPlayer::Players[PlayerNumNeutral]->get_minimap_color();
			}
			with_non_land_color = color;
		} else {
			if (mf.get_owner() != nullptr) {
				with_non_land_color = mf.get_owner()->get_minimap_color();
			} else {
				with_non_land_color = CPlayer::Players[PlayerNumNeutral]->get_minimap_color();
			}
			with_non_land_color.setAlpha(non_land_territory_alpha);
		}

		const CMapField *settlement_center_tile = mf.get_settlement()->get_site_unit()->get_center_tile();
		const bool is_settlement_water = settlement_center_tile->Flags & (MapFieldWaterAllowed | MapFieldCoastAllowed);
		const bool is_settlement_space = settlement_center_tile->Flags & MapFieldSpace;
		if (is_tile_water == is_settlement_water && is_tile_space == is_settlement_space) {
			const QColor settlement_color = mf.get_settlement()->get_minimap_color();
			*(uint32_t *) &(this->settlements_surface_gl[z][pixel_index]) = Video.MapRGBA(settlement_color);
		}
	}

	const uint32_t c = Video.MapRGBA(color);
	*(uint32_t *) &(this->territories_surface_gl[z][pixel_index]) = c;

	const uint32_t with_non_land_c = Video.MapRGBA(with_non_land_color);
	*(uint32_t *) &(this->territories_with_non_land_surface_gl[z][pixel_index]) = with_non_land_c;
}

/**
**  Draw a unit on the minimap.
*/
void minimap::DrawUnitOn(CUnit &unit, int red_phase)
{
	const int z = UI.CurrentMapLayer->ID;

	const stratagus::unit_type *type;

	if (Editor.Running || ReplayRevealMap || unit.IsVisible(*CPlayer::GetThisPlayer())) {
		type = unit.Type;
	} else {
		type = unit.Seen.Type;
		// This will happen for radar if the unit has not been seen and we
		// have it on radar.
		if (!type) {
			type = unit.Type;
		}
	}

	//don't draw decorations or diminutive fauna units on the minimap
	if (type->BoolFlag[DECORATION_INDEX].value || (type->BoolFlag[DIMINUTIVE_INDEX].value && type->BoolFlag[FAUNA_INDEX].value)) {
		return;
	}

	uint32_t color;
	if (unit.GetDisplayPlayer() == PlayerNumNeutral) {
		color = Video.MapRGB(TheScreen->format, type->NeutralMinimapColorRGB);
	} else if (unit.Player == CPlayer::GetThisPlayer() && !Editor.Running) {
		if (unit.Attacked && unit.Attacked + ATTACK_BLINK_DURATION > GameCycle &&
			(red_phase || unit.Attacked + ATTACK_RED_DURATION > GameCycle)) {
			color = ColorRed;
		} else if (UI.Minimap.ShowSelected && unit.Selected) {
			color = ColorWhite;
		} else {
			color = ColorGreen;
		}
	} else {
		color = Video.MapRGB(TheScreen->format, unit.Player->get_minimap_color());
	}

	int mx = 1 + UI.Minimap.XOffset[z] + Map2MinimapX[z][unit.tilePos.x];
	int my = 1 + UI.Minimap.YOffset[z] + Map2MinimapY[z][unit.tilePos.y];
	int w = Map2MinimapX[z][type->get_tile_width()];

	if (mx + w >= UI.Minimap.W) { // clip right side
		w = UI.Minimap.W - mx;
	}

	int h0 = Map2MinimapY[z][type->get_tile_height()];
	if (my + h0 >= UI.Minimap.H) { // clip bottom side
		h0 = UI.Minimap.H - my;
	}

	while (w-- >= 0) {
		int h = h0;
		while (h-- >= 0) {
			*(uint32_t *) &(this->surface_gl[z][((mx + w) + (my + h) * MinimapTextureWidth[z]) * 4]) = color;
		}
	}
}

/**
**  Update the minimap with the current game information
*/
void minimap::Update()
{
	static int red_phase;

	int red_phase_changed = red_phase != (int)((FrameCounter / FRAMES_PER_SECOND) & 1);
	if (red_phase_changed) {
		red_phase = !red_phase;
	}

	const int z = UI.CurrentMapLayer->ID;

	// Clear Minimap background if not transparent
	if (!Transparent) {
		memset(this->surface_gl[z], 0, MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);
	}

	switch (this->get_mode()) {
		case minimap_mode::territories:
			memcpy(this->surface_gl[z], this->territories_surface_gl[z], MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);
			break;
		case minimap_mode::territories_with_non_land:
			memcpy(this->surface_gl[z], this->territories_with_non_land_surface_gl[z], MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);
			break;
		case minimap_mode::settlements:
			memcpy(this->surface_gl[z], this->settlements_surface_gl[z], MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4);
			break;
		default:
			break;
	}

	const uint32_t unexplored_color = Video.MapRGB(nullptr, 0, 0, 0);
	const uint32_t explored_color = Video.MapRGBA(nullptr, 0, 0, 0, 128); //explored but not visible
	for (int my = 0; my < H; ++my) {
		for (int mx = 0; mx < W; ++mx) {
			if (mx < XOffset[z] || mx >= W - XOffset[z] || my < YOffset[z] || my >= H - YOffset[z]) {
				*(uint32_t *) &(this->surface_gl[z][(mx + my * MinimapTextureWidth[z]) * 4]) = Video.MapRGB(nullptr, 0, 0, 0);
				continue;
			}
			
			int visiontype; // 0 unexplored, 1 explored, >1 visible.

			if (ReplayRevealMap) {
				visiontype = 2;
			} else {
				const Vec2i tilePos(Minimap2MapX[z][mx], Minimap2MapY[z][my] / UI.CurrentMapLayer->get_width());
				visiontype = CMap::Map.Field(tilePos, z)->playerInfo.TeamVisibilityState(*CPlayer::GetThisPlayer());
			}

			switch (visiontype) {
				case 0:
					*(uint32_t *) &(this->surface_gl[z][(mx + my * MinimapTextureWidth[z]) * 4]) = unexplored_color;
					break;
				case 1:
					if (this->is_fog_of_war_visible()) {
						uint32_t *c = (uint32_t *) &(this->surface_gl[z][(mx + my * MinimapTextureWidth[z]) * 4]);
						if (*c == 0) {
							*c = explored_color;
						}
					}
					break;
				default:
					break;
			}
		}
	}

	if (this->are_units_visible()) {
		//draw units on the map
		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			CUnit &unit = **it;
			if (unit.IsVisibleOnMinimap()) {
				this->DrawUnitOn(unit, red_phase);
			}
		}
	}
}

/**
**  Draw the minimap events
*/
static void DrawEvents()
{
	const unsigned char alpha = 192;

	for (int i = 0; i < NumMinimapEvents; ++i) {
		Video.DrawTransCircleClip(MinimapEvents[i].Color,
								  MinimapEvents[i].pos.x, MinimapEvents[i].pos.y,
								  MinimapEvents[i].Size, alpha);
		MinimapEvents[i].Size -= 1;
		if (MinimapEvents[i].Size < 2) {
			MinimapEvents[i] = MinimapEvents[--NumMinimapEvents];
			--i;
		}
	}
}

/**
**  Draw the minimap on the screen
*/
void minimap::Draw() const
{
	const int z = UI.CurrentMapLayer->ID;

	if (this->is_terrain_visible()) {
		this->draw_texture(this->terrain_textures[z], this->terrain_surface_gl[z], z);
	}

	this->draw_texture(this->textures[z], this->surface_gl[z], z);
	DrawEvents();
}

void minimap::draw_texture(const GLuint &texture, const unsigned char *texture_data, const int z) const
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MinimapTextureWidth[z], MinimapTextureHeight[z], GL_RGBA, GL_UNSIGNED_BYTE, texture_data);

#ifdef USE_GLES
	float texCoord[] = {
		0.0f, 0.0f,
		(float) W / MinimapTextureWidth[z], 0.0f,
		0.0f, (float) H / MinimapTextureHeight[z],
		(float) W / MinimapTextureWidth[z], (float) H / MinimapTextureHeight[z]
	};

	float vertex[] = {
		2.0f / (GLfloat) Video.Width * X - 1.0f, -2.0f / (GLfloat) Video.Height * Y + 1.0f,
		2.0f / (GLfloat) Video.Width * (X + W) - 1.0f, -2.0f / (GLfloat) Video.Height * Y + 1.0f,
		2.0f / (GLfloat) Video.Width * X - 1.0f, -2.0f / (GLfloat) Video.Height * (Y + H) + 1.0f,
		2.0f / (GLfloat) Video.Width * (X + W) - 1.0f, -2.0f / (GLfloat) Video.Height * (Y + H) + 1.0f
	};

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
	glVertexPointer(2, GL_FLOAT, 0, vertex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
#endif
#ifdef USE_OPENGL
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(X, Y);
	glTexCoord2f(0.0f, (float) H / MinimapTextureHeight[z]);
	glVertex2i(X, Y + H);
	glTexCoord2f((float) W / MinimapTextureWidth[z], (float) H / MinimapTextureHeight[z]);
	glVertex2i(X + W, Y + H);
	glTexCoord2f((float) W / MinimapTextureWidth[z], 0.0f);
	glVertex2i(X + W, Y);
	glEnd();
#endif
}

/**
**  Convert screen position to tile map coordinate.
**
**  @param screenPos  Screen pixel coordinate.
**
**  @return   Tile coordinate.
*/
Vec2i minimap::ScreenToTilePos(const PixelPos &screenPos) const
{
	//Wyrmgus start
//	Vec2i tilePos((((screenPos.x - X - XOffset) * MINIMAP_FAC) / MinimapScaleX),
//				  (((screenPos.y - Y - YOffset) * MINIMAP_FAC) / MinimapScaleY));
	Vec2i tilePos((((screenPos.x - X - XOffset[UI.CurrentMapLayer->ID]) * MINIMAP_FAC) / MinimapScaleX[UI.CurrentMapLayer->ID]),
				  (((screenPos.y - Y - YOffset[UI.CurrentMapLayer->ID]) * MINIMAP_FAC) / MinimapScaleY[UI.CurrentMapLayer->ID]));
	//Wyrmgus end

	CMap::Map.Clamp(tilePos, UI.CurrentMapLayer->ID);

	return tilePos;
}

/**
**  Convert tile map coordinate to screen position.
**
**  @param tilePos  Tile coordinate.
**
**  @return   Screen pixel coordinate.
*/
PixelPos minimap::TilePosToScreenPos(const Vec2i &tilePos) const
{
	//Wyrmgus start
//	const PixelPos screenPos(X + XOffset + (tilePos.x * MinimapScaleX) / MINIMAP_FAC,
//							 Y + YOffset + (tilePos.y * MinimapScaleY) / MINIMAP_FAC);
	const PixelPos screenPos(X + XOffset[UI.CurrentMapLayer->ID] + (tilePos.x * MinimapScaleX[UI.CurrentMapLayer->ID]) / MINIMAP_FAC,
							 Y + YOffset[UI.CurrentMapLayer->ID] + (tilePos.y * MinimapScaleY[UI.CurrentMapLayer->ID]) / MINIMAP_FAC);
	//Wyrmgus end
	return screenPos;
}

/**
**  Destroy mini-map.
*/
void minimap::Destroy()
{
	for (size_t z = 0; z < this->terrain_surface_gl.size(); ++z) {
		if (this->terrain_surface_gl[z]) {
			glDeleteTextures(1, &this->terrain_textures[z]);
			delete[] this->terrain_surface_gl[z];
		}
	}
	this->terrain_surface_gl.clear();
	this->terrain_textures.clear();

	for (size_t z = 0; z < this->territories_surface_gl.size(); ++z) {
		delete[] this->territories_surface_gl[z];
	}
	this->territories_surface_gl.clear();

	for (size_t z = 0; z < this->territories_with_non_land_surface_gl.size(); ++z) {
		delete[] this->territories_with_non_land_surface_gl[z];
	}
	this->territories_with_non_land_surface_gl.clear();

	for (size_t z = 0; z < this->settlements_surface_gl.size(); ++z) {
		delete[] this->settlements_surface_gl[z];
	}
	this->settlements_surface_gl.clear();

	for (size_t z = 0; z < this->surface_gl.size(); ++z) {
		if (this->surface_gl[z]) {
			glDeleteTextures(1, &this->textures[z]);
			delete[] this->surface_gl[z];
		}
	}
	this->surface_gl.clear();
	this->textures.clear();

	for (size_t z = 0; z < Minimap2MapX.size(); ++z) {
		delete[] Minimap2MapX[z];
		Minimap2MapX[z] = nullptr;
	}
	Minimap2MapX.clear();
	for (size_t z = 0; z < Minimap2MapY.size(); ++z) {
		delete[] Minimap2MapY[z];
		Minimap2MapY[z] = nullptr;
	}
	Minimap2MapY.clear();
	for (size_t z = 0; z < Map2MinimapX.size(); ++z) {
		delete[] Map2MinimapX[z];
		Map2MinimapX[z] = nullptr;
	}
	Map2MinimapX.clear();
	for (size_t z = 0; z < Map2MinimapY.size(); ++z) {
		delete[] Map2MinimapY[z];
		Map2MinimapY[z] = nullptr;
	}
	Map2MinimapY.clear();
	MinimapScaleX.clear();
	MinimapScaleY.clear();
	XOffset.clear();
	YOffset.clear();
}

/**
**  Draw viewport area contour.
*/
void minimap::DrawViewportArea(const CViewport &viewport) const
{
	// Determine and save region below minimap cursor
	const PixelPos screenPos = TilePosToScreenPos(viewport.MapPos);
	//Wyrmgus start
//	int w = (viewport.MapWidth * MinimapScaleX) / MINIMAP_FAC;
//	int h = (viewport.MapHeight * MinimapScaleY) / MINIMAP_FAC;
	int w = (viewport.MapWidth * MinimapScaleX[UI.CurrentMapLayer->ID]) / MINIMAP_FAC;
	int h = (viewport.MapHeight * MinimapScaleY[UI.CurrentMapLayer->ID]) / MINIMAP_FAC;
	//Wyrmgus end

	// Draw cursor as rectangle (Note: unclipped, as it is always visible)
	//Wyrmgus start
//	Video.DrawTransRectangle(UI.ViewportCursorColor, screenPos.x, screenPos.y, w, h, 128);
	Video.DrawTransRectangle(UI.ViewportCursorColor, screenPos.x, screenPos.y, w + 1, h + 1, 128);
	//Wyrmgus end
}

/**
**  Add a minimap event
**
**  @param pos  Map tile position
*/
void minimap::AddEvent(const Vec2i &pos, int z, IntColor color)
{
	if (NumMinimapEvents == MAX_MINIMAP_EVENTS) {
		return;
	}
	if (z == UI.CurrentMapLayer->ID) {
		MinimapEvents[NumMinimapEvents].pos = TilePosToScreenPos(pos);
		MinimapEvents[NumMinimapEvents].Size = (W < H) ? W / 3 : H / 3;
		MinimapEvents[NumMinimapEvents].Color = color;
		++NumMinimapEvents;
	} else {
		CMapLayer *event_map_layer = CMap::Map.MapLayers[z];
		if (event_map_layer->world != nullptr && CMap::Map.GetCurrentWorld() != event_map_layer->world && UI.WorldButtons[event_map_layer->world->ID].X != -1) {
			MinimapEvents[NumMinimapEvents].pos.x = UI.WorldButtons[event_map_layer->world->ID].X + (UI.WorldButtons[event_map_layer->world->ID].Style->Width / 2);
			MinimapEvents[NumMinimapEvents].pos.y = UI.WorldButtons[event_map_layer->world->ID].Y + (UI.WorldButtons[event_map_layer->world->ID].Style->Height / 2);
		} else {
			return;
		}
		MinimapEvents[NumMinimapEvents].Size = (W < H) ? W / 3 : H / 3;
		MinimapEvents[NumMinimapEvents].Color = color;
		++NumMinimapEvents;
	}
}

bool minimap::Contains(const PixelPos &screenPos) const
{
	return this->X <= screenPos.x && screenPos.x < this->X + this->W
		   && this->Y <= screenPos.y && screenPos.y < this->Y + this->H;
}

void minimap::toggle_mode()
{
	minimap_mode mode = static_cast<minimap_mode>(static_cast<int>(UI.Minimap.get_mode()) + 1);
	if (mode == minimap_mode::count) {
		mode = static_cast<minimap_mode>(0);
	}
	UI.Minimap.set_mode(mode);
}

bool minimap::is_terrain_visible() const
{
	return this->get_mode() != minimap_mode::units;
}

bool minimap::are_units_visible() const
{
	switch (this->get_mode()) {
		case minimap_mode::territories:
		case minimap_mode::territories_with_non_land:
		case minimap_mode::settlements:
			return false;
		default:
			return true;
	}
}

bool minimap::is_fog_of_war_visible() const
{
	switch (this->get_mode()) {
		case minimap_mode::territories:
		case minimap_mode::territories_with_non_land:
		case minimap_mode::settlements:
			return false;
		default:
			return true;
	}
}

}
