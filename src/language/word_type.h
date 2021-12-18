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
//      (c) Copyright 2015-2021 by Andrettin
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

namespace wyrmgus {

enum class word_type {
	none = -1,
	noun,
	verb,
	adjective,
	pronoun,
	adverb,
	conjunction,
	adposition,
	article,
	numeral,
	affix
};

inline word_type string_to_word_type(const std::string &str)
{
	if (str == "none") {
		return word_type::none;
	} else if (str == "noun") {
		return word_type::noun;
	} else if (str == "verb") {
		return word_type::verb;
	} else if (str == "adjective") {
		return word_type::adjective;
	} else if (str == "pronoun") {
		return word_type::pronoun;
	} else if (str == "adverb") {
		return word_type::adverb;
	} else if (str == "conjunction") {
		return word_type::conjunction;
	} else if (str == "adposition") {
		return word_type::adposition;
	} else if (str == "article") {
		return word_type::article;
	} else if (str == "numeral") {
		return word_type::numeral;
	} else if (str == "affix") {
		return word_type::affix;
	}

	throw std::runtime_error("Invalid word type: \"" + str + "\".");
}

inline std::string word_type_to_string(const word_type type)
{
	switch (type) {
		case word_type::none:
			return "none";
		case word_type::noun:
			return "noun";
		case word_type::verb:
			return "verb";
		case word_type::adjective:
			return "adjective";
		case word_type::pronoun:
			return "pronoun";
		case word_type::adverb:
			return "adverb";
		case word_type::conjunction:
			return "conjunction";
		case word_type::adposition:
			return "adposition";
		case word_type::article:
			return "article";
		case word_type::numeral:
			return "numeral";
		case word_type::affix:
			return "affix";
		default:
			break;
	}

	throw std::runtime_error("Invalid word type: \"" + std::to_string(static_cast<int>(type)) + "\".");
}

inline std::string word_type_to_name(const word_type type)
{
	switch (type) {
		case word_type::none:
			return "None";
		case word_type::noun:
			return "Noun";
		case word_type::verb:
			return "Verb";
		case word_type::adjective:
			return "Adjective";
		case word_type::pronoun:
			return "Pronoun";
		case word_type::adverb:
			return "Adverb";
		case word_type::conjunction:
			return "Conjunction";
		case word_type::adposition:
			return "Adposition";
		case word_type::article:
			return "Article";
		case word_type::numeral:
			return "Numeral";
		case word_type::affix:
			return "Affix";
		default:
			break;
	}

	throw std::runtime_error("Invalid word type: \"" + std::to_string(static_cast<int>(type)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::word_type)
