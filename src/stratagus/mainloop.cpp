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
//      (c) Copyright 1998-2017 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "commands.h"
//Wyrmgus end
#include "editor.h"
//Wyrmgus start
#include "font.h"
//Wyrmgus end
#include "game.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "interface.h"
#include "luacallback.h"
//Wyrmgus end
#include "map.h"
#include "missile.h"
#include "network.h"
#include "particle.h"
//Wyrmgus start
#include "quest.h"
//Wyrmgus end
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
#include "upgrade.h"
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
			DrawMapLayerButtons();
			UI.StatusLine.Draw();
			UI.StatusLine.DrawCosts();
			UI.ButtonPanel.Draw();
		}
		
		DrawTimer();
		
		//Wyrmgus start
		//draw worker icon if there are idle workers
		if (UI.IdleWorkerButton && !ThisPlayer->FreeWorkers.empty()) {
			const PixelPos pos(UI.IdleWorkerButton->X, UI.IdleWorkerButton->Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaIdleWorker && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;

			ThisPlayer->FreeWorkers[0]->GetIcon().Icon->DrawUnitIcon(*UI.IdleWorkerButton->Style, flag, pos, ".", ThisPlayer->Index, ThisPlayer->FreeWorkers[0]->GetHairColor());
		}
		
		//draw icon if there are units with available level up upgrades
		if (UI.LevelUpUnitButton && !ThisPlayer->LevelUpUnits.empty()) {
			const PixelPos pos(UI.LevelUpUnitButton->X, UI.LevelUpUnitButton->Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;
								 
			ThisPlayer->LevelUpUnits[0]->GetIcon().Icon->DrawUnitIcon(*UI.LevelUpUnitButton->Style, flag, pos, "", ThisPlayer->Index, ThisPlayer->LevelUpUnits[0]->GetHairColor());
		}
		
		//draw icon if the player has a custom hero
		for (int i = 0; i < PlayerHeroMax; ++i) {
			if (UI.HeroUnitButtons[i] && (int) ThisPlayer->Heroes.size() > i) {
				const PixelPos pos(UI.HeroUnitButtons[i]->X, UI.HeroUnitButtons[i]->Y);
				const int flag = (ButtonAreaUnderCursor == ButtonAreaHeroUnit && ButtonUnderCursor == i) ? (IconActive | (MouseButtons & LeftButton)) : 0;
									 
				ThisPlayer->Heroes[i]->GetIcon().Icon->DrawUnitIcon(*UI.HeroUnitButtons[i]->Style, flag, pos, "", ThisPlayer->Index, ThisPlayer->Heroes[i]->GetHairColor());
			}
		}
		
		DrawPopups();
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
		SetMusicCondition(OAML_CONDID_MAIN_LOOP, Map.TimeOfDay[CurrentMapLayer]);
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
			//Wyrmgus start
			/*
			default: {
				// FIXME: assume that NumPlayers < (CYCLES_PER_SECOND - 7)
				int player = (GameCycle % CYCLES_PER_SECOND) - 7;
				Assert(player >= 0);
				if (player < NumPlayers) {
					PlayersEachSecond(player);
				}
			}
			*/
			//Wyrmgus end
		}
		
		//Wyrmgus start
		int player = (GameCycle - 1) % CYCLES_PER_SECOND;
		Assert(player >= 0);
		if (player < NumPlayers) {
			PlayersEachSecond(player);
			if ((player + CYCLES_PER_SECOND) < NumPlayers) {
				PlayersEachSecond(player + CYCLES_PER_SECOND);
			}
		}
		
		player = (GameCycle - 1) % CYCLES_PER_MINUTE;
		Assert(player >= 0);
		if (player < NumPlayers) {
			PlayersEachMinute(player);
		}
		//Wyrmgus end
		
		//Wyrmgus start
		for (size_t z = 0; z < Map.Fields.size(); ++z) {
			int time_of_day_seconds = DefaultTimeOfDaySeconds;
			if (Map.Worlds[z]) {
				time_of_day_seconds = Map.Worlds[z]->TimeOfDaySeconds;
			} else if (Map.Planes[z]) {
				time_of_day_seconds = Map.Planes[z]->TimeOfDaySeconds;
			}
			if (GameSettings.Inside || GameSettings.NoTimeOfDay || Map.SurfaceLayers[z] > 0 || !time_of_day_seconds) {
				Map.TimeOfDay[z] = NoTimeOfDay; //the map layer has no time of day
				continue;
			}
			if (GameCycle > 0 && GameCycle % (CYCLES_PER_SECOND * time_of_day_seconds) == 0) { 
				Map.TimeOfDay[z] += 1;
				if (Map.TimeOfDay[z] == MaxTimesOfDay) {
					Map.TimeOfDay[z] = 1;
				}

#ifdef USE_OAML
				if (enableOAML && oaml && z == CurrentMapLayer) {
					// Time of day can change our main music loop, if the current playing track is set for this
					SetMusicCondition(OAML_CONDID_MAIN_LOOP, Map.TimeOfDay[z]);
				}
#endif

				//update the sight of all units
				for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
					CUnit *unit = *it;
					if (
						unit && unit->IsAlive() && unit->MapLayer == z &&
						(
							((Map.TimeOfDay[z] == MorningTimeOfDay || Map.TimeOfDay[z] == DuskTimeOfDay) && unit->Variable[DAYSIGHTRANGEBONUS_INDEX].Value != 0) // if has day sight bonus and is entering or exiting day
							|| ((Map.TimeOfDay[z] == FirstWatchTimeOfDay || Map.TimeOfDay[z] == DawnTimeOfDay) && unit->Variable[NIGHTSIGHTRANGEBONUS_INDEX].Value != 0) // if has night sight bonus and is entering or exiting night
						)
					) {
						MapUnmarkUnitSight(*unit);
						UpdateUnitSightRange(*unit);
						MapMarkUnitSight(*unit);
					}
				}
			}
		}
		//Wyrmgus end
		
		//Wyrmgus start
