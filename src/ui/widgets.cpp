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
/**@name widgets.cpp - The stratagus ui widgets. */
//
//      (c) Copyright 2005-2006 by Francois Beerten and Jimmy Salmon
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
#include "video.h"
#include "font.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "cursor.h"
#include "ui.h"
#include "widgets.h"
#include "network.h"
#include "netconnect.h"
#include "editor.h"
#include "sound.h"
//Wyrmgus start
#include "player.h"
//Wyrmgus end
//Wyrmgus start
#include "results.h"
//Wyrmgus end

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

// Guichan stuff we need
gcn::Gui *Gui;         /// A Gui object - binds it all together
gcn::SDLInput *Input;  /// Input driver

static EventCallback GuichanCallbacks;

static std::stack<MenuScreen *> MenuStack;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/


//Wyrmgus start
//static void MenuHandleButtonDown(unsigned)
static void MenuHandleButtonDown(unsigned button)
//Wyrmgus end
{
	//Wyrmgus start
	if ((1 << button) == LeftButton && GrandStrategy && !GameRunning && GameResult == GameNoResult && !GrandStrategyGamePaused) {
		//if clicked on the "OK" button in the province interface
		if (GrandStrategyGame.SelectedProvince != -1 && UI.GrandStrategyOKButton.Contains(CursorScreenPos) && GrandStrategyInterfaceState != "Province" && GrandStrategyInterfaceState != "Diplomacy") {
			UI.GrandStrategyOKButton.Clicked = true;
		} else if (UI.MenuButton.Contains(CursorScreenPos)) {
			UI.MenuButton.Clicked = true;
		} else if (UI.GrandStrategyEndTurnButton.Contains(CursorScreenPos)) {
			UI.GrandStrategyEndTurnButton.Clicked = true;
		} else if (GrandStrategyGame.SelectedProvince != -1 && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Heroes.size() > 0 && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner != NULL && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner == GrandStrategyGame.PlayerFaction && UI.GrandStrategyShowHeroesButton.Contains(CursorScreenPos) && GrandStrategyInterfaceState == "Province") {
			UI.GrandStrategyShowHeroesButton.Clicked = true;
		} else if (GrandStrategyGame.SelectedProvince != -1 && UI.GrandStrategyShowRulerButton.X != -1 && GrandStrategyGame.PlayerFaction != NULL && GrandStrategyGame.PlayerFaction->Ruler != NULL && UI.GrandStrategyShowRulerButton.Contains(CursorScreenPos) && (GrandStrategyInterfaceState == "town-hall" || GrandStrategyInterfaceState == "stronghold")) {
			UI.GrandStrategyShowRulerButton.Clicked = true;
		}
	}
	//Wyrmgus end
}
//Wyrmgus start
//static void MenuHandleButtonUp(unsigned)
static void MenuHandleButtonUp(unsigned button)
//Wyrmgus end
{
	//Wyrmgus start
	if ((1 << button) == LeftButton && GrandStrategy && !GameRunning && GameResult == GameNoResult && !GrandStrategyGamePaused) {
		//if clicked on the "OK" button in a province's interface
		if (GrandStrategyGame.SelectedProvince != -1 && !(MouseButtons & LeftButton) && UI.GrandStrategyOKButton.Clicked && GrandStrategyInterfaceState != "Province" && GrandStrategyInterfaceState != "Diplomacy") {
			UI.GrandStrategyOKButton.Clicked = false;
			if (UI.GrandStrategyOKButton.Contains(CursorScreenPos)) { //if the mouse is still on the button, fire its action
				if (UI.GrandStrategyOKButton.Callback) {
					UI.GrandStrategyOKButton.Callback->action("");
				}
			}
		} else if (!(MouseButtons & LeftButton) && UI.MenuButton.Clicked) {
			UI.MenuButton.Clicked = false;
			if (UI.MenuButton.Contains(CursorScreenPos)) { //if the mouse is still on the button, fire its action
				if (UI.MenuButton.Callback) {
					UI.MenuButton.Callback->action("");
				}
			}
		} else if (!(MouseButtons & LeftButton) && UI.GrandStrategyEndTurnButton.Clicked) {
			UI.GrandStrategyEndTurnButton.Clicked = false;
			if (UI.GrandStrategyEndTurnButton.Contains(CursorScreenPos)) { //if the mouse is still on the button, fire its action
				if (UI.GrandStrategyEndTurnButton.Callback) {
					UI.GrandStrategyEndTurnButton.Callback->action("");
				}
			}
		} else if (GrandStrategyGame.SelectedProvince != -1 && !(MouseButtons & LeftButton) && UI.GrandStrategyShowHeroesButton.Clicked && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Heroes.size() > 0 && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner != NULL && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner == GrandStrategyGame.PlayerFaction && GrandStrategyInterfaceState == "Province") {
			UI.GrandStrategyShowHeroesButton.Clicked = false;
			if (UI.GrandStrategyShowHeroesButton.Contains(CursorScreenPos)) { //if the mouse is still on the button, fire its action
				if (UI.GrandStrategyShowHeroesButton.Callback) {
					UI.GrandStrategyShowHeroesButton.Callback->action("");
				}
			}
		} else if (GrandStrategyGame.SelectedProvince != -1 && !(MouseButtons & LeftButton) && UI.GrandStrategyShowRulerButton.Clicked && (GrandStrategyInterfaceState == "town-hall" || GrandStrategyInterfaceState == "stronghold") && UI.GrandStrategyShowRulerButton.X != -1 && GrandStrategyGame.PlayerFaction != NULL && GrandStrategyGame.PlayerFaction->Ruler != NULL) {
			UI.GrandStrategyShowRulerButton.Clicked = false;
			if (UI.GrandStrategyShowRulerButton.Contains(CursorScreenPos)) { //if the mouse is still on the button, fire its action
				if (UI.GrandStrategyShowRulerButton.Callback) {
					UI.GrandStrategyShowRulerButton.Callback->action("");
				}
			}
		//if clicked on a tile, update the grand strategy interface
		} else if (UI.MapArea.Contains(CursorScreenPos) && !(MouseButtons & LeftButton) && GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y]->Terrain != -1) {
			PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume, false);
			if (GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y]->Province != -1) {
				char buf[256];
				snprintf(buf, sizeof(buf), "if (SetSelectedProvinceLua ~= nil) then SetSelectedProvinceLua(GetTileProvince(%d, %d)) end;", GrandStrategyGame.GetTileUnderCursor().x, GrandStrategyGame.GetTileUnderCursor().y);
				CclCommand(buf);
			}
		//if clicked on the minimap, go to that part of the map
		} else if (
			CursorScreenPos.x >= UI.Minimap.X + GrandStrategyGame.MinimapOffsetX
			&& CursorScreenPos.x < UI.Minimap.X + GrandStrategyGame.MinimapOffsetX + GrandStrategyGame.MinimapTextureWidth
			&& CursorScreenPos.y >= UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY
			&& CursorScreenPos.y < UI.Minimap.Y + GrandStrategyGame.MinimapOffsetY + GrandStrategyGame.MinimapTextureHeight
			&& !(MouseButtons & LeftButton)
			&& GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y]
		) {
			PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume, false);
			CenterGrandStrategyMapOnTile(GrandStrategyGame.GetTileUnderCursor().x, GrandStrategyGame.GetTileUnderCursor().y);
		}
	}
	//Wyrmgus end
}
static void MenuHandleMouseMove(const PixelPos &screenPos)
{
	PixelPos pos(screenPos);
	HandleCursorMove(&pos.x, &pos.y);
}
static void MenuHandleKeyDown(unsigned key, unsigned keychar)
{
	//Wyrmgus start
	if (GrandStrategy && !GameRunning && GameResult == GameNoResult && !GrandStrategyGamePaused) {// if in grand strategy mode
		// scroll the map if a directional key was pressed
		bool scrolled = false;
		if (key == SDLK_UP || key == SDLK_KP8) {
			if (WorldMapOffsetY > 0) {
				if (GrandStrategyMapHeightIndent == 0) {
					WorldMapOffsetY = WorldMapOffsetY - 1;
				}
				GrandStrategyMapHeightIndent -= 32;
			} else if (WorldMapOffsetY == 0 && GrandStrategyMapHeightIndent == -32) { //this is to make the entire y 0 tiles be shown scrolling to the northmost part of the map
				GrandStrategyMapHeightIndent -= 32;
			}
			scrolled = true;
		} else if (key == SDLK_DOWN || key == SDLK_KP2) {
			if (WorldMapOffsetY < GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64)) {
				if (GrandStrategyMapHeightIndent == -32) {
					WorldMapOffsetY = WorldMapOffsetY + 1;
				}
				GrandStrategyMapHeightIndent += 32;
			} else if (WorldMapOffsetY == GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64) && GrandStrategyMapHeightIndent == 0) {
				GrandStrategyMapHeightIndent += 32;
			}
			scrolled = true;
		} else if (key == SDLK_LEFT || key == SDLK_KP4) {
			if (WorldMapOffsetX > 0) {
				if (GrandStrategyMapWidthIndent == 0) {
					WorldMapOffsetX = WorldMapOffsetX - 1;
				}
				GrandStrategyMapWidthIndent -= 32;
			} else if (WorldMapOffsetX == 0 && GrandStrategyMapWidthIndent == -32) { //this is to make the entire x 0 tiles be shown scrolling to the westmost part of the map
				GrandStrategyMapWidthIndent -= 32;
			}
			scrolled = true;
		} else if (key == SDLK_RIGHT || key == SDLK_KP6) {
			if (WorldMapOffsetX < GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64)) {
				if (GrandStrategyMapWidthIndent == -32) {
					WorldMapOffsetX = WorldMapOffsetX + 1;
				}
				GrandStrategyMapWidthIndent += 32;
			} else if (WorldMapOffsetX == GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64) && GrandStrategyMapWidthIndent == 0) {
				GrandStrategyMapWidthIndent += 32;
			}
			scrolled = true;
		//if pressed the hotkey of the "OK" button in the province interface
		} else if (GrandStrategyGame.SelectedProvince != -1 && GrandStrategyInterfaceState != "Province" && GrandStrategyInterfaceState != "Diplomacy" && key == 'o') {
			UI.GrandStrategyOKButton.HotkeyPressed = true;
		} else if (key == SDLK_F5) {
			CclCommand("if (RunEncyclopediaMenu ~= nil) then RunEncyclopediaMenu() end;");
		//if pressed the hotkey of the menu button
		} else if (key == SDLK_F10) {
			UI.MenuButton.HotkeyPressed = true;
		} else if (key == SDLK_F11) {
			CclCommand("if (RunGrandStrategySaveMenu ~= nil) then RunGrandStrategySaveMenu() end;");
		} else if (key == SDLK_F12) {
			CclCommand("if (RunGrandStrategyLoadGameMenu ~= nil) then RunGrandStrategyLoadGameMenu() end;");
		} else if (key == 'e') {
			UI.GrandStrategyEndTurnButton.HotkeyPressed = true;
		} else if (GrandStrategyGame.SelectedProvince != -1 && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Heroes.size() > 0 && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner != NULL && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner == GrandStrategyGame.PlayerFaction && GrandStrategyInterfaceState == "Province" && key == 'h') {
			UI.GrandStrategyShowHeroesButton.HotkeyPressed = true;
		} else if (GrandStrategyGame.SelectedProvince != -1 && UI.GrandStrategyShowRulerButton.X != -1 && GrandStrategyGame.PlayerFaction != NULL && GrandStrategyGame.PlayerFaction->Ruler != NULL && (GrandStrategyInterfaceState == "town-hall" || GrandStrategyInterfaceState == "stronghold") && key == 'r') {
			UI.GrandStrategyShowRulerButton.HotkeyPressed = true;
		}
		
		if (scrolled) {
			if (GrandStrategyMapWidthIndent <= -64) {
				GrandStrategyMapWidthIndent = 0;
			}
			if (GrandStrategyMapHeightIndent <= -64) {
				GrandStrategyMapHeightIndent = 0;
			}
			if (GrandStrategyMapWidthIndent > 0) {
				GrandStrategyMapWidthIndent *= -1;
			}
			if (GrandStrategyMapHeightIndent > 0) {
				GrandStrategyMapHeightIndent *= -1;
			}
		}
	}
	//Wyrmgus end
	HandleKeyModifiersDown(key, keychar);
}
static void MenuHandleKeyUp(unsigned key, unsigned keychar)
{
	//Wyrmgus start
	if (key == 'f') { // ALT+F and CTRL+F toggle fullscreen
		if (KeyModifiers & (ModifierAlt | ModifierControl)) {
			ToggleFullScreen();
			CclCommand("wyr.preferences.VideoFullScreen = Video.FullScreen;");
			SavePreferences();
		}
	/*
	} else if (key == 'g') { // ALT+G, CTRL+G grab mouse pointer
		if (KeyModifiers & (ModifierAlt | ModifierControl)) {
			UiToggleGrabMouse();
			CclCommand("wyr.preferences.GrabMouse = GetGrabMouse();");
			SavePreferences();
		}
	*/
	}
	//Wyrmgus end
	//Wyrmgus start
	if (GrandStrategy && !GameRunning && GameResult == GameNoResult && !GrandStrategyGamePaused) {// if in grand strategy mode
		if (GrandStrategyInterfaceState != "Province" && GrandStrategyInterfaceState != "Diplomacy" && key == 'o' && GrandStrategyGame.SelectedProvince != -1 && UI.GrandStrategyOKButton.HotkeyPressed) {
			UI.GrandStrategyOKButton.HotkeyPressed = false;
			if (UI.GrandStrategyOKButton.Callback) {
				UI.GrandStrategyOKButton.Callback->action("");
			}
		} else if (key == SDLK_F10 && UI.MenuButton.HotkeyPressed) {
			UI.MenuButton.HotkeyPressed = false;
			if (UI.MenuButton.Callback) {
				UI.MenuButton.Callback->action("");
			}
		} else if (key == 'e' && UI.GrandStrategyEndTurnButton.HotkeyPressed) {
			UI.GrandStrategyEndTurnButton.HotkeyPressed = false;
			if (UI.GrandStrategyEndTurnButton.Callback) {
				UI.GrandStrategyEndTurnButton.Callback->action("");
			}
		} else if (key == 'h' && GrandStrategyGame.SelectedProvince != -1 && UI.GrandStrategyShowHeroesButton.HotkeyPressed && GrandStrategyGame.SelectedProvince != -1 && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Heroes.size() > 0 && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner != NULL && GrandStrategyGame.Provinces[GrandStrategyGame.SelectedProvince]->Owner == GrandStrategyGame.PlayerFaction && GrandStrategyInterfaceState == "Province") {
			UI.GrandStrategyShowHeroesButton.HotkeyPressed = false;
			if (UI.GrandStrategyShowHeroesButton.Callback) {
				UI.GrandStrategyShowHeroesButton.Callback->action("");
			}
		} else if (key == 'r' && GrandStrategyGame.SelectedProvince != -1 && UI.GrandStrategyShowRulerButton.X != -1 && GrandStrategyGame.PlayerFaction != NULL && GrandStrategyGame.PlayerFaction->Ruler != NULL && UI.GrandStrategyShowRulerButton.HotkeyPressed && (GrandStrategyInterfaceState == "town-hall" || GrandStrategyInterfaceState == "stronghold")) {
			UI.GrandStrategyShowRulerButton.HotkeyPressed = false;
			if (UI.GrandStrategyShowRulerButton.Callback) {
				UI.GrandStrategyShowRulerButton.Callback->action("");
			}
		}
	}
	//Wyrmgus end
	HandleKeyModifiersUp(key, keychar);
}
static void MenuHandleKeyRepeat(unsigned key, unsigned keychar)
{
	Input->processKeyRepeat();
	//Wyrmgus start
	if (GrandStrategy && !GameRunning && GameResult == GameNoResult && !GrandStrategyGamePaused) { // if in grand strategy mode, scroll the map if a directional key was pressed
		bool scrolled = false;
		if (key == SDLK_UP || key == SDLK_KP8) {
			if (WorldMapOffsetY > 0) {
				if (GrandStrategyMapHeightIndent == 0) {
					WorldMapOffsetY = WorldMapOffsetY - 1;
				}
				GrandStrategyMapHeightIndent -= 32;
			} else if (WorldMapOffsetY == 0 && GrandStrategyMapHeightIndent == -32) { //this is to make the entire y 0 tiles be shown scrolling to the northmost part of the map
				GrandStrategyMapHeightIndent -= 32;
			}
			scrolled = true;
		} else if (key == SDLK_DOWN || key == SDLK_KP2) {
			if (WorldMapOffsetY < GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64)) {
				if (GrandStrategyMapHeightIndent == -32) {
					WorldMapOffsetY = WorldMapOffsetY + 1;
				}
				GrandStrategyMapHeightIndent += 32;
			} else if (WorldMapOffsetY == GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64) && GrandStrategyMapHeightIndent == 0) {
				GrandStrategyMapHeightIndent += 32;
			}
			scrolled = true;
		} else if (key == SDLK_LEFT || key == SDLK_KP4) {
			if (WorldMapOffsetX > 0) {
				if (GrandStrategyMapWidthIndent == 0) {
					WorldMapOffsetX = WorldMapOffsetX - 1;
				}
				GrandStrategyMapWidthIndent -= 32;
			} else if (WorldMapOffsetX == 0 && GrandStrategyMapWidthIndent == -32) { //this is to make the entire x 0 tiles be shown scrolling to the westmost part of the map
				GrandStrategyMapWidthIndent -= 32;
			}
			scrolled = true;
		} else if (key == SDLK_RIGHT || key == SDLK_KP6) {
			if (WorldMapOffsetX < GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64)) {
				if (GrandStrategyMapWidthIndent == -32) {
					WorldMapOffsetX = WorldMapOffsetX + 1;
				}
				GrandStrategyMapWidthIndent += 32;
			} else if (WorldMapOffsetX == GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64) && GrandStrategyMapWidthIndent == 0) {
				GrandStrategyMapWidthIndent += 32;
			}
			scrolled = true;
		}
		if (scrolled) {
			if (GrandStrategyMapWidthIndent <= -64) {
				GrandStrategyMapWidthIndent = 0;
			}
			if (GrandStrategyMapHeightIndent <= -64) {
				GrandStrategyMapHeightIndent = 0;
			}
			if (GrandStrategyMapWidthIndent > 0) {
				GrandStrategyMapWidthIndent *= -1;
			}
			if (GrandStrategyMapHeightIndent > 0) {
				GrandStrategyMapHeightIndent *= -1;
			}
		}
	}
	//Wyrmgus end
	HandleKeyModifiersDown(key, keychar);
}


