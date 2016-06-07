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
/**@name mainloop.cpp - The main game loop. */
//
//      (c) Copyright 1998-2006 by Lutz Sammer and Jimmy Salmon
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

//----------------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------------

#include "stratagus.h"

#include "actions.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "editor.h"
//Wyrmgus start
#include "font.h"
//Wyrmgus end
#include "game.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "interface.h"
//Wyrmgus end
#include "map.h"
#include "missile.h"
#include "network.h"
#include "particle.h"
#include "replay.h"
#include "results.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "sound.h"
#include "sound_server.h"
//Wyrmgus start
#include "tileset.h" // for tile animation
//Wyrmgus end
#include "translate.h"
#include "trigger.h"
#include "ui.h"
#include "unit.h"
//Wyrmgus start
#include "unit_manager.h"
//Wyrmgus end
#include "video.h"

#include <guichan.h>

#ifdef USE_OAML
#include <oaml.h>

extern oamlApi *oaml;
extern bool enableOAML;
#endif

void DrawGuichanWidgets();

//----------------------------------------------------------------------------
// Variables
//----------------------------------------------------------------------------

/// variable set when we are scrolling via keyboard
int KeyScrollState = ScrollNone;

/// variable set when we are scrolling via mouse
int MouseScrollState = ScrollNone;

EventCallback GameCallbacks;   /// Game callbacks
EventCallback EditorCallbacks; /// Editor callbacks

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

/**
**  Handle scrolling area.
**
**  @param state  Scroll direction/state.
**  @param fast   Flag scroll faster.
**
**  @todo  Support dynamic acceleration of scroll speed.
**  @todo  If the scroll key is longer pressed the area is scrolled faster.
*/
void DoScrollArea(int state, bool fast, bool isKeyboard)
{
	CViewport *vp;
	int stepx;
	int stepy;
	static int remx = 0; // FIXME: docu
	static int remy = 0; // FIXME: docu

	int speed = isKeyboard ? UI.KeyScrollSpeed : UI.MouseScrollSpeed;
	
	if (state == ScrollNone) {
		return;
	}

	vp = UI.SelectedViewport;

	if (fast) {
		//Wyrmgus start
//		stepx = (int)(speed * vp->MapWidth / 2 * PixelTileSize.x * FRAMES_PER_SECOND / 4);
//		stepy = (int)(speed * vp->MapHeight / 2 * PixelTileSize.y * FRAMES_PER_SECOND / 4);
		stepx = (int)(speed * PixelTileSize.x * FRAMES_PER_SECOND / 4 * 4);
		stepy = (int)(speed * PixelTileSize.y * FRAMES_PER_SECOND / 4 * 4);
		//Wyrmgus end
	} else {// dynamic: let these variables increase up to fast..
		// FIXME: pixels per second should be configurable
		stepx = (int)(speed * PixelTileSize.x * FRAMES_PER_SECOND / 4);
		stepy = (int)(speed * PixelTileSize.y * FRAMES_PER_SECOND / 4);
	}
	if ((state & (ScrollLeft | ScrollRight)) && (state & (ScrollLeft | ScrollRight)) != (ScrollLeft | ScrollRight)) {
		stepx = stepx * 100 * 100 / VideoSyncSpeed / FRAMES_PER_SECOND / (SkipFrames + 1);
		remx += stepx - (stepx / 100) * 100;
		stepx /= 100;
		if (remx > 100) {
			++stepx;
			remx -= 100;
		}
	} else {
		stepx = 0;
	}
	if ((state & (ScrollUp | ScrollDown)) && (state & (ScrollUp | ScrollDown)) != (ScrollUp | ScrollDown)) {
		stepy = stepy * 100 * 100 / VideoSyncSpeed / FRAMES_PER_SECOND / (SkipFrames + 1);
		remy += stepy - (stepy / 100) * 100;
		stepy /= 100;
		if (remy > 100) {
			++stepy;
			remy -= 100;
		}
	} else {
		stepy = 0;
	}

	if (state & ScrollUp) {
		stepy = -stepy;
	}
	if (state & ScrollLeft) {
		stepx = -stepx;
	}
	const PixelDiff offset(stepx, stepy);

	vp->Set(vp->MapPos, vp->Offset + offset);

	// This recalulates some values
	HandleMouseMove(CursorScreenPos);
}

