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
/**@name viewport.h - The Viewport header file. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#include "vec2i.h"

class CUnit;

namespace wyrmgus {
	class renderer;
	class tile;
}

/**
**  A map viewport.
**
**  A part of the map displayed on screen.
**
**  CViewport::TopLeftPos
**  CViewport::BottomRightPos
**
**    upper left corner of this viewport is located at pixel
**    coordinates (TopLeftPosTopLeftPos) with respect to upper left corner of
**    stratagus's window, similarly lower right corner of this
**    viewport is (BottomRightPos) pixels away from the UL corner of
**    stratagus's window.
**
**  CViewport::MapX CViewport::MapY
**  CViewport::MapWidth CViewport::MapHeight
**
**    Tile coordinates of UL corner of this viewport with respect to
**    UL corner of the whole map.
**
**  CViewport::Unit
**
**    Viewport is bound to a unit. If the unit moves the viewport
**    changes the position together with the unit.
*/
class CViewport
{
public:
	CViewport();
	~CViewport();

	/// Check if pos pixels are within map area
	bool IsInsideMapArea(const PixelPos &screenPixelPos) const;

	/// Convert screen coordinates into map pixel coordinates
	PixelPos screen_to_map_pixel_pos(const PixelPos &screenPixelPos) const;
	PixelPos screen_to_scaled_map_pixel_pos(const PixelPos &screenPixelPos) const;
	// Convert map pixel coordinates into screen coordinates
	PixelPos map_to_screen_pixel_pos(const PixelPos &mapPixelPos) const;
	PixelPos scaled_map_to_screen_pixel_pos(const PixelPos &mapPixelPos) const;

	/// convert screen coordinate into tilepos
	Vec2i ScreenToTilePos(const PixelPos &screenPixelPos) const;
	/// convert tilepos coordonates into screen (take the top left of the tile)
	PixelPos TilePosToScreen_TopLeft(const Vec2i &tilePos) const;
	/// convert tilepos coordonates into screen (take the center of the tile)
	PixelPos TilePosToScreen_Center(const Vec2i &tilePos) const;
	QPoint screen_center_to_tile_pos() const;

	/// Set the current map view to x,y(upper,left corner)
	void Set(const Vec2i &tilePos, const PixelDiff &offset);
	/// Center map on point in viewport
	void Center(const PixelPos &mapPixelPos);

	void SetClipping() const;

	/// Draw the full Viewport.
	void Draw(std::vector<std::function<void(renderer *)>> &render_commands) const;
	void DrawBorder(std::vector<std::function<void(renderer *)>> &render_commands) const;
	/// Check if any part of an area is visible in viewport
	bool AnyMapAreaVisibleInViewport(const Vec2i &boxmin, const Vec2i &boxmax) const;

	bool Contains(const PixelPos &screenPos) const;

	void Restrict(int &screenPosX, int &screenPosY) const;

	PixelSize GetPixelSize() const;
	const PixelPos &GetTopLeftPos() const { return TopLeftPos;}
	const PixelPos &GetBottomRightPos() const { return BottomRightPos;}
private:
	/// Set the current map view to x,y(upper,left corner)
	void Set(const PixelPos &mapPixelPos);

	template <typename function_type>
	void for_each_map_tile(const function_type &function) const;

	void draw_map_tile(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const;
	void draw_map_tile_top(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const;
	void draw_map_tile_overlay_terrain(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const;
	void draw_map_tile_border(const tile *tile, const QPoint &pixel_pos, std::vector<std::function<void(renderer *)>> &render_commands) const;
	void draw_map(std::vector<std::function<void(renderer *)>> &render_commands) const;
	void draw_map_fog_of_war(std::vector<std::function<void(renderer *)>> &render_commands) const;

public:
	PixelPos TopLeftPos;      /// Screen pixel top-left corner
	PixelPos BottomRightPos;  /// Screen pixel bottom-right corner

public:
	Vec2i MapPos;             /// Map tile left-upper corner
	PixelDiff Offset;         /// Offset within MapX, MapY
	int MapWidth = 0;             /// Width in map tiles
	int MapHeight = 0;            /// Height in map tiles

	CUnit *Unit = nullptr;              /// Bound to this unit
};