/**
**  Initializes the GUI stuff
*/
void initGuichan()
{
	gcn::Graphics *graphics;

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		graphics = new MyOpenGLGraphics();
	} else
#endif
	{
		graphics = new gcn::SDLGraphics();

		// Set the target for the graphics object to be the screen.
		// In other words, we will draw to the screen.
		// Note, any surface will do, it doesn't have to be the screen.
		((gcn::SDLGraphics *)graphics)->setTarget(TheScreen);
	}

	Input = new gcn::SDLInput();

	Gui = new gcn::Gui();
	Gui->setGraphics(graphics);
	Gui->setInput(Input);
	Gui->setTop(NULL);

#if defined(USE_OPENGL) || defined(USE_GLES)
	Gui->setUseDirtyDrawing(!UseOpenGL);
#else
	Gui->setUseDirtyDrawing(1);
#endif

	GuichanCallbacks.ButtonPressed = &MenuHandleButtonDown;
	GuichanCallbacks.ButtonReleased = &MenuHandleButtonUp;
	GuichanCallbacks.MouseMoved = &MenuHandleMouseMove;
	GuichanCallbacks.MouseExit = &HandleMouseExit;
	GuichanCallbacks.KeyPressed = &MenuHandleKeyDown;
	GuichanCallbacks.KeyReleased = &MenuHandleKeyUp;
	GuichanCallbacks.KeyRepeated = &MenuHandleKeyRepeat;
	GuichanCallbacks.NetworkEvent = NetworkEvent;
}

/**
**  Free all guichan infrastructure
*/
void freeGuichan()
{
	if (Gui) {
		delete Gui->getGraphics();
		delete Gui;
		Gui = NULL;
	}

	delete Input;
	Input = NULL;
}

/**
**  Handle input events
**
**  @param event  event to handle, null if no more events for this frame
*/
void handleInput(const SDL_Event *event)
{
	if (event) {
		if (Input) {
			try {
				Input->pushInput(*event);
			} catch (const gcn::Exception &) {
				// ignore unhandled buttons
			}
		}
	} else {
		if (Gui) {
			Gui->logic();
		}
	}
}

