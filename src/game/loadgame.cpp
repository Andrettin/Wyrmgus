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

#include "achievement.h"
#include "actions.h"
#include "age.h"
#include "ai.h"
#include "campaign.h"
#include "character.h"
#include "civilization.h"
#include "commands.h"
#include "construct.h"
#include "currency.h"
#include "dialogue.h"
#include "font.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "hair_color.h"
#include "icon.h"
#include "item.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "missile.h"
#include "pathfinder.h"
#include "include/plane.h"
#include "player_color.h"
#include "quest.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "replay.h"
#include "school_of_magic.h"
#include "script.h"
#include "skin_color.h"
#include "sound.h"
#include "sound_server.h"
#include "species/species.h"
#include "species/species_category.h"
#include "species/species_category_rank.h"
#include "spells.h"
#include "text.h"
#include "time/calendar.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
#include "time/timeline.h"
#include "trigger.h"
#include "ui/button_action.h"
#include "ui/button_level.h"
#include "ui/ui.h"
#include "unit/historical_unit.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/dependency.h"
#include "upgrade/upgrade.h"
#include "video.h"
#include "world.h"

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
	CTrigger::ClearTriggers();
	FreeAi();
	PlayerRaces.Clean();
	CAchievement::Clear();
	CAge::Clear();
	CCalendar::ClearCalendars();
	CCampaign::Clear();
	CCharacter::ClearCharacters();
	CCivilization::ClearCivilizations();
	CleanConstructions();
	CCurrency::ClearCurrencies();
	CleanDecorations();
	CDeity::ClearDeities();
	//Wyrmgus start
	CDialogue::ClearDialogues();
	CDeityDomain::ClearDeityDomains();
	CleanGrandStrategyEvents();
	//Wyrmgus end
	CHistoricalUnit::ClearHistoricalUnits();
	CMapTemplate::Clear();
	CleanMissiles();
	CPantheon::Clear();
	CPlane::ClearPlanes();
	//Wyrmgus start
	CleanProvinces();
	CleanQuests();
	//Wyrmgus end
	CReligion::ClearReligions();
	CSchoolOfMagic::ClearSchoolsOfMagic();
	CSite::Clear();
	CSpecies::Clear();
	CSpeciesCategory::Clear();
	CSpeciesCategoryRank::Clear();
	//Wyrmgus start
	CleanTexts();
	CleanUniqueItems();
	//Wyrmgus end
	CTimeline::Clear();
	CleanUnits();
	CleanUnitTypes();
	CWorld::ClearWorlds();
	CleanPlayers();
	CleanSelections();
	CleanGroups();
	CleanUpgrades();
	CleanButtons();
	CButtonLevel::ClearButtonLevels();
	CleanMissileTypes();
	CMap::Map.Clean();
	CMap::Map.CleanFogOfWar();
	CTerrainType::ClearTerrainTypes();
	CTimeOfDay::ClearTimesOfDay();
	CTimeOfDaySchedule::ClearTimeOfDaySchedules();
	CSeason::Clear();
	CSeasonSchedule::ClearSeasonSchedules();
	CleanReplayLog();
	CSpell::ClearSpells();
	FreePathfinder();

	UnitTypeVar.Init(); // internal script. should be to a better place, don't find for restart.
	
	CPlayerColor::ClearPlayerColors();
	CHairColor::ClearHairColors();
	CSkinColor::ClearSkinColors();
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
	const unsigned long long current_total_hours = CDate::CurrentTotalHours;
	const unsigned syncrand = SyncRandSeed;
	const unsigned synchash = SyncHash;

	InitModules();
	LoadModules();

	GameCycle = game_cycle;
	CDate::CurrentTotalHours = current_total_hours;
	SyncRandSeed = syncrand;
	SyncHash = synchash;
	SelectionChanged();
}
