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
/**@name map_draw.cpp - The map drawing. */
//
//      (c) Copyright 1999-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "viewport.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "editor.h"
#include "engine_interface.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/world.h"
#include "map/world_game_data.h"
#include "missile.h"
#include "particle.h"
#include "pathfinder/pathfinder.h"
#include "player/player.h"
#include "player/player_color.h"
#include "translator.h"
#include "ui/cursor.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/colorization_type.h"
#include "util/point_util.h"
#include "util/size_util.h"
#include "util/vector_util.h"
#include "video/font.h"
#include "video/font_color.h"
#include "video/renderer.h"
#include "video/video.h"

CViewport::CViewport()
{
	this->MapPos.x = this->MapPos.y = 0;
	this->Offset.x = this->Offset.y = 0;
}

CViewport::~CViewport()
{
}

void CViewport::Restrict(int &screenPosX, int &screenPosY) const
{
	screenPosX = std::clamp(screenPosX, this->get_top_left_pos().x(), this->get_bottom_right_pos().x() - 1);
	screenPosY = std::clamp(screenPosY, this->get_top_left_pos().y(), this->get_bottom_right_pos().y() - 1);
}

void CViewport::SetClipping() const
{
	::SetClipping(this->get_top_left_pos().x(), this->get_top_left_pos().y(), this->get_bottom_right_pos().x(), this->get_bottom_right_pos().y());
}

/**
**  Check if any part of an area is visible in a viewport.
**
**  @param boxmin  map tile position of area in map to be checked.
**  @param boxmax  map tile position of area in map to be checked.
**
**  @return    True if any part of area is visible, false otherwise
*/
bool CViewport::AnyMapAreaVisibleInViewport(const Vec2i &boxmin, const Vec2i &boxmax) const
{
	assert_throw(boxmin.x <= boxmax.x && boxmin.y <= boxmax.y);

	if (boxmax.x < this->MapPos.x
		|| boxmax.y < this->MapPos.y
		|| boxmin.x >= this->MapPos.x + this->MapWidth
		|| boxmin.y >= this->MapPos.y + this->MapHeight) {
		return false;
	}
	return true;
}

bool CViewport::IsInsideMapArea(const PixelPos &screenPixelPos) const
{
	const Vec2i tilePos = ScreenToTilePos(screenPixelPos);

	return CMap::get()->Info->IsPointOnMap(tilePos, UI.CurrentMapLayer);
}

QPoint CViewport::get_scaled_map_top_left_pixel_pos() const
{
	return CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(this->MapPos) + this->Offset;
}

// Convert viewport coordinates into map pixel coordinates
PixelPos CViewport::screen_to_map_pixel_pos(const PixelPos &screenPixelPos) const
{
	return QPoint(this->screen_to_scaled_map_pixel_pos(screenPixelPos)) / preferences::get()->get_scale_factor();
}

PixelPos CViewport::screen_to_scaled_map_pixel_pos(const PixelPos &screenPixelPos) const
{
	const PixelDiff relPos = screenPixelPos - this->get_top_left_pos() + this->Offset;
	const PixelPos mapPixelPos = relPos + CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(this->MapPos);

	return mapPixelPos;
}

// Convert map pixel coordinates into viewport coordinates
PixelPos CViewport::map_to_screen_pixel_pos(const PixelPos &mapPixelPos) const
{
	return this->scaled_map_to_screen_pixel_pos(QPoint(mapPixelPos) * preferences::get()->get_scale_factor());
}

PixelPos CViewport::scaled_map_to_screen_pixel_pos(const PixelPos &mapPixelPos) const
{
	const PixelDiff relPos = mapPixelPos - CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(this->MapPos);

	return this->get_top_left_pos() + relPos - this->Offset;
}

/// convert screen coordinate into tilepos
Vec2i CViewport::ScreenToTilePos(const PixelPos &screenPixelPos) const
{
	const PixelPos mapPixelPos = this->screen_to_scaled_map_pixel_pos(screenPixelPos);
	const Vec2i tilePos = CMap::get()->scaled_map_pixel_pos_to_tile_pos(mapPixelPos);

	return tilePos;
}

/// convert tilepos coordonates into screen (take the top left of the tile)
PixelPos CViewport::TilePosToScreen_TopLeft(const Vec2i &tilePos) const
{
	const PixelPos mapPos = CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(tilePos);

	return this->scaled_map_to_screen_pixel_pos(mapPos);
}

