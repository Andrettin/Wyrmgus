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

#include "stratagus.h"

#include "language/language.h"

#include "language/word.h"

namespace wyrmgus {

word *language::GetWord(const std::string word, int word_type, std::vector<std::string> &word_meanings) const
{
	for (size_t i = 0; i < this->LanguageWords.size(); ++i) {
		if (
			this->LanguageWords[i]->get_name() == word
			&& (word_type == -1 || this->LanguageWords[i]->Type == word_type)
			&& (word_meanings.size() == 0 || this->LanguageWords[i]->Meanings == word_meanings)
			) {
			return this->LanguageWords[i];
		}
	}

	return nullptr;
}

std::string language::GetArticle(int gender, int grammatical_case, int article_type, int grammatical_number)
{
	for (size_t i = 0; i < this->LanguageWords.size(); ++i) {
		if (this->LanguageWords[i]->Type != WordTypeArticle || this->LanguageWords[i]->ArticleType != article_type) {
			continue;
		}

		if (grammatical_number != -1 && this->LanguageWords[i]->GrammaticalNumber != -1 && this->LanguageWords[i]->GrammaticalNumber != grammatical_number) {
			continue;
		}

		if (gender == -1 || this->LanguageWords[i]->Gender == -1 || gender == this->LanguageWords[i]->Gender) {
			if (grammatical_case == GrammaticalCaseNominative && !this->LanguageWords[i]->Nominative.empty()) {
				return this->LanguageWords[i]->Nominative;
			} else if (grammatical_case == GrammaticalCaseAccusative && !this->LanguageWords[i]->Accusative.empty()) {
				return this->LanguageWords[i]->Accusative;
			} else if (grammatical_case == GrammaticalCaseDative && !this->LanguageWords[i]->Dative.empty()) {
				return this->LanguageWords[i]->Dative;
			} else if (grammatical_case == GrammaticalCaseGenitive && !this->LanguageWords[i]->Genitive.empty()) {
				return this->LanguageWords[i]->Genitive;
			}
		}
	}
	return "";
}

std::string language::GetNounEnding(int grammatical_number, int grammatical_case, int word_junction_type)
{
	if (word_junction_type == -1) {
		word_junction_type = WordJunctionTypeNoWordJunction;
	}

	if (!this->NounEndings[grammatical_number][grammatical_case][word_junction_type].empty()) {
		return this->NounEndings[grammatical_number][grammatical_case][word_junction_type];
	} else if (!this->NounEndings[grammatical_number][grammatical_case][WordJunctionTypeNoWordJunction].empty()) {
		return this->NounEndings[grammatical_number][grammatical_case][WordJunctionTypeNoWordJunction];
	}

	return "";
}

std::string language::GetAdjectiveEnding(int article_type, int grammatical_case, int grammatical_number, int grammatical_gender)
{
	if (grammatical_number == -1) {
		grammatical_number = GrammaticalNumberNoNumber;
	}

	if (grammatical_gender == -1) {
		grammatical_gender = GrammaticalGenderNoGender;
	}

	if (!this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][grammatical_gender].empty()) {
		return this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][grammatical_gender];
	} else if (!this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][GrammaticalGenderNoGender].empty()) {
		return this->AdjectiveEndings[article_type][grammatical_case][grammatical_number][GrammaticalGenderNoGender];
	} else if (!this->AdjectiveEndings[article_type][grammatical_case][GrammaticalNumberNoNumber][GrammaticalGenderNoGender].empty()) {
		return this->AdjectiveEndings[article_type][grammatical_case][GrammaticalNumberNoNumber][GrammaticalGenderNoGender];
	}

	return "";
}

void language::RemoveWord(word *word)
{
	if (std::find(this->LanguageWords.begin(), this->LanguageWords.end(), word) != this->LanguageWords.end()) {
		this->LanguageWords.erase(std::remove(this->LanguageWords.begin(), this->LanguageWords.end(), word), this->LanguageWords.end());
	}
}

}

std::string GetWordTypeNameById(int word_type)
{
	if (word_type == WordTypeNoun) {
		return "noun";
	} else if (word_type == WordTypeVerb) {
		return "verb";
	} else if (word_type == WordTypeAdjective) {
		return "adjective";
	} else if (word_type == WordTypePronoun) {
		return "pronoun";
	} else if (word_type == WordTypeAdverb) {
		return "adverb";
	} else if (word_type == WordTypeConjunction) {
		return "conjunction";
	} else if (word_type == WordTypeAdposition) {
		return "adposition";
	} else if (word_type == WordTypeArticle) {
		return "article";
	} else if (word_type == WordTypeNumeral) {
		return "numeral";
	} else if (word_type == WordTypeAffix) {
		return "affix";
	}

	return "";
}

