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
//      (c) Copyright 2015-2020 by Andrettin
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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "language/language.h"

struct lua_State;

int CclDefineLanguageWord(lua_State *l);

namespace wyrmgus {

class language;

class word final : public named_data_entry, public data_type<word>
{
public:
	static constexpr const char *class_identifier = "word";
	static constexpr const char *database_folder = "words";

	explicit word(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	bool HasMeaning(const std::string &meaning);
	std::string GetNounInflection(int grammatical_number, int grammatical_case, int word_junction_type = -1);
	std::string GetVerbInflection(int grammatical_number, int grammatical_person, int grammatical_tense, int grammatical_mood);
	std::string GetAdjectiveInflection(int comparison_degree, int article_type = -1, int grammatical_case = -1, int grammatical_number = -1, int grammatical_gender = -1);
	std::string GetParticiple(int grammatical_tense);
	void RemoveFromVector(std::vector<word *> &word_vector);

	language *language = nullptr;
	int Type = -1;						/// Word type
	int Gender = -1;					/// What is the gender of the noun or article (Masculine, Feminine or Neuter)
	int GrammaticalNumber = -1;			/// Grammatical number (i.e. whether the word is necessarily plural or not)
	bool Archaic = false;				/// Whether the word is archaic (whether it is used in current speech)
	std::map<std::tuple<int, int>, std::string> NumberCaseInflections;	/// For nouns, mapped to grammatical number and grammatical case
	std::map<std::tuple<int, int, int, int>, std::string> NumberPersonTenseMoodInflections;	/// For verbs, mapped to grammatical number, grammatical person, grammatical tense and grammatical mood
	std::string ComparisonDegreeCaseInflections[MaxComparisonDegrees][MaxGrammaticalCases];	/// For adjectives
	std::string Participles[MaxGrammaticalTenses];		/// For verbs
	std::vector<std::string> Meanings;					/// Meanings of the word in English.
	word *DerivesFrom = nullptr;    			/// From which word does this word derive
	std::vector<word *> DerivesTo;				/// Which words derive from this word
	word *CompoundElements[MaxAffixTypes];    	/// From which compound elements is this word formed
	std::vector<word *> CompoundElementOf[MaxAffixTypes]; /// Which words are formed from this word as a compound element

	// noun-specific variables
	bool Uncountable = false;				/// Whether the noun is uncountable or not.

	//pronoun and article-specific variables
	std::string Nominative;			/// Nominative case for the pronoun (if any)
	std::string Accusative;			/// Accusative case for the pronoun (if any)
	std::string Dative;				/// Dative case for the pronoun (if any)
	std::string Genitive;			/// Genitive case for the pronoun (if any)

	//article-specific variables
	int ArticleType = -1;				/// Which article type this article belongs to

	//numeral-specific variables
	int Number = -1;

	std::string Mod;				/// To which mod (or map), if any, this word belongs
};

}