/**
**  Draw map area
*/
void DrawMapArea()
{
	// Draw all of the viewports
	for (CViewport *vp = UI.Viewports; vp < UI.Viewports + UI.NumViewports; ++vp) {
		// Center viewport on tracked unit
		if (vp->Unit) {
			if (vp->Unit->Destroyed || vp->Unit->CurrentAction() == UnitActionDie) {
				vp->Unit = NULL;
			} else {
				vp->Center(vp->Unit->GetMapPixelPosCenter());
			}
		}
		vp->Draw();
	}
}

/**
**  Display update.
**
**  This functions updates everything on screen. The map, the gui, the
**  cursors.
*/
void UpdateDisplay()
{
	if (GameRunning || Editor.Running == EditorEditing) {
		// to prevent empty spaces in the UI
#if defined(USE_OPENGL) || defined(USE_GLES)
		Video.FillRectangleClip(ColorBlack, 0, 0, Video.ViewportWidth, Video.ViewportHeight);
#else
		Video.FillRectangleClip(ColorBlack, 0, 0, Video.Width, Video.Height);
#endif
		DrawMapArea();
		DrawMessages();

		if (CursorState == CursorStateRectangle) {
			DrawCursor();
		}

		//Wyrmgus start
		if (CursorBuilding && CursorOn == CursorOnMap) {
			DrawBuildingCursor();
		}
		//Wyrmgus end
	
		if ((Preference.BigScreen && !BigMapMode) || (!Preference.BigScreen && BigMapMode)) {
			UiToggleBigMap();
		}

		if (!BigMapMode) {
			for (size_t i = 0; i < UI.Fillers.size(); ++i) {
				UI.Fillers[i].G->DrawSubClip(0, 0,
											 UI.Fillers[i].G->Width,
											 UI.Fillers[i].G->Height,
											 UI.Fillers[i].X, UI.Fillers[i].Y);
			}
			DrawMenuButtonArea();
			DrawUserDefinedButtons();

			UI.Minimap.Draw();
			UI.Minimap.DrawViewportArea(*UI.SelectedViewport);

			UI.InfoPanel.Draw();
			DrawResources();
			DrawDayTime();
			UI.StatusLine.Draw();
			UI.StatusLine.DrawCosts();
			UI.ButtonPanel.Draw();
		}
		
		DrawTimer();
		
		//Wyrmgus start
		//draw worker icon if there are idle workers
		if (UI.IdleWorkerButton && !ThisPlayer->FreeWorkers.empty()) {
			int worker_unit_type_id = PlayerRaces.GetFactionClassUnitType(ThisPlayer->Race, ThisPlayer->Faction, GetUnitTypeClassIndexByName("worker"));
			if (worker_unit_type_id != -1) {
				const CUnitType &type = *UnitTypes[worker_unit_type_id];
				
				const PixelPos pos(UI.IdleWorkerButton->X, UI.IdleWorkerButton->Y);
				const int flag = (ButtonAreaUnderCursor == ButtonAreaIdleWorker && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;
								 
				VariationInfo *varinfo = type.GetDefaultVariation(*ThisPlayer);
				if (varinfo && varinfo->Icon.Icon) { // check if the default variation is valid, and if it is, then make the button use the variation's icon
					varinfo->Icon.Icon->DrawUnitIcon(*UI.IdleWorkerButton->Style, flag, pos, ".", ThisPlayer->Index, type.GetDefaultSkinColor(*ThisPlayer), type.GetDefaultHairColor(*ThisPlayer));
				} else {
					type.Icon.Icon->DrawUnitIcon(*UI.IdleWorkerButton->Style, flag, pos, ".", ThisPlayer->Index, type.GetDefaultSkinColor(*ThisPlayer), type.GetDefaultHairColor(*ThisPlayer));
				}
				
				if (ButtonAreaUnderCursor == ButtonAreaIdleWorker && ButtonUnderCursor == 0) { //if the mouse is hovering over the idle worker button, draw a tooltip
					std::string idle_worker_tooltip = "Find Idle Worker (~!.)";
					if (!Preference.NoStatusLineTooltips) {
						CLabel label(GetGameFont());
						label.Draw(2 + 16, Video.Height + 2 - 16, idle_worker_tooltip);
					}
					DrawGenericPopup(idle_worker_tooltip, UI.IdleWorkerButton->X, UI.IdleWorkerButton->Y);
				}
			}
		}
		
		//draw icon if there are units with available level up upgrades
		if (UI.LevelUpUnitButton && !ThisPlayer->LevelUpUnits.empty()) {
			const PixelPos pos(UI.LevelUpUnitButton->X, UI.LevelUpUnitButton->Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;
								 
			ThisPlayer->LevelUpUnits[0]->GetIcon().Icon->DrawUnitIcon(*UI.LevelUpUnitButton->Style, flag, pos, "", ThisPlayer->Index, ThisPlayer->LevelUpUnits[0]->GetSkinColor(), ThisPlayer->LevelUpUnits[0]->GetHairColor());
				
			if (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit && ButtonUnderCursor == 0) { //if the mouse is hovering over the level up unit button, draw a tooltip
				std::string level_up_unit_tooltip = "Find Unit with Available Level Up";
				if (!Preference.NoStatusLineTooltips) {
					CLabel label(GetGameFont());
					label.Draw(2 + 16, Video.Height + 2 - 16, level_up_unit_tooltip);
				}
				DrawGenericPopup(level_up_unit_tooltip, UI.LevelUpUnitButton->X, UI.LevelUpUnitButton->Y);
			}
		}
		//Wyrmgus end
	//Wyrmgus start
	} else if (GrandStrategy && !GameRunning && GameResult == GameNoResult) { //grand strategy mode
		if (!GrandStrategyGamePaused) {
			//scroll map if mouse is in the scroll area
			int scroll_up = 7; //the scroll area in the upper part of the screen is smaller to allow clicking on the menu buttons and etc. more comfortably
			int scroll_down = (Video.Height - 16);
			int scroll_left = 15;
			int scroll_right = (Video.Width - 16);
			bool scrolled = false;
			if (CursorScreenPos.y < scroll_up) {
				if (WorldMapOffsetY > 0) {
					if (GrandStrategyMapHeightIndent == 0) {
						WorldMapOffsetY = WorldMapOffsetY - 1;
					}
					GrandStrategyMapHeightIndent -= 32;
				} else if (WorldMapOffsetY == 0 && GrandStrategyMapHeightIndent == -32) { //this is to make the entire y 0 tiles be shown scrolling to the northmost part of the map
					GrandStrategyMapHeightIndent -= 32;
				}
				GameCursor = UI.ArrowN.Cursor;
				scrolled = true;
			} else if (CursorScreenPos.y > scroll_down) {
				if (WorldMapOffsetY < GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64)) {
					if (GrandStrategyMapHeightIndent == -32) {
						WorldMapOffsetY = WorldMapOffsetY + 1;
					}
					GrandStrategyMapHeightIndent += 32;
				} else if (WorldMapOffsetY == GetWorldMapHeight() - 1 - ((UI.MapArea.EndY - UI.MapArea.Y) / 64) && GrandStrategyMapHeightIndent == 0) {
					GrandStrategyMapHeightIndent += 32;
				}
				GameCursor = UI.ArrowS.Cursor;
				scrolled = true;
			}
			if (CursorScreenPos.x < scroll_left) {
				if (WorldMapOffsetX > 0) {
					if (GrandStrategyMapWidthIndent == 0) {
						WorldMapOffsetX = WorldMapOffsetX - 1;
					}
					GrandStrategyMapWidthIndent -= 32;
				} else if (WorldMapOffsetX == 0 && GrandStrategyMapWidthIndent == -32) { //this is to make the entire x 0 tiles be shown scrolling to the westmost part of the map
					GrandStrategyMapWidthIndent -= 32;
				}
				if (GameCursor == UI.ArrowN.Cursor) {
					GameCursor = UI.ArrowNW.Cursor;
				} else if (GameCursor == UI.ArrowS.Cursor) {
					GameCursor = UI.ArrowSW.Cursor;
				} else {
					GameCursor = UI.ArrowW.Cursor;
				}
				scrolled = true;
			} else if (CursorScreenPos.x > scroll_right) {
				if (WorldMapOffsetX < GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64)) {
					if (GrandStrategyMapWidthIndent == -32) {
						WorldMapOffsetX = WorldMapOffsetX + 1;
					}
					GrandStrategyMapWidthIndent += 32;
				} else if (WorldMapOffsetX == GetWorldMapWidth() - 1 - ((UI.MapArea.EndX - UI.MapArea.X) / 64) && GrandStrategyMapWidthIndent == 0) {
					GrandStrategyMapWidthIndent += 32;
				}
				if (GameCursor == UI.ArrowN.Cursor) {
					GameCursor = UI.ArrowNE.Cursor;
				} else if (GameCursor == UI.ArrowS.Cursor) {
					GameCursor = UI.ArrowSE.Cursor;
				} else {
					GameCursor = UI.ArrowE.Cursor;
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
			} else {
				GameCursor = UI.Point.Cursor;
			}
		}
		
		bool draw_grand_strategy = true;
		
#if defined(USE_OPENGL) || defined(USE_GLES)
		if (UseOpenGL) {
		} else
#endif
		{
			if (GrandStrategyGamePaused) {
				draw_grand_strategy = false;
			}
		}
		
		if (draw_grand_strategy) {
			//draw map
			GrandStrategyGame.DrawMap();
			
			// Fillers
			for (size_t i = 0; i != UI.Fillers.size(); ++i) {
				UI.Fillers[i].G->DrawClip(UI.Fillers[i].X, UI.Fillers[i].Y);
			}
			
			GrandStrategyGame.DrawMinimap();
			
			GrandStrategyGame.DrawInterface();
			
			if (UI.MapArea.Contains(CursorScreenPos) && GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y] && !GrandStrategyGamePaused) {
				GrandStrategyGame.DrawTileTooltip(GrandStrategyGame.GetTileUnderCursor().x, GrandStrategyGame.GetTileUnderCursor().y);
				
				CGrandStrategyProvince *province = GrandStrategyGame.WorldMapTiles[GrandStrategyGame.GetTileUnderCursor().x][GrandStrategyGame.GetTileUnderCursor().y]->Province;
				if (province != NULL && province != GrandStrategyGame.SelectedProvince) {
					if (GrandStrategyGame.SelectedUnits.size() > 0 || !SelectedHero.empty()) {
						std::string tooltip;
						if (GrandStrategyGame.SelectedProvince->CanAttackProvince(province)) {
							if (
								province->Owner == NULL
								&& GrandStrategyGame.SelectedProvince->Owner->OwnedProvinces.size() == 1
								&& PlayerRaces.Factions[GrandStrategyGame.SelectedProvince->Owner->Civilization][GrandStrategyGame.SelectedProvince->Owner->Faction]->Type == "tribe"
							) {
								tooltip += "Migrate to ";
							} else {
								tooltip += "Attack ";
							}
						} else if (GrandStrategyGame.SelectedProvince->Owner == province->Owner) {
							tooltip += "Move units to ";
						}
						if (!tooltip.empty()) {
							tooltip += province->GetCulturalName();
							DrawGenericPopup(tooltip, CursorScreenPos.x, CursorScreenPos.y);
						}
					}
				}
			}
		}
	//Wyrmgus end
	}

	DrawPieMenu(); // draw pie menu only if needed

	DrawGuichanWidgets();
	
	if (CursorState != CursorStateRectangle) {
		DrawCursor();
	}

	//
	// Update changes to display.
	//
	Invalidate();
}

