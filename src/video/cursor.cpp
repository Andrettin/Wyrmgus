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
/**@name cursor.cpp - The cursors. */
//
//      (c) Copyright 1998-2007 by Lutz Sammer, Nehal Mistry,
//                                 and Jimmy Salmon
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "cursor.h"
#include "intern_video.h"

#include "editor.h"
#include "interface.h"
#include "map.h"
#include "tileset.h"
#include "translate.h"
#include "ui.h"
#include "unit.h"
#include "unittype.h"
#include "video.h"
//Wyrmgus start
#include "upgrade.h"
//Wyrmgus end

#include <vector>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/**
**  Define cursor-types.
**
**  @todo FIXME: Should this be move to ui part?
*/
static std::vector<CCursor *> AllCursors;

CursorStates CursorState;    /// current cursor state (point,...)
int CursorAction;            /// action for selection
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
			ShowLoadProgress(_("Loading Cursor \"%s\""), cursor.G->File.c_str());
			cursor.G->Load();
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
	for (std::vector<CCursor *>::iterator i = AllCursors.begin(); i != AllCursors.end(); ++i) {
		CCursor &cursor = **i;

		if (cursor.Ident != ident || !cursor.G->IsLoaded()) {
			continue;
		}
		//Wyrmgus start
		if (!ThisPlayer && cursor.Race != PlayerRaces.Name[Players[0].Race] && !cursor.Race.empty()) {
			continue;
		}
		//Wyrmgus end
		
		if (cursor.Race.empty() || !ThisPlayer || cursor.Race == PlayerRaces.Name[ThisPlayer->Race]) {
			return &cursor;
		}
	}
	DebugPrint("Cursor '%s' not found, please check your code.\n" _C_ ident.c_str());
	return NULL;
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

	CUnit *ontop = NULL;

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
	//Wyrmgus start
	int hair_color = CursorBuilding->GetDefaultHairColor(*ThisPlayer);
	
//	DrawShadow(*CursorBuilding, CursorBuilding->StillFrame, screenPos);
	if (CursorBuilding->GetDefaultVariation(*ThisPlayer) && CursorBuilding->GetDefaultVariation(*ThisPlayer)->ShadowSprite) {
		DrawShadow(*CursorBuilding, CursorBuilding->GetDefaultVariation(*ThisPlayer)->ShadowSprite, CursorBuilding->StillFrame, screenPos);
	} else if (CursorBuilding->ShadowSprite) {
		DrawShadow(*CursorBuilding, CursorBuilding->ShadowSprite, CursorBuilding->StillFrame, screenPos);
	}
	//Wyrmgus end
	//Wyrmgus start
	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, BackpackImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, MountImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

//	DrawUnitType(*CursorBuilding, CursorBuilding->Sprite, ThisPlayer->Index,
//				 CursorBuilding->StillFrame, screenPos);
	// get the first variation which has the proper upgrades for this player (to have the proper appearance of buildings drawn in the cursor, according to the upgrades)
	if (CursorBuilding->GetDefaultVariation(*ThisPlayer) && CursorBuilding->GetDefaultVariation(*ThisPlayer)->Sprite) {
		DrawUnitType(*CursorBuilding, CursorBuilding->GetDefaultVariation(*ThisPlayer)->Sprite, ThisPlayer->Index,
				CursorBuilding->StillFrame, screenPos, hair_color);
	} else {
		DrawUnitType(*CursorBuilding, CursorBuilding->Sprite, ThisPlayer->Index,
				CursorBuilding->StillFrame, screenPos, hair_color);
	}
	//Wyrmgus end
	
	//Wyrmgus start
	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, HairImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos, hair_color);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, PantsImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, ClothingImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, HelmetImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, BootsImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, LeftArmImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos, hair_color);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, ClothingLeftArmImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

	DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, ShieldImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);
	
	if (CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, RightHandImageLayer) != NULL) {
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, RightArmImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos, hair_color);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, ClothingRightArmImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, WeaponImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);
		
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, RightHandImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos, hair_color);
	} else {
		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, WeaponImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, RightArmImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos, hair_color);

		DrawPlayerColorOverlay(*CursorBuilding, CursorBuilding->GetDefaultLayerSprite(*ThisPlayer, ClothingRightArmImageLayer), ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);
	}

	DrawOverlay(*CursorBuilding, CursorBuilding->LightSprite, ThisPlayer->Index, CursorBuilding->StillFrame, screenPos);
	//Wyrmgus end
	
	//Wyrmgus start
