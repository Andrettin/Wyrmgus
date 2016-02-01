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
/**@name util.cpp - General utilites. */
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

#include "stratagus.h"

#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_STACKTRACE
#include <stdexcept>
#include <stacktrace/call_stack.hpp>
#include <stacktrace/stack_exception.hpp>
#endif

#ifdef USE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

/*----------------------------------------------------------------------------
--  Random
----------------------------------------------------------------------------*/

unsigned SyncRandSeed;               /// sync random seed value.

/**
**  Inititalize sync rand seed.
*/
void InitSyncRand()
{
	SyncRandSeed = 0x87654321;
}

/**
**  Synchronized random number.
**
**  @note This random value must be same on all machines in network game.
**  Very simple random generations, enough for us.
*/
int SyncRand()
{
	int val;

	val = SyncRandSeed >> 16;

	SyncRandSeed = SyncRandSeed * (0x12345678 * 4 + 1) + 1;

	return val;
}

/**
**  Synchronized random number.
**
**  @param max  Max value of random number to return
*/
int SyncRand(int max)
{
	return SyncRand() % max;
}



int MyRand()
{
	return rand();
}

/*----------------------------------------------------------------------------
--  Math
----------------------------------------------------------------------------*/

/**
**  Compute a square root using ints
**
**  Uses John Halleck's method, see
**  http://www.cc.utah.edu/~nahaj/factoring/isqrt.legalize.c.html
**
**  @param num  Calculate the square root of this number
**
**  @return     The integer square root.
*/
long isqrt(long num)
{
	long squaredbit;
	long remainder;
	long root;

	if (num < 1) {
		return 0;
	}

	//
	//  Load the binary constant 01 00 00 ... 00, where the number
	//  of zero bits to the right of the single one bit
	//  is even, and the one bit is as far left as is consistent
	//  with that condition.)
	//
	//  This portable load replaces the loop that used to be
	//  here, and was donated by  legalize@xmission.com
	//
	squaredbit  = (long)((((unsigned long)~0L) >> 1) & ~(((unsigned long)~0L) >> 2));

	// Form bits of the answer.
	remainder = num;
	root = 0;
	while (squaredbit > 0) {
		if (remainder >= (squaredbit | root)) {
			remainder -= (squaredbit | root);
			root >>= 1;
			root |= squaredbit;
		} else {
			root >>= 1;
		}
		squaredbit >>= 2;
	}

	return root;
}


/*----------------------------------------------------------------------------
--  Strings
----------------------------------------------------------------------------*/

#ifndef HAVE_STRCPYS
errno_t strcpy_s(char *dst, size_t dstsize, const char *src)
{
	if (dst == NULL || src == NULL) {
		return EINVAL;
	}
	if (strlen(src) >= dstsize) {
		return ERANGE;
	}
	strcpy(dst, src);
	return 0;
}
#endif

#ifndef HAVE_STRNLEN
size_t strnlen(const char *str, size_t strsize)
{
	size_t len = 0;
	while (len < strsize) {
		if (*str == '\0') {
			break;
		}
		++str;
		++len;
	}
	return len;
}
#endif

#ifndef HAVE_STRNCPYS
errno_t strncpy_s(char *dst, size_t dstsize, const char *src, size_t count)
{
	if (dst == NULL || src == NULL || dstsize == 0) {
		return EINVAL;
	}

	size_t mincount;
	if (count == _TRUNCATE) {
		mincount = strnlen(src, dstsize);
	} else {
		mincount = strnlen(src, count);
	}
	if (mincount >= dstsize) {
		if (count != _TRUNCATE) {
			dst[0] = '\0';
			return EINVAL;
		} else {
			mincount = dstsize - 1;
		}
	}
	for (size_t i = 0; i < mincount; ++i) {
		*dst++ = *src++;
	}
	*dst = '\0';
	return 0;
}
#endif

#ifndef HAVE_STRCATS
errno_t strcat_s(char *dst, size_t dstsize, const char *src)
{
	if (dst == NULL || src == NULL) {
		return EINVAL;
	}
	char *enddst = dst;
	size_t count = dstsize;
	while (count > 0 && *enddst != '\0') {
		++enddst;
		count--;
	}
	if (count == 0) {
		return EINVAL;
	}
	if (strlen(src) >= count) {
		return ERANGE;
	}
	strcpy(enddst, src);
	return 0;
}
#endif

#if !defined(HAVE_STRCASESTR)
/**
**  Case insensitive version of strstr
**
**  @param a  String to search in
**  @param b  Substring to search for
**
**  @return   Pointer to first occurrence of b or NULL if not found.
*/
char *strcasestr(const char *a, const char *b)
{
	int x;

	if (!a || !*a || !b || !*b || strlen(a) < strlen(b)) {
		return NULL;
	}

	x = 0;
	while (*a) {
		if (a[x] && (tolower(a[x]) == tolower(b[x]))) {
			++x;
		} else if (b[x]) {
			++a;
			x = 0;
		} else {
			return (char *)a;
		}
	}

	return NULL;
}
#endif // !HAVE_STRCASESTR


/*----------------------------------------------------------------------------
--  Getopt
----------------------------------------------------------------------------*/

#ifndef HAVE_GETOPT

/**
**  Standard implementation of getopt(3).
**
**  One extension: If the first character of the optionsstring is a ':'
**  the error return for 'argument required' is a ':' not a '?'.
**  This makes it easier to differentiate between an 'illegal option' and
**  an 'argument required' error.
*/

#include <string.h>

int opterr = 1;
int optind = 1;
int optopt;
char *optarg;

static void getopt_err(const char *argv0, const char *str, char opt)
{
	if (opterr) {
		const char *x;

		while ((x = strchr(argv0, '/'))) {
			argv0 = x + 1;
		}

		fprintf(stderr, "%s%s%c\n", argv0, str, opt);
	}
}

int getopt(int argc, char *const *argv, const char *opts)
{
	static int sp = 1;
	register int c;
	register const char *cp;

	optarg = NULL;

	if (sp == 1) {
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
			return EOF;
		} else if (!strcmp(argv[optind], "--")) {
			optind++;
			return EOF;
		}
	}
	optopt = c = argv[optind][sp];
	if (c == ':' || (cp = strchr(opts, c)) == NULL) {
		getopt_err(argv[0], ": illegal option -", (char)c);
		cp = "xx"; /* make the next if false */
		c = '?';
	}
	if (*++cp == ':') {
		if (argv[optind][++sp] != '\0') {
			optarg = &argv[optind++][sp];
		} else if (++optind < argc) {
			optarg = argv[optind++];
		} else {
			getopt_err(argv[0], ": option requires an argument -", (char)c);
			c = (*opts == ':') ? ':' : '?';
		}
		sp = 1;
	} else if (argv[optind][++sp] == '\0') {
		optind++;
		sp = 1;
	}
	return c;
}

#endif


/*----------------------------------------------------------------------------
--  Clipboard
----------------------------------------------------------------------------*/