void DrawGuichanWidgets()
{
	if (Gui) {
#if defined(USE_OPENGL) || defined(USE_GLES)
		//Wyrmgus start
//		Gui->setUseDirtyDrawing(!UseOpenGL && !GameRunning && !Editor.Running);
		Gui->setUseDirtyDrawing(!UseOpenGL && !GameRunning && !Editor.Running && (!GrandStrategy || GrandStrategyGamePaused));
		//Wyrmgus end
#else
		//Wyrmgus start
//		Gui->setUseDirtyDrawing(!GameRunning && !Editor.Running);
		Gui->setUseDirtyDrawing(!GameRunning && !Editor.Running && (!GrandStrategy || GrandStrategyGamePaused);
		//Wyrmgus end
#endif
		Gui->draw();
	}
}


/*----------------------------------------------------------------------------
--  LuaActionListener
----------------------------------------------------------------------------*/


/**
**  LuaActionListener constructor
**
**  @param l  Lua state
**  @param f  Listener function
*/
LuaActionListener::LuaActionListener(lua_State *l, lua_Object f) :
	callback(l, f)
{
}

/**
**  Called when an action is received from a Widget. It is used
**  to be able to receive a notification that an action has
**  occurred.
**
**  @param eventId  the identifier of the Widget
*/
void LuaActionListener::action(const std::string &eventId)
{
	callback.pushPreamble();
	callback.pushString(eventId.c_str());
	callback.run();
}

/**
**  LuaActionListener destructor
*/
LuaActionListener::~LuaActionListener()
{
}

#if defined(USE_OPENGL) || defined(USE_GLES)

/*----------------------------------------------------------------------------
--  MyOpenGLGraphics
----------------------------------------------------------------------------*/

void MyOpenGLGraphics::_beginDraw()
{
	gcn::Rectangle area(0, 0, Video.Width, Video.Height);
	pushClipArea(area);
}

void MyOpenGLGraphics::_endDraw()
{
	popClipArea();
}

//Wyrmgus start
//void MyOpenGLGraphics::drawImage(const gcn::Image *image, int srcX, int srcY,
//								 int dstX, int dstY, int width, int height)
void MyOpenGLGraphics::drawImage(const gcn::Image *image, int srcX, int srcY,
								 int dstX, int dstY, int width, int height, int player)
//Wyrmgus end
{
	const gcn::ClipRectangle &r = this->getCurrentClipArea();
	int right = std::min<int>(r.x + r.width - 1, Video.Width - 1);
	int bottom = std::min<int>(r.y + r.height - 1, Video.Height - 1);

	if (r.x > right || r.y > bottom) {
		return;
	}

	PushClipping();
	SetClipping(r.x, r.y, right, bottom);
	//Wyrmgus start
//	((CGraphic *)image)->DrawSubClip(srcX, srcY, width, height,
//									 dstX + mClipStack.top().xOffset, dstY + mClipStack.top().yOffset);
	if (player != -1) {
		((CPlayerColorGraphic *)image)->DrawPlayerColorSubClip(player, srcX, srcY, width, height,
										 dstX + mClipStack.top().xOffset, dstY + mClipStack.top().yOffset);
	} else {
		((CGraphic *)image)->DrawSubClip(srcX, srcY, width, height,
										 dstX + mClipStack.top().xOffset, dstY + mClipStack.top().yOffset);
	}
	//Wyrmgus end
	PopClipping();
}

void MyOpenGLGraphics::drawPoint(int x, int y)
{
	gcn::Color c = this->getColor();
	Video.DrawPixelClip(Video.MapRGBA(0, c.r, c.g, c.b, c.a),
						x + mClipStack.top().xOffset, y + mClipStack.top().yOffset);
}

void MyOpenGLGraphics::drawLine(int x1, int y1, int x2, int y2)
{
	gcn::Color c = this->getColor();
	const PixelPos pos1(x1 + mClipStack.top().xOffset, y1 + mClipStack.top().yOffset);
	const PixelPos pos2(x2 + mClipStack.top().xOffset, y2 + mClipStack.top().yOffset);

	Video.DrawLineClip(Video.MapRGBA(0, c.r, c.g, c.b, c.a), pos1, pos2);
}

void MyOpenGLGraphics::drawRectangle(const gcn::Rectangle &rectangle)
{
	gcn::Color c = this->getColor();
	if (c.a == 0) {
		return;
	}

	const gcn::ClipRectangle top = mClipStack.top();
	gcn::Rectangle area = gcn::Rectangle(rectangle.x + top.xOffset,
										 rectangle.y + top.yOffset,
										 rectangle.width, rectangle.height);

	if (!area.intersect(top)) {
		return;
	}

	int x1 = std::max<int>(area.x, top.x);
	int y1 = std::max<int>(area.y, top.y);
	int x2 = std::min<int>(area.x + area.width, top.x + top.width);
	int y2 = std::min<int>(area.y + area.height, top.y + top.height);

	Video.DrawTransRectangle(Video.MapRGB(0, c.r, c.g, c.b),
							 x1, y1, x2 - x1, y2 - y1, mColor.a);
}

void MyOpenGLGraphics::fillRectangle(const gcn::Rectangle &rectangle)
{
	const gcn::Color c = this->getColor();

	if (c.a == 0) {
		return;
	}

	const gcn::ClipRectangle top = mClipStack.top();
	gcn::Rectangle area = gcn::Rectangle(rectangle.x + top.xOffset,
										 rectangle.y + top.yOffset,
										 rectangle.width, rectangle.height);

	if (!area.intersect(top)) {
		return;
	}

	int x1 = std::max<int>(area.x, top.x);
	int y1 = std::max<int>(area.y, top.y);
	int x2 = std::min<int>(area.x + area.width, top.x + top.width);
	int y2 = std::min<int>(area.y + area.height, top.y + top.height);

	Video.FillTransRectangle(Video.MapRGB(0, c.r, c.g, c.b),
							 x1, y1, x2 - x1, y2 - y1, c.a);
}

#endif

//Wyrmgus start
/*----------------------------------------------------------------------------
--  PlayerColorImageWidget
----------------------------------------------------------------------------*/

void PlayerColorImageWidget::draw(gcn::Graphics* graphics)
{
	int WidgetPlayerColorIndexFromName = -1;
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColorNames[i] == WidgetPlayerColor) {
			WidgetPlayerColorIndexFromName = i;
			break;
		}
	}
	if (WidgetPlayerColorIndexFromName == -1) {
		fprintf(stderr, "Color %s not defined\n", WidgetPlayerColor.c_str());
		ExitFatal(1);
	}
	
	// make the widget's image be player-colored
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
	} else
#endif
	{
		WidgetGraphicPlayerPixels(WidgetPlayerColor, *((CPlayerColorGraphic *)mImage));
	}
	
	graphics->drawImage(mImage, ImageOrigin.x, ImageOrigin.y, 0, 0, mImage->getWidth(), mImage->getHeight(), WidgetPlayerColorIndexFromName);
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  ImageButton
----------------------------------------------------------------------------*/


/**
**  ImageButton constructor
*/
ImageButton::ImageButton() :
	Button(), normalImage(NULL), pressedImage(NULL),
	//Wyrmgus start
//	disabledImage(NULL)
	disabledImage(NULL), frameImage(NULL), pressedframeImage(NULL), Transparency(0)
	//Wyrmgus end
{
	setForegroundColor(0xffffff);
	//Wyrmgus start
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
	//Wyrmgus end
}

/**
**  ImageButton constructor
**
**  @param caption  Caption text
*/
ImageButton::ImageButton(const std::string &caption) :
	Button(caption), normalImage(NULL), pressedImage(NULL),
	//Wyrmgus start
//	disabledImage(NULL)
	disabledImage(NULL), frameImage(NULL), pressedframeImage(NULL), Transparency(0)
	//Wyrmgus end
{
	setForegroundColor(0xffffff);
	//Wyrmgus start
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
	//Wyrmgus end
}

/**
**  Draw the image button
**
**  @param graphics  Graphics object to draw with
*/
void ImageButton::draw(gcn::Graphics *graphics)
{
	if (!normalImage) {
		Button::draw(graphics);
		return;
	}

	gcn::Image *img;

	if (!isEnabled()) {
		img = disabledImage ? disabledImage : normalImage;
	} else if (isPressed()) {
		img = pressedImage ? pressedImage : normalImage;
	} else if (0 && hasMouse()) {
		// FIXME: add mouse-over image
		img = NULL;
	} else {
		img = normalImage;
	}
	
	//Wyrmgus start
	if (img) {
		if (Transparency) {
		#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
			} else
		#endif
			{
				WidgetGraphicTransparency(int(256 - 2.56 * Transparency), *((CGraphic *)img));
			}
		}
	}

//	graphics->drawImage(img, 0, 0, 0, 0,
//						img->getWidth(), img->getHeight());

	if (frameImage) {
        graphics->setColor(ColorBlack);
		graphics->fillRectangle(gcn::Rectangle((frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2, img->getWidth(), img->getHeight()));
		graphics->drawImage(frameImage, 0, 0, 0, 0,
							frameImage->getWidth(), frameImage->getHeight());
		if (isPressed()) {
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
				}
			#endif
			}
			graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, ((frameImage->getWidth() - img->getWidth()) / 2) + 1, ((frameImage->getHeight() - img->getHeight()) / 2) + 1,
								img->getWidth() - 1, img->getHeight() - 1);
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				}
			#endif
			}
			if (pressedframeImage) {
				graphics->drawImage(pressedframeImage, 0, 0, 0, 0,
									pressedframeImage->getWidth(), pressedframeImage->getHeight());
			}
		} else {
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
				}
			#endif
			}
			graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, (frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2,
								img->getWidth(), img->getHeight());
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				}
			#endif
			}
		}
	} else {
		if (Transparency) {
		#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
			}
		#endif
		}
		graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, 0, 0,
							img->getWidth(), img->getHeight());
		if (Transparency) {
		#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		#endif
		}
	}
	//Wyrmgus end

	graphics->setColor(getForegroundColor());

	int textX;
	int textY = getHeight() / 2 - getFont()->getHeight() / 2;

	switch (getAlignment()) {
		case gcn::Graphics::LEFT:
			textX = 4;
			break;
		case gcn::Graphics::CENTER:
			textX = getWidth() / 2;
			break;
		case gcn::Graphics::RIGHT:
			textX = getWidth() - 4;
			break;
		default:
			textX = 0;
			Assert(!"Unknown alignment.");
			//throw GCN_EXCEPTION("Unknown alignment.");
	}

	graphics->setFont(getFont());
	//Wyrmgus start
	/*
	if (isPressed()) {
		graphics->drawText(getCaption(), textX + 4, textY + 4, getAlignment());
	} else {
		graphics->drawText(getCaption(), textX + 2, textY + 2, getAlignment());
	}
	*/
	bool is_normal = true;
	if (hasMouse()) {
		is_normal = false;
	}
	if (isPressed()) {
		textX += 4;
		textY += 4;
		if ((textY + 11) > (getHeight() - 2)) {
			textY += (getHeight() - 2) - (textY + 11);
		}
		if (textY < 1) {
			textY = 1;
		}
	} else {
		textX += 2;
		textY += 2;
		if ((textY + 11) > (getHeight() - 3)) {
			textY += (getHeight() - 3) - (textY + 11);
		}
		if (textY < 0) {
			textY = 0;
		}
	}
	graphics->drawText(getCaption(), textX, textY, getAlignment(), is_normal);
	//Wyrmgus end

	//Wyrmgus start
//	if (hasFocus()) {
	if (isPressed() && !frameImage) {
	//Wyrmgus end
		//Wyrmgus start
//		graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));
		if (getWidth() == getHeight() && getWidth() > 64 && getHeight() > 64) {
			graphics->drawRectangle(gcn::Rectangle(0 + ((getWidth() - 64) / 2), 0 + ((getHeight() - 64) / 2), getWidth() - (getWidth() - 64), getHeight() - (getHeight() - 64))); //hack to make it appear properly in grand strategy mode
		} else {
			graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));
		}
		//Wyrmgus end
	}
	
	//Wyrmgus start
	//restore old alpha
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
	} else
#endif
	{
		WidgetGraphicTransparency(255, *((CGraphic *)img));
	}
	//Wyrmgus end
}

/**
**  Automatically adjust the size of an image button
*/
void ImageButton::adjustSize()
{
	//Wyrmgus start
//	if (normalImage) {
	if (frameImage) {
		setWidth(frameImage->getWidth());
		setHeight(frameImage->getHeight());
		setPosition(getX(), getY()); //reset position, to make it appropriate for the frame image
	} else if (normalImage) {
	//Wyrmgus end
		setWidth(normalImage->getWidth());
		setHeight(normalImage->getHeight());
	} else {
		Button::adjustSize();
	}
}

//Wyrmgus start
void ImageButton::setPosition(int x, int y)
{
	if (frameImage) {
		mDimension.x = x - ((frameImage->getWidth() - normalImage->getWidth()) / 2);
		mDimension.y = y - ((frameImage->getHeight() - normalImage->getWidth()) / 2);
	} else {
		mDimension.x = x;
		mDimension.y = y;
	}
}

/*----------------------------------------------------------------------------
--  PlayerColorImageButtonImageButton
----------------------------------------------------------------------------*/


/**
**  PlayerColorImageButton constructor
*/
PlayerColorImageButton::PlayerColorImageButton() :
	Button(), normalImage(NULL), pressedImage(NULL),
	disabledImage(NULL), frameImage(NULL), pressedframeImage(NULL), ButtonPlayerColor(""), Transparency(0)
{
	setForegroundColor(0xffffff);
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
}