/// convert tilepos coordinates into screen (take the center of the tile)
PixelPos CViewport::TilePosToScreen_Center(const Vec2i &tilePos) const
{
	const PixelPos topLeft = TilePosToScreen_TopLeft(tilePos);

	return topLeft + size::to_point(defines::get()->get_scaled_tile_size()) / 2;
}

/// convert tilepos coordonates into screen (take the center of the tile)
QPoint CViewport::screen_center_to_tile_pos() const
{
	return this->ScreenToTilePos(this->get_top_left_pos() + size::to_point(this->get_pixel_size()) / 2);
}

const time_of_day *CViewport::get_center_tile_time_of_day() const
{
	if (UI.CurrentMapLayer == nullptr) {
		return nullptr;
	}

	const QPoint tile_pos = this->screen_center_to_tile_pos();
	return UI.CurrentMapLayer->get_tile_time_of_day(tile_pos);
}

const season *CViewport::get_center_tile_season() const
{
	if (UI.CurrentMapLayer == nullptr) {
		return nullptr;
	}

	const QPoint tile_pos = this->screen_center_to_tile_pos();
	return UI.CurrentMapLayer->get_tile_season(tile_pos);
}

QRect CViewport::get_unit_type_box_rect(const unit_type *unit_type, const QPoint &tile_pos) const
{
	const QPoint pixel_center_pos = this->TilePosToScreen_TopLeft(tile_pos) + unit_type->get_scaled_half_tile_pixel_size();

	return unit_type->get_scaled_box_rect(pixel_center_pos);
}

const QRect CViewport::get_map_rect() const
{
	const QPoint top_left = CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(this->MapPos) - QPoint(1, 1);

	return QRect(top_left, this->get_pixel_size());
}

/**
**  Change viewpoint of map viewport v to tilePos.
**
**  @param tilePos  map tile position.
**  @param offset   offset in tile.
*/
void CViewport::Set(const PixelPos &mapPos)
{
	int x = mapPos.x;
	int y = mapPos.y;

	x = std::max(x, -UI.MapArea.ScrollPaddingLeft);
	y = std::max(y, -UI.MapArea.ScrollPaddingTop);

	const QSize pixel_size = this->get_pixel_size();

	x = std::min(x, (CMap::get()->Info->MapWidths.size() && UI.CurrentMapLayer ? UI.CurrentMapLayer->get_width() : CMap::get()->Info->get_map_width()) * defines::get()->get_scaled_tile_width() - (pixel_size.width()) - 1 + UI.MapArea.ScrollPaddingRight);
	y = std::min(y, (CMap::get()->Info->MapHeights.size() && UI.CurrentMapLayer ? UI.CurrentMapLayer->get_height() : CMap::get()->Info->get_map_height()) * defines::get()->get_scaled_tile_height() - (pixel_size.height()) - 1 + UI.MapArea.ScrollPaddingBottom);

	this->MapPos.x = x / defines::get()->get_scaled_tile_width();
	if (x < 0 && x % defines::get()->get_scaled_tile_width()) {
		this->MapPos.x--;
	}
	this->MapPos.y = y / defines::get()->get_scaled_tile_height();
	if (y < 0 && y % defines::get()->get_scaled_tile_height()) {
		this->MapPos.y--;
	}
	this->Offset.x = x % defines::get()->get_scaled_tile_width();
	if (this->Offset.x < 0) {
		this->Offset.x += defines::get()->get_scaled_tile_width();
	}
	this->Offset.y = y % defines::get()->get_scaled_tile_height();
	if (this->Offset.y < 0) {
		this->Offset.y += defines::get()->get_scaled_tile_height();
	}
	this->MapWidth = (pixel_size.width() + this->Offset.x - 1) / defines::get()->get_scaled_tile_width() + 1;
	this->MapHeight = (pixel_size.height() + this->Offset.y - 1) / defines::get()->get_scaled_tile_height() + 1;

	if (this == UI.SelectedViewport) {
		engine_interface::get()->update_current_time_of_day();
		engine_interface::get()->update_current_season();
	}
}

/**
**  Change viewpoint of map viewport v to tilePos.
**
**  @param tilePos  map tile position.
**  @param offset   offset in tile.
*/
void CViewport::Set(const Vec2i &tilePos, const PixelDiff &offset)
{
	const PixelPos mapPixelPos = CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(tilePos) + offset;

	this->Set(mapPixelPos);
}