/**
** Paste text from the clipboard
*/
int GetClipboard(std::string &str)
{
#if defined(USE_WIN32) || defined(USE_X11)
	int i;
	unsigned char *clipboard;
#ifdef USE_WIN32
	HGLOBAL handle;
#elif defined(USE_X11)
	Display *display;
	Window window;
	Atom rettype;
	unsigned long nitem;
	unsigned long dummy;
	int retform;
	XEvent event;
#endif

#ifdef USE_WIN32
	if (!IsClipboardFormatAvailable(CF_TEXT) || !OpenClipboard(NULL)) {
		return -1;
	}
	handle = GetClipboardData(CF_TEXT);
	if (!handle) {
		CloseClipboard();
		return -1;
	}
	clipboard = (unsigned char *)GlobalLock(handle);
	if (!clipboard) {
		CloseClipboard();
		return -1;
	}
#elif defined(USE_X11)
	if (!(display = XOpenDisplay(NULL))) {
		return -1;
	}

	// Creates a non maped temporary X window to hold the selection
	if (!(window = XCreateSimpleWindow(display,
									   DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0))) {
		XCloseDisplay(display);
		return -1;
	}

	XConvertSelection(display, XA_PRIMARY, XA_STRING, XA_STRING,
					  window, CurrentTime);

	XNextEvent(display, &event);

	if (event.type != SelectionNotify || event.xselection.property != XA_STRING) {
		return -1;
	}

	XGetWindowProperty(display, window, XA_STRING, 0, 1024, False,
					   XA_STRING, &rettype, &retform, &nitem, &dummy, &clipboard);

	XDestroyWindow(display, window);
	XCloseDisplay(display);

	if (rettype != XA_STRING || retform != 8) {
		if (clipboard != NULL) {
			XFree(clipboard);
		}
		clipboard = NULL;
	}

	if (clipboard == NULL) {
		return -1;
	}
#endif
	// Only allow ascii characters
	for (i = 0; clipboard[i] != '\0'; ++i) {
		if (clipboard[i] < 32 || clipboard[i] > 126) {
			return -1;
		}
	}
	str = (char *)clipboard;
#ifdef USE_WIN32
	GlobalUnlock(handle);
	CloseClipboard();
#elif defined(USE_X11)
	if (clipboard != NULL) {
		XFree(clipboard);
	}
#endif
	return 0;
#else
	return -1;
#endif
}


/*----------------------------------------------------------------------------
--  UTF8
----------------------------------------------------------------------------*/

int UTF8GetPrev(const std::string &text, int curpos)
{
	--curpos;
	if (curpos < 0) {
		return curpos;
	}
	while (curpos >= 0) {
		if ((text[curpos] & 0xC0) != 0x80) {
			return curpos;
		}
		--curpos;
	}
	if (curpos < 0) {
		fprintf(stderr, "Invalid UTF8.\n");
	}
	return 0;
}

int UTF8GetNext(const std::string &text, int curpos)
{
	if (curpos == (int)text.size()) {
		return curpos + 1;
	}
	char c = text[curpos];
	if (!(c & 0x80)) {
		return curpos + 1;
	}
	if ((c & 0xE0) == 0xC0) {
		return curpos + 2;
	}
	if ((c & 0xF0) == 0xE0) {
		return curpos + 3;
	}
	fprintf(stderr, "Invalid UTF8.\n");
	return text.size();
}


/*----------------------------------------------------------------------------
--  others
----------------------------------------------------------------------------*/

void PrintLocation(const char *file, int line, const char *funcName)
{
	fprintf(stdout, "%s:%d: %s: ", file, line, funcName);
}

void AbortAt(const char *file, int line, const char *funcName, const char *conditionStr)
{
	char buf[1024];
	snprintf(buf, 1024, "Assertion failed at %s:%d: %s: %s\n", file, line, funcName, conditionStr);
#ifdef USE_STACKTRACE
	throw stacktrace::stack_runtime_error((const char*)buf);
#else
	fprintf(stderr, "%s\n", buf);
#endif
	fflush(stdout);
	fflush(stderr);
	abort();
}

void PrintOnStdOut(const char *format, ...)
{
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
	fflush(stdout);
}

//Wyrmgus start
#include "editor.h" //for personal name generation
#include "player.h" //for personal name generation
#include "unittype.h" //for personal name generation
#include "upgrade.h" //for personal name generation

std::string FindAndReplaceString(std::string text, const std::string& find, const std::string& replace) {
    size_t pos = 0;
    while ((pos = text.find(find, pos)) != std::string::npos) {
         text.replace(pos, find.length(), replace);
         pos += replace.length();
    }
    return text;
}

std::string FindAndReplaceStringEnding(std::string text, const std::string& find, const std::string& replace) {
	size_t pos = text.find(find, text.length() - find.length());
	if (pos != std::string::npos) {
		text.replace(pos, find.length(), replace);
	}
    return text;
}

std::string FindAndReplaceStringBeginning(std::string text, const std::string& find, const std::string& replace) {
	size_t pos = text.find(find, 0);
	if (pos != std::string::npos && pos == 0) {
		text.replace(pos, find.length(), replace);
	}
    return text;
}