/**
**  PlayerColorImageButton constructor
**
**  @param caption  Caption text
*/
PlayerColorImageButton::PlayerColorImageButton(const std::string &caption, const std::string &playercolor) :
	Button(caption), normalImage(NULL), pressedImage(NULL),
	disabledImage(NULL), frameImage(NULL), pressedframeImage(NULL), ButtonPlayerColor(playercolor), Transparency(0)
{
	setForegroundColor(0xffffff);
	ImageOrigin.x = 0;
	ImageOrigin.y = 0;
}

/**
**  Draw the image button
**
**  @param graphics  Graphics object to draw with
*/
void PlayerColorImageButton::draw(gcn::Graphics *graphics)
{
	if (!normalImage) {
		Button::draw(graphics);
		return;
	}

	gcn::Image *img;

	if (!isEnabled()) {
		img = disabledImage ? disabledImage : normalImage;
	} else if (isPressed()) {
		img = pressedImage ? pressedImage : normalImage;
	} else if (0 && hasMouse()) {
		// FIXME: add mouse-over image
		img = NULL;
	} else {
		img = normalImage;
	}

	int WidgetPlayerColorIndexFromName = -1;
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColorNames[i] == ButtonPlayerColor) {
			WidgetPlayerColorIndexFromName = i;
			break;
		}
	}
	if (WidgetPlayerColorIndexFromName == -1) {
		fprintf(stderr, "Color %s not defined\n", ButtonPlayerColor.c_str());
		ExitFatal(1);
	}
		
	if (img) {
		// make the button's image be player-colored
	#if defined(USE_OPENGL) || defined(USE_GLES)
		if (UseOpenGL) {
		} else
	#endif
		{
			WidgetGraphicPlayerPixels(ButtonPlayerColor, *((CPlayerColorGraphic *)img));
		}
		
		if (Transparency) {
		#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
			} else
		#endif
			{
				WidgetGraphicTransparency(int(256 - 2.56 * Transparency), *((CPlayerColorGraphic *)img));
			}
		}
	}

	if (frameImage) {
        graphics->setColor(ColorBlack);
		graphics->fillRectangle(gcn::Rectangle((frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2, img->getWidth(), img->getHeight()));
		graphics->drawImage(frameImage, 0, 0, 0, 0,
							frameImage->getWidth(), frameImage->getHeight());
		if (isPressed()) {
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
				}
			#endif
			}
			graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, ((frameImage->getWidth() - img->getWidth()) / 2) + 1, ((frameImage->getHeight() - img->getHeight()) / 2) + 1,
								img->getWidth() - 1, img->getHeight() - 1, WidgetPlayerColorIndexFromName);
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				}
			#endif
			}
			if (pressedframeImage) {
				graphics->drawImage(pressedframeImage, 0, 0, 0, 0,
									pressedframeImage->getWidth(), pressedframeImage->getHeight());
			}
		} else {
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
				}
			#endif
			}
			graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, (frameImage->getWidth() - img->getWidth()) / 2, (frameImage->getHeight() - img->getHeight()) / 2,
								img->getWidth(), img->getHeight(), WidgetPlayerColorIndexFromName);
			if (Transparency) {
			#if defined(USE_OPENGL) || defined(USE_GLES)
				if (UseOpenGL) {
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				}
			#endif
			}
		}
	} else {
		if (Transparency) {
		#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				glColor4ub(255, 255, 255, int(256 - 2.56 * Transparency));
			}
		#endif
		}
		graphics->drawImage(img, ImageOrigin.x, ImageOrigin.y, 0, 0,
							img->getWidth(), img->getHeight(), WidgetPlayerColorIndexFromName);
		if (Transparency) {
		#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			}
		#endif
		}
	}

	graphics->setColor(getForegroundColor());

	int textX;
	int textY = getHeight() / 2 - getFont()->getHeight() / 2;

	switch (getAlignment()) {
		case gcn::Graphics::LEFT:
			textX = 4;
			break;
		case gcn::Graphics::CENTER:
			textX = getWidth() / 2;
			break;
		case gcn::Graphics::RIGHT:
			textX = getWidth() - 4;
			break;
		default:
			textX = 0;
			Assert(!"Unknown alignment.");
			//throw GCN_EXCEPTION("Unknown alignment.");
	}

	graphics->setFont(getFont());
	bool is_normal = true;
	if (hasMouse()) {
		is_normal = false;
	}
	if (isPressed()) {
		graphics->drawText(getCaption(), textX + 4, textY + 4, getAlignment(), is_normal);
	} else {
		graphics->drawText(getCaption(), textX + 2, textY + 2, getAlignment(), is_normal);
	}

	if (isPressed() && !frameImage) {
//		graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));
		if (getWidth() == getHeight() && getWidth() > 64 && getHeight() > 64) {
			graphics->drawRectangle(gcn::Rectangle(0 + ((getWidth() - 64) / 2), 0 + ((getHeight() - 64) / 2), getWidth() - (getWidth() - 64), getHeight() - (getHeight() - 64))); //hack to make it appear properly in grand strategy mode
		} else {
			graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));
		}
	}

	//restore old alpha
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
	} else
#endif
	{
		WidgetGraphicTransparency(255, *((CGraphic *)img));
	}
}

/**
**  Automatically adjust the size of an image button
*/
void PlayerColorImageButton::adjustSize()
{
	if (frameImage) {
		setWidth(frameImage->getWidth());
		setHeight(frameImage->getHeight());
		setPosition(getX(), getY()); //reset position, to make it appropriate for the frame image
	} else if (normalImage) {
		setWidth(normalImage->getWidth());
		setHeight(normalImage->getHeight());
	} else {
		Button::adjustSize();
	}
}

void PlayerColorImageButton::setPosition(int x, int y)
{
	if (frameImage) {
		mDimension.x = x - ((frameImage->getWidth() - normalImage->getWidth()) / 2);
		mDimension.y = y - ((frameImage->getHeight() - normalImage->getWidth()) / 2);
	} else {
		mDimension.x = x;
		mDimension.y = y;
	}
}

/**
**  Change current color set to new player.
**
**  FIXME: use function pointer here.
**
**  @param player  Pointer to player.
**  @param sprite  The sprite in which the colors should be changed.
*/
void WidgetGraphicPlayerPixels(const std::string &WidgetPlayerColorName, const CGraphic &sprite)
{
	Assert(PlayerColorIndexCount);

	int WidgetPlayerColorIndexFromName = 15;
	for (int i = 0; i < PlayerColorMax; ++i) {
		if (PlayerColorNames[i] == WidgetPlayerColorName) {
			WidgetPlayerColorIndexFromName = i;
			break;
		}
	}

	SDL_LockSurface(sprite.Surface);
	std::vector<SDL_Color> sdlColors(PlayerColorsRGB[WidgetPlayerColorIndexFromName].begin(), PlayerColorsRGB[WidgetPlayerColorIndexFromName].end());
	SDL_SetColors(sprite.Surface, &sdlColors[0], PlayerColorIndexStart, PlayerColorIndexCount);
	if (sprite.SurfaceFlip) {
		SDL_SetColors(sprite.SurfaceFlip, &sdlColors[0], PlayerColorIndexStart, PlayerColorIndexCount);
	}
	SDL_UnlockSurface(sprite.Surface);
}

