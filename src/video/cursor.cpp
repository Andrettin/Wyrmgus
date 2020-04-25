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
//      (c) Copyright 1998-2020 by Lutz Sammer, Nehal Mistry,
//                                 Jimmy Salmon and Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "cursor.h"
#include "intern_video.h"

#include "civilization.h"
#include "database/defines.h"
#include "editor.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/tileset.h"
#include "translate.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/**
**  Define cursor-types.
**
**  @todo FIXME: Should this be move to ui part?
*/
static std::vector<CCursor *> AllCursors;

CursorState CurrentCursorState;    /// current cursor state (point,...)
ButtonCmd CursorAction;            /// action for selection
int CursorValue;             /// value for CursorAction (spell type f.e.)
std::string CustomCursor;    /// custom cursor for button

// Event changed mouse position, can alter at any moment
PixelPos CursorScreenPos;    /// cursor position on screen
PixelPos CursorStartScreenPos;  /// rectangle started on screen
PixelPos CursorStartMapPos;/// position of starting point of selection rectangle, in Map pixels.


/*--- DRAW BUILDING  CURSOR ------------------------------------------------*/
CUnitType *CursorBuilding;           /// building cursor


/*--- DRAW SPRITE CURSOR ---------------------------------------------------*/
CCursor *GameCursor;                 /// current shown cursor-type

static SDL_Surface *HiddenSurface;
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Get the amount of cursors sprites to load
*/
int GetCursorsCount(const std::string &race)
{
	int count = 0;
	for (std::vector<CCursor *>::iterator i = AllCursors.begin(); i != AllCursors.end(); ++i) {
		CCursor &cursor = **i;

		//  Only load cursors of this race or universal cursors.
		if (!cursor.Race.empty() && cursor.Race != race) {
			continue;
		}

		if (cursor.G && !cursor.G->IsLoaded()) {
			count++;
		}
	}

	return count;
}

/**
**  Load all cursor sprites.
**
**  @param race  Cursor graphics of this race to load.
*/
//Wyrmgus start
//void LoadCursors(const std::string &race)
void LoadCursors(const std::string civilization_name)
//Wyrmgus end
{
	ShowLoadProgress("%s", _("Loading Cursors"));
			
	for (std::vector<CCursor *>::iterator i = AllCursors.begin(); i != AllCursors.end(); ++i) {
		CCursor &cursor = **i;

		//  Only load cursors of this race or universal cursors.
		//Wyrmgus start
//		if (!cursor.Race.empty() && cursor.Race != race) {
		if (!civilization_name.empty() && !cursor.Race.empty() && cursor.Race != civilization_name) {
		//Wyrmgus end
			continue;
		}

		if (cursor.G && !cursor.G->IsLoaded()) {
			UpdateLoadProgress();
			cursor.G->Load(false, stratagus::defines::get()->get_scale_factor());
			cursor.G->UseDisplayFormat();

			IncItemsLoaded();
		}
	}
}

/**
**  Find the cursor of this identifier.
**
**  @param ident  Identifier for the cursor (from config files).
**
**  @return       Returns the matching cursor.
**
**  @note If we have more cursors, we should add hash to find them faster.
*/
CCursor *CursorByIdent(const std::string &ident)
{
	CCursor *found_cursor = nullptr;
	for (std::vector<CCursor *>::iterator i = AllCursors.begin(); i != AllCursors.end(); ++i) {
		CCursor &cursor = **i;

		if (cursor.Ident != ident || !cursor.G->IsLoaded()) {
			continue;
		}

		if (!CPlayer::GetThisPlayer() && cursor.Race != stratagus::civilization::get_all()[CPlayer::Players[0]->Race]->get_identifier() && !cursor.Race.empty()) {
			continue;
		}
		
		if (cursor.Race.empty() || !CPlayer::GetThisPlayer() || cursor.Race == stratagus::civilization::get_all()[CPlayer::GetThisPlayer()->Race]->get_identifier()) {
			found_cursor = &cursor;
			if (!cursor.Race.empty()) { //if this is a generic cursor, keep searching for a civilization-specific one; stop searching otherwise
				break;
			}
		}
	}
	
	if (!found_cursor) {
		DebugPrint("Cursor '%s' not found, please check your code.\n" _C_ ident.c_str());
	}
	
	return found_cursor;
}