std::string TransliterateText(std::string text) //convert special characters into ones more legible for English-speakers
{
	text = FindAndReplaceString(text, "Á", "A");
	text = FindAndReplaceString(text, "á", "a");
	text = FindAndReplaceString(text, "À", "A");
	text = FindAndReplaceString(text, "à", "a");
	text = FindAndReplaceString(text, "Ä", "A");
	text = FindAndReplaceString(text, "ä", "a");
	text = FindAndReplaceString(text, "Ā", "A");
	text = FindAndReplaceString(text, "ā", "a");
	text = FindAndReplaceString(text, "Â", "A");
	text = FindAndReplaceString(text, "â", "a");
	text = FindAndReplaceString(text, "Ā́", "A");
	text = FindAndReplaceString(text, "ā́", "a");
	text = FindAndReplaceString(text, "Æ", "Ae");
	text = FindAndReplaceString(text, "æ", "ae");
	text = FindAndReplaceString(text, "Ǣ", "Ae");
	text = FindAndReplaceString(text, "ǣ", "ae");
	text = FindAndReplaceString(text, "Ǽ", "Ae");
	text = FindAndReplaceString(text, "ǽ", "ae");
	text = FindAndReplaceString(text, "Ð", "D");
	text = FindAndReplaceString(text, "ð", "d");
	text = FindAndReplaceString(text, "Ḍ", "D");
	text = FindAndReplaceString(text, "ḍ", "d");
	text = FindAndReplaceString(text, "Đ", "d");
	text = FindAndReplaceString(text, "đ", "d");
	text = FindAndReplaceString(text, "Ē", "E");
	text = FindAndReplaceString(text, "ē", "e");
	text = FindAndReplaceString(text, "Ê", "E");
	text = FindAndReplaceString(text, "ê", "e");
	text = FindAndReplaceString(text, "Ě", "E");
	text = FindAndReplaceString(text, "ě", "e");
	text = FindAndReplaceString(text, "Ḗ", "E");
	text = FindAndReplaceString(text, "ḗ", "e");
	text = FindAndReplaceString(text, "Ə", "E");
	text = FindAndReplaceString(text, "ə", "e");
	text = FindAndReplaceString(text, "Í", "I");
	text = FindAndReplaceString(text, "í", "i");
	text = FindAndReplaceString(text, "Ī", "I");
	text = FindAndReplaceString(text, "ī", "i");
	text = FindAndReplaceString(text, "Î", "I");
	text = FindAndReplaceString(text, "î", "i");
	text = FindAndReplaceString(text, "Ĭ", "I");
	text = FindAndReplaceString(text, "ĭ", "i");
	text = FindAndReplaceString(text, "Ī̆", "I");
	text = FindAndReplaceString(text, "ī̆", "i");
	text = FindAndReplaceString(text, "Ö", "O");
	text = FindAndReplaceString(text, "ö", "o");
	text = FindAndReplaceString(text, "Ó", "O");
	text = FindAndReplaceString(text, "ó", "o");
	text = FindAndReplaceString(text, "Ō", "O");
	text = FindAndReplaceString(text, "ō", "o");
	text = FindAndReplaceString(text, "Ô", "O");
	text = FindAndReplaceString(text, "ô", "o");
	text = FindAndReplaceString(text, "Ø", "Ö"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "ø", "ö"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ǫ", "O"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "ǫ", "o"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ṓ", "O");
	text = FindAndReplaceString(text, "ṓ", "o");
	text = FindAndReplaceString(text, "Φ", "Ph");
	text = FindAndReplaceString(text, "φ", "ph");
	text = FindAndReplaceString(text, "Ș", "S");
	text = FindAndReplaceString(text, "ș", "s");
	text = FindAndReplaceString(text, "ß", "ss"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ț", "T");
	text = FindAndReplaceString(text, "ț", "t");
	text = FindAndReplaceString(text, "Þ", "Th"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "þ", "th"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ü", "U");
	text = FindAndReplaceString(text, "ü", "u");
	text = FindAndReplaceString(text, "Ū", "U");
	text = FindAndReplaceString(text, "ū", "u");
	text = FindAndReplaceString(text, "Û", "U");
	text = FindAndReplaceString(text, "û", "u");
	text = FindAndReplaceString(text, "Ŭ", "U");
	text = FindAndReplaceString(text, "ŭ", "u");
	text = FindAndReplaceString(text, "Ū́", "U");
	text = FindAndReplaceString(text, "ū́", "u");
	text = FindAndReplaceString(text, "Ȳ", "Y");
	text = FindAndReplaceString(text, "ȳ", "y");
	text = FindAndReplaceString(text, "Ž", "Z");
	text = FindAndReplaceString(text, "ž", "z");
	
	//replace endings in -r after consonants (which happens in the nominative for Old Norse); Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceStringEnding(text, "dr", "d");
	text = FindAndReplaceStringEnding(text, "fr", "f");
	text = FindAndReplaceStringEnding(text, "gr", "g");
	text = FindAndReplaceStringEnding(text, "kr", "k");
	text = FindAndReplaceStringEnding(text, "nr", "n");
	text = FindAndReplaceStringEnding(text, "rr", "r");
	text = FindAndReplaceStringEnding(text, "tr", "t");
	
	//Greek characters
	text = FindAndReplaceString(text, "Α", "A");
	text = FindAndReplaceString(text, "α", "a");
	text = FindAndReplaceString(text, "Χ", "Ch");
	text = FindAndReplaceString(text, "χ", "ch");
	text = FindAndReplaceString(text, "Έ", "E");
	text = FindAndReplaceString(text, "έ", "e");
	text = FindAndReplaceString(text, "Ι", "I");
	text = FindAndReplaceString(text, "ι", "i");
	text = FindAndReplaceString(text, "Μ", "M");
	text = FindAndReplaceString(text, "μ", "m");
	text = FindAndReplaceString(text, "Ν", "N");
	text = FindAndReplaceString(text, "ν", "n");
	text = FindAndReplaceString(text, "Ο", "O");
	text = FindAndReplaceString(text, "ο", "o");
	text = FindAndReplaceString(text, "Ό", "O");
	text = FindAndReplaceString(text, "ό", "o");
	text = FindAndReplaceString(text, "Σ", "S");
	text = FindAndReplaceString(text, "σ", "s");
	text = FindAndReplaceString(text, "ς", "s");
	text = FindAndReplaceString(text, "Θ", "Th");
	text = FindAndReplaceString(text, "θ", "th");
	text = FindAndReplaceString(text, "Ύ", "Y");
	text = FindAndReplaceString(text, "ύ", "y");
	
	return text;
}

std::string CapitalizeString(std::string text)
{
	text[0] = toupper(text[0]);
	
	// replace special characters which may not have been uppered with the previous method
	text = FindAndReplaceStringBeginning(text, "ā", "Ā");
	text = FindAndReplaceStringBeginning(text, "â", "Â");
	text = FindAndReplaceStringBeginning(text, "æ", "Æ");
	text = FindAndReplaceStringBeginning(text, "ǣ", "Ǣ");
	text = FindAndReplaceStringBeginning(text, "ǽ", "Ǽ");
	text = FindAndReplaceStringBeginning(text, "ð", "Ð");
	text = FindAndReplaceStringBeginning(text, "ḍ", "Ḍ");
	text = FindAndReplaceStringBeginning(text, "ē", "Ē");
	text = FindAndReplaceStringBeginning(text, "ê", "Ê");
	text = FindAndReplaceStringBeginning(text, "ě", "Ě");
	text = FindAndReplaceStringBeginning(text, "ī", "Ī");
	text = FindAndReplaceStringBeginning(text, "î", "Î");
	text = FindAndReplaceStringBeginning(text, "ĭ", "Ĭ");
	text = FindAndReplaceStringBeginning(text, "ī̆", "Ī̆");
	text = FindAndReplaceStringBeginning(text, "ō", "Ō");
	text = FindAndReplaceStringBeginning(text, "ô", "Ô");
	text = FindAndReplaceStringBeginning(text, "ø", "Ø");
	text = FindAndReplaceStringBeginning(text, "ǫ", "Ǫ");
	text = FindAndReplaceStringBeginning(text, "ș", "Ș");
	text = FindAndReplaceStringBeginning(text, "ț", "Ț");
	text = FindAndReplaceStringBeginning(text, "þ", "Þ");
	text = FindAndReplaceStringBeginning(text, "ū", "Ū");
	text = FindAndReplaceStringBeginning(text, "û", "Û");
	text = FindAndReplaceStringBeginning(text, "ŭ", "Ŭ");
	text = FindAndReplaceStringBeginning(text, "ȳ", "Ȳ");
	text = FindAndReplaceStringBeginning(text, "ž", "Ž");
	
	//Greek characters
	text = FindAndReplaceStringBeginning(text, "α", "Α");
	text = FindAndReplaceStringBeginning(text, "χ", "Χ");
	text = FindAndReplaceStringBeginning(text, "έ", "Έ");
	text = FindAndReplaceStringBeginning(text, "ι", "Ι");
	text = FindAndReplaceStringBeginning(text, "μ", "Μ");
	text = FindAndReplaceStringBeginning(text, "ν", "Ν");
	text = FindAndReplaceStringBeginning(text, "ο", "Ο");
	text = FindAndReplaceStringBeginning(text, "ό", "Ό");
	text = FindAndReplaceStringBeginning(text, "σ", "Σ");
	text = FindAndReplaceStringBeginning(text, "θ", "Θ");
	text = FindAndReplaceStringBeginning(text, "ύ", "Ύ");
	
	return text;
}

std::string DecapitalizeString(std::string text)
{
	text[0] = tolower(text[0]);
	
	// replace special characters which may not have been lowered with the previous method
	text = FindAndReplaceStringBeginning(text, "Ā", "ā");
	text = FindAndReplaceStringBeginning(text, "Â", "â");
	text = FindAndReplaceStringBeginning(text, "Æ", "æ");
	text = FindAndReplaceStringBeginning(text, "Ǣ", "ǣ");
	text = FindAndReplaceStringBeginning(text, "Ǽ", "ǽ");
	text = FindAndReplaceStringBeginning(text, "Ð", "ð");
	text = FindAndReplaceStringBeginning(text, "Ḍ", "ḍ");
	text = FindAndReplaceStringBeginning(text, "Ē", "ē");
	text = FindAndReplaceStringBeginning(text, "Ê", "ê");
	text = FindAndReplaceStringBeginning(text, "Ě", "ě");
	text = FindAndReplaceStringBeginning(text, "Ī", "ī");
	text = FindAndReplaceStringBeginning(text, "Î", "î");
	text = FindAndReplaceStringBeginning(text, "Ĭ", "ĭ");
	text = FindAndReplaceStringBeginning(text, "Ī̆", "ī̆");
	text = FindAndReplaceStringBeginning(text, "Ō", "ō");
	text = FindAndReplaceStringBeginning(text, "Ô", "ô");
	text = FindAndReplaceStringBeginning(text, "Ø", "ø");
	text = FindAndReplaceStringBeginning(text, "Ǫ", "ǫ");
	text = FindAndReplaceStringBeginning(text, "Þ", "þ");
	text = FindAndReplaceStringBeginning(text, "Ū", "ū");
	text = FindAndReplaceStringBeginning(text, "Û", "û");
	text = FindAndReplaceStringBeginning(text, "Ŭ", "ŭ");
	text = FindAndReplaceStringBeginning(text, "Ȳ", "ȳ");
	text = FindAndReplaceStringBeginning(text, "Ž", "ž");
	
	//Greek characters
	text = FindAndReplaceStringBeginning(text, "Α", "α");
	text = FindAndReplaceStringBeginning(text, "Χ", "χ");
	text = FindAndReplaceStringBeginning(text, "Έ", "έ");
	text = FindAndReplaceStringBeginning(text, "Ι", "ι");
	text = FindAndReplaceStringBeginning(text, "Μ", "μ");
	text = FindAndReplaceStringBeginning(text, "Ν", "ν");
	text = FindAndReplaceStringBeginning(text, "Ο", "ο");
	text = FindAndReplaceStringBeginning(text, "Ό", "ό");
	text = FindAndReplaceStringBeginning(text, "Σ", "σ");
	text = FindAndReplaceStringBeginning(text, "Θ", "θ");
	text = FindAndReplaceStringBeginning(text, "Ύ", "ύ");
	
	return text;
}

std::string FullyCapitalizeString(std::string text)
{
	text = CapitalizeString(text);
	
    size_t pos = 0;
    while ((pos = text.find(" ", pos)) != std::string::npos) {
		text.substr(pos + 1, pos + 2) = CapitalizeString(text.substr(pos + 1, pos + 2));
        pos += 1;
    }
	
	return text;
}

std::string FullyDecapitalizeString(std::string text)
{
	text = DecapitalizeString(text);
	
    size_t pos = 0;
    while ((pos = text.find(" ", pos)) != std::string::npos) {
		text.substr(pos + 1, pos + 2) = DecapitalizeString(text.substr(pos + 1, pos + 2));
        pos += 1;
    }
	
	return text;
}

std::string IdentToName(std::string text)
{
	text = FindAndReplaceString(text, "-", " ");
	text = FullyCapitalizeString(text);
	
	return text;
}

std::string NameToIdent(std::string text)
{
	text = FullyDecapitalizeString(text);
	text = FindAndReplaceString(text, " ", "-");
	
	return text;
}

std::string SeparateCapitalizedStringElements(std::string text)
{
	for (size_t pos = 1; pos < text.length(); ++pos) {
		if (isupper(text[pos])) {
			text.replace(pos, 1, " " + text.substr(pos, 1));
			pos += 1;
		}
	}
    return text;
}

/**
**  Generates a personal name.
*/
std::string GeneratePersonalName(int language, int unit_type_id)
{
	const CUnitType &type = *UnitTypes[unit_type_id];
	std::string personal_name;

	if (Editor.Running == EditorEditing) { // don't set the personal name if in the editor
		personal_name = "";
	} else if (!type.PersonalNames[0].empty() || !type.PersonalNamePrefixes[0].empty()) {
		int PersonalNameCount = 0;
		int PersonalNamePrefixCount = 0;
		int PersonalNameSuffixCount = 0;
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!type.PersonalNames[i].empty()) {
				PersonalNameCount += 1;
			}
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!type.PersonalNamePrefixes[i].empty()) {
				PersonalNamePrefixCount += 1;
			}
		}
		for (int i = 0; i < PersonalNameMax; ++i) {
			if (!type.PersonalNameSuffixes[i].empty()) {
				PersonalNameSuffixCount += 1;
			}
		}
		if (PersonalNameCount > 0 || PersonalNamePrefixCount > 0) {
			int PersonalNameProbability = PersonalNameCount * 10000 / (PersonalNameCount + (PersonalNamePrefixCount * PersonalNameSuffixCount));
			if (SyncRand(10000) < PersonalNameProbability) {
				personal_name = type.PersonalNames[SyncRand(PersonalNameCount)];
			} else {
				personal_name = type.PersonalNamePrefixes[SyncRand(PersonalNamePrefixCount)];
				personal_name += type.PersonalNameSuffixes[SyncRand(PersonalNameSuffixCount)];
			}
		}
	} else if (
		language != -1
		&& (
			PlayerRaces.Languages[language]->LanguageNouns.size() > 0
			|| PlayerRaces.Languages[language]->LanguageVerbs.size() > 0
			|| PlayerRaces.Languages[language]->LanguageAdjectives.size() > 0
		)
	) {
		if (type.BoolFlag[ORGANIC_INDEX].value) {
			personal_name = GenerateName(language, "person");
		} else {
			personal_name = GenerateName(language, type.Ident);
		}
	}
	
	personal_name = TransliterateText(personal_name);
	
	return personal_name;
}

