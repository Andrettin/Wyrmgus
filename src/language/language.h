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

enum ArticleTypes {
	ArticleTypeNoArticle,
	ArticleTypeDefinite,
	ArticleTypeIndefinite,

	MaxArticleTypes
};

enum GrammaticalCases {
	GrammaticalCaseNoCase,
	GrammaticalCaseNominative,
	GrammaticalCaseAccusative,
	GrammaticalCaseDative,
	GrammaticalCaseGenitive,

	MaxGrammaticalCases
};

enum GrammaticalNumbers {
	GrammaticalNumberNoNumber,
	GrammaticalNumberSingular,
	GrammaticalNumberPlural,

	MaxGrammaticalNumbers
};

enum GrammaticalPersons {
	GrammaticalPersonFirstPerson,
	GrammaticalPersonSecondPerson,
	GrammaticalPersonThirdPerson,

	MaxGrammaticalPersons
};

enum GrammaticalGenders {
	GrammaticalGenderNoGender,
	GrammaticalGenderMasculine,
	GrammaticalGenderFeminine,
	GrammaticalGenderNeuter,

	MaxGrammaticalGenders
};

enum GrammaticalTenses {
	GrammaticalTenseNoTense,
	GrammaticalTensePresent,
	GrammaticalTensePast,
	GrammaticalTenseFuture,

	MaxGrammaticalTenses
};

enum GrammaticalMoods {
	GrammaticalMoodIndicative,
	GrammaticalMoodSubjunctive,

	MaxGrammaticalMoods
};

enum ComparisonDegrees {
	ComparisonDegreePositive,
	ComparisonDegreeComparative,
	ComparisonDegreeSuperlative,

	MaxComparisonDegrees
};

enum AffixTypes {
	AffixTypePrefix,
	AffixTypeSuffix,
	AffixTypeInfix,

	MaxAffixTypes
};

enum WordJunctionTypes {
	WordJunctionTypeNoWordJunction,
	WordJunctionTypeCompound,
	WordJunctionTypeSeparate,

	MaxWordJunctionTypes
};

namespace wyrmgus {

class word;
enum class word_type;

class language final : public named_data_entry, public data_type<language>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "language";
	static constexpr const char *database_folder = "languages";

	explicit language(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	word *GetWord(const std::string &word, const word_type word_type, const std::vector<std::string> &word_meanings) const;
	std::string GetArticle(int gender, int grammatical_case, int article_type, int grammatical_number);
	std::string GetNounEnding(int grammatical_number, int grammatical_case, int word_junction_type = -1);
	std::string GetAdjectiveEnding(int article_type, int grammatical_case, int grammatical_number, int grammatical_gender);
	void RemoveWord(word *word);

	std::string Family;											/// Family of the language
	std::string NounEndings[MaxGrammaticalNumbers][MaxGrammaticalCases][MaxWordJunctionTypes];
	std::string AdjectiveEndings[MaxArticleTypes][MaxGrammaticalCases][MaxGrammaticalNumbers][MaxGrammaticalGenders];
	bool used_by_civilization_or_faction = false;
	language *DialectOf = nullptr; /// Of which language this is a dialect of (if at all); dialects inherit the words from the parent language unless specified otherwise
	std::vector<language *> Dialects;							/// Dialects of this language
	std::vector<word *> LanguageWords;					/// Words of the language
	std::map<std::string, std::vector<std::string>> NameTranslations;	/// Name translations; possible translations mapped to the name to be translated
};

}

extern std::string GetArticleTypeNameById(int article_type);
extern int GetArticleTypeIdByName(const std::string &article_type);
extern std::string GetGrammaticalCaseNameById(int grammatical_case);
extern int GetGrammaticalCaseIdByName(const std::string &grammatical_case);
extern std::string GetGrammaticalNumberNameById(int grammatical_number);
extern int GetGrammaticalNumberIdByName(const std::string &grammatical_number);
extern std::string GetGrammaticalPersonNameById(int grammatical_person);
extern int GetGrammaticalPersonIdByName(const std::string &grammatical_person);
extern std::string GetGrammaticalGenderNameById(int grammatical_gender);
extern int GetGrammaticalGenderIdByName(const std::string &grammatical_gender);
extern std::string GetGrammaticalTenseNameById(int grammatical_tense);
extern int GetGrammaticalTenseIdByName(const std::string &grammatical_tense);
extern std::string GetGrammaticalMoodNameById(int grammatical_mood);
extern int GetGrammaticalMoodIdByName(const std::string &grammatical_mood);
extern std::string GetComparisonDegreeNameById(int comparison_degree);
extern int GetComparisonDegreeIdByName(const std::string &comparison_degree);
extern std::string GetAffixTypeNameById(int affix_type);
extern int GetAffixTypeIdByName(const std::string &affix_type);
extern std::string GetWordJunctionTypeNameById(int word_junction_type);
extern int GetWordJunctionTypeIdByName(const std::string &word_junction_type);
