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
/**@name word.cpp - The word source file. */
//
//      (c) Copyright 2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "language/word.h"

#include "language/language.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Set the language of the word
**
**	@param	language	The language to be set
*/
void CWord::SetLanguage(CLanguage *language)
{
	if (this->Language != nullptr) {
		this->Language->Words.erase(std::remove(this->Language->Words.begin(), this->Language->Words.end(), this), this->Language->Words.end());
		for (CLanguage *dialect : this->Language->Dialects) {
			dialect->Words.erase(std::remove(dialect->Words.begin(), dialect->Words.end(), this), dialect->Words.end());
		}
	}
	
	*this->Language.Value = language;
	
	if (this->Language != nullptr) {
		this->Language->Words.push_back(this);
		for (CLanguage *dialect : this->Language->Dialects) {
			dialect->Words.push_back(this); //copy the word over for dialects
		}
	}
}

bool CWord::HasMeaning(const String &meaning)
{
	return std::find(this->Meanings.begin(), this->Meanings.end(), meaning) != this->Meanings.end();
}

String CWord::GetNounInflection(const int grammatical_number, const int grammatical_case, const int word_junction_type)
{
	if (this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case)) != this->NumberCaseInflections.end()) {
		return this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case))->second;
	}
	
	return this->Name + this->Language->GetNounEnding(grammatical_number, grammatical_case, word_junction_type);
}

String CWord::GetVerbInflection(const int grammatical_number, const int grammatical_person, const int grammatical_tense, const int grammatical_mood)
{
	if (this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood)) != this->NumberPersonTenseMoodInflections.end()) {
		return this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood))->second;
	}

	return this->Name;
}

String CWord::GetAdjectiveInflection(const int comparison_degree, const int article_type, int grammatical_case, const int grammatical_number, const int grammatical_gender)
{
	String inflected_word;
	
	if (grammatical_case == -1) {
		grammatical_case = GrammaticalCaseNoCase;
	}
	
	if (!this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case].empty()) {
		inflected_word = this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case];
	} else if (!this->ComparisonDegreeCaseInflections[comparison_degree][GrammaticalCaseNoCase].empty()) {
		inflected_word = this->ComparisonDegreeCaseInflections[comparison_degree][GrammaticalCaseNoCase];
	} else {
		inflected_word = this->Name;
	}
	
	if (article_type != -1 && grammatical_case != GrammaticalCaseNoCase && this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case].empty()) {
		inflected_word += this->Language->GetAdjectiveEnding(article_type, grammatical_case, grammatical_number, grammatical_gender);
	}
	
	return inflected_word;
}

String CWord::GetParticiple(const int grammatical_tense)
{
	if (!this->Participles[grammatical_tense].empty()) {
		return this->Participles[grammatical_tense];
	}
	
	return this->Name;
}

void CWord::RemoveFromVector(std::vector<CWord *> &word_vector)
{
	std::vector<CWord *> word_vector_copy = word_vector;
	
	if (std::find(word_vector.begin(), word_vector.end(), this) != word_vector.end()) {
		word_vector.erase(std::remove(word_vector.begin(), word_vector.end(), this), word_vector.end());
	}
	
	if (word_vector.size() == 0) { // if removing the word from the vector left it empty, undo the removal
		word_vector = word_vector_copy;
	}
}

void CWord::_bind_methods()
{
	BIND_PROPERTIES();
}