int GetWordTypeIdByName(const std::string &word_type)
{
	if (word_type == "noun") {
		return WordTypeNoun;
	} else if (word_type == "verb") {
		return WordTypeVerb;
	} else if (word_type == "adjective") {
		return WordTypeAdjective;
	} else if (word_type == "pronoun") {
		return WordTypePronoun;
	} else if (word_type == "adverb") {
		return WordTypeAdverb;
	} else if (word_type == "conjunction") {
		return WordTypeConjunction;
	} else if (word_type == "adposition") {
		return WordTypeAdposition;
	} else if (word_type == "article") {
		return WordTypeArticle;
	} else if (word_type == "numeral") {
		return WordTypeNumeral;
	} else if (word_type == "affix") {
		return WordTypeAffix;
	}

	return -1;
}

std::string GetArticleTypeNameById(int article_type)
{
	if (article_type == ArticleTypeNoArticle) {
		return "no-article";
	} else if (article_type == ArticleTypeDefinite) {
		return "definite";
	} else if (article_type == ArticleTypeIndefinite) {
		return "indefinite";
	}

	return "";
}

int GetArticleTypeIdByName(const std::string &article_type)
{
	if (article_type == "no-article") {
		return ArticleTypeNoArticle;
	} else if (article_type == "definite") {
		return ArticleTypeDefinite;
	} else if (article_type == "indefinite") {
		return ArticleTypeIndefinite;
	}

	return -1;
}

std::string GetGrammaticalCaseNameById(int grammatical_case)
{
	if (grammatical_case == GrammaticalCaseNoCase) {
		return "no-case";
	} else if (grammatical_case == GrammaticalCaseNominative) {
		return "nominative";
	} else if (grammatical_case == GrammaticalCaseAccusative) {
		return "accusative";
	} else if (grammatical_case == GrammaticalCaseDative) {
		return "dative";
	} else if (grammatical_case == GrammaticalCaseGenitive) {
		return "genitive";
	}

	return "";
}

int GetGrammaticalCaseIdByName(const std::string &grammatical_case)
{
	if (grammatical_case == "no-case") {
		return GrammaticalCaseNoCase;
	} else if (grammatical_case == "nominative") {
		return GrammaticalCaseNominative;
	} else if (grammatical_case == "accusative") {
		return GrammaticalCaseAccusative;
	} else if (grammatical_case == "dative") {
		return GrammaticalCaseDative;
	} else if (grammatical_case == "genitive") {
		return GrammaticalCaseGenitive;
	}

	return -1;
}

std::string GetGrammaticalNumberNameById(int grammatical_number)
{
	if (grammatical_number == GrammaticalNumberNoNumber) {
		return "no-number";
	} else if (grammatical_number == GrammaticalNumberSingular) {
		return "singular";
	} else if (grammatical_number == GrammaticalNumberPlural) {
		return "plural";
	}

	return "";
}

int GetGrammaticalNumberIdByName(const std::string &grammatical_number)
{
	if (grammatical_number == "no-number") {
		return GrammaticalNumberNoNumber;
	} else if (grammatical_number == "singular") {
		return GrammaticalNumberSingular;
	} else if (grammatical_number == "plural") {
		return GrammaticalNumberPlural;
	}

	return -1;
}

std::string GetGrammaticalPersonNameById(int grammatical_person)
{
	if (grammatical_person == GrammaticalPersonFirstPerson) {
		return "first-person";
	} else if (grammatical_person == GrammaticalPersonSecondPerson) {
		return "second-person";
	} else if (grammatical_person == GrammaticalPersonThirdPerson) {
		return "third-person";
	}

	return "";
}

int GetGrammaticalPersonIdByName(const std::string &grammatical_person)
{
	if (grammatical_person == "first-person") {
		return GrammaticalPersonFirstPerson;
	} else if (grammatical_person == "second-person") {
		return GrammaticalPersonSecondPerson;
	} else if (grammatical_person == "third-person") {
		return GrammaticalPersonThirdPerson;
	}

	return -1;
}

