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
//      (c) Copyright 1998-2021 by Lutz Sammer and Jimmy Salmon, Pali Rohár and Andrettin
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
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "plane.h"
#include "player.h"
#include "player_color.h"
#include "province.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "util/vector_util.h"
#include "video/video.h"
#include "world.h"

#ifdef USE_OPENGL
#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <SDL_opengl.h>
#endif

static constexpr int MINIMAP_FAC = 16 * 3;  /// integer scale factor

/// unit attacked are shown red for at least this amount of cycles
static constexpr int ATTACK_RED_DURATION = 1 * CYCLES_PER_SECOND;
/// unit attacked are shown blinking for this amount of cycles
static constexpr int ATTACK_BLINK_DURATION = 7 * CYCLES_PER_SECOND;

static constexpr int SCALE_PRECISION = 100;

static std::vector<int> MinimapTextureWidth;
static std::vector<int> MinimapTextureHeight;

static std::vector<std::vector<int>> Minimap2MapX;                  /// fast conversion table
static std::vector<std::vector<int>> Minimap2MapY;                  /// fast conversion table
static std::vector<std::vector<int>> Map2MinimapX;      /// fast conversion table
static std::vector<std::vector<int>> Map2MinimapY;     /// fast conversion table

// MinimapScale:
// 32x32 64x64 96x96 128x128 256x256 512x512 ...
// *4    *2    *4/3  *1      *1/2    *1/4
static std::vector<int> MinimapScaleX;                  /// Minimap scale to fit into window
static std::vector<int> MinimapScaleY;                  /// Minimap scale to fit into window

static constexpr int MAX_MINIMAP_EVENTS = 8;

struct MinimapEvent {
	PixelPos pos;
	int Size;
	uint32_t Color;
} MinimapEvents[MAX_MINIMAP_EVENTS];
int NumMinimapEvents;

namespace wyrmgus {

minimap::minimap() : mode(minimap_mode::terrain)
{
}

void minimap::create_textures(const int z)
{
	this->create_texture(this->terrain_textures[z], this->terrain_texture_data[z].data(), z);
	this->create_texture(this->overlay_textures[z], this->overlay_texture_data[z].data(), z);
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
	this->overlay_textures.resize(CMap::Map.MapLayers.size());
	MinimapTextureWidth.resize(CMap::Map.MapLayers.size());
	MinimapTextureHeight.resize(CMap::Map.MapLayers.size());

	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		// Scale to biggest value.
		int n = std::max(CMap::Map.Info.MapWidths[z], CMap::Map.Info.MapHeights[z]);
		n = std::max(n, 32);

		const int texture_width = this->get_texture_width(z);
		const int texture_height = this->get_texture_height(z);

		MinimapScaleX.push_back((texture_width * MINIMAP_FAC + n - 1) / n);
		MinimapScaleY.push_back((texture_height * MINIMAP_FAC + n - 1) / n);

		XOffset.push_back((texture_width - (CMap::Map.Info.MapWidths[z] * MinimapScaleX[z]) / MINIMAP_FAC + 1) / 2);
		YOffset.push_back((texture_height - (CMap::Map.Info.MapHeights[z] * MinimapScaleY[z]) / MINIMAP_FAC + 1) / 2);

		DebugPrint("MinimapScale %d %d (%d %d), X off %d, Y off %d\n" _C_
				   MinimapScaleX[z] / MINIMAP_FAC _C_ MinimapScaleY[z] / MINIMAP_FAC _C_
				   MinimapScaleX[z] _C_ MinimapScaleY[z] _C_
				   XOffset[z] _C_ YOffset[z]);

		//
		// Calculate minimap fast lookup tables.
		//
		Minimap2MapX.push_back(std::vector<int>(texture_width * texture_height, 0));
		Minimap2MapY.push_back(std::vector<int>(texture_width * texture_height, 0));
		for (int i = XOffset[z]; i < texture_width - XOffset[z]; ++i) {
			Minimap2MapX[z][i] = ((i - XOffset[z]) * MINIMAP_FAC) / MinimapScaleX[z];
		}
		for (int i = YOffset[z]; i < texture_height - YOffset[z]; ++i) {
			Minimap2MapY[z][i] = (((i - YOffset[z]) * MINIMAP_FAC) / MinimapScaleY[z]) * CMap::Map.Info.MapWidths[z];
		}
		Map2MinimapX.push_back(std::vector<int>(CMap::Map.Info.MapWidths[z], 0));
		Map2MinimapY.push_back(std::vector<int>(CMap::Map.Info.MapHeights[z], 0));
		for (int i = 0; i < CMap::Map.Info.MapWidths[z]; ++i) {
			Map2MinimapX[z][i] = (i * MinimapScaleX[z]) / MINIMAP_FAC;
		}
		for (int i = 0; i < CMap::Map.Info.MapHeights[z]; ++i) {
			Map2MinimapY[z][i] = (i * MinimapScaleY[z]) / MINIMAP_FAC;
		}

		// Palette updated from UpdateMinimapTerrain()
		for (MinimapTextureWidth[z] = 1; MinimapTextureWidth[z] < texture_width; MinimapTextureWidth[z] <<= 1) {
		}
		for (MinimapTextureHeight[z] = 1; MinimapTextureHeight[z] < texture_height; MinimapTextureHeight[z] <<= 1) {
		}

		this->terrain_texture_data.push_back(std::vector<unsigned char>(MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4, 0));

		for (int i = 0; i < static_cast<int>(minimap_mode::count); ++i) {
			const minimap_mode mode = static_cast<minimap_mode>(i);
			if (minimap_mode_has_overlay(mode)) {
				this->mode_overlay_texture_data[mode].push_back(std::vector<unsigned char>(MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4, 0));
			}
		}

		this->overlay_texture_data.push_back(std::vector<unsigned char>(MinimapTextureWidth[z] * MinimapTextureHeight[z] * 4, 0));

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

	for (size_t z = 0; z < this->overlay_textures.size(); ++z) {
		glDeleteTextures(1, &this->overlay_textures[z]);
	}
	this->overlay_textures.clear();
}

/**
**  Reload OpenGL minimap
*/
void minimap::Reload()
{
	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		this->create_textures(z);
	}
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
	