std::string GeneratePersonalName(std::string language_ident, std::string unit_type_ident)
{
	int language = PlayerRaces.GetLanguageIndexByIdent(language_ident);
	int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
	return GeneratePersonalName(language, unit_type_id);
}

std::string GenerateName(int language, std::string type)
{
	std::string name;
	
	if (
		PlayerRaces.Languages[language]->LanguageNouns.size() > 0
		|| PlayerRaces.Languages[language]->LanguageVerbs.size() > 0
		|| PlayerRaces.Languages[language]->LanguageAdjectives.size() > 0
		|| PlayerRaces.Languages[language]->LanguageNumerals.size() > 0
	) {
		int noun_name_count = 0;
		std::string noun_names[PersonalNameMax];
		int noun_name_ids[PersonalNameMax];
		int adjective_name_count = 0;
		std::string adjective_names[PersonalNameMax];
		int adjective_name_ids[PersonalNameMax];
		
		int noun_prefix_count = 0;
		std::string noun_prefixes[PersonalNameMax];
		int noun_prefix_ids[PersonalNameMax];
		int verb_prefix_count = 0;
		std::string verb_prefixes[PersonalNameMax];
		int verb_prefix_ids[PersonalNameMax];
		int adjective_prefix_count = 0;
		std::string adjective_prefixes[PersonalNameMax];
		int adjective_prefix_ids[PersonalNameMax];
		int numeral_prefix_count = 0;
		std::string numeral_prefixes[PersonalNameMax];
		int numeral_prefix_ids[PersonalNameMax];

		int noun_infix_count = 0;
		std::string noun_infixes[PersonalNameMax];
		int noun_infix_ids[PersonalNameMax];
		int verb_infix_count = 0;
		std::string verb_infixes[PersonalNameMax];
		int verb_infix_ids[PersonalNameMax];
		int adjective_infix_count = 0;
		std::string adjective_infixes[PersonalNameMax];
		int adjective_infix_ids[PersonalNameMax];
		int numeral_infix_count = 0;
		std::string numeral_infixes[PersonalNameMax];
		int numeral_infix_ids[PersonalNameMax];

		int noun_suffix_count = 0;
		std::string noun_suffixes[PersonalNameMax];
		int noun_suffix_ids[PersonalNameMax];
		int verb_suffix_count = 0;
		std::string verb_suffixes[PersonalNameMax];
		int verb_suffix_ids[PersonalNameMax];
		int adjective_suffix_count = 0;
		std::string adjective_suffixes[PersonalNameMax];
		int adjective_suffix_ids[PersonalNameMax];
		int numeral_suffix_count = 0;
		std::string numeral_suffixes[PersonalNameMax];
		int numeral_suffix_ids[PersonalNameMax];
		
		int noun_separate_prefix_count = 0;
		std::string noun_separate_prefixes[PersonalNameMax];
		int noun_separate_prefix_ids[PersonalNameMax];
		int verb_separate_prefix_count = 0;
		std::string verb_separate_prefixes[PersonalNameMax];
		int verb_separate_prefix_ids[PersonalNameMax];
		int adjective_separate_prefix_count = 0;
		std::string adjective_separate_prefixes[PersonalNameMax];
		int adjective_separate_prefix_ids[PersonalNameMax];
		int numeral_separate_prefix_count = 0;
		std::string numeral_separate_prefixes[PersonalNameMax];
		int numeral_separate_prefix_ids[PersonalNameMax];

		int noun_separate_infix_count = 0;
		std::string noun_separate_infixes[PersonalNameMax];
		int noun_separate_infix_ids[PersonalNameMax];
		int verb_separate_infix_count = 0;
		std::string verb_separate_infixes[PersonalNameMax];
		int verb_separate_infix_ids[PersonalNameMax];
		int adjective_separate_infix_count = 0;
		std::string adjective_separate_infixes[PersonalNameMax];
		int adjective_separate_infix_ids[PersonalNameMax];
		int numeral_separate_infix_count = 0;
		std::string numeral_separate_infixes[PersonalNameMax];
		int numeral_separate_infix_ids[PersonalNameMax];

		int noun_separate_suffix_count = 0;
		std::string noun_separate_suffixes[PersonalNameMax];
		int noun_separate_suffix_ids[PersonalNameMax];
		int verb_separate_suffix_count = 0;
		std::string verb_separate_suffixes[PersonalNameMax];
		int verb_separate_suffix_ids[PersonalNameMax];
		int adjective_separate_suffix_count = 0;
		std::string adjective_separate_suffixes[PersonalNameMax];
		int adjective_separate_suffix_ids[PersonalNameMax];
		int numeral_separate_suffix_count = 0;
		std::string numeral_separate_suffixes[PersonalNameMax];
		int numeral_separate_suffix_ids[PersonalNameMax];
		
		for (size_t i = 0; i < PlayerRaces.Languages[language]->LanguageNouns.size(); ++i) {
			if (PlayerRaces.Languages[language]->LanguageNouns[i]->HasTypeName(type)) { // nouns which can be used as names for this type without compounding
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->NameSingular) {
					noun_names[noun_name_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
					noun_name_ids[noun_name_count] = i;
					noun_name_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->NamePlural) {
					noun_names[noun_name_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
					noun_name_ids[noun_name_count] = i;
					noun_name_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNouns[i]->HasPrefixTypeName(type)) {
				if (PlayerRaces.Languages[language]->LanguageNouns[i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixSingular) {
						noun_prefixes[noun_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
						noun_prefix_ids[noun_prefix_count] = i;
						noun_prefix_count += 1;
					}
					if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixPlural) {
						noun_prefixes[noun_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
						noun_prefix_count += 1;
					}
				} else {
					if (PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixSingular) {
						if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularGenitive.empty()) {
							noun_prefixes[noun_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularGenitive;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						} else if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty()) { //if no genitive is present, use the nominative instead
							noun_prefixes[noun_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						}
					}
					if (PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixPlural) {
						if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralGenitive.empty()) {
							noun_prefixes[noun_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralGenitive;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						} else if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty()) { //if no genitive is present, use the nominative instead
							noun_prefixes[noun_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
							noun_prefix_ids[noun_prefix_count] = i;
							noun_prefix_count += 1;
						}
					}
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNouns[i]->HasSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->SuffixSingular) {
					noun_suffixes[noun_suffix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
					noun_suffix_ids[noun_suffix_count] = i;
					noun_suffix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->SuffixPlural) {
					noun_suffixes[noun_suffix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
					noun_suffix_ids[noun_suffix_count] = i;
					noun_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNouns[i]->HasInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->InfixSingular) {
					noun_infixes[noun_infix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
					noun_infix_ids[noun_infix_count] = i;
					noun_infix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->InfixPlural) {
					noun_infixes[noun_infix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
					noun_infix_ids[noun_infix_count] = i;
					noun_infix_count += 1;
				}
			}
			
			if (PlayerRaces.Languages[language]->LanguageNouns[i]->HasSeparatePrefixTypeName(type)) {
				if (PlayerRaces.Languages[language]->LanguageNouns[i]->Uncountable) { // if is uncountable, use the nominative instead of the genitive
					if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixSingular) {
						noun_separate_prefixes[noun_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
						noun_separate_prefix_ids[noun_separate_prefix_count] = i;
						noun_separate_prefix_count += 1;
					}
					if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixPlural) {
						noun_separate_prefixes[noun_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
						noun_separate_prefix_count += 1;
					}
				} else {
					if (PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixSingular) {
						if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularGenitive.empty()) {
							noun_separate_prefixes[noun_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularGenitive;
							noun_separate_prefix_ids[noun_separate_prefix_count] = i;
							noun_separate_prefix_count += 1;
						} else if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty()) { //if no genitive is present, use the nominative instead
							noun_separate_prefixes[noun_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
							noun_separate_prefix_ids[noun_separate_prefix_count] = i;
							noun_separate_prefix_count += 1;
						}
					}
					if (PlayerRaces.Languages[language]->LanguageNouns[i]->PrefixPlural) {
						if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralGenitive.empty()) {
							noun_separate_prefixes[noun_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralGenitive;
							noun_separate_prefix_ids[noun_separate_prefix_count] = i;
							noun_separate_prefix_count += 1;
						} else if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty()) { //if no genitive is present, use the nominative instead
							noun_separate_prefixes[noun_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
							noun_separate_prefix_ids[noun_separate_prefix_count] = i;
							noun_separate_prefix_count += 1;
						}
					}
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNouns[i]->HasSeparateSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->SuffixSingular) {
					noun_separate_suffixes[noun_separate_suffix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
					noun_separate_suffix_ids[noun_separate_suffix_count] = i;
					noun_separate_suffix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->SuffixPlural) {
					noun_separate_suffixes[noun_separate_suffix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
					noun_separate_suffix_ids[noun_separate_suffix_count] = i;
					noun_separate_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNouns[i]->HasSeparateInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->InfixSingular) {
					noun_separate_infixes[noun_separate_infix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->SingularNominative;
					noun_separate_infix_ids[noun_separate_infix_count] = i;
					noun_separate_infix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative.empty() && PlayerRaces.Languages[language]->LanguageNouns[i]->InfixPlural) {
					noun_separate_infixes[noun_separate_infix_count] = PlayerRaces.Languages[language]->LanguageNouns[i]->PluralNominative;
					noun_separate_infix_ids[noun_separate_infix_count] = i;
					noun_separate_infix_count += 1;
				}
			}
		}
		
		for (size_t i = 0; i < PlayerRaces.Languages[language]->LanguageVerbs.size(); ++i) {
			if (PlayerRaces.Languages[language]->LanguageVerbs[i]->HasPrefixTypeName(type)) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent.empty()) {
					verb_prefixes[verb_prefix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent;
					verb_prefix_ids[verb_prefix_count] = i;
					verb_prefix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast.empty()) {
					verb_prefixes[verb_prefix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast;
					verb_prefix_ids[verb_prefix_count] = i;
					verb_prefix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageVerbs[i]->HasSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent.empty()) {
					verb_suffixes[verb_suffix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent;
					verb_suffix_ids[verb_suffix_count] = i;
					verb_suffix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast.empty()) {
					verb_suffixes[verb_suffix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast;
					verb_suffix_ids[verb_suffix_count] = i;
					verb_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageVerbs[i]->HasInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent.empty()) {
					verb_infixes[verb_infix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent;
					verb_infix_ids[verb_infix_count] = i;
					verb_infix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast.empty()) {
					verb_infixes[verb_infix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast;
					verb_infix_ids[verb_infix_count] = i;
					verb_infix_count += 1;
				}
			}
			
			if (PlayerRaces.Languages[language]->LanguageVerbs[i]->HasSeparatePrefixTypeName(type)) { // only using verb participles for now; maybe should add more possibilities?
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent.empty()) {
					verb_separate_prefixes[verb_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent;
					verb_separate_prefix_ids[verb_separate_prefix_count] = i;
					verb_separate_prefix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast.empty()) {
					verb_separate_prefixes[verb_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast;
					verb_separate_prefix_ids[verb_separate_prefix_count] = i;
					verb_separate_prefix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageVerbs[i]->HasSeparateSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent.empty()) {
					verb_separate_suffixes[verb_separate_suffix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent;
					verb_separate_suffix_ids[verb_separate_suffix_count] = i;
					verb_separate_suffix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast.empty()) {
					verb_separate_suffixes[verb_separate_suffix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast;
					verb_separate_suffix_ids[verb_separate_suffix_count] = i;
					verb_separate_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageVerbs[i]->HasSeparateInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent.empty()) {
					verb_separate_infixes[verb_separate_infix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePresent;
					verb_separate_infix_ids[verb_separate_infix_count] = i;
					verb_separate_infix_count += 1;
				}
				if (!PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast.empty()) {
					verb_separate_infixes[verb_separate_infix_count] = PlayerRaces.Languages[language]->LanguageVerbs[i]->ParticiplePast;
					verb_separate_infix_ids[verb_separate_infix_count] = i;
					verb_separate_infix_count += 1;
				}
			}
		}
		
		for (size_t i = 0; i < PlayerRaces.Languages[language]->LanguageAdjectives.size(); ++i) {
			if (PlayerRaces.Languages[language]->LanguageAdjectives[i]->HasTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word.empty()) {
					adjective_names[adjective_name_count] = PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word;
					adjective_name_ids[adjective_name_count] = i;
					adjective_name_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageAdjectives[i]->HasPrefixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word.empty()) {
					adjective_prefixes[adjective_prefix_count] = PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word;
					adjective_prefix_ids[adjective_prefix_count] = i;
					adjective_prefix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageAdjectives[i]->HasSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word.empty()) {
					adjective_suffixes[adjective_suffix_count] = PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word;
					adjective_suffix_ids[adjective_suffix_count] = i;
					adjective_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageAdjectives[i]->HasInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word.empty()) {
					adjective_infixes[adjective_infix_count] = PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word;
					adjective_infix_ids[adjective_infix_count] = i;
					adjective_infix_count += 1;
				}
			}
			
			if (PlayerRaces.Languages[language]->LanguageAdjectives[i]->HasSeparatePrefixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word.empty()) {
					adjective_separate_prefixes[adjective_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word;
					adjective_separate_prefix_ids[adjective_separate_prefix_count] = i;
					adjective_separate_prefix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageAdjectives[i]->HasSeparateSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word.empty()) {
					adjective_separate_suffixes[adjective_separate_suffix_count] = PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word;
					adjective_separate_suffix_ids[adjective_separate_suffix_count] = i;
					adjective_separate_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageAdjectives[i]->HasSeparateInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word.empty()) {
					adjective_separate_infixes[adjective_separate_infix_count] = PlayerRaces.Languages[language]->LanguageAdjectives[i]->Word;
					adjective_separate_infix_ids[adjective_separate_infix_count] = i;
					adjective_separate_infix_count += 1;
				}
			}
		}
		
		for (size_t i = 0; i < PlayerRaces.Languages[language]->LanguageNumerals.size(); ++i) {
			if (PlayerRaces.Languages[language]->LanguageNumerals[i]->HasPrefixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNumerals[i]->Word.empty()) {
					numeral_prefixes[numeral_prefix_count] = PlayerRaces.Languages[language]->LanguageNumerals[i]->Word;
					numeral_prefix_ids[numeral_prefix_count] = i;
					numeral_prefix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNumerals[i]->HasSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNumerals[i]->Word.empty()) {
					numeral_suffixes[numeral_suffix_count] = PlayerRaces.Languages[language]->LanguageNumerals[i]->Word;
					numeral_suffix_ids[numeral_suffix_count] = i;
					numeral_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNumerals[i]->HasInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNumerals[i]->Word.empty()) {
					numeral_infixes[numeral_infix_count] = PlayerRaces.Languages[language]->LanguageNumerals[i]->Word;
					numeral_infix_ids[numeral_infix_count] = i;
					numeral_infix_count += 1;
				}
			}
			
			if (PlayerRaces.Languages[language]->LanguageNumerals[i]->HasSeparatePrefixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNumerals[i]->Word.empty()) {
					numeral_separate_prefixes[numeral_separate_prefix_count] = PlayerRaces.Languages[language]->LanguageNumerals[i]->Word;
					numeral_separate_prefix_ids[numeral_separate_prefix_count] = i;
					numeral_separate_prefix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNumerals[i]->HasSeparateSuffixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNumerals[i]->Word.empty()) {
					numeral_separate_suffixes[numeral_separate_suffix_count] = PlayerRaces.Languages[language]->LanguageNumerals[i]->Word;
					numeral_separate_suffix_ids[numeral_separate_suffix_count] = i;
					numeral_separate_suffix_count += 1;
				}
			}
			if (PlayerRaces.Languages[language]->LanguageNumerals[i]->HasSeparateInfixTypeName(type)) {
				if (!PlayerRaces.Languages[language]->LanguageNumerals[i]->Word.empty()) {
					numeral_separate_infixes[numeral_separate_infix_count] = PlayerRaces.Languages[language]->LanguageNumerals[i]->Word;
					numeral_separate_infix_ids[numeral_separate_infix_count] = i;
					numeral_separate_infix_count += 1;
				}
			}
		}
		
		if (noun_name_count > 0 || adjective_name_count > 0 || noun_prefix_count > 0 || verb_prefix_count > 0 || adjective_prefix_count > 0 || numeral_prefix_count > 0 || noun_separate_prefix_count > 0 || verb_separate_prefix_count > 0 || adjective_separate_prefix_count > 0 || numeral_separate_prefix_count > 0) {
			int total_prefix_count = noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count;
			int total_suffix_count = noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count;
			int total_infix_count = noun_infix_count + verb_infix_count + adjective_infix_count + numeral_infix_count;
			int total_separate_prefix_count = noun_separate_prefix_count + verb_separate_prefix_count + adjective_separate_prefix_count + numeral_separate_prefix_count;
			int total_separate_suffix_count = noun_separate_suffix_count + verb_separate_suffix_count + adjective_separate_suffix_count + numeral_separate_suffix_count;
			int total_separate_infix_count = noun_separate_infix_count + verb_separate_infix_count + adjective_separate_infix_count + numeral_separate_infix_count;
			int random_number = SyncRand(noun_name_count + adjective_name_count + (total_prefix_count * total_suffix_count) + ((total_prefix_count + total_suffix_count) / 2) * total_infix_count + (total_separate_prefix_count * total_separate_suffix_count) + ((total_separate_prefix_count + total_separate_suffix_count) / 2) * total_separate_infix_count);
			if (random_number < noun_name_count + adjective_name_count) { //entire name
				random_number = SyncRand(noun_name_count + adjective_name_count);
				if (random_number < noun_name_count) {
					name = noun_names[SyncRand(noun_name_count)];
				} else {
					name = adjective_names[SyncRand(adjective_name_count)];
				}
			} else if (random_number < (noun_name_count + (total_prefix_count * total_suffix_count))) { //prefix + suffix
				std::string prefix;
				std::string suffix;
				int prefix_id;
				int suffix_id;
				std::string prefix_word_type;
				std::string suffix_word_type;
				
				//choose the word type of the prefix, and the prefix itself
				random_number = SyncRand(noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count);
				if (random_number < noun_prefix_count) {
					prefix_word_type = "noun";
					prefix_id = SyncRand(noun_prefix_count);
					prefix = noun_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count)) {
					prefix_word_type = "verb";
					prefix_id = SyncRand(verb_prefix_count);
					prefix = verb_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count)) {
					prefix_word_type = "adjective";
					prefix_id = SyncRand(adjective_prefix_count);
					prefix = adjective_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count)) {
					prefix_word_type = "numeral";
					prefix_id = SyncRand(numeral_prefix_count);
					prefix = numeral_prefixes[prefix_id];
				}

				//choose the word type of the suffix, and the suffix itself
				random_number = SyncRand(noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count);
				if (random_number < noun_suffix_count) {
					suffix_word_type = "noun";
					suffix_id = SyncRand(noun_suffix_count);
					suffix = noun_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count)) {
					suffix_word_type = "verb";
					suffix_id = SyncRand(verb_suffix_count);
					suffix = verb_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count)) {
					suffix_word_type = "adjective";
					suffix_id = SyncRand(adjective_suffix_count);
					suffix = adjective_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count)) {
					suffix_word_type = "numeral";
					suffix_id = SyncRand(numeral_suffix_count);
					suffix = numeral_suffixes[suffix_id];
				}

				if (prefix_word_type == "numeral" && PlayerRaces.Languages[language]->LanguageNumerals[numeral_prefix_ids[prefix_id]]->Number > 1 && suffix_word_type == "noun") { // if requires plural (by being a numeral greater than one) and suffix is a noun
					//then replace the suffix with its plural form
					suffix = PlayerRaces.Languages[language]->LanguageNouns[noun_suffix_ids[suffix_id]]->PluralNominative;
				}
					
				suffix = DecapitalizeString(suffix);
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "ß" && suffix.substr(0, 1) == "s") { //prevent triple "s"s in names
					suffix = FindAndReplaceStringBeginning(suffix, "s", "");
				}
				name = prefix;
				name += suffix;
			} else if (random_number < (noun_name_count + (total_prefix_count * total_suffix_count) + ((total_prefix_count + total_suffix_count) / 2) * total_infix_count)) { //prefix + infix + suffix
				std::string prefix;
				std::string infix;
				std::string suffix;
				int prefix_id;
				int infix_id;
				int suffix_id;
				std::string prefix_word_type;
				std::string infix_word_type;
				std::string suffix_word_type;
				
				//choose the word type of the prefix, and the prefix itself
				random_number = SyncRand(noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count);
				if (random_number < noun_prefix_count) {
					prefix_word_type = "noun";
					prefix_id = SyncRand(noun_prefix_count);
					prefix = noun_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count)) {
					prefix_word_type = "verb";
					prefix_id = SyncRand(verb_prefix_count);
					prefix = verb_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count)) {
					prefix_word_type = "adjective";
					prefix_id = SyncRand(adjective_prefix_count);
					prefix = adjective_prefixes[prefix_id];
				} else if (random_number < (noun_prefix_count + verb_prefix_count + adjective_prefix_count + numeral_prefix_count)) {
					prefix_word_type = "numeral";
					prefix_id = SyncRand(numeral_prefix_count);
					prefix = numeral_prefixes[prefix_id];
				}

				//choose the word type of the infix, and the infix itself
				random_number = SyncRand(noun_infix_count + verb_infix_count + adjective_infix_count + numeral_infix_count);
				if (random_number < noun_infix_count) {
					infix_word_type = "noun";
					infix_id = SyncRand(noun_infix_count);
					infix = noun_infixes[infix_id];
				} else if (random_number < (noun_infix_count + verb_infix_count)) {
					infix_word_type = "verb";
					infix_id = SyncRand(verb_infix_count);
					infix = verb_infixes[infix_id];
				} else if (random_number < (noun_infix_count + verb_infix_count + adjective_infix_count)) {
					infix_word_type = "adjective";
					infix_id = SyncRand(adjective_infix_count);
					infix = adjective_infixes[infix_id];
				} else if (random_number < (noun_infix_count + verb_infix_count + adjective_infix_count + numeral_infix_count)) {
					infix_word_type = "numeral";
					infix_id = SyncRand(numeral_infix_count);
					infix = numeral_infixes[infix_id];
				}

				//choose the word type of the suffix, and the suffix itself
				random_number = SyncRand(noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count);
				if (random_number < noun_suffix_count) {
					suffix_word_type = "noun";
					suffix_id = SyncRand(noun_suffix_count);
					suffix = noun_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count)) {
					suffix_word_type = "verb";
					suffix_id = SyncRand(verb_suffix_count);
					suffix = verb_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count)) {
					suffix_word_type = "adjective";
					suffix_id = SyncRand(adjective_suffix_count);
					suffix = adjective_suffixes[suffix_id];
				} else if (random_number < (noun_suffix_count + verb_suffix_count + adjective_suffix_count + numeral_suffix_count)) {
					suffix_word_type = "numeral";
					suffix_id = SyncRand(numeral_suffix_count);
					suffix = numeral_suffixes[suffix_id];
				}

				if (prefix_word_type == "numeral" && PlayerRaces.Languages[language]->LanguageNumerals[numeral_prefix_ids[prefix_id]]->Number > 1 && suffix_word_type == "noun") { // if requires plural (by being a numeral greater than one) and suffix is a noun
					//then replace the suffix with its plural form
					suffix = PlayerRaces.Languages[language]->LanguageNouns[noun_suffix_ids[suffix_id]]->PluralNominative;
				}
					
				infix = DecapitalizeString(infix);
				suffix = DecapitalizeString(suffix);
				if (prefix.substr(prefix.size() - 1, 1) == "d" && infix.substr(0, 1) == "d") { //if the prefix ends with "d" and the infix begins with "d", eliminate one instance of "d"
					prefix = FindAndReplaceStringEnding(prefix, "d", "");
				}
				if (prefix.substr(prefix.size() - 2, 2) == "gs" && infix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the infix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
					prefix = FindAndReplaceStringEnding(prefix, "gs", "g");
				}
				if (prefix.substr(prefix.size() - 1, 1) == "s" && infix.substr(0, 1) == "s") { //if the prefix ends in "s" and the infix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					prefix = FindAndReplaceStringEnding(prefix, "s", "");
				}
				if (infix.substr(infix.size() - 2, 2) == "gs" && suffix.substr(0, 1) == "g") { //if the last two characters of the infix are "gs", and the first character of the suffix is "g", then remove the final "s" from the infix (as in "Königgrätz")
					infix = FindAndReplaceStringEnding(infix, "gs", "g");
				}
				if (infix.substr(infix.size() - 1, 1) == "s" && suffix.substr(0, 1) == "s") { //if the infix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the infix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
					infix = FindAndReplaceStringEnding(infix, "s", "");
				}
				if (infix.substr(infix.size() - 2, 2) == "th" && suffix.substr(0, 2) == "th") { //if the last two characters of the infix are "th", and the suffix begins with "th", then eliminate the infix's "th", to make this be just one instance of "th"
					infix = FindAndReplaceStringEnding(infix, "th", "");
				}
				if (infix.substr(infix.size() - 1, 1) == "d" && suffix.substr(0, 1) == "d") { //if the infix ends with "d" and the suffix begins with "d", eliminate one instance of "d"
					infix = FindAndReplaceStringEnding(infix, "d", "");
				}
				name = prefix;
				name += infix;
				name += suffix;
			} else if (random_number < (noun_name_count + (total_prefix_count * total_suffix_count) + ((total_prefix_count + total_suffix_count) / 2) * total_infix_count + (total_separate_prefix_count * total_separate_suffix_count))) { //separate prefix + separate suffix
				std::string prefix;
				std::string suffix;
				int prefix_id;
				int suffix_id;
				std::string prefix_word_type;
				std::string suffix_word_type;
				
				//choose the word type of the prefix, and the prefix itself
				random_number = SyncRand(noun_separate_prefix_count + verb_separate_prefix_count + adjective_separate_prefix_count + numeral_separate_prefix_count);
				if (random_number < noun_separate_prefix_count) {
					prefix_word_type = "noun";
					prefix_id = SyncRand(noun_separate_prefix_count);
					prefix = noun_separate_prefixes[prefix_id];
				} else if (random_number < (noun_separate_prefix_count + verb_separate_prefix_count)) {
					prefix_word_type = "verb";
					prefix_id = SyncRand(verb_separate_prefix_count);
					prefix = verb_separate_prefixes[prefix_id];
				} else if (random_number < (noun_separate_prefix_count + verb_separate_prefix_count + adjective_separate_prefix_count)) {
					prefix_word_type = "adjective";
					prefix_id = SyncRand(adjective_separate_prefix_count);
					prefix = adjective_separate_prefixes[prefix_id];
				} else if (random_number < (noun_separate_prefix_count + verb_separate_prefix_count + adjective_separate_prefix_count + numeral_separate_prefix_count)) {
					prefix_word_type = "numeral";
					prefix_id = SyncRand(numeral_separate_prefix_count);
					prefix = numeral_separate_prefixes[prefix_id];
				}

				//choose the word type of the suffix, and the suffix itself
				random_number = SyncRand(noun_separate_suffix_count + verb_separate_suffix_count + adjective_separate_suffix_count + numeral_separate_suffix_count);
				if (random_number < noun_separate_suffix_count) {
					suffix_word_type = "noun";
					suffix_id = SyncRand(noun_separate_suffix_count);
					suffix = noun_separate_suffixes[suffix_id];
				} else if (random_number < (noun_separate_suffix_count + verb_separate_suffix_count)) {
					suffix_word_type = "verb";
					suffix_id = SyncRand(verb_separate_suffix_count);
					suffix = verb_separate_suffixes[suffix_id];
				} else if (random_number < (noun_separate_suffix_count + verb_separate_suffix_count + adjective_separate_suffix_count)) {
					suffix_word_type = "adjective";
					suffix_id = SyncRand(adjective_separate_suffix_count);
					suffix = adjective_separate_suffixes[suffix_id];
				} else if (random_number < (noun_separate_suffix_count + verb_separate_suffix_count + adjective_separate_suffix_count + numeral_separate_suffix_count)) {
					suffix_word_type = "numeral";
					suffix_id = SyncRand(numeral_separate_suffix_count);
					suffix = numeral_separate_suffixes[suffix_id];
				}

				if (prefix_word_type == "numeral" && PlayerRaces.Languages[language]->LanguageNumerals[numeral_separate_prefix_ids[prefix_id]]->Number > 1 && suffix_word_type == "noun") { // if requires plural (by being a numeral greater than one) and suffix is a noun
					//then replace the suffix with its plural form
					suffix = PlayerRaces.Languages[language]->LanguageNouns[noun_separate_suffix_ids[suffix_id]]->PluralNominative;
				}
				
				if (suffix_word_type == "noun" && type != "province" && type != "settlement") { //if type is neither a province nor a settlement, add an article at the beginning
					name += PlayerRaces.Languages[language]->GetArticle(PlayerRaces.Languages[language]->LanguageNouns[noun_separate_suffix_ids[suffix_id]]->Gender, "nominative", true);
					name += " ";
				}
					
				name += prefix;
				name += " ";
				name += suffix;
			} else if (random_number < (noun_name_count + (total_prefix_count * total_suffix_count) + ((total_prefix_count + total_suffix_count) / 2) * total_infix_count + (total_separate_prefix_count * total_separate_suffix_count) + ((total_separate_prefix_count + total_separate_suffix_count) / 2) * total_separate_infix_count)) { //separate prefix + separate infix + separate suffix
				std::string prefix;
				std::string infix;
				std::string suffix;
				int prefix_id;
				int infix_id;
				int suffix_id;
				std::string prefix_word_type;
				std::string infix_word_type;
				std::string suffix_word_type;
				
				//choose the word type of the prefix, and the prefix itself
				random_number = SyncRand(noun_separate_prefix_count + verb_separate_prefix_count + adjective_separate_prefix_count + numeral_separate_prefix_count);
				if (random_number < noun_separate_prefix_count) {
					prefix_word_type = "noun";
					prefix_id = SyncRand(noun_separate_prefix_count);
					prefix = noun_separate_prefixes[prefix_id];
				} else if (random_number < (noun_separate_prefix_count + verb_separate_prefix_count)) {
					prefix_word_type = "verb";
					prefix_id = SyncRand(verb_separate_prefix_count);
					prefix = verb_separate_prefixes[prefix_id];
				} else if (random_number < (noun_separate_prefix_count + verb_separate_prefix_count + adjective_separate_prefix_count)) {
					prefix_word_type = "adjective";
					prefix_id = SyncRand(adjective_separate_prefix_count);
					prefix = adjective_separate_prefixes[prefix_id];
				} else if (random_number < (noun_separate_prefix_count + verb_separate_prefix_count + adjective_separate_prefix_count + numeral_separate_prefix_count)) {
					prefix_word_type = "numeral";
					prefix_id = SyncRand(numeral_separate_prefix_count);
					prefix = numeral_separate_prefixes[prefix_id];
				}

				//choose the word type of the infix, and the infix itself
				random_number = SyncRand(noun_separate_infix_count + verb_separate_infix_count + adjective_separate_infix_count + numeral_separate_infix_count);
				if (random_number < noun_separate_infix_count) {
					infix_word_type = "noun";
					infix_id = SyncRand(noun_separate_infix_count);
					infix = noun_separate_infixes[infix_id];
				} else if (random_number < (noun_separate_infix_count + verb_separate_infix_count)) {
					infix_word_type = "verb";
					infix_id = SyncRand(verb_separate_infix_count);
					infix = verb_separate_infixes[infix_id];
				} else if (random_number < (noun_separate_infix_count + verb_separate_infix_count + adjective_separate_infix_count)) {
					infix_word_type = "adjective";
					infix_id = SyncRand(adjective_separate_infix_count);
					infix = adjective_separate_infixes[infix_id];
				} else if (random_number < (noun_separate_infix_count + verb_separate_infix_count + adjective_separate_infix_count + numeral_separate_infix_count)) {
					infix_word_type = "numeral";
					infix_id = SyncRand(numeral_separate_infix_count);
					infix = numeral_separate_infixes[infix_id];
				}

				//choose the word type of the suffix, and the suffix itself
				random_number = SyncRand(noun_separate_suffix_count + verb_separate_suffix_count + adjective_separate_suffix_count + numeral_separate_suffix_count);
				if (random_number < noun_separate_suffix_count) {
					suffix_word_type = "noun";
					suffix_id = SyncRand(noun_separate_suffix_count);
					suffix = noun_separate_suffixes[suffix_id];
				} else if (random_number < (noun_separate_suffix_count + verb_separate_suffix_count)) {
					suffix_word_type = "verb";
					suffix_id = SyncRand(verb_separate_suffix_count);
					suffix = verb_separate_suffixes[suffix_id];
				} else if (random_number < (noun_separate_suffix_count + verb_separate_suffix_count + adjective_separate_suffix_count)) {
					suffix_word_type = "adjective";
					suffix_id = SyncRand(adjective_separate_suffix_count);
					suffix = adjective_separate_suffixes[suffix_id];
				} else if (random_number < (noun_separate_suffix_count + verb_separate_suffix_count + adjective_separate_suffix_count + numeral_separate_suffix_count)) {
					suffix_word_type = "numeral";
					suffix_id = SyncRand(numeral_separate_suffix_count);
					suffix = numeral_separate_suffixes[suffix_id];
				}

				if (prefix_word_type == "numeral" && PlayerRaces.Languages[language]->LanguageNumerals[numeral_separate_prefix_ids[prefix_id]]->Number > 1 && suffix_word_type == "noun") { // if requires plural (by being a numeral greater than one) and suffix is a noun
					//then replace the suffix with its plural form
					suffix = PlayerRaces.Languages[language]->LanguageNouns[noun_separate_suffix_ids[suffix_id]]->PluralNominative;
				}
					
				if (suffix_word_type == "noun" && type != "province" && type != "settlement") { //if type is neither a province nor a settlement, add an article at the beginning
					name += PlayerRaces.Languages[language]->GetArticle(PlayerRaces.Languages[language]->LanguageNouns[noun_separate_suffix_ids[suffix_id]]->Gender, "nominative", true);
					name += " ";
				}
					
				name += prefix;
				name += " ";
				name += infix;
				name += " ";
				name += suffix;
			}
		}
	}
	
	name = TransliterateText(name);
	
	return name;
}
//Wyrmgus end
