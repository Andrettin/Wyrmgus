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
//      (c) Copyright 2001-2021 by Lutz Sammer, Andreas Arens and Andrettin
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
#include "ai.h"
#include "commands.h"
#include "currency.h"
#include "database/database.h"
#include "dialogue.h"
#include "game.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "luacallback.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "missile.h"
#include "particle.h"
#include "pathfinder.h"
#include "quest/quest.h"
#include "replay.h"
#include "script.h"
#include "script/condition/condition.h"
#include "script/trigger.h"
#include "sound/music.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "spell/spell.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "ui/button.h"
#include "ui/button_level.h"
#include "ui/cursor.h"
#include "ui/icon.h"
#include "ui/resource_icon.h"
#include "ui/ui.h"
#include "unit/construction.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/log_util.h"
#include "util/random.h"
#include "video/font.h"
#include "video/video.h"

bool SaveGameLoading;                 /// If a Saved Game is Loading

static void delete_lua_callbacks()
{
	for (wyrmgus::character *character : wyrmgus::character::get_all()) {
		character->Conditions.reset();
	}

	for (wyrmgus::dialogue *dialogue : wyrmgus::dialogue::get_all()) {
		dialogue->delete_lua_callbacks();
	}

	for (wyrmgus::missile_type *missile_type : wyrmgus::missile_type::get_all()) {
		missile_type->ImpactParticle.reset();
		missile_type->SmokeParticle.reset();
		missile_type->OnImpact.reset();
	}

	for (wyrmgus::quest *quest : wyrmgus::quest::get_all()) {
		quest->Conditions.reset();
		quest->AcceptEffects.reset();
		quest->CompletionEffects.reset();
		quest->FailEffects.reset();
	}

	for (wyrmgus::spell *spell : wyrmgus::spell::get_all()) {
		spell->delete_lua_callbacks();
	}

	for (wyrmgus::trigger *trigger : wyrmgus::trigger::get_all()) {
		trigger->Conditions.reset();
		trigger->Effects.reset();
	}

	for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		unit_type->DeathExplosion.reset();
		unit_type->OnHit.reset();
		unit_type->OnEachCycle.reset();
		unit_type->OnEachSecond.reset();
		unit_type->OnInit.reset();
		unit_type->TeleportEffectIn.reset();
		unit_type->TeleportEffectOut.reset();
	}
}

/**
**  Cleanup modules.
**
**  Call each module to clean up.
*/
void CleanModules()
{
	CGraphic::free_all_textures();

	EndReplayLog();
	CleanMessages();

	CleanUserInterface();
	FreeAi();
	CCurrency::ClearCurrencies();
	CleanDecorations();
	//Wyrmgus start
	CleanGrandStrategyEvents();
	//Wyrmgus end
	CleanMissiles();
	//Wyrmgus start
	CleanProvinces();
	//Wyrmgus end
	CleanUnits();
	CleanUnitTypeVariables();
	CleanPlayers();
	CleanSelections();
	CleanGroups();
	CleanUpgradeModifiers();
	CleanButtons();
	CMap::get()->Clean();
	CMap::get()->CleanFogOfWar();
	CParticleManager::exit();
	CleanReplayLog();
	FreePathfinder();

	//delete lua callbacks, as that needs to be done before closing the Lua state, and database clearing is done in the Qt main thread
	delete_lua_callbacks();

	database::get()->clear();

	UnitTypeVar.Init(); // internal script. should be to a better place, don't find for restart.
	CPlayer::Players.clear();
}

/**
**  Initialize all modules.
**
**  Call each module to initialize.
*/
void InitModules()
{
	GameCycle = 0;
	game::get()->set_current_total_hours(0);
	FastForwardCycle = 0;
	SyncHash = 0;

	CallbackMusicOn();
	InitUserInterface();
	InitPlayers();
	InitMissileTypes();

	// LUDO : 0 = don't reset player stats ( units level , upgrades, ... ) !
	InitUnitTypes(0);

	InitUnits();
	InitUpgrades();

	InitButtons();
	wyrmgus::trigger::InitActiveTriggers();

	InitAiModule();

	CMap::get()->Init();
}

/**
**  Load all.
**
**  Call each module to load additional files (graphics,sounds).
*/
void LoadModules()
{
	UI.Load();
	//Wyrmgus start
	terrain_type::LoadTerrainTypeGraphics();
	//Wyrmgus end
	resource_icon::load_all();
	icon::load_all();
#ifndef DYNAMIC_LOAD
	LoadMissileSprites();
#endif
	LoadConstructions();
	LoadDecorations();
	LoadUnitTypes();

	InitPathfinder();

	MapUnitSounds();
	if (SoundEnabled()) {
		InitSoundClient();
	}

	SetPlayersPalette();
	UI.get_minimap()->Create();

	//Wyrmgus start
	ResetItemsToLoad();
	//Wyrmgus end
}

static void PlaceUnits()
{
	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		if (!unit->Removed) {
			unit->Removed = 1;
			unit->Place(unit->tilePos, unit->MapLayer->ID);
		}
		
		//Wyrmgus start
		//calculate attack range for containers now, as when loading a game it couldn't be done when the container was initially loaded
		if (unit->BoardCount > 0 && unit->InsideCount > 0) {
			unit->UpdateContainerAttackRange();
		}
		
		unit->UpdateXPRequired();
		//Wyrmgus end
	}
}

/**
**  Load a game to file.
**
**  @param filename  File name to be loaded.
*/
void LoadGame(const std::string &filename)
{
	//Wyrmgus start
	CleanPlayers(); //clean players, as they may not have been cleansed after a scenario
	//Wyrmgus end
	
	// log will be enabled if found in the save game
	CommandLogDisabled = true;
	SaveGameLoading = true;

	//Wyrmgus start
	InitPlayers();
	//Wyrmgus end

	LuaGarbageCollect();
	InitUnitTypes(1);
	//Wyrmgus start
	CalculateItemsToLoad();
	//Wyrmgus end
	LuaLoadFile(filename);
	LuaGarbageCollect();

	//clear the base reference for destroyed units
	const std::vector<CUnit *> units = wyrmgus::unit_manager::get()->get_units();
	for (CUnit *unit : units) {
		if (!unit->Destroyed) {
			continue;
		}

		if (unit->get_ref_count() <= 1) {
			wyrmgus::log::log_error("Destroyed unit has a reference count less than or equal to 1 before having its base reference cleared when loading a game, which means that no other unit refers to it, in which case it should not have been part of the unit manager's units vector, as it should have been fully released already.");
		}

		unit->clear_base_reference();
	}

	PlaceUnits();

	const unsigned long game_cycle = GameCycle;
	const uint64_t current_total_hours = game::get()->get_current_total_hours();
	const unsigned syncrand = random::get()->get_seed();
	const unsigned synchash = SyncHash;

	InitModules();
	LoadModules();

	GameCycle = game_cycle;
	game::get()->set_current_total_hours(current_total_hours);
	wyrmgus::random::get()->set_seed(syncrand);
	SyncHash = synchash;
	SelectionChanged();

	//set owners for settlements
	for (const CUnit *settlement_unit : CMap::get()->get_settlement_units()) {
		if (settlement_unit->Player->is_neutral_player()) {
			continue;
		}

		settlement_unit->settlement->get_game_data()->set_owner(settlement_unit->Player);
	}

	CMap::get()->calculate_settlement_resource_units();
}