/**
**  Set transparency.
**
**  FIXME: use function pointer here.
**
**  @param player  Pointer to player.
**  @param sprite  The sprite in which the colors should be changed.
*/
void WidgetGraphicTransparency(int alpha, const CGraphic &sprite)
{
	SDL_LockSurface(sprite.Surface);
//	int oldalpha = Surface->format->alpha;
	SDL_SetAlpha(sprite.Surface, SDL_SRCALPHA, alpha);
	SDL_UnlockSurface(sprite.Surface);
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  ImageRadioButton
----------------------------------------------------------------------------*/


/**
**  ImageRadioButton constructor
*/
ImageRadioButton::ImageRadioButton() : gcn::RadioButton(),
	uncheckedNormalImage(NULL), uncheckedPressedImage(NULL), uncheckedDisabledImage(NULL),
	checkedNormalImage(NULL), checkedPressedImage(NULL), checkedDisabledImage(NULL),
	mMouseDown(false)
{
}

/**
**  ImageRadioButton constructor
*/
ImageRadioButton::ImageRadioButton(const std::string &caption,
								   const std::string &group, bool marked) :
	gcn::RadioButton(caption, group, marked),
	uncheckedNormalImage(NULL), uncheckedPressedImage(NULL), uncheckedDisabledImage(NULL),
	checkedNormalImage(NULL), checkedPressedImage(NULL), checkedDisabledImage(NULL),
	mMouseDown(false)
{
}

/**
**  Draw the image radio button (not the caption)
*/
void ImageRadioButton::drawBox(gcn::Graphics *graphics)
{
	gcn::Image *img = NULL;

	if (isMarked()) {
		if (isEnabled() == false) {
			img = checkedDisabledImage;
		} else if (mMouseDown) {
			img = checkedPressedImage;
		} else {
			img = checkedNormalImage;
		}
	} else {
		if (isEnabled() == false) {
			img = uncheckedDisabledImage;
		} else if (mMouseDown) {
			img = uncheckedPressedImage;
		} else {
			img = uncheckedNormalImage;
		}
	}

	if (img) {
		graphics->drawImage(img, 0, 0, 0, (getHeight() - img->getHeight()) / 2,
							img->getWidth(), img->getHeight());
	} else {
		RadioButton::drawBox(graphics);
	}
}

/**
**  Draw the image radio button
*/
void ImageRadioButton::draw(gcn::Graphics *graphics)
{
	drawBox(graphics);

	graphics->setFont(getFont());
	graphics->setColor(getForegroundColor());

	int width;
	if (uncheckedNormalImage) {
		width = uncheckedNormalImage->getWidth();
		width += width / 2;
	} else {
		width = getHeight();
		width += width / 2;
	}

	graphics->drawText(getCaption(), width - 2, 0);

	if (hasFocus()) {
		graphics->drawRectangle(gcn::Rectangle(width - 4, 0, getWidth() - width + 3, getHeight()));
	}
}

/**
**  Mouse button pressed callback
*/
void ImageRadioButton::mousePress(int, int, int button)
{
	if (button == gcn::MouseInput::LEFT && hasMouse()) {
		mMouseDown = true;
	}
}

/**
**  Mouse button released callback
*/
void ImageRadioButton::mouseRelease(int, int, int button)
{
	if (button == gcn::MouseInput::LEFT) {
		mMouseDown = false;
	}
}

/**
**  Mouse clicked callback
*/
void ImageRadioButton::mouseClick(int, int, int button, int)
{
	if (button == gcn::MouseInput::LEFT) {
		setMarked(true);
		generateAction();
	}
}

/**
**  Adjusts the size to fit the image and font size
*/
void ImageRadioButton::adjustSize()
{
	int width, height;

	height = getFont()->getHeight();
	if (uncheckedNormalImage) {
		width = uncheckedNormalImage->getWidth();
		width += width / 2;
		height = std::max(height, uncheckedNormalImage->getHeight());
	} else {
		width = getFont()->getHeight();
		width += width / 2;
	}

	setHeight(height);
	setWidth(getFont()->getWidth(mCaption) + width);
}


/*----------------------------------------------------------------------------
--  ImageCheckbox
----------------------------------------------------------------------------*/


/**
**  Image checkbox constructor
*/
ImageCheckBox::ImageCheckBox() : gcn::CheckBox(),
	uncheckedNormalImage(NULL), uncheckedPressedImage(NULL), uncheckedDisabledImage(NULL),
	checkedNormalImage(NULL), checkedPressedImage(NULL), checkedDisabledImage(NULL),
	mMouseDown(false)
{
}

/**
**  Image checkbox constructor
*/
ImageCheckBox::ImageCheckBox(const std::string &caption, bool marked) :
	gcn::CheckBox(caption, marked),
	uncheckedNormalImage(NULL), uncheckedPressedImage(NULL), uncheckedDisabledImage(NULL),
	checkedNormalImage(NULL), checkedPressedImage(NULL), checkedDisabledImage(NULL),
	mMouseDown(false)
{
}

/**
**  Draw the image checkbox
*/
void ImageCheckBox::draw(gcn::Graphics *graphics)
{
	drawBox(graphics);

	graphics->setFont(getFont());
	graphics->setColor(getForegroundColor());

	int width;
	if (uncheckedNormalImage) {
		width = uncheckedNormalImage->getWidth();
		width += width / 2;
	} else {
		width = getHeight();
		width += width / 2;
	}

	graphics->drawText(getCaption(), width - 2, 0);

	if (hasFocus()) {
		graphics->drawRectangle(gcn::Rectangle(width - 4, 0, getWidth() - width + 3, getHeight()));
	}
}

/**
**  Draw the checkbox (not the caption)
*/
void ImageCheckBox::drawBox(gcn::Graphics *graphics)
{
	gcn::Image *img = NULL;

	if (mMarked) {
		if (isEnabled() == false) {
			img = checkedDisabledImage;
		} else if (mMouseDown) {
			img = checkedPressedImage;
		} else {
			img = checkedNormalImage;
		}
	} else {
		if (isEnabled() == false) {
			img = uncheckedDisabledImage;
		} else if (mMouseDown) {
			img = uncheckedPressedImage;
		} else {
			img = uncheckedNormalImage;
		}
	}

	if (img) {
		graphics->drawImage(img, 0, 0, 0, (getHeight() - img->getHeight()) / 2,
							img->getWidth(), img->getHeight());
	} else {
		CheckBox::drawBox(graphics);
	}
}

/**
**  Mouse button pressed callback
*/
void ImageCheckBox::mousePress(int, int, int button)
{
	if (button == gcn::MouseInput::LEFT && hasMouse()) {
		mMouseDown = true;
	}
}

/**
**  Mouse button released callback
*/
void ImageCheckBox::mouseRelease(int, int, int button)
{
	if (button == gcn::MouseInput::LEFT) {
		mMouseDown = false;
	}
}

/**
**  Mouse clicked callback
*/
void ImageCheckBox::mouseClick(int, int, int button, int)
{
	if (button == gcn::MouseInput::LEFT) {
		toggle();
	}
}

/**
**  Adjusts the size to fit the image and font size
*/
void ImageCheckBox::adjustSize()
{
	int width, height;

	height = getFont()->getHeight();
	if (uncheckedNormalImage) {
		width = uncheckedNormalImage->getWidth();
		width += width / 2;
		height = std::max(height, uncheckedNormalImage->getHeight());
	} else {
		width = getFont()->getHeight();
		width += width / 2;
	}

	setHeight(height);
	setWidth(getFont()->getWidth(mCaption) + width);
}


/*----------------------------------------------------------------------------
--  ImageSlider
----------------------------------------------------------------------------*/


/**
**  Image slider constructor
*/
ImageSlider::ImageSlider(double scaleEnd) :
	Slider(scaleEnd), markerImage(NULL), backgroundImage(NULL), disabledBackgroundImage(NULL)
{
}

/**
**  Image slider constructor
*/
ImageSlider::ImageSlider(double scaleStart, double scaleEnd) :
	Slider(scaleStart, scaleEnd), markerImage(NULL), backgroundImage(NULL), disabledBackgroundImage(NULL)
{
}

/**
**  Draw the image slider marker
*/
void ImageSlider::drawMarker(gcn::Graphics *graphics)
{
	gcn::Image *img = markerImage;

	if (isEnabled()) {
		if (img) {
			if (getOrientation() == HORIZONTAL) {
				int v = getMarkerPosition();
				graphics->drawImage(img, 0, 0, v, 0,
					img->getWidth(), img->getHeight());
			} else {
				int v = (getHeight() - getMarkerLength()) - getMarkerPosition();
				graphics->drawImage(img, 0, 0, 0, v,
					img->getWidth(), img->getHeight());
			}
		} else {
			Slider::drawMarker(graphics);
		}
	}
}

/**
**  Draw the image slider
*/
void ImageSlider::draw(gcn::Graphics *graphics)
{
	gcn::Image *img = NULL;

	if (isEnabled()) {
		img = backgroundImage;
	} else {
		img = disabledBackgroundImage;
	}

	if (img) {
		graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
		if (isEnabled()) {
			drawMarker(graphics);
		}
	} else {
		Slider::draw(graphics);
	}
}

/**
**  Set the marker image
*/
void ImageSlider::setMarkerImage(gcn::Image *image)
{
	markerImage = image;
	setMarkerLength(image->getWidth());
}

/**
**  Set the background image
*/
void ImageSlider::setBackgroundImage(gcn::Image *image)
{
	backgroundImage = image;
}

/**
**  Set the disabled background image
*/
void ImageSlider::setDisabledBackgroundImage(gcn::Image *image)
{
	disabledBackgroundImage = image;
}


/*----------------------------------------------------------------------------
--  MultiLineLabel
----------------------------------------------------------------------------*/


/**
**  MultiLineLabel constructor
*/
MultiLineLabel::MultiLineLabel()
{
	this->mAlignment = LEFT;
	this->mVerticalAlignment = TOP;
	this->mLineWidth = 0;
}

/**
**  MultiLineLabel constructor
*/
MultiLineLabel::MultiLineLabel(const std::string &caption)
{
	this->mCaption = caption;
	this->mAlignment = LEFT;
	this->mVerticalAlignment = TOP;

	this->mLineWidth = 999999;
	this->wordWrap();
	this->adjustSize();
}

/**
**  Set the caption
*/
void MultiLineLabel::setCaption(const std::string &caption)
{
	this->mCaption = caption;
	this->wordWrap();
	this->setDirty(true);
}

/**
**  Get the caption
*/
const std::string &MultiLineLabel::getCaption() const
{
	return this->mCaption;
}

/**
**  Set the horizontal alignment
*/
void MultiLineLabel::setAlignment(unsigned int alignment)
{
	this->mAlignment = alignment;
}

/**
**  Get the horizontal alignment
*/
unsigned int MultiLineLabel::getAlignment()
{
	return this->mAlignment;
}

/**
**  Set the vertical alignment
*/
void MultiLineLabel::setVerticalAlignment(unsigned int alignment)
{
	this->mVerticalAlignment = alignment;
}

/**
**  Get the vertical alignment
*/
unsigned int MultiLineLabel::getVerticalAlignment()
{
	return this->mVerticalAlignment;
}

/**
**  Set the line width
*/
void MultiLineLabel::setLineWidth(int width)
{
	this->mLineWidth = width;
	this->wordWrap();
}

/**
**  Get the line width
*/
int MultiLineLabel::getLineWidth()
{
	return this->mLineWidth;
}

/**
**  Adjust the size
*/
void MultiLineLabel::adjustSize()
{
	int width = 0;
	for (int i = 0; i < (int)this->mTextRows.size(); ++i) {
		int w = this->getFont()->getWidth(this->mTextRows[i]);
		if (width < w) {
			width = std::min(w, this->mLineWidth);
		}
	}
	this->setWidth(width);
	this->setHeight(this->getFont()->getHeight() * this->mTextRows.size());
}

/**
**  Draw the label
*/
void MultiLineLabel::draw(gcn::Graphics *graphics)
{
	graphics->setFont(getFont());
	graphics->setColor(getForegroundColor());

	int textX, textY;
	switch (this->getAlignment()) {
		case LEFT:
			textX = 0;
			break;
		case CENTER:
			textX = this->getWidth() / 2;
			break;
		case RIGHT:
			textX = this->getWidth();
			break;
		default:
			textX = 0;
			Assert(!"Unknown alignment.");
			//throw GCN_EXCEPTION("Unknown alignment.");
	}
	switch (this->getVerticalAlignment()) {
		case TOP:
			textY = 0;
			break;
		case CENTER:
			textY = (this->getHeight() - (int)this->mTextRows.size() * this->getFont()->getHeight()) / 2;
			break;
		case BOTTOM:
			textY = this->getHeight() - (int)this->mTextRows.size() * this->getFont()->getHeight();
			break;
		default:
			textY = 0;
			Assert(!"Unknown alignment.");
			//throw GCN_EXCEPTION("Unknown alignment.");
	}

	for (int i = 0; i < (int)this->mTextRows.size(); ++i) {
		graphics->drawText(this->mTextRows[i], textX, textY + i * this->getFont()->getHeight(),
						   this->getAlignment());
	}
}

/**
**  Draw the border
*/
void MultiLineLabel::drawBorder(gcn::Graphics *graphics)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	for (unsigned int i = 0; i < getBorderSize(); ++i) {
		graphics->setColor(shadowColor);
		graphics->drawLine(i, i, width - i, i);
		graphics->drawLine(i, i + 1, i, height - i - 1);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i, i + 1, width - i, height - i);
		graphics->drawLine(i, height - i, width - i - 1, height - i);
	}
}

/**
**  Do word wrap
*/
void MultiLineLabel::wordWrap()
{
	gcn::Font *font = this->getFont();
	int lineWidth = this->getLineWidth();
	std::string str = this->getCaption();
	size_t pos, lastPos;
	std::string substr;
	bool done = false;
	bool first = true;

	this->mTextRows.clear();

	while (!done) {
		if (str.find('\n') != std::string::npos || font->getWidth(str) > lineWidth) {
			// string too wide or has a newline, split it up
			first = true;
			lastPos = 0;
			while (1) {
				// look for any whitespace
				pos = str.find_first_of(" \t\n", first ? 0 : lastPos + 1);
				if (pos != std::string::npos) {
					// found space, now check width
					substr = str.substr(0, pos);
					if (font->getWidth(substr) > lineWidth) {
						// sub-string is too big, use last good position
						if (first) {
							// didn't find a good last position
							substr = str.substr(0, pos);
							this->mTextRows.push_back(substr);
							str = str.substr(pos + 1);
							break;
						} else {
							substr = str.substr(0, lastPos);
							this->mTextRows.push_back(substr);
							// If we stopped at a space then skip any extra spaces but stop at a newline
							if (str[lastPos] != '\n') {
								while (str[lastPos + 1] == ' ' || str[lastPos + 1] == '\t' || str[lastPos + 1] == '\n') {
									++lastPos;
									if (str[lastPos] == '\n') {
										break;
									}
								}
							}
							str = str.substr(lastPos + 1);
							break;
						}
					} else {
						// sub-string is small enough
						// stop if we found a newline, otherwise look for next space
						if (str[pos] == '\n') {
							substr = str.substr(0, pos);
							this->mTextRows.push_back(substr);
							str = str.substr(pos + 1);
							break;
						}
					}
				} else {
					// no space found
					if (first) {
						// didn't find a good last position, we're done
						this->mTextRows.push_back(str);
						done = true;
						break;
					} else {
						substr = str.substr(0, lastPos);
						this->mTextRows.push_back(substr);
						str = str.substr(lastPos + 1);
						break;
					}
				}
				lastPos = pos;
				first = false;
			}
		} else {
			// string small enough
			this->mTextRows.push_back(str);
			done = true;
		}
	}
}


