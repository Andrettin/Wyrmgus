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

#include "achievement.h"
#include "actions.h"
#include "ai.h"
#include "campaign.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "construct.h"
#include "currency.h"
#include "database/database.h"
#include "dialogue.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "missile.h"
#include "particle.h"
#include "pathfinder.h"
#include "plane.h"
#include "player_color.h"
#include "quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "replay.h"
#include "school_of_magic.h"
#include "script.h"
#include "script/condition/condition.h"
#include "script/trigger.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "spells.h"
#include "text.h"
#include "time/calendar.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "time/timeline.h"
#include "ui/button.h"
#include "ui/button_level.h"
#include "ui/cursor.h"
#include "ui/ui.h"
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
	PlayerRaces.Clean();
	CAchievement::ClearAchievements();
	CleanConstructions();
	CCurrency::ClearCurrencies();
	CleanDecorations();
	//Wyrmgus start
	CleanGrandStrategyEvents();
	//Wyrmgus end
	CleanMissiles();
	CPantheon::ClearPantheons();
	//Wyrmgus start
	CleanProvinces();
	CSchoolOfMagic::ClearSchoolsOfMagic();
	CleanTexts();
	//Wyrmgus end
	CleanUnits();
	CleanUnitTypeVariables();
	CleanPlayers();
	CleanSelections();
	CleanGroups();
	CleanUpgradeModifiers();
	CleanButtons();
	CMap::Map.Clean();
	CMap::Map.CleanFogOfWar();
	CTimeOfDaySchedule::ClearTimeOfDaySchedules();
	CSeasonSchedule::ClearSeasonSchedules();
	CParticleManager::exit();
	CleanReplayLog();
	FreePathfinder();

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
	InitConstructions();

	// LUDO : 0 = don't reset player stats ( units level , upgrades, ... ) !
	InitUnitTypes(0);

	InitUnits();
	InitSpells();
	InitUpgrades();

	InitButtons();
	wyrmgus::trigger::InitActiveTriggers();

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
//	SetDefaultTextColors(FontYellow, FontWhite);
	SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);
	//Wyrmgus end
	
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
