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
/**@name util.h - General utilities. */
//
//      (c) Copyright 1998-2006 by Lutz Sammer and Jimmy Salmon
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

#ifndef __UTIL_H__
#define __UTIL_H__

//@{

#ifdef USE_WIN32
#undef NOUSER
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#endif

#include <cstdlib>

/*----------------------------------------------------------------------------
--  Random
----------------------------------------------------------------------------*/

#include <cmath>

extern unsigned SyncRandSeed;           /// Sync random seed value

extern void InitSyncRand();             /// Initialize the syncron rand
extern int SyncRand();                  /// Syncron rand
extern int SyncRand(int max);           /// Syncron rand

///  rand only used on this computer.
extern int MyRand();

/*----------------------------------------------------------------------------
--  Math
----------------------------------------------------------------------------*/

/// Compute a square root using ints
extern long isqrt(long num);

inline int square(int v)
{
	return v * v;
}

template <typename T, typename U>
void clamp(T *value, U minValue, U maxValue)
{
	Assert(minValue <= maxValue);

	if (*value < minValue) {
		*value = minValue;
	} else if (maxValue < *value) {
		*value = maxValue;
	}
}

/*----------------------------------------------------------------------------
--  Strings
----------------------------------------------------------------------------*/

#include <string.h>

#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

#ifndef HAVE_ERRNOT
typedef int errno_t;
#endif

#ifndef HAVE_STRCPYS
extern errno_t strcpy_s(char *dst, size_t dstsize, const char *src);
#endif

#ifndef HAVE_STRNCPYS
extern errno_t strncpy_s(char *dst, size_t dstsize, const char *src, size_t count);
#endif

#ifndef HAVE_STRCATS
extern errno_t strcat_s(char *dst, size_t dstsize, const char *src);
#endif

#ifndef HAVE_STRCASESTR
/// case insensitive strstr
extern char *strcasestr(const char *str, const char *substr);
#endif // !HAVE_STRCASESTR

#ifndef HAVE_STRNLEN
/// determine length of a fixed-length string
extern size_t strnlen(const char *str, size_t strsize);
#endif // !HAVE_STRNLEN

/*----------------------------------------------------------------------------
--  Getopt
----------------------------------------------------------------------------*/

#ifdef HAVE_GETOPT
#include <unistd.h>
#else
extern char *optarg;
extern int optind, opterr, optopt;
int getopt(int argc, char *const argv[], const char *optstring);
#endif

/*----------------------------------------------------------------------------
--  Clipboard
----------------------------------------------------------------------------*/

#include <string>

int GetClipboard(std::string &str);

/*----------------------------------------------------------------------------
--  UTF8
----------------------------------------------------------------------------*/

int UTF8GetNext(const std::string &text, int curpos);
int UTF8GetPrev(const std::string &text, int curpos);

//@}

//Wyrmgus start
extern std::string FindAndReplaceString(std::string text, const std::string& find, const std::string& replace);
extern std::string FindAndReplaceStringEnding(std::string text, const std::string& find, const std::string& replace);
extern std::string FindAndReplaceStringBeginning(std::string text, const std::string& find, const std::string& replace);
extern int GetFileLastModified(std::string file_name);
extern std::string TransliterateText(std::string text);				/// Convert special characters into ones more legible for English-speakers
extern std::string CapitalizeString(std::string text);				/// Make the string become capitalized
extern std::string DecapitalizeString(std::string text);			/// Make the string lose capitalization
extern std::string FullyCapitalizeString(std::string text);			/// Make every part of the string after a space become capitalized
extern std::string FullyDecapitalizeString(std::string text);		/// Make every part of the string lose capitalization
extern std::string GetPluralForm(std::string name);
extern std::string IdentToName(std::string text);					/// Make the ident string become a display name
extern std::string NameToIdent(std::string text);					/// Make the name be formatted like an ident string
extern std::string SeparateCapitalizedStringElements(std::string text);	/// Make the string's capitalized elements become separated
extern std::string GeneratePersonalName(std::string unit_type_ident);
//Wyrmgus end

#endif /* __UTIL_H__ */
