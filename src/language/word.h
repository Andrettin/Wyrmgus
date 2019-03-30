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
/**@name word.h - The word header file. */
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

#ifndef __WORD_H__
#define __WORD_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CLanguage;
struct lua_State;

/*----------------------------------------------------------------------------
--  Enumerations
----------------------------------------------------------------------------*/

enum WordTypes {
	WordTypeNoun,
	WordTypeVerb,
	WordTypeAdjective,
	WordTypePronoun,
	WordTypeAdverb,
	WordTypeConjunction,
	WordTypeAdposition,
	WordTypeArticle,
	WordTypeNumeral,
	WordTypeAffix,
	
	MaxWordTypes
};

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

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CWord : public DataElement, public DataType<CWord>
{
	GDCLASS(CWord, DataElement)
	
public:
	static constexpr const char *GetClassIdentifier()
	{
		return "word";
	}
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	const String &GetName() const
	{
		return this->Name;
	}
	
	CLanguage *GetLanguage() const
	{
		return this->Language;
	}
	
	bool HasMeaning(const String &meaning);
	String GetNounInflection(const int grammatical_number, const int grammatical_case, const int word_junction_type = -1);
	String GetVerbInflection(const int grammatical_number, const int grammatical_person, const int grammatical_tense, const int grammatical_mood);
	String GetAdjectiveInflection(const int comparison_degree, const int article_type = -1, int grammatical_case = -1, const int grammatical_number = -1, const int grammatical_gender = -1);
	String GetParticiple(int grammatical_tense);
	void RemoveFromVector(std::vector<CWord *> &word_vector);

private:
	String Name;										/// the name of the word
	CLanguage *Language = nullptr;						/// The language the word belongs to
	
public:
	int Type = -1;										/// Word type
	int Gender = -1;									/// What is the gender of the noun or article (Masculine, Feminine or Neuter)
	int GrammaticalNumber = -1;							/// Grammatical number (i.e. whether the word is necessarily plural or not)
	bool Archaic = false;								/// Whether the word is archaic (whether it is used in current speech)
	std::map<std::tuple<int, int>, String> NumberCaseInflections;	/// For nouns, mapped to grammatical number and grammatical case
	std::map<std::tuple<int, int, int, int>, String> NumberPersonTenseMoodInflections;	/// For verbs, mapped to grammatical number, grammatical person, grammatical tense and grammatical mood
	String ComparisonDegreeCaseInflections[MaxComparisonDegrees][MaxGrammaticalCases];	/// For adjectives
	String Participles[MaxGrammaticalTenses];	/// For verbs
	std::vector<String> Meanings;				/// Meanings of the word in English.
	CWord *DerivesFrom = nullptr;    			/// From which word does this word derive
	std::vector<CWord *> DerivesTo;				/// Which words derive from this word
	CWord *CompoundElements[MaxAffixTypes];    	/// From which compound elements is this word formed
	std::vector<CWord *> CompoundElementOf[MaxAffixTypes];	/// Which words are formed from this word as a compound element
	
	// noun-specific variables
	bool Uncountable = false;		/// Whether the noun is uncountable or not.
	
	//pronoun and article-specific variables
	String Nominative;				/// Nominative case for the pronoun (if any)
	String Accusative;				/// Accusative case for the pronoun (if any)
	String Dative;					/// Dative case for the pronoun (if any)
	String Genitive;				/// Genitive case for the pronoun (if any)
	
	//article-specific variables
	int ArticleType = -1;			/// Which article type this article belongs to
	
	//numeral-specific variables
	int Number = -1;
	
	String Mod;						/// To which mod (or map), if any, this word belongs
	
	friend int CclDefineLanguageWord(lua_State *l);

protected:
	static void _bind_methods();
};

#endif