/*----------------------------------------------------------------------------
--  ScrollingWidget
----------------------------------------------------------------------------*/


/**
**  ScrollingWidget constructor.
**
**  @param width   Width of the widget.
**  @param height  Height of the widget.
*/
ScrollingWidget::ScrollingWidget(int width, int height) :
	gcn::ScrollArea(NULL, gcn::ScrollArea::SHOW_NEVER, gcn::ScrollArea::SHOW_NEVER),
	speedY(1.f), containerY(0.f), finished(false)
{
	container.setDimension(gcn::Rectangle(0, 0, width, height));
	container.setOpaque(false);
	setContent(&container);
	setDimension(gcn::Rectangle(0, 0, width, height));
}

/**
**  Add a widget in the window.
**
**  @param widget  Widget to add.
**  @param x       Position of the widget in the window.
**  @param y       Position of the widget in the window.
*/
void ScrollingWidget::add(gcn::Widget *widget, int x, int y)
{
	container.add(widget, x, y);
	if (x + widget->getWidth() > container.getWidth()) {
		container.setWidth(x + widget->getWidth());
	}
	if (y + widget->getHeight() > container.getHeight()) {
		container.setHeight(y + widget->getHeight());
	}
}

/**
**  Scrolling the content when possible.
*/
void ScrollingWidget::logic()
{
	setDirty(true);
	if (container.getHeight() + containerY - speedY > 0) {
		// the bottom of the container is lower than the top
		// of the widget. It is thus still visible.
		containerY -= speedY;
		container.setY((int)containerY);
	} else if (!finished) {
		finished = true;
		generateAction();
	}
}

/**
**  Restart animation to the beginning.
*/
void ScrollingWidget::restart()
{
	container.setY(0);
	containerY = 0.f;
	finished = (container.getHeight() == getHeight());
}


/*----------------------------------------------------------------------------
--  Windows
----------------------------------------------------------------------------*/


/**
**  Windows constructor.
**
**  @param title   Title of the window.
**  @param width   Width of the window.
**  @param height  Height of the window.
*/
Windows::Windows(const std::string &title, int width, int height) :
	Window(title), blockwholewindow(true)
{
	container.setDimension(gcn::Rectangle(0, 0, width, height));
	scroll.setDimension(gcn::Rectangle(0, 0, width, height));
	this->setContent(&scroll);
	scroll.setContent(&container);
	this->resizeToContent();
}

/**
**  Add a widget in the window.
**
**  @param widget  Widget to add.
**  @param x       Position of the widget in the window.
**  @param y       Position of the widget in the window.
*/
void Windows::add(gcn::Widget *widget, int x, int y)
{
	container.add(widget, x, y);
	if (x + widget->getWidth() > container.getWidth()) {
		container.setWidth(x + widget->getWidth());
	}
	if (y + widget->getHeight() > container.getHeight()) {
		container.setHeight(y + widget->getHeight());
	}
}

/**
**  Move the window when it is dragged.
**
**  @param x   X coordinate of the mouse relative to the window.
**  @param y   Y coordinate of the mouse relative to the widndow.
**
**  @note Once dragged, without release the mouse,
**    if you go virtually outside the container then go back,
**    you have to wait the virtual cursor are in the container.
**    It is because x, y argument refer to a virtual cursor :(
**  @note An another thing is strange
**    when the container is a "scrollable" ScrollArea with the cursor.
**    The cursor can go outside the visual area.
*/
void Windows::mouseMotion(int x, int y)
{
	gcn::BasicContainer *bcontainer = getParent();
	int diffx;
	int diffy;
	int criticalx;
	int criticaly;
	int absx;
	int absy;

	if (!mMouseDrag || !isMovable()) {
		return;
	}

	diffx = x - mMouseXOffset;
	diffy = y - mMouseYOffset;
	if (blockwholewindow) {
		criticalx = getX();
		criticaly = getY();
	} else {
		criticalx = getX() + mMouseXOffset;
		criticaly = getY() + mMouseYOffset;
	}


	if (criticalx + diffx < 0) {
		diffx = -criticalx;
	}
	if (criticaly + diffy < 0) {
		diffy = -criticaly;
	}

	if (blockwholewindow) {
		criticalx = getX() + getWidth();
		criticaly = getY() + getHeight();
	}
	if (criticalx + diffx >= bcontainer->getWidth()) {
		diffx = bcontainer->getWidth() - criticalx;
	}
	if (criticaly + diffy >= bcontainer->getHeight()) {
		diffy = bcontainer->getHeight() - criticaly;
	}

	// Place the window.
	x = getX() + diffx;
	y = getY() + diffy;
	setPosition(x, y);

	// Move the cursor.
	// Useful only when window reachs the limit.
	getAbsolutePosition(absx, absy);
	CursorScreenPos.x = absx + mMouseXOffset;
	CursorScreenPos.y = absy + mMouseYOffset;
}

/**
**  Set background color of the window.
**
**  @param color  Color to set.
*/
void Windows::setBackgroundColor(const gcn::Color &color)
{
	Window::setBackgroundColor(color);
	scroll.setBackgroundColor(color);
}

/**
**  Set base color of the windows.
**
**  @param color  Color to set.
*/
void Windows::setBaseColor(const gcn::Color &color)
{
	Window::setBaseColor(color);
	container.setBaseColor(color);
}

/*----------------------------------------------------------------------------
--  ImageTextField
----------------------------------------------------------------------------*/

void ImageTextField::draw(gcn::Graphics *graphics)
{
	gcn::Font *font;
	int x, y;
	CGraphic *img = this->itemImage;
	if (!img) {
		fprintf(stderr, "Not all graphics for ImageTextField were set\n");
		ExitFatal(1);
	}
	img->Resize(getWidth(), img->getHeight());
	graphics->drawImage(img, 0, 0, 0, 0, getWidth(), img->getHeight());

	if (hasFocus())
	{
		drawCaret(graphics, getFont()->getWidth(mText.substr(0, mCaretPosition)) - mXScroll);
	}

	graphics->setColor(getForegroundColor());
	font = getFont();
	graphics->setFont(font);

	x = 1 - mXScroll;
	y = 1;

	if (mSelectEndOffset != 0)
	{
		unsigned int first;
		unsigned int len;
		int selX;
		int selW;
		std::string tmpStr;

		getTextSelectionPositions(&first, &len);

		tmpStr = std::string(mText.substr(0, first));
		selX = font->getWidth(tmpStr);

		tmpStr = std::string(mText.substr(first, len));
		selW = font->getWidth(tmpStr);

		graphics->setColor(gcn::Color(127, 127, 127));
		graphics->fillRectangle(gcn::Rectangle(x + selX, y, selW, font->getHeight()));
	}

	graphics->drawText(mText, x, y);
}

void ImageTextField::drawBorder(gcn::Graphics *graphics)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	height = itemImage ? std::max<int>(height, itemImage->getHeight()) : height;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i);
		graphics->drawLine(i,i + 1, i, height - i - 1);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i);
		graphics->drawLine(i,height - i, width - i - 1, height - i);
	}
}

/*----------------------------------------------------------------------------
--  LuaListModel
----------------------------------------------------------------------------*/


/**
**  Set the list
*/
void LuaListModel::setList(lua_State *lua, lua_Object *lo)
{
	list.clear();

	const int args = lua_rawlen(lua, *lo);
	for (int j = 0; j < args; ++j) {
		list.push_back(std::string(LuaToString(lua, *lo, j + 1)));
	}
}

/*----------------------------------------------------------------------------
--  ImageListBox
----------------------------------------------------------------------------*/

ImageListBox::ImageListBox() : gcn::ListBox(), itemImage(NULL)
{
}

ImageListBox::ImageListBox(gcn::ListModel *listModel) : gcn::ListBox(listModel), itemImage(NULL)
{
}

void ImageListBox::draw(gcn::Graphics *graphics)
{
	if (mListModel == NULL) {
		return;
	}

	graphics->setColor(getForegroundColor());
	graphics->setFont(getFont());

	int i, fontHeight;
	int y = 0;
	CGraphic *img = itemImage;
	//Wyrmgus start
//	img->Resize(getWidth(), img->getHeight());
	//Wyrmgus end

	fontHeight = std::max<int>(getFont()->getHeight(), img->getHeight());

    /**
        * @todo Check cliprects so we do not have to iterate over elements in the list model
        */
	for (i = 0; i < mListModel->getNumberOfElements(); ++i) {
		graphics->drawImage(img, 0, 0, 0, y, getWidth(), img->getHeight());
		if (i == mSelected) {
			graphics->drawText("~<" + mListModel->getElementAt(i) + "~>", 1, y + (fontHeight - getFont()->getHeight()) / 2);
		} else {
			graphics->drawText(mListModel->getElementAt(i), 1, y + (fontHeight - getFont()->getHeight()) / 2);
		}

		y += fontHeight;
	}
	img->SetOriginalSize();
}

void ImageListBox::drawBorder(gcn::Graphics *graphics)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i);
		graphics->drawLine(i,i + 1, i, height - i - 1);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i);
		graphics->drawLine(i,height - i, width - i - 1, height - i);
	}
}

void ImageListBox::adjustSize()
{
	if (mListModel != NULL)
	{
		setHeight((itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight()) * mListModel->getNumberOfElements());
	}
}

void ImageListBox::mousePress(int, int y, int button)
{
	if (button == gcn::MouseInput::LEFT && hasMouse())
	{
		setSelected(y / (itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight()));
		generateAction();
	}
}

void ImageListBox::setSelected(int selected)
{
	if (mListModel == NULL)
	{
		mSelected = -1;
	}
	else
	{
		if (selected < 0)
		{
			mSelected = -1;
		}
		else if (selected >= mListModel->getNumberOfElements())
		{
			mSelected = mListModel->getNumberOfElements() - 1;
		}
		else
		{
			mSelected = selected;
		}

		Widget *par = getParent();
		if (par == NULL)
		{
			return;
		}

		gcn::ScrollArea* scrollArea = dynamic_cast<gcn::ScrollArea *>(par);
		if (scrollArea != NULL)
		{
			gcn::Rectangle scroll;
			scroll.y = (itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight()) * mSelected;
			scroll.height = (itemImage ? std::max<int>(getFont()->getHeight(), itemImage->getHeight()) : getFont()->getHeight());
			scrollArea->scrollToRectangle(scroll);
		}
	}
}

void ImageListBox::setListModel(gcn::ListModel *listModel)
{
	mSelected = -1;
	mListModel = listModel;
	adjustSize();
}


/*----------------------------------------------------------------------------
--  ListBoxWidget
----------------------------------------------------------------------------*/


/**
**  ListBoxWidget constructor.
**
**  @todo  Size should be parametrable, maybe remove default constructor?
*/
ListBoxWidget::ListBoxWidget(unsigned int width, unsigned int height)
{
	setDimension(gcn::Rectangle(0, 0, width, height));
	setContent(&listbox);
	setBackgroundColor(gcn::Color(128, 128, 128));
}

/**
**  ImageListBoxWidget constructor.
**
**  @todo  Size should be parametrable, maybe remove default constructor?
*/
ImageListBoxWidget::ImageListBoxWidget(unsigned int width, unsigned int height) : ListBoxWidget(width, height),
	upButtonImage(NULL), downButtonImage(NULL), leftButtonImage(NULL), rightButtonImage(NULL), hBarButtonImage(NULL), 
	vBarButtonImage(NULL),	markerImage(NULL)
{
	setDimension(gcn::Rectangle(0, 0, width, height));
	setContent(&listbox);
}


