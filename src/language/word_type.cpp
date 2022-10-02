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
//      (c) Copyright 2015-2022 by Andrettin
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

#include "language/word_type.h"

namespace wyrmgus {

template class enum_converter<word_type>;

template <>
const std::string enum_converter<word_type>::property_class_identifier = "wyrmgus::word_type";

template <>
const std::map<std::string, word_type> enum_converter<word_type>::string_to_enum_map = {
	{ "none", word_type::none },
	{ "noun", word_type::noun },
	{ "verb", word_type::verb },
	{ "adjective", word_type::adjective },
	{ "pronoun", word_type::pronoun },
	{ "adverb", word_type::adverb },
	{ "conjunction", word_type::conjunction },
	{ "adposition", word_type::adposition },
	{ "article", word_type::article },
	{ "numeral", word_type::numeral },
	{ "affix", word_type::affix }
};

template <>
const bool enum_converter<word_type>::initialized = enum_converter::initialize();

}
