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
//      (c) Copyright 1999-2005 by Lutz Sammer and Jimmy Salmon
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

#include "viewport.h"

#include "font.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "missile.h"
#include "pathfinder.h"
#include "player.h"
#include "translate.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "video.h"


CViewport::CViewport() : MapWidth(0), MapHeight(0), Unit(nullptr)
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
	clamp(&screenPosX, this->GetTopLeftPos().x, this->GetBottomRightPos().x - 1);
	clamp(&screenPosY, this->GetTopLeftPos().y, this->GetBottomRightPos().y - 1);
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

	return CMap::Map.Info.IsPointOnMap(tilePos, UI.CurrentMapLayer);
}

// Convert viewport coordinates into map pixel coordinates
PixelPos CViewport::ScreenToMapPixelPos(const PixelPos &screenPixelPos) const
{
	const PixelDiff relPos = screenPixelPos - this->TopLeftPos + this->Offset;
	const PixelPos mapPixelPos = relPos + CMap::Map.TilePosToMapPixelPos_TopLeft(this->MapPos, UI.CurrentMapLayer);

	return mapPixelPos;
}

// Convert map pixel coordinates into viewport coordinates
PixelPos CViewport::MapToScreenPixelPos(const PixelPos &mapPixelPos) const
{
	const PixelDiff relPos = mapPixelPos - CMap::Map.TilePosToMapPixelPos_TopLeft(this->MapPos, UI.CurrentMapLayer);

	return this->TopLeftPos + relPos - this->Offset;
}

/// convert screen coordinate into tilepos
Vec2i CViewport::ScreenToTilePos(const PixelPos &screenPixelPos) const
{
	const PixelPos mapPixelPos = ScreenToMapPixelPos(screenPixelPos);
	const Vec2i tilePos = CMap::Map.MapPixelPosToTilePos(mapPixelPos, UI.CurrentMapLayer->ID);

	return tilePos;
}

/// convert tilepos coordonates into screen (take the top left of the tile)
PixelPos CViewport::TilePosToScreen_TopLeft(const Vec2i &tilePos) const
{
	const PixelPos mapPos = CMap::Map.TilePosToMapPixelPos_TopLeft(tilePos, UI.CurrentMapLayer);

	return MapToScreenPixelPos(mapPos);
}

/// convert tilepos coordonates into screen (take the center of the tile)
PixelPos CViewport::TilePosToScreen_Center(const Vec2i &tilePos) const
{
	const PixelPos topLeft = TilePosToScreen_TopLeft(tilePos);

	return topLeft + CMap::Map.GetCurrentPixelTileSize() / 2;
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

	x = std::min(x, (CMap::Map.Info.MapWidths.size() && UI.CurrentMapLayer ? UI.CurrentMapLayer->GetWidth() : CMap::Map.Info.MapWidth) * CMap::Map.GetCurrentPixelTileSize().x - (pixelSize.x) - 1 + UI.MapArea.ScrollPaddingRight);
	y = std::min(y, (CMap::Map.Info.MapHeights.size() && UI.CurrentMapLayer ? UI.CurrentMapLayer->GetHeight() : CMap::Map.Info.MapHeight) * CMap::Map.GetCurrentPixelTileSize().y - (pixelSize.y) - 1 + UI.MapArea.ScrollPaddingBottom);

	this->MapPos.x = x / CMap::Map.GetCurrentPixelTileSize().x;
	if (x < 0 && x % CMap::Map.GetCurrentPixelTileSize().x) {
		this->MapPos.x--;
	}
	this->MapPos.y = y / CMap::Map.GetCurrentPixelTileSize().y;
	if (y < 0 && y % CMap::Map.GetCurrentPixelTileSize().y) {
		this->MapPos.y--;
	}
	this->Offset.x = x % CMap::Map.GetCurrentPixelTileSize().x;
	if (this->Offset.x < 0) {
		this->Offset.x += CMap::Map.GetCurrentPixelTileSize().x;
	}
	this->Offset.y = y % CMap::Map.GetCurrentPixelTileSize().y;
	if (this->Offset.y < 0) {
		this->Offset.y += CMap::Map.GetCurrentPixelTileSize().y;
	}
	this->MapWidth = (pixelSize.x + this->Offset.x - 1) / CMap::Map.GetCurrentPixelTileSize().x + 1;
	this->MapHeight = (pixelSize.y + this->Offset.y - 1) / CMap::Map.GetCurrentPixelTileSize().y + 1;
}