/**
**  Draw rectangle cursor when visible
**
**  @param corner1   Screen start position of rectangle
**  @param corner2   Screen end position of rectangle
*/
static void DrawVisibleRectangleCursor(PixelPos corner1, PixelPos corner2)
{
	const CViewport &vp = *UI.SelectedViewport;

	//  Clip to map window.
	//  FIXME: should re-use CLIP_RECTANGLE in some way from linedraw.c ?
	vp.Restrict(corner2.x, corner2.y);

	if (corner1.x > corner2.x) {
		std::swap(corner1.x, corner2.x);
	}
	if (corner1.y > corner2.y) {
		std::swap(corner1.y, corner2.y);
	}
	const int w = corner2.x - corner1.x + 1;
	const int h = corner2.y - corner1.y + 1;

	Video.DrawRectangleClip(ColorGreen, corner1.x, corner1.y, w, h);
}

/**
**  Draw cursor for selecting building position.
*/
//Wyrmgus start
//static void DrawBuildingCursor()
void DrawBuildingCursor()
//Wyrmgus end
{
	// Align to grid
	const CViewport &vp = *UI.MouseViewport;
	const Vec2i mpos = vp.ScreenToTilePos(CursorScreenPos);
	const PixelPos screenPos = vp.TilePosToScreen_TopLeft(mpos);

	CUnit *ontop = nullptr;

	//
	//  Draw building
	//
#ifdef DYNAMIC_LOAD
	if (!CursorBuilding->G->IsLoaded()) {
		LoadUnitTypeSprite(CursorBuilding);
	}
#endif
	PushClipping();
	vp.SetClipping();

	const QPoint center_tile_pos = mpos + CursorBuilding->get_tile_center_pos_offset();
	const bool is_underground = UI.CurrentMapLayer->Field(center_tile_pos)->Flags & MapFieldUnderground;
	const stratagus::time_of_day *time_of_day = is_underground ? stratagus::defines::get()->get_underground_time_of_day() : UI.CurrentMapLayer->GetTimeOfDay();

//	DrawShadow(*CursorBuilding, CursorBuilding->StillFrame, screenPos);
	if (CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer()) && CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->ShadowSprite) {
		DrawShadow(*CursorBuilding, CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->ShadowSprite, CursorBuilding->StillFrame, screenPos);
	} else if (CursorBuilding->ShadowSprite) {
		DrawShadow(*CursorBuilding, CursorBuilding->ShadowSprite, CursorBuilding->StillFrame, screenPos);
	}
	//Wyrmgus end
	//Wyrmgus start
	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), BackpackImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), MountImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