/**
**  Center map viewport v on map tile (pos).
**
**  @param mapPixelPos     map pixel position.
*/
void CViewport::Center(const PixelPos &mapPixelPos)
{
	this->Set(mapPixelPos - this->get_pixel_size() / 2);
}

template <typename function_type>
void CViewport::for_each_map_tile(const function_type &function) const
{
	const int ex = this->get_bottom_right_pos().x();
	const int ey = this->get_bottom_right_pos().y();
	int sy = this->MapPos.y;
	int dy = this->get_top_left_pos().y() - this->Offset.y;
	const int map_max = UI.CurrentMapLayer->get_width() * UI.CurrentMapLayer->get_height();

	while (sy < 0) {
		sy++;
		dy += defines::get()->get_scaled_tile_height();
	}
	sy *= UI.CurrentMapLayer->get_width();

	while (dy <= ey && sy < map_max) {
		int sx = this->MapPos.x + sy;
		int dx = this->get_top_left_pos().x() - this->Offset.x;
		while (dx <= ex && (sx - sy < UI.CurrentMapLayer->get_width())) {
			if (sx - sy < 0) {
				++sx;
				dx += defines::get()->get_scaled_tile_width();
				continue;
			}

			const tile *tile = UI.CurrentMapLayer->Field(sx);
			const QPoint pixel_pos(dx, dy);

			function(tile, pixel_pos);

			++sx;
			dx += defines::get()->get_scaled_tile_width();
		}

		sy += UI.CurrentMapLayer->get_width();
		dy += defines::get()->get_scaled_tile_height();
	}
}

void CViewport::draw_map_tile(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	const terrain_type *terrain = ReplayRevealMap ? tile->get_terrain() : tile->player_info->SeenTerrain;
	const terrain_type *overlay_terrain = ReplayRevealMap ? tile->get_overlay_terrain() : tile->player_info->SeenOverlayTerrain;
	const int solid_tile = ReplayRevealMap ? tile->SolidTile : tile->player_info->SeenSolidTile;
	const int overlay_solid_tile = ReplayRevealMap ? tile->OverlaySolidTile : tile->player_info->SeenOverlaySolidTile;

	const std::vector<tile_transition> &transition_tiles = ReplayRevealMap ? tile->TransitionTiles : tile->player_info->SeenTransitionTiles;

	const wyrmgus::time_of_day *time_of_day = UI.CurrentMapLayer->get_tile_time_of_day(tile, terrain->get_flags());
	const season *season = UI.CurrentMapLayer->get_tile_season(tile);

	const wyrmgus::player_color *player_color = tile->get_player_color();

	if (terrain != nullptr) {
		const std::shared_ptr<CPlayerColorGraphic> &terrain_graphics = terrain->get_graphics(season);
		if (terrain_graphics != nullptr) {
			const int frame_index = solid_tile + (terrain == tile->get_terrain() ? tile->AnimationFrame : 0);
			const color_modification color_modification(terrain->get_hue_rotation(), terrain->get_colorization(), color_set(), player_color, time_of_day);
			terrain_graphics->render_frame(frame_index, pixel_pos, color_modification, render_commands);
		}
	}

	for (size_t i = 0; i != transition_tiles.size(); ++i) {
		const terrain_type *transition_terrain = transition_tiles[i].terrain;
		const wyrmgus::season *transition_season = UI.CurrentMapLayer->get_tile_season(tile, transition_terrain->Flags);
		const std::shared_ptr<CPlayerColorGraphic> &transition_terrain_graphics = transition_terrain->get_graphics(transition_season);

		if (transition_terrain_graphics != nullptr) {
			const wyrmgus::time_of_day *transition_time_of_day = UI.CurrentMapLayer->get_tile_time_of_day(tile, transition_terrain->Flags);

			const color_modification color_modification(transition_terrain->get_hue_rotation(), transition_terrain->get_colorization(), color_set(), player_color, transition_time_of_day);
			transition_terrain_graphics->render_frame(transition_tiles[i].tile_frame, pixel_pos, color_modification, render_commands);
		}
	}

	const bool is_impassable = overlay_terrain && overlay_terrain->has_flag(tile_flag::impassable) && !vector::contains(overlay_terrain->get_destroyed_tiles(), overlay_solid_tile);

	//if the tile is not passable, draw the border under its overlay, but otherwise, draw the border over it
	if (!is_impassable) {
		this->draw_map_tile_overlay_terrain(tile, pixel_pos, render_commands);
	}
}

