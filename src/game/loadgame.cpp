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
//      (c) Copyright 2001-2020 by Lutz Sammer, Andreas Arens and Andrettin
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

#include "actions.h"
#include "ai.h"
#include "commands.h"
#include "currency.h"
#include "database/database.h"
#include "faction.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "luacallback.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "missile.h"
#include "particle.h"
#include "pathfinder.h"
#include "quest.h"
#include "replay.h"
#include "script.h"
#include "script/condition/condition.h"
#include "script/trigger.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "ui/button.h"
#include "ui/button_level.h"
#include "ui/cursor.h"
#include "ui/ui.h"
#include "unit/construction.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/random.h"
#include "video/font.h"
#include "video/video.h"
#include "world.h"

bool SaveGameLoading;                 /// If a Saved Game is Loading

/**
**  Cleanup modules.
**
**  Call each module to clean up.
*/
void CleanModules()
{
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
	CTimeOfDaySchedule::ClearTimeOfDaySchedules();
	CSeasonSchedule::ClearSeasonSchedules();
	CParticleManager::exit();
	CleanReplayLog();
	FreePathfinder();

	//delete lua callbacks, as that needs to be done before closing the Lua state, and database clearing is done in the Qt main thread
	for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		unit_type->DeathExplosion.reset();
		unit_type->OnHit.reset();
		unit_type->OnEachCycle.reset();
		unit_type->OnEachSecond.reset();
		unit_type->OnInit.reset();
		unit_type->TeleportEffectIn.reset();
		unit_type->TeleportEffectOut.reset();
	}

	wyrmgus::database::get()->clear();

	UnitTypeVar.Init(); // internal script. should be to a better place, don't find for restart.
}

/**
**  Initialize all modules.
**
**  Call each module to initialize.
*/
void InitModules()
{
	GameCycle = 0;
	CDate::CurrentTotalHours = 0;
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
	wyrmgus::terrain_type::LoadTerrainTypeGraphics();
	//Wyrmgus end
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
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		if (!unit.Removed) {
			unit.Removed = 1;
			unit.Place(unit.tilePos, unit.MapLayer->ID);
		}
		
		//Wyrmgus start
		//calculate attack range for containers now, as when loading a game it couldn't be done when the container was initially loaded
		if (unit.BoardCount > 0 && unit.InsideCount > 0) {
			unit.UpdateContainerAttackRange();
		}
		
		unit.UpdateXPRequired();
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
	CurrentCustomHero = nullptr; //otherwise the loaded game will have an extra hero for the current custom hero
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

	PlaceUnits();

	const unsigned long game_cycle = GameCycle;
	const unsigned long long current_total_hours = CDate::CurrentTotalHours;
	const unsigned syncrand = wyrmgus::random::get()->get_seed();
	const unsigned synchash = SyncHash;

	InitModules();
	LoadModules();

	GameCycle = game_cycle;
	CDate::CurrentTotalHours = current_total_hours;
	wyrmgus::random::get()->set_seed(syncrand);
	SyncHash = synchash;
	SelectionChanged();
}