static void InitGameCallbacks()
{
	GameCallbacks.ButtonPressed = HandleButtonDown;
	GameCallbacks.ButtonReleased = HandleButtonUp;
	GameCallbacks.MouseMoved = HandleMouseMove;
	GameCallbacks.MouseExit = HandleMouseExit;
	GameCallbacks.KeyPressed = HandleKeyDown;
	GameCallbacks.KeyReleased = HandleKeyUp;
	GameCallbacks.KeyRepeated = HandleKeyRepeat;
	GameCallbacks.NetworkEvent = NetworkEvent;
}

static void GameLogicLoop()
{
	// Can't find a better place.
	// FIXME: We need find better place!
	SaveGameLoading = false;

#ifdef USE_OAML
	if (enableOAML && oaml) {
		// Time of day can change our main music loop, if the current playing track is set for this
		SetMusicCondition(OAML_CONDID_MAIN_LOOP, GameTimeOfDay);
	}
#endif

	//
	// Game logic part
	//
	if (!GamePaused && NetworkInSync && !SkipGameCycle) {
		SinglePlayerReplayEachCycle();
		++GameCycle;
		MultiPlayerReplayEachCycle();
		NetworkCommands(); // Get network commands
		TriggersEachCycle();// handle triggers
		UnitActions();      // handle units
		MissileActions();   // handle missiles
		PlayersEachCycle(); // handle players
		UpdateTimer();      // update game timer


		//
		// Work todo each second.
		// Split into different frames, to reduce cpu time.
		// Increment mana of magic units.
		// Update mini-map.
		// Update map fog of war.
		// Call AI.
		// Check game goals.
		// Check rescue of units.
		//
		switch (GameCycle % CYCLES_PER_SECOND) {
			case 0: // At cycle 0, start all ai players...
				if (GameCycle == 0) {
					for (int player = 0; player < NumPlayers; ++player) {
						PlayersEachSecond(player);
					}
				}
				break;
			case 1:
				break;
			case 2:
				break;
			case 3: // minimap update
				UI.Minimap.UpdateCache = true;
				break;
			case 4:
				break;
			case 5: // forest grow
				Map.RegenerateForest();
				break;
			case 6: // overtaking units
				RescueUnits();
				break;
			default: {
				// FIXME: assume that NumPlayers < (CYCLES_PER_SECOND - 7)
				int player = (GameCycle % CYCLES_PER_SECOND) - 7;
				Assert(player >= 0);
				if (player < NumPlayers) {
					PlayersEachSecond(player);
				}
			}
		}
		
		//Wyrmgus start
		if (GameCycle > 0 && GameCycle % (CYCLES_PER_SECOND * 10 * 3) == 0) { // every 10 seconds of gameplay = 1 hour for time of day calculations, change time of day every three hours
			if (!GameSettings.Inside && !GameSettings.NoTimeOfDay) { // only change the time of the day if outdoors
				GameTimeOfDay += 1;
				if (GameTimeOfDay == MaxTimesOfDay) {
					GameTimeOfDay = 1;
				}
			} else {
				// indoors it is always dark (maybe would be better to allow a special setting to have bright indoor places?
				GameTimeOfDay = NoTimeOfDay; // make indoors have no time of day setting until it is possible to make light sources change their surrounding "time of day"
			}

#ifdef USE_OAML
			if (enableOAML && oaml) {
				// Time of day can change our main music loop, if the current playing track is set for this
				SetMusicCondition(OAML_CONDID_MAIN_LOOP, GameTimeOfDay);
			}
#endif

			//update the sight of all units
			for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
				CUnit &unit = **it;
				if (!unit.Destroyed) {
					MapUnmarkUnitSight(unit);
					UpdateUnitSightRange(unit);
					MapMarkUnitSight(unit);
				}
			}
		}
		//Wyrmgus end
		
		//Wyrmgus start
//		if (Preference.AutosaveMinutes != 0 && !IsNetworkGame() && GameCycle > 0 && (GameCycle % (CYCLES_PER_SECOND * 60 * Preference.AutosaveMinutes)) == 0) { // autosave every X minutes, if the option is enabled
		if (Preference.AutosaveMinutes != 0 && !IsNetworkGame() && !GrandStrategy && GameCycle > 0 && (GameCycle % (CYCLES_PER_SECOND * 60 * Preference.AutosaveMinutes)) == 0) { // autosave every X minutes, if the option is enabled
		//Wyrmgus end
			UI.StatusLine.Set(_("Autosave"));
			//Wyrmgus start
//			SaveGame("autosave.sav");
			CclCommand("if (RunSaveGame ~= nil) then RunSaveGame(\"autosave.sav\") end;");
			//Wyrmgus end
		}
	}

	UpdateMessages();     // update messages
	ParticleManager.update(); // handle particles
	CheckMusicFinished(); // Check for next song

	if (FastForwardCycle <= GameCycle || !(GameCycle & 0x3f)) {
		WaitEventsOneFrame();
	}

	if (!NetworkInSync) {
		NetworkRecover(); // recover network
	}
}