//	DrawUnitType(*CursorBuilding, CursorBuilding->Sprite, CPlayer::GetThisPlayer()->Index,
//				 CursorBuilding->StillFrame, screenPos);
	// get the first variation which has the proper upgrades for this player (to have the proper appearance of buildings drawn in the cursor, according to the upgrades)
	if (CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer()) && CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->Sprite) {
		DrawUnitType(*CursorBuilding, CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->Sprite, CPlayer::GetThisPlayer()->Index,
				CursorBuilding->StillFrame, screenPos, time_of_day);
	} else {
		DrawUnitType(*CursorBuilding, CursorBuilding->Sprite, CPlayer::GetThisPlayer()->Index,
				CursorBuilding->StillFrame, screenPos, time_of_day);
	}
	//Wyrmgus end
	
	//Wyrmgus start
	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), HairImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), PantsImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), HelmetImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), BootsImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), LeftArmImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingLeftArmImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ShieldImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);
	
	if (CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightHandImageLayer) != nullptr) {
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightArmImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingRightArmImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), WeaponImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);
		
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightHandImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);
	} else {
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), WeaponImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), RightArmImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(CPlayer::GetThisPlayer(), ClothingRightArmImageLayer), CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);
	}

	if (CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer()) && CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->LightSprite) {
		DrawOverlay(*CursorBuilding, CursorBuilding->GetDefaultVariation(CPlayer::GetThisPlayer())->LightSprite, CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);
	} else if (CursorBuilding->LightSprite) {
		DrawOverlay(*CursorBuilding, CursorBuilding->LightSprite, CPlayer::GetThisPlayer()->Index, CursorBuilding->StillFrame, screenPos, time_of_day);
	}
	//Wyrmgus end
	
	if (CursorBuilding->CanAttack && CursorBuilding->Stats->Variables[ATTACKRANGE_INDEX].Value > 0) {
		const PixelPos center(screenPos + CursorBuilding->get_scaled_half_tile_pixel_size());
		const int radius = (CursorBuilding->Stats->Variables[ATTACKRANGE_INDEX].Max + (CursorBuilding->TileSize.x - 1)) * stratagus::defines::get()->get_scaled_tile_width() + 1;
		Video.DrawCircleClip(ColorRed, center.x, center.y, radius);
	}

	//
	//  Draw the allow overlay
	//
	int f;
	if (!Selected.empty()) {
		f = 1;
		for (size_t i = 0; f && i < Selected.size(); ++i) {
			//Wyrmgus start
//			f = ((ontop = CanBuildHere(Selected[i], *CursorBuilding, mpos)) != nullptr);
			f = ((ontop = CanBuildHere(Selected[i], *CursorBuilding, mpos, UI.CurrentMapLayer->ID)) != nullptr);
			//Wyrmgus end
			// Assign ontop or null
			ontop = (ontop == Selected[i] ? nullptr : ontop);
		}
	} else {
		f = ((ontop = CanBuildHere(NoUnitP, *CursorBuilding, mpos, UI.CurrentMapLayer->ID)) != nullptr);
		if (!Editor.Running || ontop == (CUnit *)1) {
			ontop = nullptr;
		}
	}

	const int mask = CursorBuilding->MovementMask;
	int h = CursorBuilding->TileSize.y;
	// reduce to view limits
	h = std::min(h, vp.MapPos.y + vp.MapHeight - mpos.y);
	int w0 = CursorBuilding->TileSize.x;
	w0 = std::min(w0, vp.MapPos.x + vp.MapWidth - mpos.x);

	while (h--) {
		int w = w0;
		while (w--) {
			const Vec2i posIt(mpos.x + w, mpos.y + h);
			Uint32 color;

			if (f && (ontop ||
					  CanBuildOn(posIt, MapFogFilterFlags(*CPlayer::GetThisPlayer(), posIt,
														  mask & ((!Selected.empty() && Selected[0]->tilePos == posIt) ?
																  //Wyrmgus start
//																  ~(MapFieldLandUnit | MapFieldSeaUnit) : -1))))
																  ~(MapFieldLandUnit | MapFieldSeaUnit) : -1), UI.CurrentMapLayer->ID), UI.CurrentMapLayer->ID))
																  //Wyrmgus end
				&& UI.CurrentMapLayer->Field(posIt)->playerInfo.IsTeamExplored(*CPlayer::GetThisPlayer())) {
				color = ColorGreen;
			} else {
				color = ColorRed;
			}
			Video.FillTransRectangleClip(color, screenPos.x + w * stratagus::defines::get()->get_scaled_tile_width(),
										 screenPos.y + h * stratagus::defines::get()->get_scaled_tile_height(), stratagus::defines::get()->get_scaled_tile_width(), stratagus::defines::get()->get_scaled_tile_height(), 95);
		}
	}
	PopClipping();
}


/**
**  Draw the cursor.
*/
void DrawCursor()
{
	// Selecting rectangle
	if (CurrentCursorState == CursorState::Rectangle && CursorStartScreenPos != CursorScreenPos) {
		const PixelPos cursorStartScreenPos = UI.MouseViewport->scaled_map_to_screen_pixel_pos(CursorStartMapPos);

		DrawVisibleRectangleCursor(cursorStartScreenPos, CursorScreenPos);
	//Wyrmgus start
	/*
	} else if (CursorBuilding && CursorOn == cursor_on::map) {
		// Selecting position for building
		DrawBuildingCursor();
	*/
	//Wyrmgus end
	}

	//  Cursor may not exist if we are loading a game or something.
	//  Only draw it if it exists
	if (GameCursor == nullptr) {
		return;
	}
	const PixelPos pos = CursorScreenPos - GameCursor->HotPos * stratagus::defines::get()->get_scale_factor();

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL &&
#else
	if (
#endif
		!GameRunning && !Editor.Running) {
		if (!HiddenSurface
			|| HiddenSurface->w != GameCursor->G->getWidth()
			|| HiddenSurface->h != GameCursor->G->getHeight()) {
			if (HiddenSurface) {
				VideoPaletteListRemove(HiddenSurface);
				SDL_FreeSurface(HiddenSurface);
			}

			HiddenSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,
												 GameCursor->G->getWidth(),
												 GameCursor->G->getHeight(),
												 TheScreen->format->BitsPerPixel,
												 TheScreen->format->Rmask,
												 TheScreen->format->Gmask,
												 TheScreen->format->Bmask,
												 TheScreen->format->Amask);
		}

		SDL_Rect srcRect = { Sint16(pos.x), Sint16(pos.y), Uint16(GameCursor->G->getWidth()), Uint16(GameCursor->G->getHeight())};
		SDL_BlitSurface(TheScreen, &srcRect, HiddenSurface, nullptr);
	}

	//  Last, Normal cursor.
	if (!GameCursor->G->IsLoaded()) {
		GameCursor->G->Load();
	}
	GameCursor->G->DrawFrameClip(GameCursor->SpriteFrame, pos.x, pos.y);
}

