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
/**@name loadgame.cpp - Load game. */
//
//      (c) Copyright 2001-2019 by Lutz Sammer, Andreas Arens and Andrettin
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

#include "action/actions.h"
#include "ai/ai.h"
#include "character.h"
#include "commands.h"
#include "construct.h"
#include "dependency/dependency.h"
#include "game/replay.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "item/item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "missile/missile.h"
#include "missile/missile_type.h"
#include "pathfinder/pathfinder.h"
#include "quest/quest.h"
#include "script.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "spell/spells.h"
#include "time/calendar.h"
#include "trigger/trigger.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "ui/icon.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "video/font.h"
#include "video/video.h"

#include <core/math/random_number_generator.h>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

bool SaveGameLoading;                 /// If a Saved Game is Loading

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/


/**
**  Cleanup modules.
**
**  Call each module to clean up.
*/
void CleanModules()
{
	EndReplayLog();
	CleanMessages();
	
	CleanCursors();
	CleanUserInterface();
	CleanFonts();
	CTrigger::ClearTriggers();
	FreeAi();
	CleanDecorations();
	//Wyrmgus start
	CleanGrandStrategyEvents();
	//Wyrmgus end
	CleanMissiles();
	//Wyrmgus start
	CleanTerrainFeatures();
	//Wyrmgus end
	CleanUnits();
	CleanPlayers();
	CleanSelections();
	CleanGroups();
	CleanUpgrades();
	CleanButtons();
	CButtonLevel::ClearButtonLevels();
	CleanMissileTypes();
	CMap::Map.Clean();
	CMap::Map.CleanFogOfWar();
	CleanReplayLog();
	CSpell::ClearSpells();
	FreePathfinder();
	
	for (const std::function<void()> &clear_function : ClassClearFunctions) {
		clear_function();
	}

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

	InitSyncRand();
	InitVideoCursors();
	InitUserInterface();
	CPlayer::InitPlayers();
	InitMissileTypes();
	InitMissiles();

	// LUDO : 0 = don't reset player stats ( units level , upgrades, ... ) !
	InitUnitTypes(0);

	InitUnits();
	InitSpells();
	InitUpgrades();

	InitButtons();
	CTrigger::InitActiveTriggers();

	InitAiModule();

	CMap::Map.Init();
}

/**
**  Load all.
**
**  Call each module to load additional files (graphics,sounds).
*/
void LoadModules()
{
	LoadFonts();
	LoadIcons();
	LoadCursors();
	UI.Load();
	//Wyrmgus start
	CTerrainType::LoadTerrainTypeGraphics();
	//Wyrmgus end
#ifndef DYNAMIC_LOAD
	LoadMissileSprites();
#endif
	LoadConstructions();
	LoadDecorations();
	LoadUnitTypes();

	InitPathfinder();

	LoadUnitSounds();
	MapUnitSounds();
	if (SoundEnabled()) {
		InitSoundClient();
	}

	SetPlayersPalette();
	UI.Minimap.Create();

	SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);
	
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
			unit.Place(unit.GetTilePos(), unit.GetMapLayer()->GetIndex());
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
//	SetDefaultTextColors(FontYellow, FontWhite);
	SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);
	//Wyrmgus end
	LoadFonts();
	
	//Wyrmgus start
	CPlayer::InitPlayers();
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
	const uint64_t syncrand = RNG->get_seed();
	const uint64_t synchash = SyncHash;

	InitModules();
	LoadModules();

	GameCycle = game_cycle;
	CDate::CurrentTotalHours = current_total_hours;
	RNG->set_seed(syncrand);
	SyncHash = synchash;
	SelectionChanged();
}