void CViewport::draw_map_tile_top(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	const terrain_type *terrain = ReplayRevealMap ? tile->get_terrain() : tile->player_info->SeenTerrain;
	const terrain_type *overlay_terrain = ReplayRevealMap ? tile->get_overlay_terrain() : tile->player_info->SeenOverlayTerrain;
	const int overlay_solid_tile = ReplayRevealMap ? tile->OverlaySolidTile : tile->player_info->SeenOverlaySolidTile;

	const bool is_impassable = overlay_terrain && overlay_terrain->has_flag(tile_flag::impassable) && !vector::contains(overlay_terrain->get_destroyed_tiles(), overlay_solid_tile);

	//if the tile is not passable, draw the border under its overlay, but otherwise, draw the border over it
	if (is_impassable) {
		this->draw_map_tile_overlay_terrain(tile, pixel_pos, render_commands);
	}

	const wyrmgus::time_of_day *time_of_day = UI.CurrentMapLayer->get_tile_time_of_day(tile, terrain->get_flags());

	const std::vector<tile_transition> &overlay_transition_tiles = ReplayRevealMap ? tile->OverlayTransitionTiles : tile->player_info->SeenOverlayTransitionTiles;

	for (size_t i = 0; i != overlay_transition_tiles.size(); ++i) {
		const terrain_type *overlay_transition_terrain = overlay_transition_tiles[i].terrain;
		if (overlay_transition_terrain->get_elevation_graphics()) {
			overlay_transition_terrain->get_elevation_graphics()->DrawFrameClip(overlay_transition_tiles[i].tile_frame, pixel_pos.x(), pixel_pos.y(), time_of_day, render_commands);
		}
	}
}

void CViewport::draw_map_tile_overlay_terrain(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	const terrain_type *terrain = ReplayRevealMap ? tile->get_terrain() : tile->player_info->SeenTerrain;
	const terrain_type *overlay_terrain = ReplayRevealMap ? tile->get_overlay_terrain() : tile->player_info->SeenOverlayTerrain;
	const int overlay_solid_tile = ReplayRevealMap ? tile->OverlaySolidTile : tile->player_info->SeenOverlaySolidTile;

	const wyrmgus::player_color *player_color = tile->get_player_color();
	const wyrmgus::time_of_day *time_of_day = UI.CurrentMapLayer->get_tile_time_of_day(tile, terrain->get_flags());
	const season *season = UI.CurrentMapLayer->get_tile_season(tile);

	const std::vector<tile_transition> &overlay_transition_tiles = ReplayRevealMap ? tile->OverlayTransitionTiles : tile->player_info->SeenOverlayTransitionTiles;

	if (overlay_terrain != nullptr && (overlay_transition_tiles.empty() || overlay_terrain->has_transition_mask())) {
		const bool is_overlay_space = overlay_terrain->has_flag(tile_flag::space);
		const std::shared_ptr<CPlayerColorGraphic> &overlay_terrain_graphics = overlay_terrain->get_graphics(season);

		if (overlay_terrain_graphics != nullptr) {
			const int frame_index = overlay_solid_tile + (overlay_terrain == tile->get_overlay_terrain() ? tile->OverlayAnimationFrame : 0);
			const wyrmgus::time_of_day *overlay_time_of_day = is_overlay_space ? nullptr : time_of_day;

			const color_modification color_modification(overlay_terrain->get_hue_rotation(), overlay_terrain->get_colorization(), color_set(), player_color, overlay_time_of_day);
			overlay_terrain_graphics->render_frame(frame_index, pixel_pos, color_modification, render_commands);
		}
	}

	for (size_t i = 0; i != overlay_transition_tiles.size(); ++i) {
		const terrain_type *overlay_transition_terrain = overlay_transition_tiles[i].terrain;
		if (overlay_transition_terrain->has_transition_mask()) {
			continue;
		}

		const bool is_overlay_transition_space = overlay_transition_terrain->has_flag(tile_flag::space);
		const std::shared_ptr<CPlayerColorGraphic> &overlay_transition_graphics = overlay_transition_terrain->get_transition_graphics(season);
		if (overlay_transition_graphics != nullptr) {
			const wyrmgus::time_of_day *overlay_transition_time_of_day = is_overlay_transition_space ? nullptr : time_of_day;

			const color_modification color_modification(overlay_transition_terrain->get_hue_rotation(), overlay_transition_terrain->get_colorization(), color_set(), player_color, overlay_transition_time_of_day);
			overlay_transition_graphics->render_frame(overlay_transition_tiles[i].tile_frame, pixel_pos, color_modification, render_commands);
		}
	}
}

