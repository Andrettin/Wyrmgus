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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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
#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "dialogue.h"
#include "editor.h"
#include "engine_interface.h"
#include "game/game.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "missile.h"
#include "network/network.h"
#include "particle.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
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
#include "sound/music_player.h"
#include "sound/music_type.h"
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
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/path_util.h"
#include "util/string_util.h"
#include "util/thread_pool.h"
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
void DoScrollArea(int state, bool fast, bool isKeyboard, const Qt::KeyboardModifiers key_modifiers)
{
	CViewport *vp;
	int stepx;
	int stepy;
	static int remx = 0; // FIXME: docu
	static int remy = 0; // FIXME: docu

	const int speed = isKeyboard ? preferences::get()->get_key_scroll_speed() : preferences::get()->get_mouse_scroll_speed();
	
	if (state == ScrollNone) {
		return;
	}

	vp = UI.SelectedViewport;

	if (fast) {
		//Wyrmgus start
//		stepx = (int)(speed * vp->MapWidth / 2 * defines::get()->get_tile_width() * FRAMES_PER_SECOND / 4);
//		stepy = (int)(speed * vp->MapHeight / 2 * defines::get()->get_tile_height() * FRAMES_PER_SECOND / 4);
		stepx = (int)(speed * defines::get()->get_scaled_tile_width() * FRAMES_PER_SECOND / 4 * 4);
		stepy = (int)(speed * defines::get()->get_scaled_tile_height() * FRAMES_PER_SECOND / 4 * 4);
		//Wyrmgus end
	} else {// dynamic: let these variables increase up to fast..
		// FIXME: pixels per second should be configurable
		stepx = (int)(speed * defines::get()->get_scaled_tile_width() * FRAMES_PER_SECOND / 4);
		stepy = (int)(speed * defines::get()->get_scaled_tile_height() * FRAMES_PER_SECOND / 4);
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
	HandleMouseMove(CursorScreenPos, key_modifiers);
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

	if (GameRunning || CEditor::get()->is_running()) {
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

		for (size_t i = 0; i < UI.Fillers.size(); ++i) {
			UI.Fillers[i].G->render(QPoint(UI.Fillers[i].X, UI.Fillers[i].Y), render_commands);
		}

		DrawUserDefinedButtons(render_commands);

		UI.get_minimap()->Draw(render_commands);
		UI.get_minimap()->DrawViewportArea(*UI.SelectedViewport, render_commands);

		UI.InfoPanel.Draw(render_commands);
		DrawMapLayerButtons(render_commands);
		UI.StatusLine.Draw(render_commands);
		UI.StatusLine.DrawCosts(render_commands);
		UI.ButtonPanel.Draw(render_commands);

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
}

[[nodiscard]]
static boost::asio::awaitable<void> GameLogicLoop()
{
	// Can't find a better place.
	// FIXME: We need to find a better place!
	SaveGameLoading = false;

	//
	// Game logic part
	//
	if (!game::get()->is_paused() && NetworkInSync && !SkipGameCycle) {
		SinglePlayerReplayEachCycle();
		++GameCycle;
		MultiPlayerReplayEachCycle();

		if (game::get()->is_multiplayer()) {
			co_await NetworkCommands(); //get network commands
		}

		TriggersEachCycle(); //handle triggers
		UnitActions(); //handle units
		MissileActions(); //handle missiles
		PlayersEachCycle(); //handle players

		CMap::get()->do_per_cycle_loop();
		
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
				assert_throw(player >= 0);
				if (player < NumPlayers) {
					PlayersEachSecond(player);
				}
			}
			*/
			//Wyrmgus end
		}
		
		//Wyrmgus start
		int player = (GameCycle - 1) % CYCLES_PER_SECOND;
		assert_throw(player >= 0);
		if (player < NumPlayers) {
			PlayersEachSecond(player);
			if ((player + CYCLES_PER_SECOND) < NumPlayers) {
				PlayersEachSecond(player + CYCLES_PER_SECOND);
			}
		}
		
		player = (GameCycle - 1) % (CYCLES_PER_MINUTE / 2);
		assert_throw(player >= 0);
		if (player < NumPlayers) {
			PlayersEachHalfMinute(player);
		}

		player = (GameCycle - 1) % CYCLES_PER_MINUTE;
		assert_throw(player >= 0);
		if (player < NumPlayers) {
			PlayersEachMinute(player);
		}
		//Wyrmgus end
		
		if (GameCycle > 0) {
			game::get()->do_cycle();
		}
		
		if (preferences::get()->is_autosave_enabled() && !IsNetworkGame() && GameCycle > 0 && (GameCycle % (CYCLES_PER_MINUTE * preferences::autosave_minutes)) == 0) {
			//autosave every X minutes, if the option is enabled
			const std::filesystem::path filepath = database::get_save_path() / "autosave.sav";

			//Wyrmgus start
//			SaveGame(path::to_string(filepath));
			CclCommand("if (RunSaveGame ~= nil) then RunSaveGame(\""+ string::escaped("file:" + path::to_string(filepath)) + "\") end;");
			//Wyrmgus end

			UI.StatusLine.Set(_("Autosave"));
		}
	}

	UpdateMessages();     // update messages
	ParticleManager.update(); // handle particles
	CheckMusicFinished(); // Check for next song

	if (FastForwardCycle <= GameCycle || !(GameCycle & 0x3f)) {
		co_await WaitEventsOneFrame();
	}

	if (!NetworkInSync) {
		co_await NetworkRecover(); // recover network
	}
}

