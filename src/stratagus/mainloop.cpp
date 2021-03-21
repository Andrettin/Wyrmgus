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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "actions.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "database/defines.h"
#include "dialogue.h"
#include "editor.h"
#include "engine_interface.h"
#include "faction.h"
#include "game.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "missile.h"
#include "network.h"
#include "particle.h"
#include "quest/campaign.h"
//Wyrmgus start
#include "quest/quest.h"
//Wyrmgus end
#include "replay.h"
#include "results.h"
#include "script.h"
#include "script/context.h"
#include "script/trigger.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "sound/music.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "time/calendar.h"
#include "time/time_of_day.h"
#include "translate.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_manager.h"
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "video/font.h"
#include "video/render_context.h"
#include "video/video.h"

#include <guichan.h>

void DrawGuichanWidgets(std::vector<std::function<void(renderer *)>> &render_commands);

/// variable set when we are scrolling via keyboard
int KeyScrollState = ScrollNone;

/// variable set when we are scrolling via mouse
int MouseScrollState = ScrollNone;

EventCallback GameCallbacks;   /// Game callbacks
EventCallback EditorCallbacks; /// Editor callbacks

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
//		stepx = (int)(speed * vp->MapWidth / 2 * wyrmgus::defines::get()->get_tile_width() * FRAMES_PER_SECOND / 4);
//		stepy = (int)(speed * vp->MapHeight / 2 * wyrmgus::defines::get()->get_tile_height() * FRAMES_PER_SECOND / 4);
		stepx = (int)(speed * wyrmgus::defines::get()->get_scaled_tile_width() * FRAMES_PER_SECOND / 4 * 4);
		stepy = (int)(speed * wyrmgus::defines::get()->get_scaled_tile_height() * FRAMES_PER_SECOND / 4 * 4);
		//Wyrmgus end
	} else {// dynamic: let these variables increase up to fast..
		// FIXME: pixels per second should be configurable
		stepx = (int)(speed * wyrmgus::defines::get()->get_scaled_tile_width() * FRAMES_PER_SECOND / 4);
		stepy = (int)(speed * wyrmgus::defines::get()->get_scaled_tile_height() * FRAMES_PER_SECOND / 4);
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
void DrawMapArea(std::vector<std::function<void(renderer *)>> &render_commands)
{
	// Draw all of the viewports
	for (CViewport *vp = UI.Viewports; vp < UI.Viewports + UI.NumViewports; ++vp) {
		// Center viewport on tracked unit
		if (vp->Unit != nullptr) {
			if (vp->Unit->Destroyed || vp->Unit->CurrentAction() == UnitAction::Die) {
				vp->Unit = nullptr;
			} else {
				if (UI.CurrentMapLayer != vp->Unit->MapLayer) {
					ChangeCurrentMapLayer(vp->Unit->MapLayer->ID);
				}
				vp->Center(vp->Unit->get_scaled_map_pixel_pos_center());
			}
		}
		vp->Draw(render_commands);
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
	std::vector<std::function<void(renderer *)>> render_commands;

	if (GameRunning || Editor.Running == EditorEditing) {
		//to prevent empty spaces in the UI
		Video.FillRectangleClip(ColorBlack, 0, 0, Video.ViewportWidth, Video.ViewportHeight, render_commands);

		DrawMapArea(render_commands);
		DrawMessages(render_commands);

		if (CurrentCursorState == CursorState::Rectangle) {
			DrawCursor(render_commands);
		}

		//Wyrmgus start
		if (CursorBuilding && CursorOn == cursor_on::map) {
			DrawBuildingCursor(render_commands);
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
											 UI.Fillers[i].X, UI.Fillers[i].Y, render_commands);
			}
			DrawMenuButtonArea(render_commands);
			DrawUserDefinedButtons(render_commands);

			UI.get_minimap()->Draw(render_commands);
			UI.get_minimap()->DrawViewportArea(*UI.SelectedViewport, render_commands);

			UI.InfoPanel.Draw(render_commands);
			DrawResources(render_commands);
			DrawTime(render_commands);
			DrawAge(render_commands);
			DrawMapLayerButtons(render_commands);
			UI.StatusLine.Draw(render_commands);
			UI.StatusLine.DrawCosts(render_commands);
			UI.ButtonPanel.Draw(render_commands);
		}
		
		//Wyrmgus start
		//draw worker icon if there are idle workers
		if (UI.IdleWorkerButton && !CPlayer::GetThisPlayer()->FreeWorkers.empty()) {
			const PixelPos pos(UI.IdleWorkerButton->X, UI.IdleWorkerButton->Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaIdleWorker && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;

			CPlayer::GetThisPlayer()->FreeWorkers[0]->get_icon()->DrawUnitIcon(*UI.IdleWorkerButton->Style, flag, pos, ".", CPlayer::GetThisPlayer()->get_player_color(), render_commands);
		}
		
		//draw icon if there are units with available level up upgrades
		if (UI.LevelUpUnitButton && !CPlayer::GetThisPlayer()->LevelUpUnits.empty()) {
			const PixelPos pos(UI.LevelUpUnitButton->X, UI.LevelUpUnitButton->Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaLevelUpUnit && ButtonUnderCursor == 0) ? (IconActive | (MouseButtons & LeftButton)) : 0;
								 
			CPlayer::GetThisPlayer()->LevelUpUnits[0]->get_icon()->DrawUnitIcon(*UI.LevelUpUnitButton->Style, flag, pos, "", CPlayer::GetThisPlayer()->get_player_color(), render_commands);
		}
		
		//draw icon if the player has a hero
		for (size_t i = 0; i < UI.HeroUnitButtons.size() && i < CPlayer::GetThisPlayer()->Heroes.size(); ++i) {
			const PixelPos pos(UI.HeroUnitButtons[i].X, UI.HeroUnitButtons[i].Y);
			const int flag = (ButtonAreaUnderCursor == ButtonAreaHeroUnit && ButtonUnderCursor == static_cast<int>(i)) ? (IconActive | (MouseButtons & LeftButton)) : 0;
									 
			CPlayer::GetThisPlayer()->Heroes[i]->get_icon()->DrawUnitIcon(*UI.HeroUnitButtons[i].Style, flag, pos, "", CPlayer::GetThisPlayer()->get_player_color(), render_commands);
		}
		
		DrawPopups(render_commands);
		//Wyrmgus end
	}

	DrawPieMenu(render_commands); // draw pie menu only if needed

	DrawGuichanWidgets(render_commands);
	
	if (CurrentCursorState != CursorState::Rectangle) {
		DrawCursor(render_commands);
	}

	render_context::get()->set_commands(std::move(render_commands));
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

		for (const std::unique_ptr<CMapLayer> &map_layer : CMap::get()->MapLayers) {
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
				UI.get_minimap()->UpdateCache = true;
				break;
			case 4:
				break;
			case 5:
				//regrow forests and remove other destroyed overlay tiles after a delay
				CMap::get()->handle_destroyed_overlay_terrain();
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
			wyrmgus::game::get()->do_cycle();
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
	/* update only if screen changed */
	ValidateOpenGLScreen();

	/* update only if viewmode changed */
	CheckViewportMode();

	/*
	 *	update only if Update flag is set
	 *	FIXME: still not secure
	 */
	if (UI.get_minimap()->UpdateCache) {
		UI.get_minimap()->Update();
		UI.get_minimap()->UpdateCache = false;
	}

	//
	// Map scrolling
	//
	DoScrollArea(MouseScrollState | KeyScrollState, (KeyModifiers & ModifierControl) != 0, MouseScrollState == 0 && KeyScrollState > 0);

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
		engine_interface::get()->run_event_loop();

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
	cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);
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
		const wyrmgus::campaign *current_campaign = wyrmgus::game::get()->get_current_campaign();
		if (current_campaign != nullptr) {
			wyrmgus::game::get()->apply_player_history();
		}
		
		//if the person player has no faction, bring up the faction choice interface
		if (CPlayer::GetThisPlayer() && CPlayer::GetThisPlayer()->Faction == -1) {
			std::array<char, 256> buf{};
			snprintf(buf.data(), sizeof(buf), "if (ChooseFaction ~= nil) then ChooseFaction(\"%s\", \"%s\") end", CPlayer::GetThisPlayer()->Race != -1 ? wyrmgus::civilization::get_all()[CPlayer::GetThisPlayer()->Race]->get_identifier().c_str() : "", "");
			CclCommand(buf.data());
		}
		
		if (!IsNetworkGame() && CPlayer::GetThisPlayer() && CurrentCustomHero != nullptr) {
			Vec2i resPos;
			FindNearestDrop(*CurrentCustomHero->get_unit_type(), CPlayer::GetThisPlayer()->StartPos, resPos, LookingW, CPlayer::GetThisPlayer()->StartMapLayer);
			CUnit *custom_hero = MakeUnitAndPlace(resPos, *CurrentCustomHero->get_unit_type(), CPlayer::GetThisPlayer(), CPlayer::GetThisPlayer()->StartMapLayer);
			custom_hero->set_character(CurrentCustomHero);	
		}
		
		//update the sold units of all units before starting, to make sure they fit the current conditions
		//make a copy of the units list, as updating the sold units can change the list
		const std::vector<CUnit *> units = wyrmgus::unit_manager::get()->get_units();
		for (CUnit *unit : units) {
			if (unit && unit->IsAlive()) {
				unit->UpdateSoldUnits();
			}
		}
		
		if (CurrentQuest != nullptr && CurrentQuest->IntroductionDialogue != nullptr) {
			wyrmgus::context ctx;
			ctx.current_player = CPlayer::GetThisPlayer();
			CurrentQuest->IntroductionDialogue->call(CPlayer::GetThisPlayer(), ctx);
		}

		if (current_campaign != nullptr && current_campaign->get_quest() != nullptr) {
			CPlayer::GetThisPlayer()->accept_quest(current_campaign->get_quest());
		}
	}
	//Wyrmgus end

	game::get()->set_running(true);

	engine_interface::get()->set_waiting_for_interface(true);
	engine_interface::get()->get_map_view_created_future().wait();
	engine_interface::get()->reset_map_view_created_promise();
	engine_interface::get()->set_waiting_for_interface(false);

	engine_interface::get()->set_loading_message("");

	SingleGameLoop();

#ifdef REALVIDEO
	if (FastForwardCycle > GameCycle) {
		VideoSyncSpeed = RealVideoSyncSpeed;
	}
#endif
	NetworkQuitGame();
	EndReplayLog();

	GameCycle = 0;//????
	game::get()->set_current_total_hours(0);
	CParticleManager::exit();
	FlagRevealMap = 0;
	ReplayRevealMap = 0;
	GamePaused = false;
	GodMode = false;

	SetCallbacks(old_callbacks);
}
