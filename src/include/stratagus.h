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
/**@name stratagus.h - The main header file. */
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

#pragma once

//ensure the contents of the PCH will be recognized by IDEs as being included in each .cpp file
#include "pch.h"

/*============================================================================
==  Config definitions
============================================================================*/

// Dynamic loading.
//#define DYNAMIC_LOAD

/*============================================================================
==  Compiler repairs
============================================================================*/

#ifdef _MSC_VER

#define WIN32_LEAN_AND_MEAN
#define WINDOWS_IGNORE_PACKING_MISMATCH //for SDL

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#if !(_MSC_VER >= 1500 && _MSC_VER < 1600)
#define vsnprintf _vsnprintf
#endif
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

#endif  // } _MSC_VER

/*============================================================================
==  Macro
============================================================================*/

// To remove warning for unused variable.
#ifdef __GNUC__
#define UNUSED(var) do {__typeof__ (&var) __attribute__ ((unused)) __tmp = &var; } while(0)
#else
#define UNUSED(var) (var)
#endif

#ifdef __GNUC__
#define PRINTF_VAARG_ATTRIBUTE(a, b) __attribute__((format (printf, a, b)))
#else
#define PRINTF_VAARG_ATTRIBUTE(a, b)
#endif

/*============================================================================
==  Debug definitions
============================================================================*/

#ifndef NDEBUG
//ensure DEBUG is defined is NDEBUG is not defined
#ifndef DEBUG
#define DEBUG
#endif
#endif

/**
**  This simulates vararg macros.
**  Example:
**    DebugPrint("Test %d %d\n" _C_ 1 _C_ 2);
*/
#define _C_  ,    /// Debug , to simulate vararg macros

extern void PrintLocation(const char *file, int line, const char *funcName, std::ostream &output_stream);

/// Print function in debug macros
#define PrintFunction() PrintLocation(__FILE__, __LINE__, __func__, std::cout);
#define PrintErrorFunction() PrintLocation(__FILE__, __LINE__, __func__, std::cerr);

extern bool EnableDebugPrint;

extern void AbortAt(const char *file, int line, const char *funcName, const char *conditionStr);
extern void PrintOnStdOut(const char *format, ...);

/**
**  Print debug information with function name.
*/
#define DebugPrint(args) \
	do { if (EnableDebugPrint) { PrintFunction(); PrintOnStdOut(args); } } while (0)

/*============================================================================
==  Definitions
============================================================================*/

/*----------------------------------------------------------------------------
--  Some limits
----------------------------------------------------------------------------*/

constexpr int PlayerMax = 64;                 /// How many players are supported
constexpr int UnitTypeMax = 2048;                /// How many unit types supported
constexpr int UpgradeMax = 2048;                /// How many upgrades supported
//Wyrmgus start
//constexpr int MAX_RACES = 8;
constexpr int MAX_RACES = 256;

constexpr int AuraRange = 6;					/// Range of auras
constexpr int PlayerHeroMax = 4;				/// Maximum heroes per player
constexpr int PlayerMajorDeityMax = 1;	/// Maximum major deities per player/character
//constexpr int PlayerMinorDeityMax = 3;	/// Maximum minor deities per player/character
constexpr int PlayerMinorDeityMax = 1;	/// Maximum minor deities per player/character
constexpr int PlayerDeityMax = (PlayerMajorDeityMax + PlayerMinorDeityMax);

constexpr int DEFAULT_HOURS_PER_DAY = 24;
constexpr int DEFAULT_DAYS_PER_YEAR = 365;
//Wyrmgus end

/// Frames per second to display (original 30-40)
constexpr int FRAMES_PER_SECOND = 30;  // 1/30s
/// Game cycles per second to simulate (original 30-40)
constexpr int CYCLES_PER_SECOND = 30;  // 1/30s 0.33ms
constexpr int CYCLES_PER_MINUTE = (CYCLES_PER_SECOND * 60);

constexpr int CYCLES_PER_IN_GAME_HOUR = (CYCLES_PER_SECOND * 10); // every 10 seconds of gameplay = 1 hour for the purposes of in-game date/time

constexpr int DEFAULT_DAY_MULTIPLIER_PER_YEAR = (DEFAULT_DAYS_PER_YEAR * DEFAULT_HOURS_PER_DAY * CYCLES_PER_IN_GAME_HOUR / CYCLES_PER_SECOND / 60 / 60); //the purpose of the day multiplier is so that we can effectively have months only taking e.g. 10 days to pass, so that we can have a day/night cycle, different days of the week and months/years, with months and years still not taking an overly long time to pass; the day multiplier should as such NOT affect the changes in day of the week; here we define the day multiplier to be such that one year equals roughly one hour of gameplay

constexpr int HOUR_MULTIPLIER_DIVIDER = 4; //for calculating the multipliers for schedules; the maximum duration length will be the default multiplier * the HOUR_MULTIPLIER_DIVIDER

constexpr int HeroCooldownCycles = CYCLES_PER_MINUTE; /// Cooldown (in cycles) for recruiting a hero

constexpr int DefaultTileMovementCost = 8;

/*----------------------------------------------------------------------------
--  stratagus.cpp
----------------------------------------------------------------------------*/

namespace wyrmgus {
	class renderer;
	enum class direction;
}

//Wyrmgus start
extern std::string PlayerFaction;
//Wyrmgus end

extern unsigned long GameCycle;				/// Game simulation cycle counter
extern unsigned long FastForwardCycle;		/// Game Replay Fast Forward Counter

extern void Exit(int err);                  /// Exit

extern void UpdateDisplay();            /// Game display update
extern void DrawMapArea(std::vector<std::function<void(wyrmgus::renderer *)>> &render_commands);              /// Draw the map area
extern void GameMainLoop();             /// Game main loop
extern void stratagusMain(int argc, char **argv); /// main entry

//Wyrmgus start
enum Difficulties {
	DifficultyNoDifficulty = 0,
	DifficultyEasy,
	DifficultyNormal,
	DifficultyHard,
	DifficultyBrutal
};

enum TechLevels {
	NoTechLevel = 0,
	AgrarianBronzeTechLevel,
	AgrarianIronTechLevel,
	CivilizedBronzeTechLevel,
	CivilizedIronTechLevel,
	CivilizedGunpowderTechLevel
};

extern wyrmgus::direction GetDirectionFromOffset(const int x, const int y);
//Wyrmgus end

extern void load_database(const bool initial_definition);
extern void load_defines();
extern bool is_test_run();
extern void initialize_database();
extern void save_preferences();

int get_difficulty_index();
void set_difficulty_index(const int difficulty_index);

using namespace wyrmgus;
