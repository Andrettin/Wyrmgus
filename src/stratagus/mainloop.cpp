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
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

//----------------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------------

#include "stratagus.h"

#include "action/actions.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "editor/editor.h"
#include "faction.h"
#include "game/game.h"
#include "game/replay.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "map/map_layer.h"
#include "map/terrain_type.h"
#include "map/tileset.h" //for tile animation
#include "missile/missile.h"
#include "network/network.h"
#include "quest/campaign.h"
#include "quest/dialogue.h"
//Wyrmgus start
#include "quest/quest.h"
//Wyrmgus end
#include "results.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "time/calendar.h"
#include "time/time_of_day.h"
#include "translate.h"
#include "trigger/trigger.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_manager.h"
#include "upgrade/upgrade.h"
//Wyrmgus end
//Wyrmgus start
#include "video/font.h"
//Wyrmgus end
#include "video/video.h"
#include "world/plane.h"
#include "world/world.h"
#include "wyrmgus.h"

#include <guichan.h>

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
//		stepx = (int)(speed * vp->MapWidth / 2 * CMap::Map.GetCurrentPixelTileSize().x * FRAMES_PER_SECOND / 4);
//		stepy = (int)(speed * vp->MapHeight / 2 * CMap::Map.GetCurrentPixelTileSize().y * FRAMES_PER_SECOND / 4);
		stepx = (int)(speed * CMap::Map.GetCurrentPixelTileSize().x * FRAMES_PER_SECOND / 4 * 4);
		stepy = (int)(speed * CMap::Map.GetCurrentPixelTileSize().y * FRAMES_PER_SECOND / 4 * 4);
		//Wyrmgus end
	} else {// dynamic: let these variables increase up to fast..
		// FIXME: pixels per second should be configurable
		stepx = (int)(speed * CMap::Map.GetCurrentPixelTileSize().x * FRAMES_PER_SECOND / 4);
		stepy = (int)(speed * CMap::Map.GetCurrentPixelTileSize().y * FRAMES_PER_SECOND / 4);
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
				vp->Unit = nullptr;
			} else {
				if (UI.CurrentMapLayer != vp->Unit->MapLayer) {
					ChangeCurrentMapLayer(vp->Unit->MapLayer->ID);
				}
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
			DrawTime();
			DrawAge();
			DrawMapLayerButtons();
			UI.StatusLine.Draw();
			UI.StatusLine.DrawCosts();
			UI.ButtonPanel.Draw();
		}
		
		DrawTimer();
		
		//Wyrmgus start
		//draw worker icon if there are idle workers
		if (UI.IdleWorkerButton && !CPlayer::GetThisPlayer()->FreeWorkers.empty()) {
			const PixelPos pos(UI.IdleWorkerButton->X, UI.IdleWorkerButton->Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaIdleWorker && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;

			CPlayer::GetThisPlayer()->FreeWorkers[0]->GetIcon()->DrawUnitIcon(*UI.IdleWorkerButton->Style, flag, pos, ".", CPlayer::GetThisPlayer()->Index);
		}
		
		//draw icon if there are units with available level up upgrades
		if (UI.LevelUpUnitButton && !CPlayer::GetThisPlayer()->LevelUpUnits.empty()) {
			const PixelPos pos(UI.LevelUpUnitButton->X, UI.LevelUpUnitButton->Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;
								 
			CPlayer::GetThisPlayer()->LevelUpUnits[0]->GetIcon()->DrawUnitIcon(*UI.LevelUpUnitButton->Style, flag, pos, "", CPlayer::GetThisPlayer()->Index);
		}
		
		//draw icon if the player has a hero
		for (size_t i = 0; i < UI.HeroUnitButtons.size() && i < CPlayer::GetThisPlayer()->Heroes.size(); ++i) {
			const PixelPos pos(UI.HeroUnitButtons[i].X, UI.HeroUnitButtons[i].Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaHeroUnit && ButtonUnderCursor == i) ? (IconActive | (MouseButtons & LeftButton)) : 0;
									 
			CPlayer::GetThisPlayer()->Heroes[i]->GetIcon()->DrawUnitIcon(*UI.HeroUnitButtons[i].Style, flag, pos, "", CPlayer::GetThisPlayer()->Index);
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
	// FIXME: We need to find a better place!
	SaveGameLoading = false;

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

		for (CMapLayer *map_layer : CMap::Map.MapLayers) {
			map_layer->DoPerCycleLoop();
		}
		
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
				CMap::Map.RegenerateForest();
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
		
		player = (GameCycle - 1) % (CYCLES_PER_MINUTE / 2);
		Assert(player >= 0);
		if (player < NumPlayers) {
			PlayersEachHalfMinute(player);
		}

		player = (GameCycle - 1) % CYCLES_PER_MINUTE;
		Assert(player >= 0);
		if (player < NumPlayers) {
			PlayersEachMinute(player);
		}
		//Wyrmgus end
		
		if (GameCycle > 0) {
			if (GameCycle % CYCLES_PER_IN_GAME_HOUR == 0) {
				CDate::CurrentTotalHours++;
				
				for (size_t i = 0; i < CCalendar::Calendars.size(); ++i) {
					CCalendar *calendar = CCalendar::Calendars[i];

					calendar->CurrentDate.AddHours(calendar, 1, DEFAULT_DAY_MULTIPLIER_PER_YEAR);
					
					if (calendar->CurrentDayOfTheWeek != -1 && CDate::CurrentTotalHours % calendar->HoursPerDay == 0) { //day passed in the calendar
						calendar->CurrentDayOfTheWeek++;
						calendar->CurrentDayOfTheWeek %= calendar->DaysOfTheWeek.size();
					}
				}
				
				for (CMapLayer *map_layer : CMap::Map.MapLayers) {
					map_layer->DoPerHourLoop();
				}
			}
		}
		
		if (Preference.AutosaveMinutes != 0 && !IsNetworkGame() && GameCycle > 0 && (GameCycle % (CYCLES_PER_MINUTE * Preference.AutosaveMinutes)) == 0) { // autosave every X minutes, if the option is enabled
			UI.StatusLine.Set(_("Autosave"));
			//Wyrmgus start
//			SaveGame("autosave.sav");
			CclCommand("if (RunSaveGame ~= nil) then RunSaveGame(\"autosave.sav\") end;");
			//Wyrmgus end
		}
	}

	UpdateMessages();     // update messages

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

#ifdef REALVIDEO
	RealVideoSyncSpeed = VideoSyncSpeed;
#endif

	CclCommand("if (GameStarting ~= nil) then GameStarting() end");

	MultiPlayerReplayEachCycle();
	
	Wyrmgus::GetInstance()->emit_signal("time_of_day_changed", static_cast<CTimeOfDay *>(nullptr), UI.CurrentMapLayer->GetTimeOfDay());

	//Wyrmgus start
	if (GameCycle == 0) { // so that these don't trigger when loading a saved game
		const CCampaign *current_campaign = CCampaign::GetCurrentCampaign();
		if (current_campaign != nullptr) {
			const CDate start_date(current_campaign->GetStartDate());
			
			for (int i = 0; i < NumPlayers; ++i) {
				if (CPlayer::Players[i]->Type != PlayerNobody && CPlayer::Players[i]->Race != 0 && CPlayer::Players[i]->GetFaction() != nullptr) {
					if (start_date.Year) {
						CCivilization *civilization = CCivilization::Get(CPlayer::Players[i]->Race);
						const CFaction *faction = CPlayer::Players[i]->GetFaction();
						
						for (std::map<std::string, std::map<CDate, bool>>::iterator iterator = civilization->HistoricalUpgrades.begin(); iterator != civilization->HistoricalUpgrades.end(); ++iterator) {
							int upgrade_id = UpgradeIdByIdent(iterator->first);
							if (upgrade_id == -1) {
								fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", iterator->first.c_str());
								continue;
							}
							for (std::map<CDate, bool>::reverse_iterator second_iterator = iterator->second.rbegin(); second_iterator != iterator->second.rend(); ++second_iterator) {
								if (second_iterator->first.Year == 0 || start_date.ContainsDate(second_iterator->first)) {
									if (second_iterator->second && UpgradeIdentAllowed(*CPlayer::Players[i], iterator->first.c_str()) != 'R') {
										UpgradeAcquire(*CPlayer::Players[i], AllUpgrades[upgrade_id]);
									} else if (!second_iterator->second) {
										break;
									}
								}
							}
						}
						
						for (std::map<std::string, std::map<CDate, bool>>::const_iterator iterator = faction->HistoricalUpgrades.begin(); iterator != faction->HistoricalUpgrades.end(); ++iterator) {
							const int upgrade_id = UpgradeIdByIdent(iterator->first);
							if (upgrade_id == -1) {
								fprintf(stderr, "Upgrade \"%s\" doesn't exist.\n", iterator->first.c_str());
								continue;
							}
							for (std::map<CDate, bool>::const_reverse_iterator second_iterator = iterator->second.rbegin(); second_iterator != iterator->second.rend(); ++second_iterator) {
								if (second_iterator->first.Year == 0 || start_date.ContainsDate(second_iterator->first)) {
									if (second_iterator->second && UpgradeIdentAllowed(*CPlayer::Players[i], iterator->first.c_str()) != 'R') {
										UpgradeAcquire(*CPlayer::Players[i], AllUpgrades[upgrade_id]);
									} else if (!second_iterator->second) {
										break;
									}
								}
							}
						}

						for (std::map<std::pair<CDate, const CFaction *>, int>::const_iterator iterator = faction->HistoricalDiplomacyStates.begin(); iterator != faction->HistoricalDiplomacyStates.end(); ++iterator) { //set the appropriate historical diplomacy states to other factions
							if (iterator->first.first.Year == 0 || start_date.ContainsDate(iterator->first.first)) {
								CPlayer *diplomacy_state_player = GetFactionPlayer(iterator->first.second);
								if (diplomacy_state_player != nullptr) {
									CommandDiplomacy(i, iterator->second, diplomacy_state_player->Index);
									CommandDiplomacy(diplomacy_state_player->Index, iterator->second, i);
									if (iterator->second == DiplomacyAllied) {
										CommandSharedVision(i, true, diplomacy_state_player->Index);
										CommandSharedVision(diplomacy_state_player->Index, true, i);
									}
								}
							}
						}

						for (std::map<std::pair<CDate, int>, int>::const_iterator iterator = faction->HistoricalResources.begin(); iterator != faction->HistoricalResources.end(); ++iterator) { //set the appropriate historical resource quantities
							if (iterator->first.first.Year == 0 || start_date.ContainsDate(iterator->first.first)) {
								CPlayer::Players[i]->SetResource(iterator->first.second, iterator->second);
							}
						}
					}
				}
			}
		}
		
		//if the person player has no faction, bring up the faction choice interface
		if (CPlayer::GetThisPlayer() && CPlayer::GetThisPlayer()->GetFaction() == nullptr) {
			char buf[256];
			snprintf(buf, sizeof(buf), "if (ChooseFaction ~= nil) then ChooseFaction(\"%s\", \"%s\") end", CPlayer::GetThisPlayer()->Race != -1 ? CCivilization::Get(CPlayer::GetThisPlayer()->Race)->GetIdent().utf8().get_data() : "", "");
			CclCommand(buf);
		}
		
		if (!IsNetworkGame() && CPlayer::GetThisPlayer() && CurrentCustomHero != nullptr) {
			Vec2i resPos;
			FindNearestDrop(*CurrentCustomHero->UnitType, CPlayer::GetThisPlayer()->StartPos, resPos, LookingW, CPlayer::GetThisPlayer()->StartMapLayer);
			CUnit *custom_hero = MakeUnitAndPlace(resPos, *CurrentCustomHero->UnitType, CPlayer::GetThisPlayer(), CPlayer::GetThisPlayer()->StartMapLayer);
			custom_hero->SetCharacter(CurrentCustomHero->Ident, true);	
		}
		
		//update the sold units of all units before starting, to make sure they fit the current conditions
		for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
			CUnit *unit = *it;
			if (unit && unit->IsAlive()) {
				unit->UpdateSoldUnits();
			}
		}
		
		if (CurrentQuest != nullptr && CurrentQuest->IntroductionDialogue != nullptr) {
			CurrentQuest->IntroductionDialogue->Call(CPlayer::GetThisPlayer()->Index);
		}
	}
	//Wyrmgus end

	SingleGameLoop();
	
	Wyrmgus::GetInstance()->emit_signal("time_of_day_changed", UI.CurrentMapLayer->GetTimeOfDay(), static_cast<CTimeOfDay *>(nullptr));

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
	CDate::CurrentTotalHours = 0;
	FlagRevealMap = 0;
	ReplayRevealMap = 0;
	GamePaused = false;
	GodMode = false;

	SetCallbacks(old_callbacks);
}