/**
**  Change viewpoint of map viewport v to tilePos.
**
**  @param tilePos  map tile position.
**  @param offset   offset in tile.
*/
void CViewport::Set(const Vec2i &tilePos, const PixelDiff &offset)
{
	const PixelPos mapPixelPos = CMap::Map.TilePosToMapPixelPos_TopLeft(tilePos, UI.CurrentMapLayer) + offset;

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
void CViewport::DrawMapBackgroundInViewport() const
{
	int ex = this->BottomRightPos.x;
	int ey = this->BottomRightPos.y;
	int sy = this->MapPos.y;
	int dy = this->TopLeftPos.y - this->Offset.y;
	const int map_max = UI.CurrentMapLayer->GetWidth() * UI.CurrentMapLayer->GetHeight();
	const CSeason *season = CMap::Map.MapLayers[UI.CurrentMapLayer->ID]->GetSeason();

	while (sy  < 0) {
		sy++;
		dy += CMap::Map.GetCurrentPixelTileSize().y;
	}
	sy *=  UI.CurrentMapLayer->GetWidth();

	while (dy <= ey && sy  < map_max) {
		int sx = this->MapPos.x + sy;
		int dx = this->TopLeftPos.x - this->Offset.x;
		while (dx <= ex && (sx - sy < UI.CurrentMapLayer->GetWidth())) {
			if (sx - sy < 0) {
				++sx;
				dx += CMap::Map.GetCurrentPixelTileSize().x;
				continue;
			}
			const CMapField &mf = *UI.CurrentMapLayer->Field(sx);
			//Wyrmgus start
			/*
			unsigned short int tile;
			if (ReplayRevealMap) {
				tile = mf.getGraphicTile();
			} else {
				tile = mf.playerInfo.SeenTile;
			}
			*/
			//Wyrmgus end
			//Wyrmgus start
//			CMap::Map.TileGraphic->DrawFrameClip(tile, dx, dy);
			if (ReplayRevealMap) {
				bool is_unpassable = mf.OverlayTerrain && (mf.OverlayTerrain->Flags & MapFieldUnpassable) && std::find(mf.OverlayTerrain->DestroyedTiles.begin(), mf.OverlayTerrain->DestroyedTiles.end(), mf.OverlaySolidTile) == mf.OverlayTerrain->DestroyedTiles.end();
				if (mf.Terrain && mf.Terrain->GetGraphics(season)) {
					mf.Terrain->GetGraphics(season)->DrawFrameClip(mf.SolidTile + (mf.Terrain == mf.Terrain ? mf.AnimationFrame : 0), dx, dy, false);
				}
				for (size_t i = 0; i != mf.TransitionTiles.size(); ++i) {
					if (mf.TransitionTiles[i].first->GetGraphics(season)) {
						mf.TransitionTiles[i].first->GetGraphics(season)->DrawFrameClip(mf.TransitionTiles[i].second, dx, dy, false);
					}
				}
				if (mf.Owner != -1 && mf.OwnershipBorderTile != -1 && CMap::Map.BorderTerrain && is_unpassable) { //if the tile is not passable, draw the border under its overlay, but otherwise, draw the border over it
					if (CMap::Map.BorderTerrain->GetGraphics(season)) {
						CMap::Map.BorderTerrain->GetGraphics(season)->DrawFrameClip(mf.OwnershipBorderTile, dx, dy, false);
					}
					if (CMap::Map.BorderTerrain->PlayerColorGraphics) {
						CMap::Map.BorderTerrain->PlayerColorGraphics->DrawPlayerColorFrameClip(mf.Owner, mf.OwnershipBorderTile, dx, dy, false);
					}
				}
				if (mf.OverlayTerrain && mf.OverlayTransitionTiles.size() == 0) {
					if (mf.OverlayTerrain->GetGraphics(season)) {
						mf.OverlayTerrain->GetGraphics(season)->DrawFrameClip(mf.OverlaySolidTile + (mf.OverlayTerrain == mf.OverlayTerrain ? mf.OverlayAnimationFrame : 0), dx, dy, false);
					}
					if (mf.OverlayTerrain->PlayerColorGraphics) {
						mf.OverlayTerrain->PlayerColorGraphics->DrawPlayerColorFrameClip((mf.Owner != -1) ? mf.Owner : PlayerNumNeutral, mf.OverlaySolidTile + (mf.OverlayTerrain == mf.OverlayTerrain ? mf.OverlayAnimationFrame : 0), dx, dy, false);
					}
				}
				for (size_t i = 0; i != mf.OverlayTransitionTiles.size(); ++i) {
					if (mf.OverlayTransitionTiles[i].first->GetGraphics(season)) {
						mf.OverlayTransitionTiles[i].first->GetGraphics(season)->DrawFrameClip(mf.OverlayTransitionTiles[i].second, dx, dy, false);
					}
					if (mf.OverlayTransitionTiles[i].first->PlayerColorGraphics) {
						mf.OverlayTransitionTiles[i].first->PlayerColorGraphics->DrawPlayerColorFrameClip((mf.Owner != -1) ? mf.Owner : PlayerNumNeutral, mf.OverlayTransitionTiles[i].second, dx, dy, false);
					}
				}
				if (mf.Owner != -1 && mf.OwnershipBorderTile != -1 && CMap::Map.BorderTerrain && !is_unpassable) { //if the tile is not passable, draw the border under its overlay, but otherwise, draw the border over it
					if (CMap::Map.BorderTerrain->GetGraphics(season)) {
						CMap::Map.BorderTerrain->GetGraphics(season)->DrawFrameClip(mf.OwnershipBorderTile, dx, dy, false);
					}
					if (CMap::Map.BorderTerrain->PlayerColorGraphics) {
						CMap::Map.BorderTerrain->PlayerColorGraphics->DrawPlayerColorFrameClip(mf.Owner, mf.OwnershipBorderTile, dx, dy, false);
					}
				}
				for (size_t i = 0; i != mf.OverlayTransitionTiles.size(); ++i) {
					if (mf.OverlayTransitionTiles[i].first->ElevationGraphics) {
						mf.OverlayTransitionTiles[i].first->ElevationGraphics->DrawFrameClip(mf.OverlayTransitionTiles[i].second, dx, dy, false);
					}
				}
			} else {
				bool is_unpassable_seen = mf.playerInfo.SeenOverlayTerrain && (mf.playerInfo.SeenOverlayTerrain->Flags & MapFieldUnpassable) && std::find(mf.playerInfo.SeenOverlayTerrain->DestroyedTiles.begin(), mf.playerInfo.SeenOverlayTerrain->DestroyedTiles.end(), mf.playerInfo.SeenOverlaySolidTile) == mf.playerInfo.SeenOverlayTerrain->DestroyedTiles.end();
				if (mf.playerInfo.SeenTerrain && mf.playerInfo.SeenTerrain->GetGraphics(season)) {
					mf.playerInfo.SeenTerrain->GetGraphics(season)->DrawFrameClip(mf.playerInfo.SeenSolidTile + (mf.playerInfo.SeenTerrain == mf.Terrain ? mf.AnimationFrame : 0), dx, dy, false);
				}
				for (size_t i = 0; i != mf.playerInfo.SeenTransitionTiles.size(); ++i) {
					if (mf.playerInfo.SeenTransitionTiles[i].first->GetGraphics(season)) {
						mf.playerInfo.SeenTransitionTiles[i].first->GetGraphics(season)->DrawFrameClip(mf.playerInfo.SeenTransitionTiles[i].second, dx, dy, false);
					}
				}
				if (mf.Owner != -1 && mf.OwnershipBorderTile != -1 && CMap::Map.BorderTerrain && is_unpassable_seen) {
					if (CMap::Map.BorderTerrain->GetGraphics(season)) {
						CMap::Map.BorderTerrain->GetGraphics(season)->DrawFrameClip(mf.OwnershipBorderTile, dx, dy, false);
					}
					if (CMap::Map.BorderTerrain->PlayerColorGraphics) {
						CMap::Map.BorderTerrain->PlayerColorGraphics->DrawPlayerColorFrameClip(mf.Owner, mf.OwnershipBorderTile, dx, dy, false);
					}
				}
				if (mf.playerInfo.SeenOverlayTerrain && mf.playerInfo.SeenOverlayTransitionTiles.size() == 0) {
					if (mf.playerInfo.SeenOverlayTerrain->GetGraphics(season)) {
						mf.playerInfo.SeenOverlayTerrain->GetGraphics(season)->DrawFrameClip(mf.playerInfo.SeenOverlaySolidTile + (mf.playerInfo.SeenOverlayTerrain == mf.OverlayTerrain ? mf.OverlayAnimationFrame : 0), dx, dy, false);
					}
					if (mf.playerInfo.SeenOverlayTerrain->PlayerColorGraphics) {
						mf.playerInfo.SeenOverlayTerrain->PlayerColorGraphics->DrawPlayerColorFrameClip((mf.Owner != -1) ? mf.Owner : PlayerNumNeutral, mf.playerInfo.SeenOverlaySolidTile + (mf.playerInfo.SeenOverlayTerrain == mf.OverlayTerrain ? mf.OverlayAnimationFrame : 0), dx, dy, false);
					}
				}
				for (size_t i = 0; i != mf.playerInfo.SeenOverlayTransitionTiles.size(); ++i) {
					if (mf.playerInfo.SeenOverlayTransitionTiles[i].first->GetGraphics(season)) {
						mf.playerInfo.SeenOverlayTransitionTiles[i].first->GetGraphics(season)->DrawFrameClip(mf.playerInfo.SeenOverlayTransitionTiles[i].second, dx, dy, false);
					}
					if (mf.playerInfo.SeenOverlayTransitionTiles[i].first->PlayerColorGraphics) {
						mf.playerInfo.SeenOverlayTransitionTiles[i].first->PlayerColorGraphics->DrawPlayerColorFrameClip((mf.Owner != -1) ? mf.Owner : PlayerNumNeutral, mf.playerInfo.SeenOverlayTransitionTiles[i].second, dx, dy, false);
					}
				}
				if (mf.Owner != -1 && mf.OwnershipBorderTile != -1 && CMap::Map.BorderTerrain && !is_unpassable_seen) {
					if (CMap::Map.BorderTerrain->GetGraphics(season)) {
						CMap::Map.BorderTerrain->GetGraphics(season)->DrawFrameClip(mf.OwnershipBorderTile, dx, dy, false);
					}
					if (CMap::Map.BorderTerrain->PlayerColorGraphics) {
						CMap::Map.BorderTerrain->PlayerColorGraphics->DrawPlayerColorFrameClip(mf.Owner, mf.OwnershipBorderTile, dx, dy, false);
					}
				}
				for (size_t i = 0; i != mf.playerInfo.SeenOverlayTransitionTiles.size(); ++i) {
					if (mf.playerInfo.SeenOverlayTransitionTiles[i].first->ElevationGraphics) {
						mf.playerInfo.SeenOverlayTransitionTiles[i].first->ElevationGraphics->DrawFrameClip(mf.playerInfo.SeenOverlayTransitionTiles[i].second, dx, dy, false);
					}
				}
			}
			//Wyrmgus end
			++sx;
			dx += CMap::Map.GetCurrentPixelTileSize().x;
		}
		sy += UI.CurrentMapLayer->GetWidth();
		dy += CMap::Map.GetCurrentPixelTileSize().y;
	}
}

/**
**  Show unit's name under cursor or print the message if territory is invisible.
**
**  @param pos  Mouse position.
**  @param unit  Unit to show name.
**  @param hidden  If true, write "Unrevealed terrain"
**
*/
static void ShowUnitName(const CViewport &vp, PixelPos pos, CUnit *unit, bool hidden = false)
{
	CFont &font = GetSmallFont();
	int width;
	int height = font.Height() + 6;
	CLabel label(font, "white", "red");
	int x;
	int y = std::min<int>(GameCursor->G->Height + pos.y + 10, vp.BottomRightPos.y - 1 - height);
	const CPlayer *tplayer = CPlayer::GetThisPlayer();

	if (unit && unit->IsAliveOnMap()) {
		int backgroundColor;
		if (unit->Player->Index == (*tplayer).Index) {
			backgroundColor = Video.MapRGB(TheScreen->format, 0, 0, 252);
		} else if (unit->Player->IsAllied(*tplayer)) {
			backgroundColor = Video.MapRGB(TheScreen->format, 0, 176, 0);
		} else if (unit->Player->IsEnemy(*tplayer)) {
			backgroundColor = Video.MapRGB(TheScreen->format, 252, 0, 0);
		} else {
			backgroundColor = Video.MapRGB(TheScreen->format, 176, 176, 176);
		}
		//Wyrmgus start
//		width = font.getWidth(unit->Type->Name) + 10;
		width = font.getWidth(unit->GetTypeName()) + 10;
		//Wyrmgus end
		x = std::min<int>(GameCursor->G->Width + pos.x, vp.BottomRightPos.x - 1 - width);
		Video.FillTransRectangle(backgroundColor, x, y, width, height, 128);
		Video.DrawRectangle(ColorWhite, x, y, width, height);
		//Wyrmgus start
//		label.DrawCentered(x + width / 2, y + 3, unit->Type->Name);
		label.DrawCentered(x + width / 2, y + 3, unit->GetTypeName());
		//Wyrmgus end
	} else if (hidden) {
		const std::string str("Unrevealed terrain");
		width = font.getWidth(str) + 10;
		x = std::min<int>(GameCursor->G->Width + pos.x, vp.BottomRightPos.x - 1 - width);
		Video.FillTransRectangle(ColorBlue, x, y, width, height, 128);
		Video.DrawRectangle(ColorWhite, x, y, width, height);
		label.DrawCentered(x + width / 2, y + 3, str);
	}
}

/**
**  Draw a map viewport.
*/
void CViewport::Draw() const
{
	PushClipping();
	this->SetClipping();

	/* this may take while */
	this->DrawMapBackgroundInViewport();

	CurrentViewport = this;
	{
		// Now we need to sort units and missiles by draw level and draw them
		std::vector<CUnit *> unittable;
		std::vector<Missile *> missiletable;

		FindAndSortUnits(*this, unittable);
		const size_t nunits = unittable.size();
		FindAndSortMissiles(*this, missiletable);
		const size_t nmissiles = missiletable.size();
		
		size_t i = 0;
		size_t j = 0;


		while ((i < nunits && j < nmissiles)) {
			if (unittable[i]->Type->DrawLevel <= missiletable[j]->Type->DrawLevel) {
				unittable[i]->Draw(*this);
				++i;
			} else {
				missiletable[j]->DrawMissile(*this);
				++j;
			}
		}
		for (; i < nunits; ++i) {
			unittable[i]->Draw(*this);
		}
		for (; j < nmissiles; ++j) {
			missiletable[j]->DrawMissile(*this);
		}
		//Wyrmgus start
		//draw fog of war below the "click missile"
		this->DrawMapFogOfWar();
		j = 0;
		for (; j < nmissiles; ++j) {
			if (!ClickMissile.empty() && ClickMissile == missiletable[j]->Type->Ident) {
				missiletable[j]->DrawMissile(*this); //draw click missile again to make it appear on top of the fog of war
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
			ShowOrder(*Selected[i]);
		}
	}
	
	//Wyrmgus start
	//if a selected unit has a rally point, show it
	//better to not show it all the time, so that there's no clutter
	/*
	for (size_t i = 0; i != Selected.size(); ++i) {
		if (!Selected[i]Destroyed && !Selected[i]Removed && Selected[i]->RallyPointPos.x != -1 && Selected[i]->RallyPointPos.y != -1) {
			Video.FillCircleClip(ColorGreen, CurrentViewport->TilePosToScreen_Center(Selected[i]->RallyPointPos), 3);
		}
	}
	*/
	//Wyrmgus end

	//
	// Draw unit's name popup
	//
	//Wyrmgus start
	/*
	//Wyrmgus start
//	if (CursorOn == CursorOnMap && Preference.ShowNameDelay && (ShowNameDelay < GameCycle) && (GameCycle < ShowNameTime)) {
	if (CursorOn == CursorOnMap && (!Preference.ShowNameDelay || ShowNameDelay < GameCycle) && (!Preference.ShowNameTime || GameCycle < ShowNameTime)) {
	//Wyrmgus end
		const Vec2i tilePos = this->ScreenToTilePos(CursorScreenPos);
		//Wyrmgus start
//		const bool isMapFieldVisile = CMap::Map.Field(tilePos)->playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer());
		const bool isMapFieldVisile = CMap::Map.Field(tilePos, UI.CurrentMapLayer->ID)->playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer());
		//Wyrmgus end

		if (UI.MouseViewport->IsInsideMapArea(CursorScreenPos) && UnitUnderCursor
			//Wyrmgus start
//			&& ((isMapFieldVisile && !UnitUnderCursor->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value) || ReplayRevealMap)) {
			&& ((isMapFieldVisile && !UnitUnderCursor->Type->BoolFlag[ISNOTSELECTABLE_INDEX].value) || ReplayRevealMap) && UnitUnderCursor->IsAliveOnMap()) {
//			ShowUnitName(*this, CursorScreenPos, UnitUnderCursor);
			PixelPos unit_center_pos = CMap::Map.TilePosToMapPixelPos_TopLeft(UnitUnderCursor->tilePos, UnitUnderCursor->MapLayer);
			unit_center_pos = MapToScreenPixelPos(unit_center_pos);
			std::string unit_name;
			if (UnitUnderCursor->Unique || UnitUnderCursor->Prefix || UnitUnderCursor->Suffix || UnitUnderCursor->Work || UnitUnderCursor->Elixir || UnitUnderCursor->Spell || UnitUnderCursor->Character != nullptr) {
				if (!UnitUnderCursor->Identified) {
					unit_name = UnitUnderCursor->GetTypeName() + " (" + _("Unidentified") + ")";
				} else {
					unit_name = UnitUnderCursor->GetName();
				}
			} else {
				unit_name = UnitUnderCursor->GetTypeName();
			}
			if (UnitUnderCursor->Player->Index != PlayerNumNeutral) {
				unit_name += " (" + UnitUnderCursor->Player->Name + ")";
			}
			//hackish way to make the popup appear correctly for the unit under cursor
			ButtonAction *ba = new ButtonAction;
			ba->Hint = unit_name;
			ba->Action = ButtonUnit;
			ba->Value = UnitNumber(*UnitUnderCursor);
			ba->Popup = "popup-unit-under-cursor";
			DrawPopup(*ba, unit_center_pos.x, unit_center_pos.y);
			delete ba;
			LastDrawnButtonPopup = nullptr;
			//Wyrmgus end
		//Wyrmgus start
//		} else if (!isMapFieldVisile) {
//			ShowUnitName(*this, CursorScreenPos, nullptr, true);
		//Wyrmgus end
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

	Uint32 color = ColorBlack;
	if (this == UI.SelectedViewport) {
		color = ColorOrange;
	}

	const PixelSize pixelSize = this->GetPixelSize();
	Video.DrawRectangle(color, this->TopLeftPos.x, this->TopLeftPos.y, pixelSize.x + 1, pixelSize.y + 1);
}
