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
//      (c) Copyright 2001-2006 by Lutz Sammer, Andreas Arens
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

#include "actions.h"
#include "ai.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "commands.h"
#include "construct.h"
#include "depend.h"
#include "font.h"
//Wyrmgus start
#include "grand_strategy.h"
#include "item.h"
//Wyrmgus end
#include "map.h"
#include "minimap.h"
#include "missile.h"
#include "particle.h"
#include "pathfinder.h"
//Wyrmgus start
#include "quest.h"
//Wyrmgus end
#include "replay.h"
#include "script.h"
#include "sound.h"
#include "sound_server.h"
#include "spells.h"
#include "trigger.h"
#include "ui.h"
#include "unit.h"
#include "unit_manager.h"
#include "unittype.h"
#include "upgrade.h"
#include "video.h"

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

	CleanIcons();
	CleanCursors();
	CleanUserInterface();
	CleanFonts();
	CleanTriggers();
	FreeAi();
	PlayerRaces.Clean();
	//Wyrmgus start
	GrandStrategyGame.Clean();
	CleanCharacters();
	//Wyrmgus end
	CleanConstructions();
	CleanDecorations();
	CleanMissiles();
	//Wyrmgus start
	CleanQuests();
	CleanUniqueItems();
	//Wyrmgus end
	CleanUnits();
	CleanUnitTypes();
	//Wyrmgus start
	CleanWorlds();
	//Wyrmgus end
	CleanPlayers();
	CleanSelections();
	CleanGroups();
	CleanUpgrades();
	CleanDependencies();
	CleanButtons();
	CleanMissileTypes();
	Map.Clean();
	Map.CleanFogOfWar();
	CParticleManager::exit();
	CleanReplayLog();
	CleanSpells();
	FreePathfinder();

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
	FastForwardCycle = 0;
	//Wyrmgus start
	GameTimeOfDay = NoTimeOfDay;
	//Wyrmgus end
	SyncHash = 0;

	CallbackMusicOn();
	InitSyncRand();
	InitVideoCursors();
	InitUserInterface();
	InitPlayers();
	InitMissileTypes();
	InitMissiles();
	InitConstructions();

	// LUDO : 0 = don't reset player stats ( units level , upgrades, ... ) !
	InitUnitTypes(0);

	InitUnits();
	InitSpells();
	InitUpgrades();
	InitDependencies();

	InitButtons();
	InitTriggers();

	InitAiModule();

	Map.Init();
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
	//Wyrmgus start
//	LoadCursors(PlayerRaces.Name[ThisPlayer->Race]);
	LoadCursors();
	//Wyrmgus end
	UI.Load();
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
			unit.Place(unit.tilePos);
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
	CurrentCustomHero = NULL; //otherwise the loaded game will have an extra hero for the current custom hero
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
	//Wyrmgus start
	const int game_time_of_day = GameTimeOfDay;
	//Wyrmgus end
	const unsigned syncrand = SyncRandSeed;
	const unsigned synchash = SyncHash;

	InitModules();
	LoadModules();

	GameCycle = game_cycle;
	//Wyrmgus start
	GameTimeOfDay = game_time_of_day;
	//Wyrmgus end
	SyncRandSeed = syncrand;
	SyncHash = synchash;
	SelectionChanged();
}

//@}