/**
**  Set the list
*/
void ListBoxWidget::setList(lua_State *lua, lua_Object *lo)
{
	lualistmodel.setList(lua, lo);
	listbox.setListModel(&lualistmodel);
	adjustSize();
}

/**
**  Set the list
*/
void ImageListBoxWidget::setList(lua_State *lua, lua_Object *lo)
{
	lualistmodel.setList(lua, lo);
	listbox.setListModel(&lualistmodel);
	adjustSize();
}

/**
**  Sets the ListModel index of the selected element.
**
**  @param selected  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
void ListBoxWidget::setSelected(int selected)
{
	listbox.setSelected(selected);
}

/**
**  Sets the ListModel index of the selected element.
**
**  @param selected  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
void ImageListBoxWidget::setSelected(int selected)
{
	listbox.setSelected(selected);
}

/**
**  Gets the ListModel index of the selected element.
**
**  @return  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
int ListBoxWidget::getSelected() const
{
	return const_cast<gcn::ListBox &>(listbox).getSelected();
}

/**
**  Gets the ListModel index of the selected element.
**
**  @return  The ListModel index of the selected element.
**
**  @see gcn::ListBox
*/
int ImageListBoxWidget::getSelected() const
{
	return const_cast<ImageListBox &>(listbox).getSelected();
}

/**
**  Set background color of the ListBoxWidget.
**
**  @param color  Color to set.
*/
void ListBoxWidget::setBackgroundColor(const gcn::Color &color)
{
	ScrollArea::setBackgroundColor(color);
	ScrollArea::setBaseColor(color);
	listbox.setBackgroundColor(color);
}

/**
**  Set background color of the ListBoxWidget.
**
**  @param color  Color to set.
*/
void ImageListBoxWidget::setBackgroundColor(const gcn::Color &color)
{
	ScrollArea::setBackgroundColor(color);
	ScrollArea::setBaseColor(color);
	listbox.setBackgroundColor(color);
}

/**
**  Set font of the ListBox.
**
**  @param font  Font to set.
*/
void ListBoxWidget::setFont(gcn::Font *font)
{
	listbox.setFont(font);
	listbox.setWidth(getWidth());
	adjustSize();
}

/**
**  Set font of the ListBox.
**
**  @param font  Font to set.
*/
void ImageListBoxWidget::setFont(gcn::Font *font)
{
	listbox.setFont(font);
	listbox.setWidth(getWidth());
	adjustSize();
}

/**
**  Adjust size of the listBox.
**
**  @todo Fix width of the scroll area (depend of v-scroll or not).
*/
void ListBoxWidget::adjustSize()
{
	int i;
	int width;
	gcn::ListModel *listmodel;

	width = listbox.getWidth();
	Assert(listbox.getListModel());
	listmodel = listbox.getListModel();
	for (i = 0; i < listmodel->getNumberOfElements(); ++i) {
		if (width < listbox.getFont()->getWidth(listmodel->getElementAt(i))) {
			width = listbox.getFont()->getWidth(listmodel->getElementAt(i));
		}
	}
	if (width != listbox.getWidth()) {
		listbox.setWidth(width);
	}
}

/**
**  Adjust size of the listBox.
**
**  @todo Fix width of the scroll area (depend of v-scroll or not).
*/
void ImageListBoxWidget::adjustSize()
{
	int i;
	int width;
	gcn::ListModel *listmodel;

	width = listbox.getWidth();
	Assert(listbox.getListModel());
	listmodel = listbox.getListModel();
	for (i = 0; i < listmodel->getNumberOfElements(); ++i) {
		if (width < listbox.getFont()->getWidth(listmodel->getElementAt(i))) {
			width = listbox.getFont()->getWidth(listmodel->getElementAt(i));
		}
	}
	if (width != listbox.getWidth()) {
		listbox.setWidth(width);
	}
}

/**
**  Add an action listener
*/
void ListBoxWidget::addActionListener(gcn::ActionListener *actionListener)
{
	listbox.addActionListener(actionListener);
}

/**
**  Add an action listener
*/
void ImageListBoxWidget::addActionListener(gcn::ActionListener *actionListener)
{
	listbox.addActionListener(actionListener);
}



/**
**  Draw the list box  
**
**  @param  graphics Graphics to use
*/
void ImageListBoxWidget::draw(gcn::Graphics *graphics)
{
	CGraphic *img = NULL;

	// Check if we have all required graphics
	if (!this->upButtonImage || !this->downButtonImage || !this->leftButtonImage || !this->rightButtonImage
		|| !this->upPressedButtonImage || !this->downPressedButtonImage || !this->leftPressedButtonImage || !this->rightPressedButtonImage
		|| !this->markerImage || !this->hBarButtonImage || !this->vBarButtonImage) {
			fprintf(stderr, "Not all graphics for ImageListBoxWidget were set\n");
			ExitFatal(1);
	}

	gcn::Rectangle rect = getContentDimension();
	img = itemImage;
	img->Resize(rect.width, img->getHeight());
	int y = 0;
	while (y + img->getHeight() <= rect.height) {
		graphics->drawImage(img, 0, 0, 0, y, getWidth(), img->getHeight());
		y += img->getHeight();
	}
	img->SetOriginalSize();
	
	if (mVBarVisible)
	{
		if (mUpButtonPressed) {
			this->drawUpPressedButton(graphics);
		} else {
			this->drawUpButton(graphics);
		}
		if (mDownButtonPressed) {
			this->drawDownPressedButton(graphics);
		} else {
			this->drawDownButton(graphics);
		}
		this->drawVBar(graphics);
		this->drawVMarker(graphics);
	}
	if (mHBarVisible)
	{
		if (mLeftButtonPressed) {
			this->drawLeftPressedButton(graphics);
		} else {
			this->drawLeftButton(graphics);
		}
		if (mRightButtonPressed) {
			this->drawRightPressedButton(graphics);
		} else {
			this->drawRightButton(graphics);
		}
		this->drawHBar(graphics);
		this->drawHMarker(graphics);
	}
	if (mContent)
	{
		gcn::Rectangle contdim = mContent->getDimension();
		graphics->pushClipArea(getContentDimension());

		if (mContent->getBorderSize() > 0)
		{
			img = this->itemImage;
			gcn::Rectangle rec = mContent->getDimension();
			rec.x -= mContent->getBorderSize();
			rec.y -= mContent->getBorderSize();
			rec.width += 2 * mContent->getBorderSize();
			rec.height += 2 * mContent->getBorderSize();
			graphics->pushClipArea(rec);
			mContent->drawBorder(graphics);
			graphics->popClipArea();
		}

		graphics->pushClipArea(contdim);
		mContent->draw(graphics);
		graphics->popClipArea();
		graphics->popClipArea();
	}
}

/**
**  Draw the list box border 
**
**  @param  graphics Graphics to use
*/
void ImageListBoxWidget::drawBorder(gcn::Graphics *graphics)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i);
		graphics->drawLine(i,i + 1, i, height - i - 1);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i);
		graphics->drawLine(i,height - i, width - i - 1, height - i);
	}
}

void ImageListBoxWidget::drawUpButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getUpButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = upButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawDownButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getDownButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = downButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawLeftButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getLeftButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = leftButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawRightButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getRightButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = rightButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawUpPressedButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getUpButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = upPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawDownPressedButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getDownButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = downPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawLeftPressedButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getLeftButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = leftPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawRightPressedButton(gcn::Graphics* graphics)
{
	gcn::Rectangle dim = getRightButtonDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = rightPressedButtonImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	graphics->popClipArea();
}

void ImageListBoxWidget::drawHBar(gcn::Graphics *graphics)
{
	gcn::Rectangle dim = getHorizontalBarDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = hBarButtonImage;
	img->Resize(dim.width, dim.height);
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	img->SetOriginalSize();

	graphics->popClipArea();
}

void ImageListBoxWidget::drawVBar(gcn::Graphics *graphics)
{
	gcn::Rectangle dim = getVerticalBarDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = vBarButtonImage;
	img->Resize(dim.width, dim.height);
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	img->SetOriginalSize();

	graphics->popClipArea();
}

void ImageListBoxWidget::drawHMarker(gcn::Graphics *graphics)
{
	gcn::Rectangle dim = getHorizontalMarkerDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = markerImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());

	graphics->popClipArea();
}

void ImageListBoxWidget::drawVMarker(gcn::Graphics *graphics)
{
	gcn::Rectangle dim = getVerticalMarkerDimension();
	graphics->pushClipArea(dim);

	CGraphic *img = NULL;

	img = markerImage;
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());

	graphics->popClipArea();
}

gcn::Rectangle ImageListBoxWidget::getVerticalMarkerDimension()
{
	if (!mVBarVisible)
	{
		return gcn::Rectangle(0, 0, 0, 0);
	}

	int length, pos;
	gcn::Rectangle barDim = getVerticalBarDimension();

	if (mContent && mContent->getHeight() != 0)
	{
		length = this->markerImage->getHeight();
	}
	else
	{
		length = barDim.height;
	}

	if (length < mScrollbarWidth)
	{
		length = mScrollbarWidth;
	}

	if (length > barDim.height)
	{
		length = barDim.height;
	}

	if (getVerticalMaxScroll() != 0)
	{
		pos = ((barDim.height - length) * getVerticalScrollAmount())
			/ getVerticalMaxScroll();
	}
	else
	{
		pos = 0;
	}

	return gcn::Rectangle(barDim.x, barDim.y + pos, mScrollbarWidth, length);
}

gcn::Rectangle ImageListBoxWidget::getHorizontalMarkerDimension()
{
	if (!mHBarVisible)
	{
		return gcn::Rectangle(0, 0, 0, 0);
	}

	int length, pos;
	gcn::Rectangle barDim = getHorizontalBarDimension();

	if (mContent && mContent->getWidth() != 0)
	{
		length = this->markerImage->getHeight();
	}
	else
	{
		length = barDim.width;
	}

	if (length < mScrollbarWidth)
	{
		length = mScrollbarWidth;
	}

	if (length > barDim.width)
	{
		length = barDim.width;
	}

	if (getHorizontalMaxScroll() != 0)
	{
		pos = ((barDim.width - length) * getHorizontalScrollAmount())
			/ getHorizontalMaxScroll();
	}
	else
	{
		pos = 0;
	}

	return gcn::Rectangle(barDim.x + pos, barDim.y, length, mScrollbarWidth);
}


/*----------------------------------------------------------------------------
--  DropDownWidget
----------------------------------------------------------------------------*/


/**
**  Set the list
*/
void DropDownWidget::setList(lua_State *lua, lua_Object *lo)
{
	listmodel.setList(lua, lo);
	setListModel(&listmodel);
}

/**
**  Set the drop down size
*/
void DropDownWidget::setSize(int width, int height)
{
	DropDown::setSize(width, height);
	this->getListBox()->setSize(width, height);
}

/*----------------------------------------------------------------------------
--  ImageDropDownWidget
----------------------------------------------------------------------------*/

/**
**  Set the list
*/

void ImageDropDownWidget::setListModel(LuaListModel *listModel)
{
	Assert(mScrollArea && mScrollArea->getContent() != NULL);

	mListBox.setListModel(listModel);

	if (mListBox.getSelected() < 0)
	{
		mListBox.setSelected(0);
	}

	adjustHeight();
}

void ImageDropDownWidget::setList(lua_State *lua, lua_Object *lo)
{
	listmodel.setList(lua, lo);
	setListModel(&listmodel);
}

