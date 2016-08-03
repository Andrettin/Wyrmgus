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
//      (c) Copyright 1998-2007 by Lutz Sammer and Jimmy Salmon
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

#ifndef __STRATAGUS_H__
#define __STRATAGUS_H__

//@{

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
#define NOUSER

#if _MSC_VER >= 1800
// From VS2013 onwards, std::min/max are only defined if algorithm is included
#include <algorithm>
#endif

#pragma warning(disable:4244)               /// Conversion from double to uchar
#pragma warning(disable:4761)               /// Integral size mismatch
#pragma warning(disable:4786)               /// Truncated to 255 chars

#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define snprintf _snprintf
#if !(_MSC_VER >= 1500 && _MSC_VER < 1600)
#define vsnprintf _vsnprintf
#endif
#define unlink _unlink
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

/**
**  This simulates vararg macros.
**  Example:
**    DebugPrint("Test %d %d\n" _C_ 1 _C_ 2);
*/
#define _C_  ,    /// Debug , to simulate vararg macros

extern void PrintLocation(const char *file, int line, const char *funcName);

/// Print function in debug macros
#define PrintFunction() PrintLocation(__FILE__, __LINE__, __func__);

extern bool EnableDebugPrint;
extern bool EnableAssert;
extern bool EnableUnitDebug;

extern void AbortAt(const char *file, int line, const char *funcName, const char *conditionStr);
extern void PrintOnStdOut(const char *format, ...);

/**
**  Assert a condition. If cond is not true abort with file,line.
*/
#define Assert(cond) \
	do { if (EnableAssert && !(cond)) { AbortAt(__FILE__, __LINE__, __func__, #cond); }} while (0)

/**
**  Print debug information with function name.
*/
#define DebugPrint(args) \
	do { if (EnableDebugPrint) { PrintFunction(); PrintOnStdOut(args); } } while (0)

/*============================================================================
==  Definitions
============================================================================*/

#include <string.h>

#ifndef __UTIL_H__
#include "util.h"
#endif

inline char *new_strdup(const char *str)
{
	int len = strlen(str) + 1;
	char *newstr = new char[len];
	strcpy_s(newstr, len, str);
	return newstr;
}

/*----------------------------------------------------------------------------
--  General
----------------------------------------------------------------------------*/

/// Text string: Name, Version, Copyright
extern const char NameLine[];

/*----------------------------------------------------------------------------
--  Some limits
----------------------------------------------------------------------------*/

#define PlayerMax    16                 /// How many players are supported
#define UnitTypeMax  2048                /// How many unit types supported
#define UpgradeMax   2048                /// How many upgrades supported
//Wyrmgus start
//#define MAX_RACES 8
#define MAX_RACES 64
#define FactionMax 128				/// Maximum number of factions a civilization can have
#define PlayerColorMax 32			/// How many player colors are supported
#define SkinColorMax 8				/// How many skin colors are supported
#define HairColorMax SkinColorMax	/// How many hair colors are supported
#define VariationMax 32				/// Maximum number of variations a unit can have

#define WorldMapWidthMax 1024		/// Maximum width the grand strategy world map can have
#define WorldMapHeightMax 1024		/// Maximum height the grand strategy world map can have
#define WorldMapTerrainTypeMax 32	/// Maximum world map terrain types
#define WorldMapResourceMax 2048	/// Maximum quantity of resources of a given type which can exist on the world map.

#define AuraRange 6					/// Range of auras
//Wyrmgus end

/// Frames per second to display (original 30-40)
#define FRAMES_PER_SECOND  30  // 1/30s
/// Game cycles per second to simulate (original 30-40)
#define CYCLES_PER_SECOND  30  // 1/30s 0.33ms

/*----------------------------------------------------------------------------
--  stratagus.cpp
----------------------------------------------------------------------------*/

extern std::string StratagusLibPath;        /// Location of stratagus data
extern std::string MenuRace;

extern unsigned long GameCycle;             /// Game simulation cycle counter
extern unsigned long FastForwardCycle;      /// Game Replay Fast Forward Counter
//Wyrmgus start
extern int GameTimeOfDay;					/// Current time of day
//Wyrmgus end

extern void Exit(int err);                  /// Exit
extern void ExitFatal(int err);             /// Exit with fatal error

extern void UpdateDisplay();            /// Game display update
extern void DrawMapArea();              /// Draw the map area
extern void GameMainLoop();             /// Game main loop
extern int stratagusMain(int argc, char **argv); /// main entry

//Wyrmgus start
enum Directions {
	North,
	Northeast,
	East,
	Southeast,
	South,
	Southwest,
	West,
	Northwest,
	
	MaxDirections
};

enum TimesOfDay {
	NoTimeOfDay,
	DawnTimeOfDay,
	MorningTimeOfDay,
	MiddayTimeOfDay,
	AfternoonTimeOfDay,
	DuskTimeOfDay,
	FirstWatchTimeOfDay,
	MidnightTimeOfDay,
	SecondWatchTimeOfDay,
	
	MaxTimesOfDay
};

enum TransitionTypes {
	NorthTransitionType,
	EastTransitionType,
	SouthTransitionType,
	WestTransitionType,
	NortheastOuterTransitionType,
	SoutheastOuterTransitionType,
	SouthwestOuterTransitionType,
	NorthwestOuterTransitionType,
	NortheastInnerTransitionType,
	SoutheastInnerTransitionType,
	SouthwestInnerTransitionType,
	NorthwestInnerTransitionType,
	
	MaxTransitionTypes
};

enum Months {
	JanuaryMonth,
	FebruaryMonth,
	MarchMonth,
	AprilMonth,
	MayMonth,
	JuneMonth,
	JulyMonth,
	AugustMonth,
	SeptemberMonth,
	OctoberMonth,
	NovemberMonth,
	DecemberMonth,
	
	MaxMonths
};

#include <vec2i.h>

extern std::string GetMonthNameById(int month);
extern int GetMonthIdByName(std::string month);
extern int GetReverseDirection(int direction);
extern std::string GetDirectionNameById(int direction);
extern int GetDirectionIdByName(std::string direction);
extern Vec2i GetDirectionOffset(int direction);
extern std::string GetTransitionTypeNameById(int transition_type);
extern int GetTransitionTypeIdByName(std::string transition_type);
//Wyrmgus end

//@}

#endif // !__STRATAGUS_H__