//#define REALVIDEO
#ifdef REALVIDEO
static	int RealVideoSyncSpeed;
#endif

static void DisplayLoop()
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		/* update only if screen changed */
		ValidateOpenGLScreen();
	}
#endif

	/* update only if viewmode changed */
	CheckViewportMode();

	/*
	 *	update only if Update flag is set
	 *	FIXME: still not secure
	 */
	if (UI.Minimap.UpdateCache) {
		UI.Minimap.Update();
		UI.Minimap.UpdateCache = false;
	}

	//
	// Map scrolling
	//
	DoScrollArea(MouseScrollState | KeyScrollState, (KeyModifiers & ModifierControl) != 0, MouseScrollState == 0 && KeyScrollState > 0);

	ColorCycle();

	//Wyrmgus start
	//do tile animation
	if (!GamePaused && GameCycle != 0 && GameCycle && GameCycle % (CYCLES_PER_SECOND / 4) == 0) { // same speed as color-cycling
		for (int i = 0; i < Map.Info.MapWidth * Map.Info.MapHeight; ++i) {
			CMapField &mf = Map.Fields[i];
			if (Map.Tileset->solidTerrainTypes[Map.Tileset->tiles[mf.getTileIndex()].tileinfo.BaseTerrain].AnimationFrames > 0 && !Map.Tileset->tiles[mf.getTileIndex()].tileinfo.MixTerrain) {
				mf.AnimationFrame += 1;
				if (mf.AnimationFrame >= Map.Tileset->solidTerrainTypes[Map.Tileset->tiles[mf.getTileIndex()].tileinfo.BaseTerrain].AnimationFrames) {
					mf.AnimationFrame = 0;
				}
			}
		}
	}
	//Wyrmgus end
		
