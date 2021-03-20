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
//      (c) Copyright 1999-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "map/map.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/world.h"
#include "map/world_game_data.h"
#include "missile.h"
#include "particle.h"
#include "pathfinder.h"
#include "player.h"
#include "translate.h"
#include "ui/cursor.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/size_util.h"
#include "util/vector_util.h"
#include "video/font.h"
#include "video/font_color.h"
#include "video/video.h"

CViewport::CViewport()
{
	this->TopLeftPos.x = this->TopLeftPos.y = 0;
	this->BottomRightPos.x = this->BottomRightPos.y = 0;
	this->MapPos.x = this->MapPos.y = 0;
	this->Offset.x = this->Offset.y = 0;
}

CViewport::~CViewport()
{
}

bool CViewport::Contains(const PixelPos &screenPos) const
{
	return this->GetTopLeftPos().x <= screenPos.x && screenPos.x <= this->GetBottomRightPos().x
		   && this->GetTopLeftPos().y <= screenPos.y && screenPos.y <= this->GetBottomRightPos().y;
}


void CViewport::Restrict(int &screenPosX, int &screenPosY) const
{
	screenPosX = std::clamp(screenPosX, this->GetTopLeftPos().x, this->GetBottomRightPos().x - 1);
	screenPosY = std::clamp(screenPosY, this->GetTopLeftPos().y, this->GetBottomRightPos().y - 1);
}

PixelSize CViewport::GetPixelSize() const
{
	return this->BottomRightPos - this->TopLeftPos;
}

void CViewport::SetClipping() const
{
	::SetClipping(this->TopLeftPos.x, this->TopLeftPos.y, this->BottomRightPos.x, this->BottomRightPos.y);
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
	Assert(boxmin.x <= boxmax.x && boxmin.y <= boxmax.y);

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

	return CMap::get()->Info.IsPointOnMap(tilePos, UI.CurrentMapLayer);
}

// Convert viewport coordinates into map pixel coordinates
PixelPos CViewport::screen_to_map_pixel_pos(const PixelPos &screenPixelPos) const
{
	return this->screen_to_scaled_map_pixel_pos(screenPixelPos) / wyrmgus::defines::get()->get_scale_factor();
}

PixelPos CViewport::screen_to_scaled_map_pixel_pos(const PixelPos &screenPixelPos) const
{
	const PixelDiff relPos = screenPixelPos - this->TopLeftPos + this->Offset;
	const PixelPos mapPixelPos = relPos + CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(this->MapPos);

	return mapPixelPos;
}

// Convert map pixel coordinates into viewport coordinates
PixelPos CViewport::map_to_screen_pixel_pos(const PixelPos &mapPixelPos) const
{
	return this->scaled_map_to_screen_pixel_pos(mapPixelPos * wyrmgus::defines::get()->get_scale_factor());
}

PixelPos CViewport::scaled_map_to_screen_pixel_pos(const PixelPos &mapPixelPos) const
{
	const PixelDiff relPos = mapPixelPos - CMap::get()->tile_pos_to_scaled_map_pixel_pos_top_left(this->MapPos);

	return this->TopLeftPos + relPos - this->Offset;
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

	return topLeft + wyrmgus::size::to_point(wyrmgus::defines::get()->get_scaled_tile_size()) / 2;
}