void CViewport::draw_map_tile_border(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const
{
	const wyrmgus::player_color *player_color = tile->get_player_color();

	if (tile->get_owner() == nullptr) {
		return;
	}

	if (tile->get_ownership_border_tile() == -1) {
		return;
	}

	if (defines::get()->get_border_graphics() == nullptr) {
		return;
	}

	const std::shared_ptr<CPlayerColorGraphic> &border_graphics = defines::get()->get_border_graphics();
	const color_modification color_modification(0, colorization_type::none, color_set(), player_color, nullptr);
	border_graphics->render_frame(tile->get_ownership_border_tile(), pixel_pos + defines::get()->get_border_offset(), color_modification, false, false, defines::get()->get_border_opacity(), 100, render_commands);
}

/**
**  Draw the map backgrounds.
**
** StephanR: variables explained below for screen:<PRE>
** *---------------------------------------*
** |                                       |
** |        *-----------------------*      |<-TheUi.MapY,dy (in pixels)
** |        |   |   |   |   |   |   |      |        |
** |        |   |   |   |   |   |   |      |        |
** |        |---+---+---+---+---+---|      |        |
** |        |   |   |   |   |   |   |      |        |MapHeight (in tiles)
** |        |   |   |   |   |   |   |      |        |
** |        |---+---+---+---+---+---|      |        |
** |        |   |   |   |   |   |   |      |        |
** |        |   |   |   |   |   |   |      |        |
** |        *-----------------------*      |<-ey,UI.MapEndY (in pixels)
** |                                       |
** |                                       |
** *---------------------------------------*
**          ^                       ^
**        dx|-----------------------|ex,UI.MapEndX (in pixels)
**            UI.MapX MapWidth (in tiles)
** (in pixels)
** </PRE>
*/
void CViewport::draw_map(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	this->for_each_map_tile([this, &render_commands](const tile *tile, const QPoint &pixel_pos) {
		this->draw_map_tile(tile, pixel_pos, render_commands);
	});

	this->for_each_map_tile([this, &render_commands](const tile *tile, const QPoint &pixel_pos) {
		this->draw_map_tile_border(tile, pixel_pos, render_commands);
	});

	this->for_each_map_tile([this, &render_commands](const tile *tile, const QPoint &pixel_pos) {
		this->draw_map_tile_top(tile, pixel_pos, render_commands);
	});
}

/**
**  Draw a map viewport.
*/
void CViewport::Draw(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	PushClipping();
	this->SetClipping();

	/* this may take while */
	this->draw_map(render_commands);

	CurrentViewport = this;
	{
		// Now we need to sort units, missiles, particles by draw level and draw them
		std::vector<CUnit *> unittable;
		std::vector<CUnit *> celestial_bodies;
		std::vector<Missile *> missiletable;
		std::vector<CParticle *> particletable;

		FindAndSortUnits(*this, unittable);
		const size_t nunits = unittable.size();
		FindAndSortMissiles(*this, missiletable);
		const size_t nmissiles = missiletable.size();
		ParticleManager.prepareToDraw(*this, particletable);
		const size_t nparticles = particletable.size();

		//draw celestial body orbits beneath all units and missiles
		const QRect map_rect = this->get_map_rect();

		for (const CUnit *celestial_body : CMap::get()->get_orbiting_celestial_body_units()) {
			if (celestial_body->get_site() == nullptr) {
				continue;
			}

			const site *orbit_center = celestial_body->get_site()->get_orbit_center();
			if (orbit_center == nullptr) {
				continue;
			}

			const CUnit *orbit_center_unit = orbit_center->get_game_data()->get_site_unit();
			if (orbit_center_unit == nullptr) {
				continue;
			}

			const QPoint celestial_body_center_pixel_pos = celestial_body->get_scaled_map_pixel_pos_center();
			const QPoint orbit_center_pixel_pos = orbit_center_unit->get_scaled_map_pixel_pos_center();

			const int pixel_distance = point::distance_to(celestial_body_center_pixel_pos, orbit_center_pixel_pos);
			const QRect orbit_rect(orbit_center_pixel_pos - QPoint(pixel_distance, pixel_distance), orbit_center_pixel_pos + QPoint(pixel_distance, pixel_distance));

			if (!orbit_rect.intersects(map_rect)) {
				continue;
			}

			const QPoint orbit_center_screen_pixel_pos = this->scaled_map_to_screen_pixel_pos(orbit_center_pixel_pos);

			render_commands.push_back([orbit_center_screen_pixel_pos, pixel_distance](renderer *renderer) {
				renderer->draw_circle(orbit_center_screen_pixel_pos, pixel_distance, CVideo::GetRGBA(ColorBlue));
			});
		}
		
		size_t i = 0;
		size_t j = 0;
		size_t k = 0;

		while ((i < nunits && j < nmissiles) || (i < nunits && k < nparticles)
			   || (j < nmissiles && k < nparticles)) {
			if (i == nunits) {
				if (missiletable[j]->Type->get_draw_level() < particletable[k]->getDrawLevel()) {
					missiletable[j]->DrawMissile(*this, render_commands);
					++j;
				} else {
					particletable[k]->draw(render_commands);
					++k;
				}
			} else if (j == nmissiles) {
				if (unittable[i]->Type->get_draw_level() < particletable[k]->getDrawLevel()) {
					unittable[i]->Draw(*this, render_commands);
					++i;
				} else {
					particletable[k]->draw(render_commands);
					++k;
				}
			} else if (k == nparticles) {
				if (unittable[i]->Type->get_draw_level() < missiletable[j]->Type->get_draw_level()) {
					unittable[i]->Draw(*this, render_commands);
					++i;
				} else {
					missiletable[j]->DrawMissile(*this, render_commands);
					++j;
				}
			} else {
				if (unittable[i]->Type->get_draw_level() <= missiletable[j]->Type->get_draw_level()) {
					if (unittable[i]->Type->get_draw_level() < particletable[k]->getDrawLevel()) {
						unittable[i]->Draw(*this, render_commands);
						++i;
					} else {
						particletable[k]->draw(render_commands);
						++k;
					}
				} else {
					if (missiletable[j]->Type->get_draw_level() < particletable[k]->getDrawLevel()) {
						missiletable[j]->DrawMissile(*this, render_commands);
						++j;
					} else {
						particletable[k]->draw(render_commands);
						++k;
					}
				}
			}
		}
		for (; i < nunits; ++i) {
			unittable[i]->Draw(*this, render_commands);
		}
		for (; j < nmissiles; ++j) {
			missiletable[j]->DrawMissile(*this, render_commands);
		}
		for (; k < nparticles; ++k) {
			particletable[k]->draw(render_commands);
		}
		ParticleManager.endDraw();

		this->draw_map_fog_of_war(render_commands);

		//Wyrmgus start
		j = 0;
		for (; j < nmissiles; ++j) {
			if (!ClickMissile.empty() && ClickMissile == missiletable[j]->Type->get_identifier()) {
				missiletable[j]->DrawMissile(*this, render_commands); //draw click missile again to make it appear on top of the fog of war
			}
		}
		//Wyrmgus end
	}

	//
	// Draw orders of selected units.
	// Drawn here so that they are shown even when the unit is out of the screen.
	//
	if (!Preference.ShowOrders) {
	} else if (Preference.ShowOrders < 0
			   || (ShowOrdersCount >= GameCycle) || (stored_key_modifiers & Qt::ShiftModifier)) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			ShowOrder(*Selected[i], render_commands);
		}
	}
	
	DrawBorder(render_commands);
	PopClipping();
}

/**
**  Draw border around the viewport
*/
void CViewport::DrawBorder(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	// if we a single viewport, no need to denote the "selected" one
	if (UI.NumViewports == 1) {
		return;
	}

	uint32_t color = ColorBlack;
	if (this == UI.SelectedViewport) {
		color = ColorOrange;
	}

	const QSize pixel_size = this->get_pixel_size();
	Video.DrawRectangle(color, this->get_top_left_pos().x(), this->get_top_left_pos().y(), pixel_size.width() + 1, pixel_size.height() + 1, render_commands);
}