std::string GetGrammaticalGenderNameById(int grammatical_gender)
{
	if (grammatical_gender == GrammaticalGenderNoGender) {
		return "no-gender";
	} else if (grammatical_gender == GrammaticalGenderMasculine) {
		return "masculine";
	} else if (grammatical_gender == GrammaticalGenderFeminine) {
		return "feminine";
	} else if (grammatical_gender == GrammaticalGenderNeuter) {
		return "neuter";
	}

	return "";
}

int GetGrammaticalGenderIdByName(const std::string &grammatical_gender)
{
	if (grammatical_gender == "no-gender") {
		return GrammaticalGenderNoGender;
	} else if (grammatical_gender == "masculine") {
		return GrammaticalGenderMasculine;
	} else if (grammatical_gender == "feminine") {
		return GrammaticalGenderFeminine;
	} else if (grammatical_gender == "neuter") {
		return GrammaticalGenderNeuter;
	}

	return -1;
}

std::string GetGrammaticalTenseNameById(int grammatical_tense)
{
	if (grammatical_tense == GrammaticalTenseNoTense) {
		return "no-tense";
	} else if (grammatical_tense == GrammaticalTensePresent) {
		return "present";
	} else if (grammatical_tense == GrammaticalTensePast) {
		return "past";
	} else if (grammatical_tense == GrammaticalTenseFuture) {
		return "future";
	}

	return "";
}

int GetGrammaticalTenseIdByName(const std::string &grammatical_tense)
{
	if (grammatical_tense == "no-tense") {
		return GrammaticalTenseNoTense;
	} else if (grammatical_tense == "present") {
		return GrammaticalTensePresent;
	} else if (grammatical_tense == "past") {
		return GrammaticalTensePast;
	} else if (grammatical_tense == "future") {
		return GrammaticalTenseFuture;
	}

	return -1;
}

std::string GetGrammaticalMoodNameById(int grammatical_mood)
{
	if (grammatical_mood == GrammaticalMoodIndicative) {
		return "indicative";
	} else if (grammatical_mood == GrammaticalMoodSubjunctive) {
		return "subjunctive";
	}

	return "";
}

int GetGrammaticalMoodIdByName(const std::string &grammatical_mood)
{
	if (grammatical_mood == "indicative") {
		return GrammaticalMoodIndicative;
	} else if (grammatical_mood == "subjunctive") {
		return GrammaticalMoodSubjunctive;
	}

	return -1;
}

std::string GetComparisonDegreeNameById(int comparison_degree)
{
	if (comparison_degree == ComparisonDegreePositive) {
		return "positive";
	} else if (comparison_degree == ComparisonDegreeComparative) {
		return "comparative";
	} else if (comparison_degree == ComparisonDegreeSuperlative) {
		return "superlative";
	}

	return "";
}

int GetComparisonDegreeIdByName(const std::string &comparison_degree)
{
	if (comparison_degree == "positive") {
		return ComparisonDegreePositive;
	} else if (comparison_degree == "comparative") {
		return ComparisonDegreeComparative;
	} else if (comparison_degree == "superlative") {
		return ComparisonDegreeSuperlative;
	}

	return -1;
}

std::string GetAffixTypeNameById(int affix_type)
{
	if (affix_type == AffixTypePrefix) {
		return "prefix";
	} else if (affix_type == AffixTypeSuffix) {
		return "suffix";
	} else if (affix_type == AffixTypeInfix) {
		return "infix";
	}

	return "";
}

int GetAffixTypeIdByName(const std::string &affix_type)
{
	if (affix_type == "prefix") {
		return AffixTypePrefix;
	} else if (affix_type == "suffix") {
		return AffixTypeSuffix;
	} else if (affix_type == "infix") {
		return AffixTypeInfix;
	}

	return -1;
}

std::string GetWordJunctionTypeNameById(int word_junction_type)
{
	if (word_junction_type == WordJunctionTypeNoWordJunction) {
		return "no-word-junction";
	} else if (word_junction_type == WordJunctionTypeCompound) {
		return "compound";
	} else if (word_junction_type == WordJunctionTypeSeparate) {
		return "separate";
	}

	return "";
}

int GetWordJunctionTypeIdByName(const std::string &word_junction_type)
{
	if (word_junction_type == "no-word-junction") {
		return WordJunctionTypeNoWordJunction;
	} else if (word_junction_type == "compound") {
		return WordJunctionTypeCompound;
	} else if (word_junction_type == "separate") {
		return WordJunctionTypeSeparate;
	}

	return -1;
}