static void DisplayLoop()
{
	/* update only if screen changed */
	ValidateOpenGLScreen();

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
	DoScrollArea(MouseScrollState | KeyScrollState, (stored_key_modifiers & Qt::ControlModifier) != 0, MouseScrollState == 0 && KeyScrollState > 0, stored_key_modifiers);

	if (FastForwardCycle <= GameCycle || GameCycle <= 10 || !(GameCycle & 0x3f)) {
		//FIXME: this might be better placed somewhere at front of the
		// program, as we now still have a game on the background and
		// need to go through the game-menu or supply a map file
		UpdateDisplay();
	}
}

[[nodiscard]]
static boost::asio::awaitable<void> SingleGameLoop()
{
	while (GameRunning) {
		DisplayLoop();
		co_await GameLogicLoop();
	}

	if (!load_game_file.empty()) {
		engine_interface::get()->load_game_deferred(load_game_file);
		load_game_file.clear();
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
boost::asio::awaitable<void> GameMainLoop()
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

	music_player::get()->play_music_type(music_type::map);

	MultiPlayerReplayEachCycle();
	
	game::get()->set_running(true);

	engine_interface::get()->set_waiting_for_interface(true);

	//run the display loop once, so that the map is visible when we start
	DisplayLoop();

	co_await thread_pool::get()->await_future(engine_interface::get()->get_map_view_created_future());

	engine_interface::get()->reset_map_view_created_promise();
	engine_interface::get()->set_waiting_for_interface(false);

	engine_interface::get()->set_loading_message("");

	if (GameCycle == 0) {
		if (game::get()->get_current_campaign() != nullptr) {
			if (game::get()->get_current_campaign()->get_quest() != nullptr) {
				CPlayer::GetThisPlayer()->accept_quest(game::get()->get_current_campaign()->get_quest());
			}
		}

		if (CurrentQuest != nullptr && CurrentQuest->IntroductionDialogue != nullptr) {
			context ctx;
			ctx.current_player = CPlayer::GetThisPlayer();
			CurrentQuest->IntroductionDialogue->call(CPlayer::GetThisPlayer(), ctx);
		}

		//if the person player has no faction, bring up the faction choice interface
		if (CPlayer::GetThisPlayer() != nullptr && CPlayer::GetThisPlayer()->get_faction() == nullptr) {
			CPlayer::GetThisPlayer()->set_government_type(government_type::tribe);

			std::vector<faction *> potential_factions = CPlayer::GetThisPlayer()->get_potential_factions();

			if (!potential_factions.empty()) {
				std::sort(potential_factions.begin(), potential_factions.end(), [](const faction *lhs, const faction *rhs) {
					return lhs->get_name() < rhs->get_name();
				});

				if (!game::get()->is_multiplayer()) {
					//pause so that the game loop won't start before the player has chosen a faction
					game::get()->set_paused(true);
				}

				emit engine_interface::get()->factionChoiceDialogOpened(container::to_qvariant_list(potential_factions));
			}
		}
	}

	co_await SingleGameLoop();

	co_await NetworkQuitGame();
	EndReplayLog();

	GameCycle = 0;//????
	game::get()->set_current_total_hours(0);
	CParticleManager::exit();
	FlagRevealMap = 0;
	ReplayRevealMap = 0;
	game::get()->set_paused(false);
	GodMode = false;

	SetCallbacks(old_callbacks);
}