/**
**  Set the drop down size
*/
void ImageDropDownWidget::setSize(int width, int height)
{
	DropDown::setSize(width, height);
	this->getListBox()->setSize(width, height);
}

void ImageDropDownWidget::draw(gcn::Graphics *graphics)
{
	Assert(mScrollArea && mScrollArea->getContent() != NULL);
	int h;

	if (mDroppedDown)
	{
		h = mOldH;
	}
	else
	{
		h = getHeight();
	}

	CGraphic *img = this->itemImage;
	if (!this->itemImage || !this->DownNormalImage || !this->DownPressedImage) {
		fprintf(stderr, "Not all graphics for ImageDropDownWidget were set\n");
		ExitFatal(1);
	}

	int alpha = getBaseColor().a;
	gcn::Color faceColor = getBaseColor();
	faceColor.a = alpha;
	gcn::Color highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	gcn::Color shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;


	//Wyrmgus start
//	img->Resize(getWidth(), h);
//	graphics->drawImage(img, 0, 0, 0, 0, getWidth(), h);
//	img->SetOriginalSize();
	graphics->drawImage(img, 0, 0, 0, 0, img->getWidth(), img->getHeight());
	//Wyrmgus end
	
	graphics->setFont(getFont());

	if (mListBox.getListModel() && mListBox.getSelected() >= 0)
	{
		graphics->drawText(mListBox.getListModel()->getElementAt(mListBox.getSelected()),
			1, (h - getFont()->getHeight()) / 2);
	}

	//Wyrmgus start
	/*
	if (hasFocus())
	{
		graphics->drawRectangle(gcn::Rectangle(0, 0, getWidth() - h, h));
	}
	*/
	//Wyrmgus end

	drawButton(graphics);

	if (mDroppedDown)
	{
		graphics->pushClipArea(mScrollArea->getDimension());
		mScrollArea->draw(graphics);
		graphics->popClipArea();

		// Draw two lines separating the ListBox with se selected
		// element view.
		//Wyrmgus start
		/*
		graphics->setColor(highlightColor);
		graphics->drawLine(0, h, getWidth(), h);
		graphics->setColor(shadowColor);
		graphics->drawLine(0, h + 1,getWidth(),h + 1);
		*/
		//Wyrmgus end
	}
}

void ImageDropDownWidget::drawBorder(gcn::Graphics *graphics)
{
	gcn::Color faceColor = getBaseColor();
	gcn::Color highlightColor, shadowColor;
	int alpha = getBaseColor().a;
	int width = getWidth() + getBorderSize() * 2 - 1;
	int height = getHeight() + getBorderSize() * 2 - 1;
	highlightColor = faceColor + 0x303030;
	highlightColor.a = alpha;
	shadowColor = faceColor - 0x303030;
	shadowColor.a = alpha;

	unsigned int i;
	for (i = 0; i < getBorderSize(); ++i)
	{
		graphics->setColor(shadowColor);
		graphics->drawLine(i,i, width - i, i);
		graphics->drawLine(i,i + 1, i, height - i - 1);
		graphics->setColor(highlightColor);
		graphics->drawLine(width - i,i + 1, width - i, height - i);
		graphics->drawLine(i,height - i, width - i - 1, height - i);
	}
}

void ImageDropDownWidget::drawButton(gcn::Graphics *graphics)
{
	int h;
	if (mDroppedDown)
	{
		h = mOldH;
	}
	else
	{
		h = getHeight();
	}
	//Wyrmgus start
//	int x = getWidth() - h;
	int x = getWidth() - (h - 1);
	//Wyrmgus end
	int y = 0;

	CGraphic *img = NULL;
	if (mDroppedDown) {
		img = this->DownPressedImage;
	} else {
		img = this->DownNormalImage;
	}
	//Wyrmgus start
//	img->Resize(h, h);
	//Wyrmgus end
	graphics->drawImage(img, 0, 0, x, y, h, h);
	//Wyrmgus start
//	img->SetOriginalSize();
	//Wyrmgus end
}

int ImageDropDownWidget::getSelected()
{
	Assert(mScrollArea && mScrollArea->getContent() != NULL);

	return mListBox.getSelected();
}

void ImageDropDownWidget::setSelected(int selected)
{
	Assert(mScrollArea && mScrollArea->getContent() != NULL);

	if (selected >= 0)
	{
		mListBox.setSelected(selected);
	}
}

void ImageDropDownWidget::adjustHeight()
{
	Assert(mScrollArea && mScrollArea->getContent() != NULL);

	int listBoxHeight = mListBox.getHeight();
	int h2 = mOldH ? mOldH : getFont()->getHeight();

	setHeight(h2);

	// The addition/subtraction of 2 compensates for the seperation lines
	// seperating the selected element view and the scroll area.

	if (mDroppedDown && getParent())
	{
		int h = getParent()->getHeight() - getY();

		if (listBoxHeight > h - h2 - 2)
		{
			mScrollArea->setHeight(h - h2 - 2);
			setHeight(h);
		}
		else
		{
			setHeight(listBoxHeight + h2 + 2);
			mScrollArea->setHeight(listBoxHeight);
		}
	}

	mScrollArea->setWidth(getWidth());
	mScrollArea->setPosition(0, h2 + 2);
}

void ImageDropDownWidget::setListBox(ImageListBox *listBox)
{
	listBox->setSelected(mListBox.getSelected());
	listBox->setListModel(mListBox.getListModel());
	listBox->addActionListener(this);

	if (mScrollArea->getContent() != NULL)
	{
		mListBox.removeActionListener(this);
	}

	mListBox = *listBox;

	mScrollArea->setContent(&mListBox);

	if (mListBox.getSelected() < 0)
	{
		mListBox.setSelected(0);
	}
}

void ImageDropDownWidget::setFont(gcn::Font *font)
{
	gcn::Widget::setFont(font);
	mListBox.setFont(font);
}

void ImageDropDownWidget::_mouseInputMessage(const gcn::MouseInput &mouseInput)
{
	gcn::BasicContainer::_mouseInputMessage(mouseInput);

	if (mDroppedDown)
	{
		Assert(mScrollArea && mScrollArea->getContent() != NULL);

		if (mouseInput.y >= mOldH)
		{
			gcn::MouseInput mi = mouseInput;
			mi.y -= mScrollArea->getY();
			mScrollArea->_mouseInputMessage(mi);

			if (mListBox.hasFocus())
			{
				mi.y -= mListBox.getY();
				mListBox._mouseInputMessage(mi);
			}
		}
	}
}

/**
**  StatBoxWidget constructor
**
**  @param width   Width of the StatBoxWidget.
**  @param height  Height of the StatBoxWidget.
*/
StatBoxWidget::StatBoxWidget(int width, int height) : percent(100)
{
	setWidth(width);
	setHeight(height);

	setBackgroundColor(gcn::Color(0, 0, 0));
	setBaseColor(gcn::Color(255, 255, 255));
	setForegroundColor(gcn::Color(128, 128, 128));
}

/**
**  Draw StatBoxWidget.
**
**  @param graphics  Graphic driver used to draw.
**
**  @todo caption seem to be placed upper than the middle.
**  @todo set direction (hor./vert.) and growing direction(up/down, left/rigth).
*/
void StatBoxWidget::draw(gcn::Graphics *graphics)
{
	int width;
	int height;

	width = getWidth();
	height = getHeight();

	graphics->setColor(getBackgroundColor());
	graphics->fillRectangle(gcn::Rectangle(0, 0, width, height));

	graphics->setColor(getBaseColor());
	graphics->drawRectangle(gcn::Rectangle(1, 1, width - 2, height - 2));

	graphics->setColor(getForegroundColor());
	width = percent * width / 100;
	graphics->fillRectangle(gcn::Rectangle(2, 2, width - 4, height - 4));
	graphics->setFont(getFont());
	graphics->drawText(getCaption(),
					   (getWidth() - getFont()->getWidth(getCaption())) / 2,
					   (height - getFont()->getHeight()) / 2);
}

/**
**  Set caption of StatBoxWidget.
**
**  @param caption  New value.
*/
void StatBoxWidget::setCaption(const std::string &caption)
{
	this->caption = caption;
	this->setDirty(true);
}

/**
**  Get caption of StatBoxWidget.
*/

const std::string &StatBoxWidget::getCaption() const
{
	return caption;
}

/**
**  Set percent of StatBoxWidget.
**
**  @param percent  New value.
*/
void StatBoxWidget::setPercent(const int percent)
{
	this->setDirty(true);
	this->percent = percent;
}

/**
**  Get percent of StatBoxWidget.
*/
int StatBoxWidget::getPercent() const
{
	return percent;
}


/*----------------------------------------------------------------------------
--  MenuScreen
----------------------------------------------------------------------------*/


/**
**  MenuScreen constructor
*/
MenuScreen::MenuScreen() :
	Container(), runLoop(true), logiclistener(0), drawUnder(false), running(false)
{
	setDimension(gcn::Rectangle(0, 0, Video.Width, Video.Height));
	setOpaque(false);

	// The gui must be set immediately as it is used by widgets
	// when they are added to the container
	oldtop = Gui->getTop();
	Gui->setTop(this);
}

/**
**  Run the menu.  Loops until stop is called.
*/
int MenuScreen::run(bool loop)
{
	loopResult = 0;
	runLoop = loop;
	running = true;

	CursorState = CursorStatePoint;
	GameCursor = UI.Point.Cursor;
	CursorOn = CursorOnUnknown;

	CallbackMusicOn();

	if (loop) {
		const EventCallback *old_callbacks = GetCallbacks();
		SetCallbacks(&GuichanCallbacks);
		while (runLoop) {
			UpdateDisplay();
			RealizeVideoMemory();
			CheckMusicFinished();
			WaitEventsOneFrame();
		}
		SetCallbacks(old_callbacks);
		Gui->setTop(this->oldtop);
	} else {
		SetCallbacks(&GuichanCallbacks);
		MenuStack.push(this);
	}

	return this->loopResult;
}

/**
**  Stop the menu from running
*/
void MenuScreen::stop(int result, bool stopAll)
{
	if (running == false)
		return;

	if (!this->runLoop) {
		Gui->setTop(this->oldtop);
		Assert(MenuStack.top() == this);
		MenuStack.pop();
		if (stopAll) {
			while (!MenuStack.empty()) {
				MenuStack.pop();
			}
		}
		if (MenuStack.empty()) {
			//InterfaceState = IfaceStateNormal;
			if (!Editor.Running) {
				SetCallbacks(&GameCallbacks);
			} else {
				SetCallbacks(&EditorCallbacks);
			}
			GamePaused = false;
			UI.StatusLine.Clear();
			if (GameRunning) {
				UIHandleMouseMove(CursorScreenPos);
			}
		}
	}

	runLoop = false;
	loopResult = result;
	running = false;
}

void MenuScreen::addLogicCallback(LuaActionListener *listener)
{
	logiclistener = listener;
}

void MenuScreen::draw(gcn::Graphics *graphics)
{
	if (this->drawUnder) {
		gcn::Rectangle r = Gui->getGraphics()->getCurrentClipArea();
		Gui->getGraphics()->popClipArea();
		Gui->draw(oldtop);
		Gui->getGraphics()->pushClipArea(r);
	}
	gcn::Container::draw(graphics);
}

void MenuScreen::logic()
{
	if (NetConnectRunning == 2) {
		NetworkProcessClientRequest();
	}
	if (NetConnectRunning == 1) {
		NetworkProcessServerRequest();
	}
	if (logiclistener) {
		logiclistener->action("");
	}
	Container::logic();
}

//@}
