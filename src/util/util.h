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

#pragma once

/*----------------------------------------------------------------------------
--  Random
----------------------------------------------------------------------------*/

extern int SyncRand(const int max);           /// Syncron rand

/*----------------------------------------------------------------------------
--  Math
----------------------------------------------------------------------------*/

inline int square(const int v)
{
	return v * v;
}

/*----------------------------------------------------------------------------
--  Strings
----------------------------------------------------------------------------*/

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

extern std::vector<std::string> SplitString(const std::string &str, const char *separators);
extern std::string NumberToRomanNumeral(unsigned number);
extern std::string FormatNumber(const int number);

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
--  UTF8
----------------------------------------------------------------------------*/

int UTF8GetNext(const std::string &text, int curpos);
int UTF8GetPrev(const std::string &text, int curpos);

//Wyrmgus start
extern std::string FindAndReplaceString(const std::string &text, const std::string &find, const std::string &replace);
extern std::string FindAndReplaceStringEnding(const std::string &text, const std::string &find, const std::string &replace);
extern std::string FindAndReplaceStringBeginning(const std::string &text, const std::string &find, const std::string &replace);
extern std::string TransliterateText(const std::string &text);			/// Convert special characters into ones more legible for English-speakers
extern std::string CapitalizeString(const std::string &text);			/// Make the string become capitalized
extern std::string DecapitalizeString(const std::string &text);			/// Make the string lose capitalization
extern std::string FullyCapitalizeString(const std::string &text);		/// Make every part of the string after a space become capitalized
extern std::string FullyDecapitalizeString(const std::string &text);	/// Make every part of the string lose capitalization
extern std::string GetPluralForm(const std::string &name);
extern std::string IdentToName(const std::string &text);				/// Make the ident string become a display name
extern std::string NameToIdent(const std::string &text);				/// Make the name be formatted like an ident string
extern std::string SeparateCapitalizedStringElements(const std::string &text);	/// Make the string's capitalized elements become separated
//Wyrmgus end