#ifdef REALVIDEO
	if (FastForwardCycle > GameCycle && RealVideoSyncSpeed != VideoSyncSpeed) {
		RealVideoSyncSpeed = VideoSyncSpeed;
		VideoSyncSpeed = 3000;
	}
#endif
	if (FastForwardCycle <= GameCycle || GameCycle <= 10 || !(GameCycle & 0x3f)) {
		//FIXME: this might be better placed somewhere at front of the
		// program, as we now still have a game on the background and
		// need to go through the game-menu or supply a map file
		UpdateDisplay();

		//
		// If double-buffered mode, we will display the contains of
		// VideoMemory. If direct mode this does nothing. In X11 it does
		// XFlush
		//
		RealizeVideoMemory();
	}
#ifdef REALVIDEO
	if (FastForwardCycle == GameCycle) {
		VideoSyncSpeed = RealVideoSyncSpeed;
	}
#endif
}

static void SingleGameLoop()
{
	while (GameRunning) {
		DisplayLoop();
		GameLogicLoop();
	}
}

/**
**  Game main loop.
**
**  Unit actions.
**  Missile actions.
**  Players (AI).
**  Cyclic events (color cycle,...)
**  Display update.
**  Input/Network/Sound.
*/
void GameMainLoop()
{
	const EventCallback *old_callbacks;

	InitGameCallbacks();

	old_callbacks = GetCallbacks();
	SetCallbacks(&GameCallbacks);

	SetVideoSync();
	GameCursor = UI.Point.Cursor;
	GameRunning = true;

	CParticleManager::init();

#ifdef REALVIDEO
	RealVideoSyncSpeed = VideoSyncSpeed;
#endif

	CclCommand("if (GameStarting ~= nil) then GameStarting() end");

	MultiPlayerReplayEachCycle();
	
	//Wyrmgus start
	//if the person player has no faction, bring up the faction choice interface
	if (!GrandStrategy && ThisPlayer && ThisPlayer->Faction == -1) {
		char buf[256];
		snprintf(buf, sizeof(buf), "if (ChooseFaction ~= nil) then ChooseFaction(\"%s\", \"%s\") end", ThisPlayer->Race != -1 ? PlayerRaces.Name[ThisPlayer->Race].c_str() : "", "");
		CclCommand(buf);
	}
	
	if (!IsNetworkGame() && ThisPlayer && CurrentCustomHero != NULL) {
		Vec2i resPos;
		FindNearestDrop(*CurrentCustomHero->Type, ThisPlayer->StartPos, resPos, LookingW);
		CUnit *custom_hero = MakeUnitAndPlace(resPos, *CurrentCustomHero->Type, ThisPlayer);
		custom_hero->SetCharacter(CurrentCustomHero->GetFullName(), true);	
	}
	//Wyrmgus end

	SingleGameLoop();

	//
	// Game over
	//
	if (GameResult == GameExit) {
		Exit(0);
		return;
	}

#ifdef REALVIDEO
	if (FastForwardCycle > GameCycle) {
		VideoSyncSpeed = RealVideoSyncSpeed;
	}
#endif
	NetworkQuitGame();
	EndReplayLog();

	GameCycle = 0;//????
	//Wyrmgus start
	GameTimeOfDay = NoTimeOfDay;
	//Wyrmgus end
	CParticleManager::exit();
	FlagRevealMap = 0;
	ReplayRevealMap = 0;
	GamePaused = false;
	GodMode = false;

	SetCallbacks(old_callbacks);
}

//@}