/// convert tilepos coordonates into screen (take the center of the tile)
QPoint CViewport::screen_center_to_tile_pos() const
{
	return this->ScreenToTilePos(this->TopLeftPos + this->GetPixelSize() / 2);
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

	const PixelSize pixelSize = this->GetPixelSize();

	x = std::min(x, (CMap::get()->Info.MapWidths.size() && UI.CurrentMapLayer ? UI.CurrentMapLayer->get_width() : CMap::get()->Info.MapWidth) * wyrmgus::defines::get()->get_scaled_tile_width() - (pixelSize.x) - 1 + UI.MapArea.ScrollPaddingRight);
	y = std::min(y, (CMap::get()->Info.MapHeights.size() && UI.CurrentMapLayer ? UI.CurrentMapLayer->get_height() : CMap::get()->Info.MapHeight) * wyrmgus::defines::get()->get_scaled_tile_height() - (pixelSize.y) - 1 + UI.MapArea.ScrollPaddingBottom);

	this->MapPos.x = x / wyrmgus::defines::get()->get_scaled_tile_width();
	if (x < 0 && x % wyrmgus::defines::get()->get_scaled_tile_width()) {
		this->MapPos.x--;
	}
	this->MapPos.y = y / wyrmgus::defines::get()->get_scaled_tile_height();
	if (y < 0 && y % wyrmgus::defines::get()->get_scaled_tile_height()) {
		this->MapPos.y--;
	}
	this->Offset.x = x % wyrmgus::defines::get()->get_scaled_tile_width();
	if (this->Offset.x < 0) {
		this->Offset.x += wyrmgus::defines::get()->get_scaled_tile_width();
	}
	this->Offset.y = y % wyrmgus::defines::get()->get_scaled_tile_height();
	if (this->Offset.y < 0) {
		this->Offset.y += wyrmgus::defines::get()->get_scaled_tile_height();
	}
	this->MapWidth = (pixelSize.x + this->Offset.x - 1) / wyrmgus::defines::get()->get_scaled_tile_width() + 1;
	this->MapHeight = (pixelSize.y + this->Offset.y - 1) / wyrmgus::defines::get()->get_scaled_tile_height() + 1;
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
	this->Set(mapPixelPos - this->GetPixelSize() / 2);
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
void CViewport::DrawMapBackgroundInViewport(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	int ex = this->BottomRightPos.x;
	int ey = this->BottomRightPos.y;
	int sy = this->MapPos.y;
	int dy = this->TopLeftPos.y - this->Offset.y;
	const int map_max = UI.CurrentMapLayer->get_width() * UI.CurrentMapLayer->get_height();

	while (sy  < 0) {
		sy++;
		dy += defines::get()->get_scaled_tile_height();
	}
	sy *=  UI.CurrentMapLayer->get_width();

	while (dy <= ey && sy  < map_max) {
		int sx = this->MapPos.x + sy;
		int dx = this->TopLeftPos.x - this->Offset.x;
		while (dx <= ex && (sx - sy < UI.CurrentMapLayer->get_width())) {
			if (sx - sy < 0) {
				++sx;
				dx += defines::get()->get_scaled_tile_width();
				continue;
			}
			const tile &mf = *UI.CurrentMapLayer->Field(sx);

			const terrain_type *terrain = nullptr;
			const terrain_type *overlay_terrain = nullptr;
			int solid_tile = 0;
			int overlay_solid_tile = 0;

			if (ReplayRevealMap) {
				terrain = mf.get_terrain();
				overlay_terrain = mf.get_overlay_terrain();
				solid_tile = mf.SolidTile;
				overlay_solid_tile = mf.OverlaySolidTile;
			} else {
				terrain = mf.player_info->SeenTerrain;
				overlay_terrain = mf.player_info->SeenOverlayTerrain;
				solid_tile = mf.player_info->SeenSolidTile;
				overlay_solid_tile = mf.player_info->SeenOverlaySolidTile;
			}

			const std::vector<tile_transition> &transition_tiles = ReplayRevealMap ? mf.TransitionTiles : mf.player_info->SeenTransitionTiles;
			const std::vector<tile_transition> &overlay_transition_tiles = ReplayRevealMap ? mf.OverlayTransitionTiles : mf.player_info->SeenOverlayTransitionTiles;

			const bool is_unpassable = overlay_terrain && overlay_terrain->has_flag(tile_flag::impassable) && !vector::contains(overlay_terrain->get_destroyed_tiles(), overlay_solid_tile);
			const bool is_space = terrain != nullptr && terrain->has_flag(tile_flag::space);

			const wyrmgus::time_of_day *time_of_day = nullptr;
			if (!is_space) {
				const bool is_underground = terrain != nullptr && terrain->has_flag(tile_flag::underground);
				if (is_underground) {
					time_of_day = defines::get()->get_underground_time_of_day();
				} else {
					time_of_day = mf.get_world() ? mf.get_world()->get_game_data()->get_time_of_day() : UI.CurrentMapLayer->GetTimeOfDay();
				}
			}

			const season *season = UI.CurrentMapLayer->get_tile_season(sx);

			const player_color *player_color = mf.get_player_color();

			if (terrain != nullptr) {
				const std::shared_ptr<CPlayerColorGraphic> &terrain_graphics = terrain->get_graphics(season);
				if (terrain_graphics != nullptr) {
					const int frame_index = solid_tile + (terrain == mf.get_terrain() ? mf.AnimationFrame : 0);
					terrain_graphics->DrawFrameClip(frame_index, dx, dy, time_of_day);
					terrain_graphics->render_frame(player_color, time_of_day, frame_index, QPoint(dx, dy), false, render_commands);
				}
			}

			for (size_t i = 0; i != transition_tiles.size(); ++i) {
				const terrain_type *transition_terrain = transition_tiles[i].terrain;
				const std::shared_ptr<CPlayerColorGraphic> &transition_terrain_graphics = transition_terrain->get_graphics(season);

				if (transition_terrain_graphics != nullptr) {
					const bool is_transition_space = transition_terrain != nullptr && transition_terrain->has_flag(tile_flag::space);

					const wyrmgus::time_of_day *transition_time_of_day = nullptr;
					if (!is_transition_space) {
						const bool is_transition_underground = transition_terrain->has_flag(tile_flag::underground);

						if (is_transition_underground) {
							transition_time_of_day = defines::get()->get_underground_time_of_day();
						} else {
							transition_time_of_day = mf.get_world() ? mf.get_world()->get_game_data()->get_time_of_day() : UI.CurrentMapLayer->GetTimeOfDay();
						}
					}

					transition_terrain_graphics->DrawFrameClip(transition_tiles[i].tile_frame, dx, dy, transition_time_of_day);
					transition_terrain_graphics->render_frame(player_color, transition_time_of_day, transition_tiles[i].tile_frame, QPoint(dx, dy), false, render_commands);
				}
			}

			if (mf.get_owner() != nullptr && mf.get_ownership_border_tile() != -1 && defines::get()->get_border_terrain_type() && is_unpassable) { //if the tile is not passable, draw the border under its overlay, but otherwise, draw the border over it
				const std::shared_ptr<CPlayerColorGraphic> &border_graphics = defines::get()->get_border_terrain_type()->get_graphics(season);
				if (border_graphics != nullptr) {
					border_graphics->DrawPlayerColorFrameClip(player_color, mf.get_ownership_border_tile(), dx, dy, nullptr, render_commands);
				}
			}

			if (overlay_terrain && (overlay_transition_tiles.size() == 0 || overlay_terrain->has_transition_mask())) {
				const bool is_overlay_space = overlay_terrain->has_flag(tile_flag::space);
				const std::shared_ptr<CPlayerColorGraphic> &overlay_terrain_graphics = overlay_terrain->get_graphics(season);
				if (overlay_terrain_graphics != nullptr) {
					const int frame_index = overlay_solid_tile + (overlay_terrain == mf.get_overlay_terrain() ? mf.OverlayAnimationFrame : 0);
					const wyrmgus::time_of_day *overlay_time_of_day = is_overlay_space ? nullptr : time_of_day;
					overlay_terrain_graphics->DrawPlayerColorFrameClip(player_color, frame_index, dx, dy, overlay_time_of_day, render_commands);
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
					overlay_transition_graphics->DrawPlayerColorFrameClip(player_color, overlay_transition_tiles[i].tile_frame, dx, dy, overlay_transition_time_of_day, render_commands);
				}
			}

			//if the tile is not passable, draw the border under its overlay, but otherwise, draw the border over it
			if (mf.get_owner() != nullptr && mf.get_ownership_border_tile() != -1 && defines::get()->get_border_terrain_type() && !is_unpassable) {
				const std::shared_ptr<CPlayerColorGraphic> &border_graphics = defines::get()->get_border_terrain_type()->get_graphics(season);
				if (border_graphics != nullptr) {
					border_graphics->DrawPlayerColorFrameClip(player_color, mf.get_ownership_border_tile(), dx, dy, nullptr, render_commands);
				}
			}

			for (size_t i = 0; i != overlay_transition_tiles.size(); ++i) {
				const terrain_type *overlay_transition_terrain = overlay_transition_tiles[i].terrain;
				if (overlay_transition_terrain->get_elevation_graphics()) {
					overlay_transition_terrain->get_elevation_graphics()->DrawFrameClip(overlay_transition_tiles[i].tile_frame, dx, dy, time_of_day);
					overlay_transition_terrain->get_elevation_graphics()->render_frame(player_color, time_of_day, overlay_transition_tiles[i].tile_frame, QPoint(dx, dy), false, render_commands);
				}
			}

			++sx;
			dx += defines::get()->get_scaled_tile_width();
		}
		sy += UI.CurrentMapLayer->get_width();
		dy += defines::get()->get_scaled_tile_height();
	}
}

/**
**  Draw a map viewport.
*/
void CViewport::Draw(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	PushClipping();
	this->SetClipping();

	/* this may take while */
	this->DrawMapBackgroundInViewport(render_commands);

	CurrentViewport = this;
	{
		// Now we need to sort units, missiles, particles by draw level and draw them
		std::vector<CUnit *> unittable;
		std::vector<Missile *> missiletable;
		std::vector<CParticle *> particletable;

		FindAndSortUnits(*this, unittable);
		const size_t nunits = unittable.size();
		FindAndSortMissiles(*this, missiletable);
		const size_t nmissiles = missiletable.size();
		ParticleManager.prepareToDraw(*this, particletable);
		const size_t nparticles = particletable.size();
		
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
					particletable[k]->draw();
					++k;
				}
			} else if (j == nmissiles) {
				if (unittable[i]->Type->get_draw_level() < particletable[k]->getDrawLevel()) {
					unittable[i]->Draw(*this, render_commands);
					++i;
				} else {
					particletable[k]->draw();
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
						particletable[k]->draw();
						++k;
					}
				} else {
					if (missiletable[j]->Type->get_draw_level() < particletable[k]->getDrawLevel()) {
						missiletable[j]->DrawMissile(*this, render_commands);
						++j;
					} else {
						particletable[k]->draw();
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
			particletable[k]->draw();
		}
		ParticleManager.endDraw();

		//Wyrmgus start
		//draw fog of war below the "click missile"
		this->DrawMapFogOfWar(render_commands);

		j = 0;
		for (; j < nmissiles; ++j) {
			if (!ClickMissile.empty() && ClickMissile == missiletable[j]->Type->Ident) {
				missiletable[j]->DrawMissile(*this, render_commands); //draw click missile again to make it appear on top of the fog of war
			}
		}
		//Wyrmgus end
	}

	//Wyrmgus start
//	this->DrawMapFogOfWar();
	//Wyrmgus end

	//
	// Draw orders of selected units.
	// Drawn here so that they are shown even when the unit is out of the screen.
	//
	if (!Preference.ShowOrders) {
	} else if (Preference.ShowOrders < 0
			   || (ShowOrdersCount >= GameCycle) || (KeyModifiers & ModifierShift)) {
		for (size_t i = 0; i != Selected.size(); ++i) {
			ShowOrder(*Selected[i], render_commands);
		}
	}
	
	//Wyrmgus start
	//if a selected unit has a rally point, show it
	//better to not show it all the time, so that there's no clutter
	/*
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (!Selected[i]Destroyed && !Selected[i]Removed && Selected[i]->get_rally_point_pos().x() != -1 && Selected[i]->get_rally_point_pos().y() != -1) {
			Video.FillCircleClip(ColorGreen, CurrentViewport->TilePosToScreen_Center(Selected[i]->get_rally_point_pos()), 3);
		}
	}
	*/
	//Wyrmgus end

	DrawBorder();
	PopClipping();
}

/**
**  Draw border around the viewport
*/
void CViewport::DrawBorder() const
{
	// if we a single viewport, no need to denote the "selected" one
	if (UI.NumViewports == 1) {
		return;
	}

	uint32_t color = ColorBlack;
	if (this == UI.SelectedViewport) {
		color = ColorOrange;
	}

	const PixelSize pixelSize = this->GetPixelSize();
	Video.DrawRectangle(color, this->TopLeftPos.x, this->TopLeftPos.y, pixelSize.x + 1, pixelSize.y + 1);
}