//	if (CursorBuilding->CanAttack && CursorBuilding->Stats->Variables[ATTACKRANGE_INDEX].Value > 0) {
	if (CursorBuilding->CanAttack && CursorBuilding->Stats->Variables[ATTACKRANGE_INDEX].Value > 0 && !CursorBuilding->CanTransport()) {
	//Wyrmgus end
		const PixelPos center(screenPos + CursorBuilding->GetPixelSize() / 2);
		const int radius = (CursorBuilding->Stats->Variables[ATTACKRANGE_INDEX].Max + (CursorBuilding->TileSize.x - 1)) * PixelTileSize.x + 1;
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
//			f = ((ontop = CanBuildHere(Selected[i], *CursorBuilding, mpos)) != NULL);
			f = ((ontop = CanBuildHere(Selected[i], *CursorBuilding, mpos, CurrentMapLayer)) != NULL);
			//Wyrmgus end
			// Assign ontop or NULL
			ontop = (ontop == Selected[i] ? NULL : ontop);
		}
	} else {
		//Wyrmgus start
//		f = ((ontop = CanBuildHere(NoUnitP, *CursorBuilding, mpos)) != NULL);
		f = ((ontop = CanBuildHere(NoUnitP, *CursorBuilding, mpos, CurrentMapLayer)) != NULL);
		//Wyrmgus end
		if (!Editor.Running || ontop == (CUnit *)1) {
			ontop = NULL;
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
					  CanBuildOn(posIt, MapFogFilterFlags(*ThisPlayer, posIt,
														  mask & ((!Selected.empty() && Selected[0]->tilePos == posIt) ?
																  //Wyrmgus start
//																  ~(MapFieldLandUnit | MapFieldSeaUnit) : -1))))
																  ~(MapFieldLandUnit | MapFieldSeaUnit) : -1), CurrentMapLayer), CurrentMapLayer))
																  //Wyrmgus end
				//Wyrmgus start
//				&& Map.Field(posIt)->playerInfo.IsExplored(*ThisPlayer)) {
				&& Map.Field(posIt, CurrentMapLayer)->playerInfo.IsTeamExplored(*ThisPlayer)) {
				//Wyrmgus end
				color = ColorGreen;
			} else {
				color = ColorRed;
			}
			Video.FillTransRectangleClip(color, screenPos.x + w * PixelTileSize.x,
										 screenPos.y + h * PixelTileSize.y, PixelTileSize.x, PixelTileSize.y, 95);
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
	if (CursorState == CursorStateRectangle && CursorStartScreenPos != CursorScreenPos) {
		const PixelPos cursorStartScreenPos = UI.MouseViewport->MapToScreenPixelPos(CursorStartMapPos);

		DrawVisibleRectangleCursor(cursorStartScreenPos, CursorScreenPos);
	//Wyrmgus start
	/*
	} else if (CursorBuilding && CursorOn == CursorOnMap) {
		// Selecting position for building
		DrawBuildingCursor();
	*/
	//Wyrmgus end
	}

	//  Cursor may not exist if we are loading a game or something.
	//  Only draw it if it exists
	if (GameCursor == NULL) {
		return;
	}
	const PixelPos pos = CursorScreenPos - GameCursor->HotPos;

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
		SDL_BlitSurface(TheScreen, &srcRect, HiddenSurface, NULL);
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
		const PixelPos pos = CursorScreenPos - GameCursor->HotPos;
		SDL_Rect dstRect = {Sint16(pos.x), Sint16(pos.y), 0, 0 };
		SDL_BlitSurface(HiddenSurface, NULL, TheScreen, &dstRect);
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

	CursorBuilding = NULL;
	GameCursor = NULL;
	UnitUnderCursor = NULL;
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
	CCursor *ct = NULL;
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


//@}
