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

#include "stratagus.h"

#include "util/util.h"

//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "util/log_util.h"
#include "util/random.h"
#include "util/string_util.h"

#include <boost/tokenizer.hpp>

/*----------------------------------------------------------------------------
--  Random
----------------------------------------------------------------------------*/

/**
**  Synchronized random number.
**
**  @param max  Max value of random number to return
*/
int SyncRand(const int max)
{
	return random::get()->generate(max);
}

/*----------------------------------------------------------------------------
--  Strings
----------------------------------------------------------------------------*/

#ifndef HAVE_STRCPYS
errno_t strcpy_s(char *dst, size_t dstsize, const char *src)
{
	if (dst == nullptr || src == nullptr) {
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
	if (dst == nullptr || src == nullptr || dstsize == 0) {
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
	if (dst == nullptr || src == nullptr) {
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
**  @return   Pointer to first occurrence of b or null if not found.
*/
char *strcasestr(const char *a, const char *b)
{
	int x;

	if (!a || !*a || !b || !*b || strlen(a) < strlen(b)) {
		return nullptr;
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

	return nullptr;
}
#endif // !HAVE_STRCASESTR

std::vector<std::string> SplitString(const std::string &str, const char *separators)
{
	std::vector<std::string> output;
	
	boost::char_separator<char> separator(separators);
	
	boost::tokenizer<boost::char_separator<char>> tokens(str, separator);
	
	for (boost::tokenizer<boost::char_separator<char>>::iterator iterator = tokens.begin(); iterator != tokens.end(); ++iterator) {
		output.push_back(*iterator);
	}
	
	return output;
}

static std::map<unsigned, std::string> RomanConversionTable = {{1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"}, {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"}, {1, "I"}};

std::string NumberToRomanNumeral(unsigned number)
{
	std::string numeral;
	
	for (std::map<unsigned, std::string>::const_reverse_iterator iterator = RomanConversionTable.rbegin(); iterator != RomanConversionTable.rend(); ++iterator) {
		while (number >= iterator->first) {
			numeral += iterator->second;
			number -= iterator->first;
		}
	}
	
	return numeral;
}

/**
**	@brief	Format a number using commas
**
**	@param	number	Number to be formatted
**
**	@return	The formatted number as a string
*/
std::string FormatNumber(const int number)
{
	std::string str;
	const char sep = ',';
	int n = abs(number);

	int loop = 0;
	while (n > 0 || loop == 0) {
		if (loop > 0 && loop % 3 == 0) {
			str.insert(0, 1, sep);
		}
		const char c = n % 10 + 48;
		str.insert(0, 1, c);
		n /= 10;
		loop++;
	}

	if (number < 0) {
		str.insert(0, 1, '-');
	}
	
	return str;
}

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
	int c;
	const char *cp;

	optarg = nullptr;

	if (sp == 1) {
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
			return EOF;
		} else if (!strcmp(argv[optind], "--")) {
			optind++;
			return EOF;
		}
	}
	optopt = c = argv[optind][sp];
	if (c == ':' || (cp = strchr(opts, c)) == nullptr) {
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
		log::log_error("Invalid UTF8.");
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
	log::log_error("Invalid UTF8.");
	return text.size();
}


/*----------------------------------------------------------------------------
--  others
----------------------------------------------------------------------------*/

void PrintLocation(const char *file, int line, const char *funcName, std::ostream &output_stream)
{
	output_stream << file << ':' << line << ": " << funcName;
}

void AbortAt(const char *file, int line, const char *funcName, const char *conditionStr)
{
	log::log_error("Assertion failed at " + std::string(file) + ":" + std::to_string(line) + ": " + funcName + ": " + conditionStr);
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
#include "unit/unit_type.h" //for personal name generation

std::string FindAndReplaceString(const std::string &text, const std::string &find, const std::string &replace)
{
	std::string result(text);
	
	size_t pos = 0;
	while ((pos = result.find(find, pos)) != std::string::npos) {
		result.replace(pos, find.length(), replace);
		pos += replace.length();
	}
	
	return result;
}

std::string FindAndReplaceStringEnding(const std::string &text, const std::string &find, const std::string &replace)
{
	std::string result(text);
	
	size_t pos = text.find(find, text.length() - find.length());
	if (pos != std::string::npos) {
		result.replace(pos, find.length(), replace);
	}
	
	return result;
}

std::string FindAndReplaceStringBeginning(const std::string &text, const std::string &find, const std::string &replace)
{
	std::string result(text);
	
	size_t pos = text.find(find, 0);
	if (pos != std::string::npos && pos == 0) {
		result.replace(pos, find.length(), replace);
	}
	
	return result;
}

std::string TransliterateText(const std::string &text) //convert special characters into ones more legible for English-speakers
{
	std::string result(text);

	wyrmgus::string::normalize(result);
	
	result = FindAndReplaceString(result, "Ā́", "A");
	result = FindAndReplaceString(result, "ā́", "a");
	result = FindAndReplaceString(result, "Ấ", "A");
	result = FindAndReplaceString(result, "ấ", "a");
	result = FindAndReplaceString(result, "Ȧ́", "A");
	result = FindAndReplaceString(result, "ȧ́", "a");
	result = FindAndReplaceString(result, "Á", "A");
	result = FindAndReplaceString(result, "á", "a");
	result = FindAndReplaceString(result, "À", "A");
	result = FindAndReplaceString(result, "à", "a");
	result = FindAndReplaceString(result, "Ã", "A");
	result = FindAndReplaceString(result, "ã", "a");
	result = FindAndReplaceString(result, "Ä", "A");
	result = FindAndReplaceString(result, "ä", "a");
	result = FindAndReplaceString(result, "Â", "A");
	result = FindAndReplaceString(result, "â", "a");
	result = FindAndReplaceString(result, "Ą", "A");
	result = FindAndReplaceString(result, "ą", "a");
	result = FindAndReplaceString(result, "ᶏ", "a");
	result = FindAndReplaceString(result, "Å", "A");
	result = FindAndReplaceString(result, "å", "a");
	result = FindAndReplaceString(result, "Ă", "A");
	result = FindAndReplaceString(result, "ă", "a");
	result = FindAndReplaceString(result, "Æ̂", "Ae");
	result = FindAndReplaceString(result, "æ̂", "ae");
	result = FindAndReplaceString(result, "Æ", "Ae");
	result = FindAndReplaceString(result, "æ", "ae");
	result = FindAndReplaceString(result, "Ǣ", "Ae");
	result = FindAndReplaceString(result, "ǣ", "ae");
	result = FindAndReplaceString(result, "Ǽ", "Ae");
	result = FindAndReplaceString(result, "ǽ", "ae");
	result = FindAndReplaceString(result, "Ƀ", "B");
	result = FindAndReplaceString(result, "ƀ", "b");
	result = FindAndReplaceString(result, "Č", "C");
	result = FindAndReplaceString(result, "č", "c");
	result = FindAndReplaceString(result, "Ð", "D");
	result = FindAndReplaceString(result, "ð", "d");
	result = FindAndReplaceString(result, "Ḍ", "D");
	result = FindAndReplaceString(result, "ḍ", "d");
	result = FindAndReplaceString(result, "Đ", "D");
	result = FindAndReplaceString(result, "đ", "d");
	result = FindAndReplaceString(result, "ẟ", "d"); //not the same character as "δ"
	result = FindAndReplaceString(result, "Ę̄", "E");
	result = FindAndReplaceString(result, "ę̄", "e");
	result = FindAndReplaceString(result, "Ḗ", "E");
	result = FindAndReplaceString(result, "ḗ", "e");
	result = FindAndReplaceString(result, "Ė́", "E");
	result = FindAndReplaceString(result, "ė́", "e");
	result = FindAndReplaceString(result, "Ë̃", "E");
	result = FindAndReplaceString(result, "ë̃", "e");
	result = FindAndReplaceString(result, "Ë̂", "E");
	result = FindAndReplaceString(result, "ë̂", "e");
	result = FindAndReplaceString(result, "É", "E"); 
	result = FindAndReplaceString(result, "é", "e");
	result = FindAndReplaceString(result, "È", "E");
	result = FindAndReplaceString(result, "è", "e");
	result = FindAndReplaceString(result, "Ê", "E");
	result = FindAndReplaceString(result, "ê", "e");
	result = FindAndReplaceString(result, "Ě", "E");
	result = FindAndReplaceString(result, "ě", "e");
	result = FindAndReplaceString(result, "Ė", "E");
	result = FindAndReplaceString(result, "ė", "e");
	result = FindAndReplaceString(result, "Ë", "E");
	result = FindAndReplaceString(result, "ë", "e");
	result = FindAndReplaceString(result, "Ę", "E");
	result = FindAndReplaceString(result, "ę", "e");
	result = FindAndReplaceString(result, "Ĕ", "E");
	result = FindAndReplaceString(result, "ĕ", "e");
	result = FindAndReplaceString(result, "Ə", "E");
	result = FindAndReplaceString(result, "ə", "e");
	result = FindAndReplaceString(result, "Ǝ", "E");
	result = FindAndReplaceString(result, "ǝ", "e");
	result = FindAndReplaceString(result, "ϵ", "e");
	result = FindAndReplaceString(result, "Ĝ", "G");
	result = FindAndReplaceString(result, "ĝ", "g");
	result = FindAndReplaceString(result, "Ī̆", "I");
	result = FindAndReplaceString(result, "ī̆", "i");
	result = FindAndReplaceString(result, "Î́", "I");
	result = FindAndReplaceString(result, "î́", "i");
	result = FindAndReplaceString(result, "Ī́", "I");
	result = FindAndReplaceString(result, "ī́", "i");
	result = FindAndReplaceString(result, "I̊", "I");
	result = FindAndReplaceString(result, "i̊", "i");
	result = FindAndReplaceString(result, "Í", "I");
	result = FindAndReplaceString(result, "í", "i");
	result = FindAndReplaceString(result, "Ì", "I");
	result = FindAndReplaceString(result, "ì", "i");
	result = FindAndReplaceString(result, "Î", "I");
	result = FindAndReplaceString(result, "î", "i");
	result = FindAndReplaceString(result, "Ĭ", "I");
	result = FindAndReplaceString(result, "ĭ", "i");
	result = FindAndReplaceString(result, "Ĩ", "I");
	result = FindAndReplaceString(result, "ĩ", "i");
	result = FindAndReplaceString(result, "Ḱ", "K");
	result = FindAndReplaceString(result, "ḱ", "k");
	result = FindAndReplaceString(result, "L̥", "L");
	result = FindAndReplaceString(result, "l̥", "l");
	result = FindAndReplaceString(result, "Ɫ", "L");
	result = FindAndReplaceString(result, "ɫ", "l");
	result = FindAndReplaceString(result, "Ň", "N");
	result = FindAndReplaceString(result, "ň", "n");
	result = FindAndReplaceString(result, "Ṇ", "N");
	result = FindAndReplaceString(result, "ṇ", "n");
	result = FindAndReplaceString(result, "Ṅ", "N");
	result = FindAndReplaceString(result, "ṅ", "n");
	result = FindAndReplaceString(result, "Ṓ", "O");
	result = FindAndReplaceString(result, "ṓ", "o");
	result = FindAndReplaceString(result, "Ŏ", "O");
	result = FindAndReplaceString(result, "ŏ", "o");
	result = FindAndReplaceString(result, "Ø", "Ö"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceString(result, "ø", "ö"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceString(result, "Ǫ", "O"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceString(result, "ǫ", "o"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceString(result, "Ö", "O");
	result = FindAndReplaceString(result, "ö", "o");
	result = FindAndReplaceString(result, "Ó", "O");
	result = FindAndReplaceString(result, "ó", "o");
	result = FindAndReplaceString(result, "Ò", "O");
	result = FindAndReplaceString(result, "ò", "o");
	result = FindAndReplaceString(result, "Ô", "O");
	result = FindAndReplaceString(result, "ô", "o");
	result = FindAndReplaceString(result, "Ǒ", "O");
	result = FindAndReplaceString(result, "ǒ", "o");
	result = FindAndReplaceString(result, "Œ", "Oe");
	result = FindAndReplaceString(result, "œ", "oe");
	result = FindAndReplaceString(result, "Ṛ́", "R");
	result = FindAndReplaceString(result, "ṛ́", "r");
	result = FindAndReplaceString(result, "Ŗ́", "R");
	result = FindAndReplaceString(result, "ŗ́", "r");
	result = FindAndReplaceString(result, "R̄", "R");
	result = FindAndReplaceString(result, "r̄", "r");
	result = FindAndReplaceString(result, "Ř", "R");
	result = FindAndReplaceString(result, "ř", "r");
	result = FindAndReplaceString(result, "Ṛ", "R");
	result = FindAndReplaceString(result, "ṛ", "r");
	result = FindAndReplaceString(result, "Ŕ", "R");
	result = FindAndReplaceString(result, "ŕ", "r");
	result = FindAndReplaceString(result, "Ṙ", "R");
	result = FindAndReplaceString(result, "ṙ", "r");
	result = FindAndReplaceString(result, "Š", "S");
	result = FindAndReplaceString(result, "š", "s");
	result = FindAndReplaceString(result, "Ș", "S");
	result = FindAndReplaceString(result, "ș", "s");
	result = FindAndReplaceString(result, "Ś", "S");
	result = FindAndReplaceString(result, "ś", "s");
	result = FindAndReplaceString(result, "ß", "ss"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceString(result, "Ṭ", "T");
	result = FindAndReplaceString(result, "ṭ", "t");
	result = FindAndReplaceString(result, "Ț", "T");
	result = FindAndReplaceString(result, "ț", "t");
	result = FindAndReplaceString(result, "ÞÞ", "Þ"); //replace double thorns with a single one
	result = FindAndReplaceString(result, "þþ", "þ"); //replace double thorns with a single one
	result = FindAndReplaceString(result, "Þ", "Th"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceString(result, "þ", "th"); //Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceString(result, "Ū́", "U");
	result = FindAndReplaceString(result, "ū́", "u");
	result = FindAndReplaceString(result, "Ů̃", "U");
	result = FindAndReplaceString(result, "ů̃", "u");
	result = FindAndReplaceString(result, "Ü", "U");
	result = FindAndReplaceString(result, "ü", "u");
	result = FindAndReplaceString(result, "Ú", "U");
	result = FindAndReplaceString(result, "ú", "u");
	result = FindAndReplaceString(result, "Ù", "U");
	result = FindAndReplaceString(result, "ù", "u");
	result = FindAndReplaceString(result, "Û", "U");
	result = FindAndReplaceString(result, "û", "u");
	result = FindAndReplaceString(result, "Ŭ", "U");
	result = FindAndReplaceString(result, "ŭ", "u");
	result = FindAndReplaceString(result, "Ů", "U");
	result = FindAndReplaceString(result, "ů", "u");
	result = FindAndReplaceString(result, "Ũ", "U");
	result = FindAndReplaceString(result, "ũ", "u");
	result = FindAndReplaceString(result, "Ṷ", "U");
	result = FindAndReplaceString(result, "ṷ", "u");
	result = FindAndReplaceString(result, "ʷ", "w");
	result = FindAndReplaceString(result, "Ȳ", "Y");
	result = FindAndReplaceString(result, "ȳ", "y");
	result = FindAndReplaceString(result, "Ŷ", "Y");
	result = FindAndReplaceString(result, "ŷ", "y");
	result = FindAndReplaceString(result, "Ỹ", "Y");
	result = FindAndReplaceString(result, "ỹ", "y");
	result = FindAndReplaceString(result, "Ý", "Y");
	result = FindAndReplaceString(result, "ý", "y");
	result = FindAndReplaceString(result, "Ž", "Z");
	result = FindAndReplaceString(result, "ž", "z");
	result = FindAndReplaceString(result, "Z̨", "Z");
	result = FindAndReplaceString(result, "z̨", "z");
	result = FindAndReplaceString(result, "Ż", "Z");
	result = FindAndReplaceString(result, "ż", "z");
	
	result = FindAndReplaceString(result, "ʔ", "'"); // glottal stop

	//replace endings in -r after consonants (which happens in the nominative for Old Norse); Source: Henry Adams Bellows (transl.), "The Poetic Edda", p. xxviii.
	result = FindAndReplaceStringEnding(result, "dr", "d");
	result = FindAndReplaceString(result, "dr ", "d ");
	result = FindAndReplaceStringEnding(result, "fr", "f");
	result = FindAndReplaceString(result, "fr ", "f ");
	result = FindAndReplaceStringEnding(result, "gr", "g");
	result = FindAndReplaceString(result, "gr ", "g ");
	result = FindAndReplaceStringEnding(result, "kr", "k");
	result = FindAndReplaceString(result, "kr ", "k ");
	result = FindAndReplaceStringEnding(result, "mr", "m");
	result = FindAndReplaceString(result, "mr ", "m ");
	result = FindAndReplaceStringEnding(result, "nr", "n");
	result = FindAndReplaceString(result, "nr ", "n ");
	result = FindAndReplaceStringEnding(result, "pr", "p");
	result = FindAndReplaceString(result, "pr ", "p ");
	result = FindAndReplaceStringEnding(result, "rr", "r");
	result = FindAndReplaceString(result, "rr ", "r ");
	result = FindAndReplaceStringEnding(result, "tr", "t");
	result = FindAndReplaceString(result, "tr ", "t ");
	
	//Greek characters
	result = FindAndReplaceString(result, "Ἄ", "A");
	result = FindAndReplaceString(result, "ἄ", "a");
	result = FindAndReplaceString(result, "Ά", "A");
	result = FindAndReplaceString(result, "ά", "a");
	result = FindAndReplaceString(result, "Ἀ", "A");
	result = FindAndReplaceString(result, "ἀ", "a");
	result = FindAndReplaceString(result, "Α", "A");
	result = FindAndReplaceString(result, "α", "a");
	result = FindAndReplaceString(result, "Β", "B");
	result = FindAndReplaceString(result, "β", "b");
	result = FindAndReplaceString(result, "Χ", "Ch");
	result = FindAndReplaceString(result, "χ", "ch");
	result = FindAndReplaceString(result, "Δ", "D");
	result = FindAndReplaceString(result, "δ", "d");
	result = FindAndReplaceString(result, "Ἑ", "E");
	result = FindAndReplaceString(result, "ἑ", "e");
	result = FindAndReplaceString(result, "Ἔ", "E");
	result = FindAndReplaceString(result, "ἔ", "e");
	result = FindAndReplaceString(result, "Έ", "E");
	result = FindAndReplaceString(result, "έ", "e");
	result = FindAndReplaceString(result, "Ε", "E");
	result = FindAndReplaceString(result, "ε", "e");
	result = FindAndReplaceString(result, "Η", "E");
	result = FindAndReplaceString(result, "η", "e");
	result = FindAndReplaceString(result, "Γ", "G");
	result = FindAndReplaceString(result, "γ", "g");
	result = FindAndReplaceString(result, "Ῑ́", "I");
	result = FindAndReplaceString(result, "ῑ́", "i");
	result = FindAndReplaceString(result, "Ί", "I");
	result = FindAndReplaceString(result, "ί", "i");
	result = FindAndReplaceString(result, "ῖ", "i");
	result = FindAndReplaceString(result, "Ι", "I");
	result = FindAndReplaceString(result, "ι", "i");
	result = FindAndReplaceString(result, "Ή", "I");
	result = FindAndReplaceString(result, "ή", "i");
	result = FindAndReplaceString(result, "Κ", "K");
	result = FindAndReplaceString(result, "κ", "k");
	result = FindAndReplaceString(result, "Λ", "L");
	result = FindAndReplaceString(result, "λ", "l");
	result = FindAndReplaceString(result, "Μ", "M");
	result = FindAndReplaceString(result, "μ", "m");
	result = FindAndReplaceString(result, "Ν", "N");
	result = FindAndReplaceString(result, "ν", "n");
	result = FindAndReplaceString(result, "Ὄ", "O");
	result = FindAndReplaceString(result, "ὄ", "o");
	result = FindAndReplaceString(result, "Ὅ", "O");
	result = FindAndReplaceString(result, "ὅ", "o");
	result = FindAndReplaceString(result, "Ό", "O");
	result = FindAndReplaceString(result, "ό", "o");
	result = FindAndReplaceString(result, "Ὀ", "O");
	result = FindAndReplaceString(result, "ὀ", "o");
	result = FindAndReplaceString(result, "Ὁ", "O");
	result = FindAndReplaceString(result, "ὁ", "o");
	result = FindAndReplaceString(result, "Ο", "O");
	result = FindAndReplaceString(result, "ο", "o");
	result = FindAndReplaceString(result, "Ώ", "O");
	result = FindAndReplaceString(result, "ώ", "o");
	result = FindAndReplaceString(result, "Ω", "O");
	result = FindAndReplaceString(result, "ω", "o");
	result = FindAndReplaceString(result, "Π", "P");
	result = FindAndReplaceString(result, "π", "p");
	result = FindAndReplaceString(result, "Φ", "Ph");
	result = FindAndReplaceString(result, "φ", "ph");
	result = FindAndReplaceString(result, "Ψ", "Ps");
	result = FindAndReplaceString(result, "ψ", "ps");
	result = FindAndReplaceString(result, "Ρ", "R");
	result = FindAndReplaceString(result, "ρ", "r");
	result = FindAndReplaceString(result, "Σ", "S");
	result = FindAndReplaceString(result, "σ", "s");
	result = FindAndReplaceString(result, "ς", "s");
	result = FindAndReplaceString(result, "Τ", "T");
	result = FindAndReplaceString(result, "τ", "t");
	result = FindAndReplaceString(result, "Θ", "Th");
	result = FindAndReplaceString(result, "θ", "th");
	result = FindAndReplaceString(result, "Ξ", "X");
	result = FindAndReplaceString(result, "ξ", "x");
	result = FindAndReplaceString(result, "Ύ", "Y");
	result = FindAndReplaceString(result, "ύ", "y");
	result = FindAndReplaceString(result, "Ὑ", "Y");
	result = FindAndReplaceString(result, "ὑ", "y");
	result = FindAndReplaceString(result, "Υ", "Y");
	result = FindAndReplaceString(result, "υ", "y");
	result = FindAndReplaceString(result, "Ζ", "Z");
	result = FindAndReplaceString(result, "ζ", "z");
	
	//remove large clusters of the same letters
	result = FindAndReplaceString(result, "nnn", "nn");
	
	return result;
}

std::string CapitalizeString(const std::string &text)
{
	if (text.empty()) {
		return text;
	}
	
	std::string result(text);
	
	result[0] = toupper(result[0]);
	
	// replace special characters which may not have been uppered with the previous method
	result = FindAndReplaceStringBeginning(result, "ā", "Ā");
	result = FindAndReplaceStringBeginning(result, "â", "Â");
	result = FindAndReplaceStringBeginning(result, "æ", "Æ");
	result = FindAndReplaceStringBeginning(result, "ǣ", "Ǣ");
	result = FindAndReplaceStringBeginning(result, "ǽ", "Ǽ");
	result = FindAndReplaceStringBeginning(result, "ð", "Ð");
	result = FindAndReplaceStringBeginning(result, "ḍ", "Ḍ");
	result = FindAndReplaceStringBeginning(result, "ē", "Ē");
	result = FindAndReplaceStringBeginning(result, "ê", "Ê");
	result = FindAndReplaceStringBeginning(result, "ě", "Ě");
	result = FindAndReplaceStringBeginning(result, "ī", "Ī");
	result = FindAndReplaceStringBeginning(result, "î", "Î");
	result = FindAndReplaceStringBeginning(result, "ĭ", "Ĭ");
	result = FindAndReplaceStringBeginning(result, "ī̆", "Ī̆");
	result = FindAndReplaceStringBeginning(result, "ō", "Ō");
	result = FindAndReplaceStringBeginning(result, "ô", "Ô");
	result = FindAndReplaceStringBeginning(result, "ø", "Ø");
	result = FindAndReplaceStringBeginning(result, "ǫ", "Ǫ");
	result = FindAndReplaceStringBeginning(result, "ș", "Ș");
	result = FindAndReplaceStringBeginning(result, "ț", "Ț");
	result = FindAndReplaceStringBeginning(result, "þ", "Þ");
	result = FindAndReplaceStringBeginning(result, "ū", "Ū");
	result = FindAndReplaceStringBeginning(result, "û", "Û");
	result = FindAndReplaceStringBeginning(result, "ŭ", "Ŭ");
	result = FindAndReplaceStringBeginning(result, "ȳ", "Ȳ");
	result = FindAndReplaceStringBeginning(result, "ž", "Ž");
	
	//Greek characters
	result = FindAndReplaceStringBeginning(result, "α", "Α");
	result = FindAndReplaceStringBeginning(result, "χ", "Χ");
	result = FindAndReplaceStringBeginning(result, "έ", "Έ");
	result = FindAndReplaceStringBeginning(result, "ι", "Ι");
	result = FindAndReplaceStringBeginning(result, "μ", "Μ");
	result = FindAndReplaceStringBeginning(result, "ν", "Ν");
	result = FindAndReplaceStringBeginning(result, "ο", "Ο");
	result = FindAndReplaceStringBeginning(result, "ό", "Ό");
	result = FindAndReplaceStringBeginning(result, "σ", "Σ");
	result = FindAndReplaceStringBeginning(result, "θ", "Θ");
	result = FindAndReplaceStringBeginning(result, "ύ", "Ύ");
	
	return result;
}

std::string DecapitalizeString(const std::string &text)
{
	if (text.empty()) {
		return text;
	}

	std::string result(text);
	
	result[0] = tolower(result[0]);
	
	// replace special characters which may not have been lowered with the previous method
	result = FindAndReplaceStringBeginning(result, "Ā", "ā");
	result = FindAndReplaceStringBeginning(result, "Â", "â");
	result = FindAndReplaceStringBeginning(result, "Æ", "æ");
	result = FindAndReplaceStringBeginning(result, "Ǣ", "ǣ");
	result = FindAndReplaceStringBeginning(result, "Ǽ", "ǽ");
	result = FindAndReplaceStringBeginning(result, "Ç", "ç");
	result = FindAndReplaceStringBeginning(result, "Ð", "ð");
	result = FindAndReplaceStringBeginning(result, "Ḍ", "ḍ");
	result = FindAndReplaceStringBeginning(result, "Ē", "ē");
	result = FindAndReplaceStringBeginning(result, "Ê", "ê");
	result = FindAndReplaceStringBeginning(result, "Ě", "ě");
	result = FindAndReplaceStringBeginning(result, "Ī", "ī");
	result = FindAndReplaceStringBeginning(result, "Î", "î");
	result = FindAndReplaceStringBeginning(result, "Ĭ", "ĭ");
	result = FindAndReplaceStringBeginning(result, "Ī̆", "ī̆");
	result = FindAndReplaceStringBeginning(result, "Ō", "ō");
	result = FindAndReplaceStringBeginning(result, "Ô", "ô");
	result = FindAndReplaceStringBeginning(result, "Ø", "ø");
	result = FindAndReplaceStringBeginning(result, "Ǫ", "ǫ");
	result = FindAndReplaceStringBeginning(result, "Þ", "þ");
	result = FindAndReplaceStringBeginning(result, "Ū", "ū");
	result = FindAndReplaceStringBeginning(result, "Û", "û");
	result = FindAndReplaceStringBeginning(result, "Ŭ", "ŭ");
	result = FindAndReplaceStringBeginning(result, "Ȳ", "ȳ");
	result = FindAndReplaceStringBeginning(result, "Ž", "ž");
	
	//Greek characters
	result = FindAndReplaceStringBeginning(result, "Α", "α");
	result = FindAndReplaceStringBeginning(result, "Χ", "χ");
	result = FindAndReplaceStringBeginning(result, "Έ", "έ");
	result = FindAndReplaceStringBeginning(result, "Ι", "ι");
	result = FindAndReplaceStringBeginning(result, "Μ", "μ");
	result = FindAndReplaceStringBeginning(result, "Ν", "ν");
	result = FindAndReplaceStringBeginning(result, "Ο", "ο");
	result = FindAndReplaceStringBeginning(result, "Ό", "ό");
	result = FindAndReplaceStringBeginning(result, "Σ", "σ");
	result = FindAndReplaceStringBeginning(result, "Θ", "θ");
	result = FindAndReplaceStringBeginning(result, "Ύ", "ύ");
	
	return result;
}

std::string FullyCapitalizeString(const std::string &text)
{
	std::string result(text);
	
	result = CapitalizeString(result);
	
	size_t pos = 0;
	while ((pos = result.find(" ", pos)) != std::string::npos) {
		std::string replace = CapitalizeString(result.substr(pos + 1, 1));
		result.replace(pos + 1, 1, replace);
		pos += replace.length();
	}
	
	return result;
}

std::string FullyDecapitalizeString(const std::string &text)
{
	std::string result(text);
	
	result = DecapitalizeString(result);
	
	size_t pos = 0;
	while ((pos = result.find(" ", pos)) != std::string::npos) {
		std::string replace = DecapitalizeString(result.substr(pos + 1, 1));
		result.replace(pos + 1, 1, replace);
		pos += replace.length();
	}
	
	return result;
}

std::string GetPluralForm(const std::string &name)
{
	return string::get_plural_form(name);
}

std::string IdentToName(const std::string &text)
{
	std::string result(text);
	
	result = FindAndReplaceString(result, "-", " ");
	result = FullyCapitalizeString(result);
	
	return result;
}

std::string NameToIdent(const std::string &text)
{
	std::string result(text);
	
	result = FullyDecapitalizeString(result);
	result = FindAndReplaceString(result, " ", "-");
	result = FindAndReplaceString(result, "'", "");
	
	return result;
}

std::string SeparateCapitalizedStringElements(const std::string &text)
{
	std::string result(text);
	
	for (size_t pos = 1; pos < result.length(); ++pos) {
		if (isupper(result[pos])) {
			result.replace(pos, 1, " " + result.substr(pos, 1));
			pos += 1;
		}
	}
	return result;
}
//Wyrmgus end
