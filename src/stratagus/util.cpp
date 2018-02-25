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

//Wyrmgus start
#include "network.h"
//Wyrmgus end

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
//Wyrmgus start
#include <time.h>
//Wyrmgus end

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

unsigned SyncRandSeed;				/// sync random seed value.

/**
**  Inititalize sync rand seed.
*/
void InitSyncRand()
{
	if (!IsNetworkGame()) { //if isn't a network game, make the seed vary according to the date and time
		time_t time_curr;
		time(&time_curr);
		SyncRandSeed = static_cast<unsigned>(time_curr);
	} else {
		SyncRandSeed = 0x87654321;
	}
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
#include "character.h" //for personal name generation
#include "iocompat.h" //for getting a file's last modified date
#include "iolib.h" //for getting a file's last modified date
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

int GetFileLastModified(std::string file_name)
{
	file_name = LibraryFileName(file_name.c_str());
	
	struct stat tmp;
	
	stat(file_name.c_str(), &tmp);
	
	int date = tmp.st_mtime;	
	
	return date;
}

std::string TransliterateText(std::string text) //convert special characters into ones more legible for English-speakers
{
	text = FindAndReplaceString(text, "Ā́", "A");
	text = FindAndReplaceString(text, "ā́", "a");
	text = FindAndReplaceString(text, "Ā", "A");
	text = FindAndReplaceString(text, "ā", "a");
	text = FindAndReplaceString(text, "Ấ", "A");
	text = FindAndReplaceString(text, "ấ", "a");
	text = FindAndReplaceString(text, "Ȧ́", "A");
	text = FindAndReplaceString(text, "ȧ́", "a");
	text = FindAndReplaceString(text, "Á", "A");
	text = FindAndReplaceString(text, "á", "a");
	text = FindAndReplaceString(text, "À", "A");
	text = FindAndReplaceString(text, "à", "a");
	text = FindAndReplaceString(text, "Ã", "A");
	text = FindAndReplaceString(text, "ã", "a");
	text = FindAndReplaceString(text, "Ä", "A");
	text = FindAndReplaceString(text, "ä", "a");
	text = FindAndReplaceString(text, "Ā", "A");
	text = FindAndReplaceString(text, "ā", "a");
	text = FindAndReplaceString(text, "Â", "A");
	text = FindAndReplaceString(text, "â", "a");
	text = FindAndReplaceString(text, "Ą", "A");
	text = FindAndReplaceString(text, "ą", "a");
	text = FindAndReplaceString(text, "ᶏ", "a");
	text = FindAndReplaceString(text, "Å", "A");
	text = FindAndReplaceString(text, "å", "a");
	text = FindAndReplaceString(text, "Ă", "A");
	text = FindAndReplaceString(text, "ă", "a");
	text = FindAndReplaceString(text, "Æ̂", "Ae");
	text = FindAndReplaceString(text, "æ̂", "ae");
	text = FindAndReplaceString(text, "Æ", "Ae");
	text = FindAndReplaceString(text, "æ", "ae");
	text = FindAndReplaceString(text, "Ǣ", "Ae");
	text = FindAndReplaceString(text, "ǣ", "ae");
	text = FindAndReplaceString(text, "Ǽ", "Ae");
	text = FindAndReplaceString(text, "ǽ", "ae");
	text = FindAndReplaceString(text, "Ƀ", "B");
	text = FindAndReplaceString(text, "ƀ", "b");
	text = FindAndReplaceString(text, "Č", "C");
	text = FindAndReplaceString(text, "č", "c");
	text = FindAndReplaceString(text, "Ð", "D");
	text = FindAndReplaceString(text, "ð", "d");
	text = FindAndReplaceString(text, "Ḍ", "D");
	text = FindAndReplaceString(text, "ḍ", "d");
	text = FindAndReplaceString(text, "Đ", "D");
	text = FindAndReplaceString(text, "đ", "d");
	text = FindAndReplaceString(text, "ẟ", "d"); //not the same character as "δ"
	text = FindAndReplaceString(text, "Ę̄", "E");
	text = FindAndReplaceString(text, "ę̄", "e");
	text = FindAndReplaceString(text, "Ḗ", "E");
	text = FindAndReplaceString(text, "ḗ", "e");
	text = FindAndReplaceString(text, "Ė́", "E");
	text = FindAndReplaceString(text, "ė́", "e");
	text = FindAndReplaceString(text, "Ë̃", "E");
	text = FindAndReplaceString(text, "ë̃", "e");
	text = FindAndReplaceString(text, "Ë̂", "E");
	text = FindAndReplaceString(text, "ë̂", "e");
	text = FindAndReplaceString(text, "É", "E"); 
	text = FindAndReplaceString(text, "é", "e");
	text = FindAndReplaceString(text, "È", "E");
	text = FindAndReplaceString(text, "è", "e");
	text = FindAndReplaceString(text, "Ē", "E");
	text = FindAndReplaceString(text, "ē", "e");
	text = FindAndReplaceString(text, "Ê", "E");
	text = FindAndReplaceString(text, "ê", "e");
	text = FindAndReplaceString(text, "Ě", "E");
	text = FindAndReplaceString(text, "ě", "e");
	text = FindAndReplaceString(text, "Ė", "E");
	text = FindAndReplaceString(text, "ė", "e");
	text = FindAndReplaceString(text, "Ë", "E");
	text = FindAndReplaceString(text, "ë", "e");
	text = FindAndReplaceString(text, "Ę", "E");
	text = FindAndReplaceString(text, "ę", "e");
	text = FindAndReplaceString(text, "Ĕ", "E");
	text = FindAndReplaceString(text, "ĕ", "e");
	text = FindAndReplaceString(text, "Ə", "E");
	text = FindAndReplaceString(text, "ə", "e");
	text = FindAndReplaceString(text, "Ǝ", "E");
	text = FindAndReplaceString(text, "ǝ", "e");
	text = FindAndReplaceString(text, "ϵ", "e");
	text = FindAndReplaceString(text, "Ĝ", "G");
	text = FindAndReplaceString(text, "ĝ", "g");
	text = FindAndReplaceString(text, "Ī̆", "I");
	text = FindAndReplaceString(text, "ī̆", "i");
	text = FindAndReplaceString(text, "Î́", "I");
	text = FindAndReplaceString(text, "î́", "i");
	text = FindAndReplaceString(text, "Ī́", "I");
	text = FindAndReplaceString(text, "ī́", "i");
	text = FindAndReplaceString(text, "Ī", "I");
	text = FindAndReplaceString(text, "ī", "i");
	text = FindAndReplaceString(text, "I̊", "I");
	text = FindAndReplaceString(text, "i̊", "i");
	text = FindAndReplaceString(text, "Í", "I");
	text = FindAndReplaceString(text, "í", "i");
	text = FindAndReplaceString(text, "Ì", "I");
	text = FindAndReplaceString(text, "ì", "i");
	text = FindAndReplaceString(text, "Ī", "I");
	text = FindAndReplaceString(text, "ī", "i");
	text = FindAndReplaceString(text, "Î", "I");
	text = FindAndReplaceString(text, "î", "i");
	text = FindAndReplaceString(text, "Ĭ", "I");
	text = FindAndReplaceString(text, "ĭ", "i");
	text = FindAndReplaceString(text, "Ĩ", "I");
	text = FindAndReplaceString(text, "ĩ", "i");
	text = FindAndReplaceString(text, "Ḱ", "K");
	text = FindAndReplaceString(text, "ḱ", "k");
	text = FindAndReplaceString(text, "L̥", "L");
	text = FindAndReplaceString(text, "l̥", "l");
	text = FindAndReplaceString(text, "Ɫ", "L");
	text = FindAndReplaceString(text, "ɫ", "l");
	text = FindAndReplaceString(text, "Ň", "N");
	text = FindAndReplaceString(text, "ň", "n");
	text = FindAndReplaceString(text, "Ṇ", "N");
	text = FindAndReplaceString(text, "ṇ", "n");
	text = FindAndReplaceString(text, "Ṅ", "N");
	text = FindAndReplaceString(text, "ṅ", "n");
	text = FindAndReplaceString(text, "Ṓ", "O");
	text = FindAndReplaceString(text, "ṓ", "o");
	text = FindAndReplaceString(text, "Ŏ", "O");
	text = FindAndReplaceString(text, "ŏ", "o");
	text = FindAndReplaceString(text, "Ø", "Ö"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "ø", "ö"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ǫ", "O"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "ǫ", "o"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ö", "O");
	text = FindAndReplaceString(text, "ö", "o");
	text = FindAndReplaceString(text, "Ó", "O");
	text = FindAndReplaceString(text, "ó", "o");
	text = FindAndReplaceString(text, "Ò", "O");
	text = FindAndReplaceString(text, "ò", "o");
	text = FindAndReplaceString(text, "Ō", "O");
	text = FindAndReplaceString(text, "ō", "o");
	text = FindAndReplaceString(text, "Ô", "O");
	text = FindAndReplaceString(text, "ô", "o");
	text = FindAndReplaceString(text, "Ǒ", "O");
	text = FindAndReplaceString(text, "ǒ", "o");
	text = FindAndReplaceString(text, "Œ", "Oe");
	text = FindAndReplaceString(text, "œ", "oe");
	text = FindAndReplaceString(text, "Ṛ́", "R");
	text = FindAndReplaceString(text, "ṛ́", "r");
	text = FindAndReplaceString(text, "Ŗ́", "R");
	text = FindAndReplaceString(text, "ŗ́", "r");
	text = FindAndReplaceString(text, "R̄", "R");
	text = FindAndReplaceString(text, "r̄", "r");
	text = FindAndReplaceString(text, "Ř", "R");
	text = FindAndReplaceString(text, "ř", "r");
	text = FindAndReplaceString(text, "Ṛ", "R");
	text = FindAndReplaceString(text, "ṛ", "r");
	text = FindAndReplaceString(text, "Ŕ", "R");
	text = FindAndReplaceString(text, "ŕ", "r");
	text = FindAndReplaceString(text, "Ṙ", "R");
	text = FindAndReplaceString(text, "ṙ", "r");
	text = FindAndReplaceString(text, "Š", "S");
	text = FindAndReplaceString(text, "š", "s");
	text = FindAndReplaceString(text, "Ș", "S");
	text = FindAndReplaceString(text, "ș", "s");
	text = FindAndReplaceString(text, "Ś", "S");
	text = FindAndReplaceString(text, "ś", "s");
	text = FindAndReplaceString(text, "ß", "ss"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ṭ", "T");
	text = FindAndReplaceString(text, "ṭ", "t");
	text = FindAndReplaceString(text, "Ț", "T");
	text = FindAndReplaceString(text, "ț", "t");
	text = FindAndReplaceString(text, "ÞÞ", "Þ"); //replace double thorns with a single one
	text = FindAndReplaceString(text, "þþ", "þ"); //replace double thorns with a single one
	text = FindAndReplaceString(text, "Þ", "Th"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "þ", "th"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceString(text, "Ū́", "U");
	text = FindAndReplaceString(text, "ū́", "u");
	text = FindAndReplaceString(text, "Ů̃", "U");
	text = FindAndReplaceString(text, "ů̃", "u");
	text = FindAndReplaceString(text, "Ü", "U");
	text = FindAndReplaceString(text, "ü", "u");
	text = FindAndReplaceString(text, "Ú", "U");
	text = FindAndReplaceString(text, "ú", "u");
	text = FindAndReplaceString(text, "Ù", "U");
	text = FindAndReplaceString(text, "ù", "u");
	text = FindAndReplaceString(text, "Ū", "U");
	text = FindAndReplaceString(text, "ū", "u");
	text = FindAndReplaceString(text, "Û", "U");
	text = FindAndReplaceString(text, "û", "u");
	text = FindAndReplaceString(text, "Ŭ", "U");
	text = FindAndReplaceString(text, "ŭ", "u");
	text = FindAndReplaceString(text, "Ů", "U");
	text = FindAndReplaceString(text, "ů", "u");
	text = FindAndReplaceString(text, "Ũ", "U");
	text = FindAndReplaceString(text, "ũ", "u");
	text = FindAndReplaceString(text, "Ṷ", "U");
	text = FindAndReplaceString(text, "ṷ", "u");
	text = FindAndReplaceString(text, "ʷ", "w");
	text = FindAndReplaceString(text, "Ȳ", "Y");
	text = FindAndReplaceString(text, "ȳ", "y");
	text = FindAndReplaceString(text, "Ŷ", "Y");
	text = FindAndReplaceString(text, "ŷ", "y");
	text = FindAndReplaceString(text, "Ỹ", "Y");
	text = FindAndReplaceString(text, "ỹ", "y");
	text = FindAndReplaceString(text, "Ý", "Y");
	text = FindAndReplaceString(text, "ý", "y");
	text = FindAndReplaceString(text, "Ž", "Z");
	text = FindAndReplaceString(text, "ž", "z");
	text = FindAndReplaceString(text, "Z̨", "Z");
	text = FindAndReplaceString(text, "z̨", "z");
	text = FindAndReplaceString(text, "Ż", "Z");
	text = FindAndReplaceString(text, "ż", "z");
	
	text = FindAndReplaceString(text, "ʔ", "'"); // glottal stop

	//replace endings in -r after consonants (which happens in the nominative for Old Norse); Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	text = FindAndReplaceStringEnding(text, "dr", "d");
	text = FindAndReplaceString(text, "dr ", "d ");
	text = FindAndReplaceStringEnding(text, "fr", "f");
	text = FindAndReplaceString(text, "fr ", "f ");
	text = FindAndReplaceStringEnding(text, "gr", "g");
	text = FindAndReplaceString(text, "gr ", "g ");
	text = FindAndReplaceStringEnding(text, "kr", "k");
	text = FindAndReplaceString(text, "kr ", "k ");
	text = FindAndReplaceStringEnding(text, "mr", "m");
	text = FindAndReplaceString(text, "mr ", "m ");
	text = FindAndReplaceStringEnding(text, "nr", "n");
	text = FindAndReplaceString(text, "nr ", "n ");
	text = FindAndReplaceStringEnding(text, "pr", "p");
	text = FindAndReplaceString(text, "pr ", "p ");
	text = FindAndReplaceStringEnding(text, "rr", "r");
	text = FindAndReplaceString(text, "rr ", "r ");
	text = FindAndReplaceStringEnding(text, "tr", "t");
	text = FindAndReplaceString(text, "tr ", "t ");
	
	//Greek characters
	text = FindAndReplaceString(text, "Ἄ", "A");
	text = FindAndReplaceString(text, "ἄ", "a");
	text = FindAndReplaceString(text, "Ά", "A");
	text = FindAndReplaceString(text, "ά", "a");
	text = FindAndReplaceString(text, "Ἀ", "A");
	text = FindAndReplaceString(text, "ἀ", "a");
	text = FindAndReplaceString(text, "Α", "A");
	text = FindAndReplaceString(text, "α", "a");
	text = FindAndReplaceString(text, "Β", "B");
	text = FindAndReplaceString(text, "β", "b");
	text = FindAndReplaceString(text, "Χ", "Ch");
	text = FindAndReplaceString(text, "χ", "ch");
	text = FindAndReplaceString(text, "Δ", "D");
	text = FindAndReplaceString(text, "δ", "d");
	text = FindAndReplaceString(text, "Ἑ", "E");
	text = FindAndReplaceString(text, "ἑ", "e");
	text = FindAndReplaceString(text, "Ἔ", "E");
	text = FindAndReplaceString(text, "ἔ", "e");
	text = FindAndReplaceString(text, "Έ", "E");
	text = FindAndReplaceString(text, "έ", "e");
	text = FindAndReplaceString(text, "Ε", "E");
	text = FindAndReplaceString(text, "ε", "e");
	text = FindAndReplaceString(text, "Η", "E");
	text = FindAndReplaceString(text, "η", "e");
	text = FindAndReplaceString(text, "Γ", "G");
	text = FindAndReplaceString(text, "γ", "g");
	text = FindAndReplaceString(text, "Ῑ́", "I");
	text = FindAndReplaceString(text, "ῑ́", "i");
	text = FindAndReplaceString(text, "Ί", "I");
	text = FindAndReplaceString(text, "ί", "i");
	text = FindAndReplaceString(text, "ῖ", "i");
	text = FindAndReplaceString(text, "Ι", "I");
	text = FindAndReplaceString(text, "ι", "i");
	text = FindAndReplaceString(text, "Ή", "I");
	text = FindAndReplaceString(text, "ή", "i");
	text = FindAndReplaceString(text, "Κ", "K");
	text = FindAndReplaceString(text, "κ", "k");
	text = FindAndReplaceString(text, "Λ", "L");
	text = FindAndReplaceString(text, "λ", "l");
	text = FindAndReplaceString(text, "Μ", "M");
	text = FindAndReplaceString(text, "μ", "m");
	text = FindAndReplaceString(text, "Ν", "N");
	text = FindAndReplaceString(text, "ν", "n");
	text = FindAndReplaceString(text, "Ὄ", "O");
	text = FindAndReplaceString(text, "ὄ", "o");
	text = FindAndReplaceString(text, "Ὅ", "O");
	text = FindAndReplaceString(text, "ὅ", "o");
	text = FindAndReplaceString(text, "Ό", "O");
	text = FindAndReplaceString(text, "ό", "o");
	text = FindAndReplaceString(text, "Ὀ", "O");
	text = FindAndReplaceString(text, "ὀ", "o");
	text = FindAndReplaceString(text, "Ὁ", "O");
	text = FindAndReplaceString(text, "ὁ", "o");
	text = FindAndReplaceString(text, "Ο", "O");
	text = FindAndReplaceString(text, "ο", "o");
	text = FindAndReplaceString(text, "Ώ", "O");
	text = FindAndReplaceString(text, "ώ", "o");
	text = FindAndReplaceString(text, "Ω", "O");
	text = FindAndReplaceString(text, "ω", "o");
	text = FindAndReplaceString(text, "Π", "P");
	text = FindAndReplaceString(text, "π", "p");
	text = FindAndReplaceString(text, "Φ", "Ph");
	text = FindAndReplaceString(text, "φ", "ph");
	text = FindAndReplaceString(text, "Ψ", "Ps");
	text = FindAndReplaceString(text, "ψ", "ps");
	text = FindAndReplaceString(text, "Ρ", "R");
	text = FindAndReplaceString(text, "ρ", "r");
	text = FindAndReplaceString(text, "Σ", "S");
	text = FindAndReplaceString(text, "σ", "s");
	text = FindAndReplaceString(text, "ς", "s");
	text = FindAndReplaceString(text, "Τ", "T");
	text = FindAndReplaceString(text, "τ", "t");
	text = FindAndReplaceString(text, "Θ", "Th");
	text = FindAndReplaceString(text, "θ", "th");
	text = FindAndReplaceString(text, "Ξ", "X");
	text = FindAndReplaceString(text, "ξ", "x");
	text = FindAndReplaceString(text, "Ύ", "Y");
	text = FindAndReplaceString(text, "ύ", "y");
	text = FindAndReplaceString(text, "Ὑ", "Y");
	text = FindAndReplaceString(text, "ὑ", "y");
	text = FindAndReplaceString(text, "Υ", "Y");
	text = FindAndReplaceString(text, "υ", "y");
	text = FindAndReplaceString(text, "Ζ", "Z");
	text = FindAndReplaceString(text, "ζ", "z");
	
	//remove large clusters of the same letters
	text = FindAndReplaceString(text, "nnn", "nn");
	
	return text;
}

std::string CapitalizeString(std::string text)
{
	if (text.empty()) {
		return text;
	}
	
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
	if (text.empty()) {
		return text;
	}

	text[0] = tolower(text[0]);
	
	// replace special characters which may not have been lowered with the previous method
	text = FindAndReplaceStringBeginning(text, "Ā", "ā");
	text = FindAndReplaceStringBeginning(text, "Â", "â");
	text = FindAndReplaceStringBeginning(text, "Æ", "æ");
	text = FindAndReplaceStringBeginning(text, "Ǣ", "ǣ");
	text = FindAndReplaceStringBeginning(text, "Ǽ", "ǽ");
	text = FindAndReplaceStringBeginning(text, "Ç", "ç");
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
		std::string replace = CapitalizeString(text.substr(pos + 1, 1));
		text.replace(pos + 1, 1, replace);
		pos += replace.length();
	}
	
	return text;
}

std::string FullyDecapitalizeString(std::string text)
{
	text = DecapitalizeString(text);
	
	size_t pos = 0;
	while ((pos = text.find(" ", pos)) != std::string::npos) {
		std::string replace = DecapitalizeString(text.substr(pos + 1, 1));
		text.replace(pos + 1, 1, replace);
		pos += replace.length();
	}
	
	return text;
}

std::string GetPluralForm(std::string name)
{
	if (name == "Einherjar" || name == "Wose") {
		return name; // no difference
	}
	
	if (name != "Monkey") {
		name = FindAndReplaceStringEnding(name, "y", "ie");
	}
	
	if (name.substr(name.size() - 2, 2) == "os" || name.substr(name.size() - 2, 2) == "us" || name.substr(name.size() - 1, 1) == "x") {
		name += "es";
	}
	
	if (name.substr(name.size() - 1, 1) != "s") {
		name += "s";
	}
	
	name = FindAndReplaceString(name, "Barracks", "Barrackses");
	name = FindAndReplaceString(name, "Dwarfs", "Dwarves");
	name = FindAndReplaceString(name, "Elfs", "Elves");
	name = FindAndReplaceString(name, "Ostrichs", "Ostriches");
	name = FindAndReplaceString(name, "Thiefs", "Thieves");
	name = FindAndReplaceString(name, "Wolfs", "Wolves");
	if (name != "Humans") {
		name = FindAndReplaceStringEnding(name, "mans", "men");
	}
	
	return name;
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
	text = FindAndReplaceString(text, "'", "");
	
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

std::string GeneratePersonalName(std::string unit_type_ident)
{
	int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
	return UnitTypes[unit_type_id]->GeneratePersonalName(NULL, UnitTypes[unit_type_id]->DefaultStat.Variables[GENDER_INDEX].Value);
}
//Wyrmgus end