//		if (Preference.AutosaveMinutes != 0 && !IsNetworkGame() && GameCycle > 0 && (GameCycle % (CYCLES_PER_SECOND * 60 * Preference.AutosaveMinutes)) == 0) { // autosave every X minutes, if the option is enabled
		if (Preference.AutosaveMinutes != 0 && !IsNetworkGame() && GameCycle > 0 && (GameCycle % (CYCLES_PER_MINUTE * Preference.AutosaveMinutes)) == 0) { // autosave every X minutes, if the option is enabled
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
		for (size_t z = 0; z < Map.Fields.size(); ++z) {
			for (int i = 0; i < Map.Info.MapWidths[z] * Map.Info.MapHeights[z]; ++i) {
				CMapField &mf = Map.Fields[z][i];
				if (mf.Terrain && mf.Terrain->SolidAnimationFrames > 0) {
					mf.AnimationFrame += 1;
					if (mf.AnimationFrame >= mf.Terrain->SolidAnimationFrames) {
						mf.AnimationFrame = 0;
					}
				}
				if (mf.OverlayTerrain && mf.Terrain->SolidAnimationFrames > 0) {
					mf.OverlayAnimationFrame += 1;
					if (mf.OverlayAnimationFrame >= mf.Terrain->SolidAnimationFrames) {
						mf.OverlayAnimationFrame = 0;
					}
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
	//Wyrmgus start
	GameEstablishing = false;
	//Wyrmgus end
	GameRunning = true;

	CParticleManager::init();

#ifdef REALVIDEO
	RealVideoSyncSpeed = VideoSyncSpeed;
#endif

	CclCommand("if (GameStarting ~= nil) then GameStarting() end");

	MultiPlayerReplayEachCycle();
	
	//Wyrmgus start
	if (GameCycle == 0) { // so that these don't trigger when loading a saved game
		if (CurrentCampaign != NULL) {
			for (int i = 0; i < NumPlayers; ++i) {
				if (Players[i].Type != PlayerNobody && Players[i].Race != 0 && Players[i].Faction != -1) {
					if (CurrentCampaign->StartDate.year) {
						CCivilization *civilization = PlayerRaces.Civilizations[Players[i].Race];
						CFaction *faction = PlayerRaces.Factions[Players[i].Faction];
						
						for (std::map<std::string, std::map<CDate, bool>>::iterator iterator = civilization->HistoricalUpgrades.begin(); iterator != civilization->HistoricalUpgrades.end(); ++iterator) {
							int upgrade_id = UpgradeIdByIdent(iterator->first);
							if (upgrade_id == -1) {
								fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", iterator->first.c_str());
								continue;
							}
							for (std::map<CDate, bool>::reverse_iterator second_iterator = iterator->second.rbegin(); second_iterator != iterator->second.rend(); ++second_iterator) {
								if (second_iterator->first.year == 0 || CurrentCampaign->StartDate.ContainsDate(second_iterator->first)) {
									if (second_iterator->second && UpgradeIdentAllowed(Players[i], iterator->first.c_str()) != 'R') {
										UpgradeAcquire(Players[i], AllUpgrades[upgrade_id]);
									} else if (!second_iterator->second) {
										break;
									}
								}
							}
						}
						
						for (std::map<std::string, std::map<CDate, bool>>::iterator iterator = faction->HistoricalUpgrades.begin(); iterator != faction->HistoricalUpgrades.end(); ++iterator) {
							int upgrade_id = UpgradeIdByIdent(iterator->first);
							if (upgrade_id == -1) {
								fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", iterator->first.c_str());
								continue;
							}
							for (std::map<CDate, bool>::reverse_iterator second_iterator = iterator->second.rbegin(); second_iterator != iterator->second.rend(); ++second_iterator) {
								if (second_iterator->first.year == 0 || CurrentCampaign->StartDate.ContainsDate(second_iterator->first)) {
									if (second_iterator->second && UpgradeIdentAllowed(Players[i], iterator->first.c_str()) != 'R') {
										UpgradeAcquire(Players[i], AllUpgrades[upgrade_id]);
									} else if (!second_iterator->second) {
										break;
									}
								}
							}
						}

						for (std::map<std::pair<int, CFaction *>, int>::iterator iterator = faction->HistoricalDiplomacyStates.begin(); iterator != faction->HistoricalDiplomacyStates.end(); ++iterator) { //set the appropriate historical diplomacy states to other factions
							if (iterator->first.first == 0 || CurrentCampaign->StartDate.year >= iterator->first.first) {
								CPlayer *diplomacy_state_player = GetFactionPlayer(iterator->first.second);
								if (diplomacy_state_player) {
									CommandDiplomacy(i, iterator->second, diplomacy_state_player->Index);
									CommandDiplomacy(diplomacy_state_player->Index, iterator->second, i);
									if (iterator->second == DiplomacyAllied) {
										CommandSharedVision(i, true, diplomacy_state_player->Index);
										CommandSharedVision(diplomacy_state_player->Index, true, i);
									}
								}
							}
						}

						for (std::map<std::pair<CDate, int>, int>::iterator iterator = faction->HistoricalResources.begin(); iterator != faction->HistoricalResources.end(); ++iterator) { //set the appropriate historical resource quantities
							if (iterator->first.first.year == 0 || CurrentCampaign->StartDate.ContainsDate(iterator->first.first)) {
								Players[i].SetResource(iterator->first.second, iterator->second);
							}
						}
					}
				}
			}
	
			if (CurrentCampaign->StartEffects) {
				CurrentCampaign->StartEffects->pushPreamble();
				CurrentCampaign->StartEffects->run();
			}
		}
		
		//if the person player has no faction, bring up the faction choice interface
		if (ThisPlayer && ThisPlayer->Faction == -1) {
			char buf[256];
			snprintf(buf, sizeof(buf), "if (ChooseFaction ~= nil) then ChooseFaction(\"%s\", \"%s\") end", ThisPlayer->Race != -1 ? PlayerRaces.Name[ThisPlayer->Race].c_str() : "", "");
			CclCommand(buf);
		}
		
		if (!IsNetworkGame() && ThisPlayer && CurrentCustomHero != NULL) {
			Vec2i resPos;
			FindNearestDrop(*CurrentCustomHero->Type, ThisPlayer->StartPos, resPos, LookingW, ThisPlayer->StartMapLayer);
			CUnit *custom_hero = MakeUnitAndPlace(resPos, *CurrentCustomHero->Type, ThisPlayer, ThisPlayer->StartMapLayer);
			custom_hero->SetCharacter(CurrentCustomHero->Ident, true);	
		}
		
		if (CurrentQuest != NULL && CurrentQuest->IntroductionDialogue != NULL) {
			CurrentQuest->IntroductionDialogue->Call(ThisPlayer->Index);
		}
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
	CParticleManager::exit();
	FlagRevealMap = 0;
	ReplayRevealMap = 0;
	GamePaused = false;
	GodMode = false;

	SetCallbacks(old_callbacks);
}

//@}
