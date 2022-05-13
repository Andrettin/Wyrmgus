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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "language/language.h"

struct lua_State;

extern int CclDefineLanguageWord(lua_State *l);

namespace wyrmgus {

class language;
enum class grammatical_gender;
enum class word_type;

class word final : public named_data_entry, public data_type<word>
{
	Q_OBJECT

	Q_PROPERTY(std::string anglicized_name MEMBER anglicized_name)
	Q_PROPERTY(wyrmgus::language* language MEMBER language WRITE set_language)
	Q_PROPERTY(wyrmgus::word_type type MEMBER type READ get_type)
	Q_PROPERTY(wyrmgus::grammatical_gender gender MEMBER gender READ get_gender)
	Q_PROPERTY(wyrmgus::word* etymon READ get_etymon WRITE set_etymon)
	Q_PROPERTY(QStringList meanings READ get_meanings_qstring_list)

public:
	static constexpr const char *class_identifier = "word";
	static constexpr const char *database_folder = "words";

	static bool compare(const word *lhs, const word *rhs);

	explicit word(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	virtual void check() const override
	{
		if (this->get_language() == nullptr) {
			throw std::runtime_error("Word \"" + this->get_identifier() + "\" has not been assigned to any language.");
		}

		if (this->get_etymon() != nullptr && !this->compound_elements.empty()) {
			throw std::runtime_error("Word \"" + this->get_identifier() + "\" has both an etymon and compound elements.");
		}
	}

	virtual bool has_encyclopedia_entry() const override
	{
		if (!this->get_meanings().empty()) {
			return true;
		}

		if (this->get_etymon() != nullptr) {
			return true;
		}

		if (!this->get_reflexes().empty()) {
			return true;
		}

		if (!this->compound_elements.empty()) {
			return true;
		}

		if (!this->compound_element_of.empty()) {
			return true;
		}

		return false;
	}

	virtual std::string get_encyclopedia_text() const override;

	const std::string &get_anglicized_name() const
	{
		if (!this->anglicized_name.empty()) {
			return this->anglicized_name;
		}

		return this->get_name();
	}

	const wyrmgus::language *get_language() const
	{
		return this->language;
	}

	void set_language(wyrmgus::language *language);

	word_type get_type() const
	{
		return this->type;
	}

	grammatical_gender get_gender() const
	{
		return this->gender;
	}

	word *get_etymon() const
	{
		return this->etymon;
	}

	void set_etymon(word *etymon)
	{
		if (etymon == this->get_etymon()) {
			return;
		}

		this->etymon = etymon;
		etymon->reflexes.push_back(this);
	}

	const std::vector<const word *> &get_reflexes() const
	{
		return this->reflexes;
	}

	const std::vector<std::string> &get_meanings() const
	{
		return this->meanings;
	}

	QStringList get_meanings_qstring_list() const;

	Q_INVOKABLE void add_meaning(const std::string &meaning)
	{
		this->meanings.push_back(meaning);
	}

	Q_INVOKABLE void remove_meaning(const std::string &meaning);

	void add_compound_element(word *word)
	{
		this->compound_elements.push_back(word);
		word->compound_element_of.push_back(this);
	}

	std::string GetNounInflection(int grammatical_number, int grammatical_case, int word_junction_type = -1);
	const std::string &GetVerbInflection(int grammatical_number, int grammatical_person, int grammatical_tense, int grammatical_mood);
	std::string GetAdjectiveInflection(int comparison_degree, int article_type, int grammatical_case, int grammatical_number, const grammatical_gender grammatical_gender);
	const std::string &GetParticiple(int grammatical_tense);

private:
	std::string anglicized_name;
	wyrmgus::language *language = nullptr;
	word_type type;
	grammatical_gender gender; //what is the gender of the word, if it is a noun or article
public:
	int GrammaticalNumber = -1;			/// Grammatical number (i.e. whether the word is necessarily plural or not)
	bool Archaic = false;				/// Whether the word is archaic (whether it is used in current speech)
private:
	word *etymon = nullptr; //the word from which this one derives
	std::vector<const word *> reflexes; //words derived from this one
public:
	std::map<std::tuple<int, int>, std::string> NumberCaseInflections;	/// For nouns, mapped to grammatical number and grammatical case
	std::map<std::tuple<int, int, int, int>, std::string> NumberPersonTenseMoodInflections;	/// For verbs, mapped to grammatical number, grammatical person, grammatical tense and grammatical mood
	std::string ComparisonDegreeCaseInflections[MaxComparisonDegrees][MaxGrammaticalCases];	/// For adjectives
	std::string Participles[MaxGrammaticalTenses];		/// For verbs
private:
	std::vector<std::string> meanings; //meanings of the word in English
	std::vector<const word *> compound_elements; //from which compound elements is this word formed
	std::vector<const word *> compound_element_of; //which words are formed from this word as a compound element

public:
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

	friend int ::CclDefineLanguageWord(lua_State *l);
};

}