	const season *season = CMap::Map.MapLayers[z]->GetSeason();

	const CMapLayer *map_layer = CMap::Map.MapLayers[z].get();
	const int texture_width = this->get_texture_width(z);
	const int texture_height = this->get_texture_height(z);
	
	for (int my = YOffset[z]; my < texture_height - YOffset[z]; ++my) {
		for (int mx = XOffset[z]; mx < texture_width - XOffset[z]; ++mx) {
			const tile &mf = *map_layer->Field(Minimap2MapX[z][mx] + Minimap2MapY[z][my]);
			const terrain_type *terrain = mf.get_top_terrain(true);
	
			const QColor color = terrain ? terrain->get_minimap_color(season) : QColor(0, 0, 0);

			const uint32_t c = CVideo::MapRGB(color);
			*(uint32_t *) &(this->terrain_texture_data[z][(mx + my * MinimapTextureWidth[z]) * 4]) = c;
		}
	}
}

void minimap::update_territories(const int z)
{
	const int texture_width = this->get_texture_width(z);
	const int texture_height = this->get_texture_height(z);

	for (int my = YOffset[z]; my < texture_height - YOffset[z]; ++my) {
		for (int mx = XOffset[z]; mx < texture_width - XOffset[z]; ++mx) {
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
void minimap::UpdateXY(const Vec2i &pos, const int z)
{
	if (z >= static_cast<int>(this->terrain_texture_data.size())) {
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

	const season *season = CMap::Map.MapLayers[z]->GetSeason();

	const int ty = pos.y * CMap::Map.Info.MapWidths[z];
	const int tx = pos.x;

	const int texture_width = this->get_texture_width(z);
	const int texture_height = this->get_texture_height(z);

	for (int my = YOffset[z]; my < texture_height - YOffset[z]; ++my) {
		const int y = Minimap2MapY[z][my];
		if (y < ty) {
			continue;
		}
		if (y > ty) {
			break;
		}

		for (int mx = XOffset[z]; mx < texture_width - XOffset[z]; ++mx) {
			const int x = Minimap2MapX[z][mx];

			if (x < tx) {
				continue;
			}
			if (x > tx) {
				break;
			}

			const tile &mf = *CMap::Map.MapLayers[z]->Field(x + y);
			const terrain_type *terrain = mf.get_top_terrain(true);

			const QColor color = terrain ? terrain->get_minimap_color(season) : QColor(0, 0, 0);

			const uint32_t c = CVideo::MapRGB(color);
			*(uint32_t *) &(this->terrain_texture_data[z][(mx + my * MinimapTextureWidth[z]) * 4]) = c;
		}
	}
}

void minimap::update_territory_xy(const QPoint &pos, const int z)
{
	const int texture_width = this->get_texture_width(z);
	const int texture_height = this->get_texture_height(z);

	const int ty = pos.y() * CMap::Map.Info.MapWidths[z];
	const int tx = pos.x();
	const int non_land_territory_alpha = defines::get()->get_minimap_non_land_territory_alpha();

	for (int my = YOffset[z]; my < texture_height - YOffset[z]; ++my) {
		const int y = Minimap2MapY[z][my];
		if (y < ty) {
			continue;
		}
		if (y > ty) {
			break;
		}

		for (int mx = XOffset[z]; mx < texture_width - XOffset[z]; ++mx) {
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
	const CMapLayer *map_layer = CMap::Map.MapLayers[z].get();
	const int non_land_territory_alpha = defines::get()->get_minimap_non_land_territory_alpha();
	const int minimap_color_index = defines::get()->get_minimap_color_index();

	const int pixel_index = (mx + my * MinimapTextureWidth[z]) * 4;
	QColor color(Qt::transparent);
	QColor with_non_land_color(Qt::transparent);
	QColor realm_color(Qt::transparent);
	QColor realm_with_non_land_color(Qt::transparent);

	const tile &mf = *map_layer->Field(Minimap2MapX[z][mx] + Minimap2MapY[z][my]);
	const site *settlement = mf.get_settlement();
	if (settlement != nullptr) {
		const bool is_tile_water = mf.is_water() && !mf.is_river();
		const bool is_tile_space = mf.is_space();

		const CPlayer *player = CPlayer::Players[PlayerNumNeutral];
		const CPlayer *realm_player = CPlayer::Players[PlayerNumNeutral];

		if (mf.get_owner() != nullptr) {
			player = mf.get_owner();
			realm_player = mf.get_realm_owner();
		}

		if (!is_tile_water && !is_tile_space) {
			color = player->get_minimap_color();
			with_non_land_color = color;
			realm_color = realm_player->get_minimap_color();
			realm_with_non_land_color = realm_color;
		} else {
			if (player != CPlayer::Players[PlayerNumNeutral]) {
				with_non_land_color = player->get_minimap_color();
				with_non_land_color.setAlpha(non_land_territory_alpha);
			}

			if (realm_player != CPlayer::Players[PlayerNumNeutral]) {
				realm_with_non_land_color = realm_player->get_minimap_color();
				realm_with_non_land_color.setAlpha(non_land_territory_alpha);
			}
		}

		const CUnit *settlement_unit = settlement->get_game_data()->get_site_unit();
		if (settlement_unit == nullptr) {
			throw std::runtime_error("Settlement \"" + settlement->get_identifier() + "\" has territory, but no settlement unit.");
		}

		const tile *settlement_center_tile = settlement_unit->get_center_tile();
		const bool is_settlement_water = settlement_center_tile->is_water() && !settlement_center_tile->is_river();
		const bool is_settlement_space = settlement_center_tile->is_space();
		if (is_tile_water == is_settlement_water && is_tile_space == is_settlement_space) {
			const QColor settlement_color = settlement->get_color();
			*(uint32_t *) &(this->mode_overlay_texture_data[minimap_mode::settlements][z][pixel_index]) = CVideo::MapRGBA(settlement_color);
		}
	}

	const uint32_t c = CVideo::MapRGBA(color);
	*(uint32_t *) &(this->mode_overlay_texture_data[minimap_mode::territories][z][pixel_index]) = c;

	const uint32_t with_non_land_c = CVideo::MapRGBA(with_non_land_color);
	*(uint32_t *) &(this->mode_overlay_texture_data[minimap_mode::territories_with_non_land][z][pixel_index]) = with_non_land_c;

	const uint32_t realm_c = CVideo::MapRGBA(realm_color);
	*(uint32_t *) &(this->mode_overlay_texture_data[minimap_mode::realms][z][pixel_index]) = realm_c;

	const uint32_t realm_with_non_land_c = CVideo::MapRGBA(realm_with_non_land_color);
	*(uint32_t *) &(this->mode_overlay_texture_data[minimap_mode::realms_with_non_land][z][pixel_index]) = realm_with_non_land_c;
}

/**
**  Draw a unit on the minimap.
*/
template <bool center_tile_only>
void minimap::draw_unit_on(const CUnit *unit, const bool red_phase)
{
	const int z = UI.CurrentMapLayer->ID;
	const int texture_width = this->get_texture_width(z);
	const int texture_height = this->get_texture_height(z);

	const unit_type *type;

	if (Editor.Running || ReplayRevealMap || unit->IsVisible(*CPlayer::GetThisPlayer())) {
		type = unit->Type;
	} else {
		type = unit->Seen.Type;
		// This will happen for radar if the unit has not been seen and we
		// have it on radar.
		if (!type) {
			type = unit->Type;
		}
	}

	//don't draw decorations or diminutive fauna units on the minimap
	if (type->BoolFlag[DECORATION_INDEX].value || (type->BoolFlag[DIMINUTIVE_INDEX].value && type->BoolFlag[FAUNA_INDEX].value)) {
		return;
	}

	uint32_t color;
	if (unit->GetDisplayPlayer() == PlayerNumNeutral) {
		color = CVideo::MapRGB(type->get_neutral_minimap_color());
	} else if (unit->Player == CPlayer::GetThisPlayer() && !Editor.Running) {
		if (unit->Attacked && unit->Attacked + ATTACK_BLINK_DURATION > GameCycle &&
			(red_phase || unit->Attacked + ATTACK_RED_DURATION > GameCycle)) {
			color = ColorRed;
		} else if (this->ShowSelected && unit->Selected) {
			color = ColorWhite;
		} else {
			color = ColorGreen;
		}
	} else {
		color = CVideo::MapRGB(unit->Player->get_minimap_color());
	}

	if constexpr (center_tile_only) {
		const QPoint center_pos = unit->get_center_tile_pos();
		const int x = 1 + this->XOffset[z] + Map2MinimapX[z][center_pos.x()];
		const int y = 1 + this->YOffset[z] + Map2MinimapY[z][center_pos.y()];

		*(uint32_t *) &(this->overlay_texture_data[z][(x + y * MinimapTextureWidth[z]) * 4]) = color;

		return;
	}

	const int mx = 1 + this->XOffset[z] + Map2MinimapX[z][unit->tilePos.x];
	const int my = 1 + this->YOffset[z] + Map2MinimapY[z][unit->tilePos.y];
	int w = Map2MinimapX[z][type->get_tile_width()];

	if (mx + w >= texture_width) { // clip right side
		w = texture_width - mx;
	}

	int h0 = Map2MinimapY[z][type->get_tile_height()];
	if (my + h0 >= texture_height) { // clip bottom side
		h0 = texture_height - my;
	}

	while (w-- >= 0) {
		int h = h0;
		while (h-- >= 0) {
			*(uint32_t *) &(this->overlay_texture_data[z][((mx + w) + (my + h) * MinimapTextureWidth[z]) * 4]) = color;
		}
	}
}

/**
**  Update the minimap with the current game information
*/
void minimap::Update()
{
	static bool red_phase = false;

	const bool red_phase_changed = red_phase != static_cast<bool>((FrameCounter / FRAMES_PER_SECOND) & 1);
	if (red_phase_changed) {
		red_phase = !red_phase;
	}

	const int z = UI.CurrentMapLayer->ID;

	//clear Minimap background if not transparent
	if (!Transparent) {
		vector::fill(this->overlay_texture_data[z], 0);
	}

	if (minimap_mode_has_overlay(this->get_mode())) {
		this->overlay_texture_data[z] = this->mode_overlay_texture_data[this->get_mode()][z];
	}

	const int texture_width = this->get_texture_width(z);
	const int texture_height = this->get_texture_height(z);

	const uint32_t unexplored_color = CVideo::MapRGB(0, 0, 0);
	const uint32_t explored_color = CVideo::MapRGBA(0, 0, 0, 128); //explored but not visible
	for (int my = 0; my < texture_height; ++my) {
		for (int mx = 0; mx < texture_width; ++mx) {
			if (mx < XOffset[z] || mx >= texture_width - XOffset[z] || my < YOffset[z] || my >= texture_height - YOffset[z]) {
				*(uint32_t *) &(this->overlay_texture_data[z][(mx + my * MinimapTextureWidth[z]) * 4]) = CVideo::MapRGB(0, 0, 0);
				continue;
			}
			
			int visiontype; // 0 unexplored, 1 explored, >1 visible.

			if (ReplayRevealMap) {
				visiontype = 2;
			} else {
				const Vec2i tilePos(Minimap2MapX[z][mx], Minimap2MapY[z][my] / UI.CurrentMapLayer->get_width());
				visiontype = CMap::Map.Field(tilePos, z)->player_info->TeamVisibilityState(*CPlayer::GetThisPlayer());
			}

			switch (visiontype) {
				case 0:
					*(uint32_t *) &(this->overlay_texture_data[z][(mx + my * MinimapTextureWidth[z]) * 4]) = unexplored_color;
					break;
				case 1:
					if (this->is_fog_of_war_visible()) {
						uint32_t *c = (uint32_t *) &(this->overlay_texture_data[z][(mx + my * MinimapTextureWidth[z]) * 4]);
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
		for (const CUnit *unit : unit_manager::get()->get_units()) {
			if (unit->IsVisibleOnMinimap()) {
				this->draw_unit_on<false>(unit, red_phase);
			}
		}
	} else if (this->get_mode() == minimap_mode::terrain_only) {
		//when drawing only terrain, draw celestial body units on their center tile
		for (const CUnit *unit : unit_manager::get()->get_units()) {
			if (!unit->Type->BoolFlag[CELESTIAL_BODY_INDEX].value) {
				continue;
			}

			if (unit->IsVisibleOnMinimap()) {
				this->draw_unit_on<true>(unit, red_phase);
			}
		}
	}
}

void minimap::draw_events() const
{
	const unsigned char alpha = 192;

	for (int i = 0; i < NumMinimapEvents; ++i) {
		QPoint screen_pos = this->texture_to_screen_pos(MinimapEvents[i].pos);
		if (screen_pos.x() < this->X) {
			screen_pos.setX(this->X);
		} else if (screen_pos.x() >= this->X + this->get_width()) {
			screen_pos.setX(this->X + this->get_width() - 1);
		}
		if (screen_pos.y() < this->Y) {
			screen_pos.setY(this->Y);
		} else if (screen_pos.y() >= this->Y + this->get_height()) {
			screen_pos.setY(this->Y + this->get_height() - 1);
		}

		Video.DrawTransCircleClip(MinimapEvents[i].Color, screen_pos.x(), screen_pos.y(), MinimapEvents[i].Size, alpha);
		MinimapEvents[i].Size -= 1;
		if (MinimapEvents[i].Size < 2) {
			MinimapEvents[i] = MinimapEvents[--NumMinimapEvents];
			--i;
		}
	}
}

void minimap::Draw() const
{
	const int z = UI.CurrentMapLayer->ID;

	if (this->is_terrain_visible()) {
		this->draw_texture(this->terrain_textures[z], this->terrain_texture_data[z].data(), z);
	}

	this->draw_texture(this->overlay_textures[z], this->overlay_texture_data[z].data(), z);
	this->draw_events();
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
	const QRect texture_draw_rect = this->get_texture_draw_rect(z);
	const float start_x = static_cast<float>(texture_draw_rect.x()) / MinimapTextureWidth[z];
	const float start_y = static_cast<float>(texture_draw_rect.y()) / MinimapTextureHeight[z];
	const float end_x = static_cast<float>(texture_draw_rect.x() + texture_draw_rect.width()) / MinimapTextureWidth[z];
	const float end_y = static_cast<float>(texture_draw_rect.y() + texture_draw_rect.height()) / MinimapTextureHeight[z];

	glBegin(GL_QUADS);
	glTexCoord2f(start_x, start_y);
	glVertex2i(X, Y);
	glTexCoord2f(start_x, end_y);
	glVertex2i(X, Y + H);
	glTexCoord2f(end_x, end_y);
	glVertex2i(X + W, Y + H);
	glTexCoord2f(end_x, start_y);
	glVertex2i(X + W, Y);
	glEnd();
#endif
}

QPoint minimap::texture_to_tile_pos(const QPoint &texture_pos) const
{
	const int z = UI.CurrentMapLayer->ID;
	Vec2i tile_pos(((texture_pos.x() - XOffset[z]) * MINIMAP_FAC) / MinimapScaleX[z],
		((texture_pos.y() - YOffset[z]) * MINIMAP_FAC) / MinimapScaleY[z]);

	CMap::Map.Clamp(tile_pos, UI.CurrentMapLayer->ID);

	return tile_pos;
}

QPoint minimap::texture_to_screen_pos(const QPoint &texture_pos) const
{
	const int z = UI.CurrentMapLayer->ID;

	QPoint screen_pos = texture_pos;

	if (this->is_zoomed() && this->can_zoom(z)) {
		const QRect rect = this->get_texture_draw_rect(z);
		screen_pos.setX(screen_pos.x() - rect.x());
		screen_pos.setY(screen_pos.y() - rect.y());
	} else {
		screen_pos.setX(screen_pos.x() * this->get_width() / this->get_texture_width(z));
		screen_pos.setY(screen_pos.y() * this->get_height() / this->get_texture_height(z));
	}

	screen_pos += QPoint(this->X, this->Y);

	return screen_pos;
}

QPoint minimap::screen_to_tile_pos(const QPoint &screen_pos) const
{
	const int z = UI.CurrentMapLayer->ID;

	QPoint texture_pos = screen_pos;
	texture_pos -= QPoint(this->X, this->Y);

	if (this->is_zoomed() && this->can_zoom(z)) {
		const QRect rect = this->get_texture_draw_rect(z);
		texture_pos.setX(texture_pos.x() + rect.x());
		texture_pos.setY(texture_pos.y() + rect.y());
	} else {
		texture_pos.setX(texture_pos.x() * this->get_texture_width(z) / this->get_width());
		texture_pos.setY(texture_pos.y() * this->get_texture_height(z) / this->get_height());
	}

	return this->texture_to_tile_pos(texture_pos);
}

QPoint minimap::tile_to_texture_pos(const QPoint &tile_pos) const
{
	const int z = UI.CurrentMapLayer->ID;
	QPoint screenPos(XOffset[z] + (tile_pos.x() * MinimapScaleX[z]) / MINIMAP_FAC, YOffset[z] + (tile_pos.y() * MinimapScaleY[z]) / MINIMAP_FAC);
	return screenPos;
}

QPoint minimap::tile_to_screen_pos(const QPoint &tile_pos) const
{
	const QPoint texture_pos = this->tile_to_texture_pos(tile_pos);
	return this->texture_to_screen_pos(texture_pos);
}

/**
**  Destroy mini-map.
*/
void minimap::Destroy()
{
	for (size_t z = 0; z < this->terrain_texture_data.size(); ++z) {
		glDeleteTextures(1, &this->terrain_textures[z]);
	}
	this->terrain_texture_data.clear();
	this->terrain_textures.clear();

	this->mode_overlay_texture_data.clear();

	for (size_t z = 0; z < this->overlay_texture_data.size(); ++z) {
		glDeleteTextures(1, &this->overlay_textures[z]);
	}
	this->overlay_texture_data.clear();
	this->overlay_textures.clear();

	Minimap2MapX.clear();
	Minimap2MapY.clear();
	Map2MinimapX.clear();
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
	const int z = UI.CurrentMapLayer->ID;
	const PixelPos screenPos = this->tile_to_screen_pos(viewport.MapPos);
	int w = (viewport.MapWidth * MinimapScaleX[z]) / MINIMAP_FAC;
	int h = (viewport.MapHeight * MinimapScaleY[z]) / MINIMAP_FAC;
	if (!this->is_zoomed() || !this->can_zoom(z)) {
		w *= this->get_width();
		w /= this->get_texture_width(z);
		h *= this->get_height();
		h /= this->get_texture_height(z);
	}

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
		MinimapEvents[NumMinimapEvents].pos = this->tile_to_texture_pos(pos);
		MinimapEvents[NumMinimapEvents].Size = (W < H) ? W / 3 : H / 3;
		MinimapEvents[NumMinimapEvents].Color = color;
		++NumMinimapEvents;
	}
}

bool minimap::Contains(const PixelPos &screenPos) const
{
	return this->X <= screenPos.x && screenPos.x < this->X + this->get_width()
		   && this->Y <= screenPos.y && screenPos.y < this->Y + this->get_height();
}

int minimap::get_texture_width(const size_t z) const
{
	return std::max(this->get_width(), CMap::Map.Info.MapWidths[z]);
}

int minimap::get_texture_height(const size_t z) const
{
	return std::max(this->get_height(), CMap::Map.Info.MapHeights[z]);
}

bool minimap::is_mode_valid(const minimap_mode mode) const
{
	switch (mode) {
		case minimap_mode::realms:
		case minimap_mode::realms_with_non_land:
			for (const CPlayer *player : CPlayer::Players) {
				if (!player->is_alive()) {
					continue;
				}

				if (player->get_overlord() != nullptr) {
					return true;
				}
			}
			return false;
		default:
			return true;
	}
}

minimap_mode minimap::get_next_mode(const minimap_mode mode) const
{
	minimap_mode next_mode = static_cast<minimap_mode>(static_cast<int>(mode) + 1);
	if (next_mode == minimap_mode::count) {
		next_mode = static_cast<minimap_mode>(0);
	}
	if (!this->is_mode_valid(next_mode)) {
		return this->get_next_mode(next_mode);
	}
	return next_mode;
}

void minimap::toggle_mode()
{
	const minimap_mode mode = this->get_next_mode(this->get_mode());
	this->set_mode(mode);
}

bool minimap::is_terrain_visible() const
{
	return this->get_mode() != minimap_mode::units;
}

bool minimap::are_units_visible() const
{
	switch (this->get_mode()) {
		case minimap_mode::terrain_only:
		case minimap_mode::territories:
		case minimap_mode::territories_with_non_land:
		case minimap_mode::realms:
		case minimap_mode::realms_with_non_land:
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
		case minimap_mode::realms:
		case minimap_mode::realms_with_non_land:
		case minimap_mode::settlements:
			return false;
		default:
			return true;
	}
}

QRect minimap::get_texture_draw_rect(const int z) const
{
	const int width = this->get_width();
	const int height = this->get_height();
	const int texture_width = this->get_texture_width(z);
	const int texture_height = this->get_texture_height(z);

	if (this->is_zoomed() && this->can_zoom(z)) {
		const CViewport *viewport = UI.SelectedViewport;
		const QPoint tile_pos = viewport->screen_center_to_tile_pos();
		const QPoint texture_pos = this->tile_to_texture_pos(tile_pos);

		const QPoint offset(width / 2, height / 2);
		const int start_x = std::min(std::max(texture_pos.x() - (offset.x() - 1), 0), texture_width - width);
		const int start_y = std::min(std::max(texture_pos.y() - (offset.y() - 1), 0), texture_height - height);
		return QRect(start_x, start_y, width, height);
	} else {
		return QRect(0, 0, texture_width, texture_height);
	}
}

}
