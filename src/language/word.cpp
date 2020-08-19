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

#include "language/word.h"

#include "language/word_type.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

word::word(const std::string &identifier) : named_data_entry(identifier), type(word_type::none)
{
}

void word::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "compound_elements") {
		for (const std::string &value : values) {
			word *other_word = word::get(value);
			this->add_compound_element(other_word);
		}
	} else {
		data_entry::process_sml_scope(scope);
	}
}

QStringList word::get_meanings_qstring_list() const
{
	return container::to_qstring_list(this->get_meanings());
}

void word::remove_meaning(const std::string &meaning)
{
	vector::remove_one(this->meanings, meaning);
}

std::string word::GetNounInflection(int grammatical_number, int grammatical_case, int word_junction_type)
{
	if (this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case)) != this->NumberCaseInflections.end()) {
		return this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case))->second;
	}

	return this->get_name() + this->language->GetNounEnding(grammatical_number, grammatical_case, word_junction_type);
}

std::string word::GetVerbInflection(int grammatical_number, int grammatical_person, int grammatical_tense, int grammatical_mood)
{
	if (this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood)) != this->NumberPersonTenseMoodInflections.end()) {
		return this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood))->second;
	}

	return this->get_name();
}

std::string word::GetAdjectiveInflection(int comparison_degree, int article_type, int grammatical_case, int grammatical_number, int grammatical_gender)
{
	std::string inflected_word;

	if (grammatical_case == -1) {
		grammatical_case = GrammaticalCaseNoCase;
	}

	if (!this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case].empty()) {
		inflected_word = this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case];
	} else if (!this->ComparisonDegreeCaseInflections[comparison_degree][GrammaticalCaseNoCase].empty()) {
		inflected_word = this->ComparisonDegreeCaseInflections[comparison_degree][GrammaticalCaseNoCase];
	} else {
		inflected_word = this->get_name();
	}

	if (article_type != -1 && grammatical_case != GrammaticalCaseNoCase && this->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case].empty()) {
		inflected_word += this->language->GetAdjectiveEnding(article_type, grammatical_case, grammatical_number, grammatical_gender);
	}

	return inflected_word;
}

std::string word::GetParticiple(int grammatical_tense)
{
	if (!this->Participles[grammatical_tense].empty()) {
		return this->Participles[grammatical_tense];
	}

	return this->get_name();
}

void word::RemoveFromVector(std::vector<word *> &word_vector)
{
	std::vector<word *> word_vector_copy = word_vector;

	if (std::find(word_vector.begin(), word_vector.end(), this) != word_vector.end()) {
		word_vector.erase(std::remove(word_vector.begin(), word_vector.end(), this), word_vector.end());
	}

	if (word_vector.size() == 0) { // if removing the word from the vector left it empty, undo the removal
		word_vector = word_vector_copy;
	}
}

}