/**
**  Hide the cursor
*/
void HideCursor()
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL &&
#else
	if (
#endif
		!GameRunning && !Editor.Running && GameCursor) {
		const PixelPos pos = CursorScreenPos - GameCursor->HotPos * stratagus::defines::get()->get_scale_factor();
		SDL_Rect dstRect = {Sint16(pos.x), Sint16(pos.y), 0, 0 };
		SDL_BlitSurface(HiddenSurface, nullptr, TheScreen, &dstRect);
	}
}

/**
**  Animate the cursor.
**
**  @param ticks  Current tick
*/
void CursorAnimate(unsigned ticks)
{
	static unsigned last = 0;

	if (!GameCursor || !GameCursor->FrameRate) {
		return;
	}
	if (ticks > last + GameCursor->FrameRate) {
		last = ticks + GameCursor->FrameRate;
		GameCursor->SpriteFrame++;
		if ((GameCursor->SpriteFrame & 127) >= GameCursor->G->NumFrames) {
			GameCursor->SpriteFrame = 0;
		}
	}
}

/**
**  Setup the cursor part.
*/
void InitVideoCursors()
{
}

/**
**  Cleanup cursor module
*/
void CleanCursors()
{
	for (std::vector<CCursor *>::iterator i = AllCursors.begin(); i != AllCursors.end(); ++i) {
		CGraphic::Free((**i).G);
		delete *i;
	}
	AllCursors.clear();

	CursorBuilding = nullptr;
	GameCursor = nullptr;
	UnitUnderCursor = nullptr;
}

/**
**  Define a cursor.
**
**  @param l  Lua state.
*/
static int CclDefineCursor(lua_State *l)
{
	std::string name;
	std::string race;
	std::string file;
	PixelPos hotpos(0, 0);
	int w = 0;
	int h = 0;
	int rate = 0;

	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	lua_pushnil(l);
	while (lua_next(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Name")) {
			name = LuaToString(l, -1);
		} else if (!strcmp(value, "Race")) {
			race = LuaToString(l, -1);
		} else if (!strcmp(value, "File")) {
			file = LuaToString(l, -1);
		} else if (!strcmp(value, "HotSpot")) {
			CclGetPos(l, &hotpos.x, &hotpos.y);
		} else if (!strcmp(value, "Size")) {
			CclGetPos(l, &w, &h);
		} else if (!strcmp(value, "Rate")) {
			rate = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
		lua_pop(l, 1);
	}

	Assert(!name.empty() && !file.empty() && w && h);

	if (race == "any") {
		race.clear();
	}

	//
	//  Look if this kind of cursor already exists.
	//
	CCursor *ct = nullptr;
	for (size_t i = 0; i < AllCursors.size(); ++i) {
		//  Race not same, not found.
		if (AllCursors[i]->Race != race) {
			continue;
		}
		if (AllCursors[i]->Ident == name) {
			ct = AllCursors[i];
			break;
		}
	}

	//
	//  Not found, make a new slot.
	//
	if (!ct) {
		ct = new CCursor();
		AllCursors.push_back(ct);
		ct->Ident = name;
		ct->Race = race;
	}
	ct->G = CGraphic::New(file, w, h);
	ct->HotPos = hotpos;
	ct->FrameRate = rate;

	return 0;
}

/**
**  Set the current game cursor.
**
**  @param l  Lua state.
*/
static int CclSetGameCursor(lua_State *l)
{
	LuaCheckArgs(l, 1);
	GameCursor = CursorByIdent(LuaToString(l, 1));
	return 0;
}

void CursorCclRegister()
{
	lua_register(Lua, "DefineCursor", CclDefineCursor);
	lua_register(Lua, "SetGameCursor", CclSetGameCursor);
}
