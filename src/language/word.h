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

class CGrammaticalGender;
class CLanguage;
class CWordType;
struct lua_State;

/*----------------------------------------------------------------------------
--  Enumerations
----------------------------------------------------------------------------*/

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
	DATA_TYPE(CWord, DataElement)
	
public:
	static constexpr const char *ClassIdentifier = "word";
	
private:
	static inline bool InitializeClass()
	{
		REGISTER_PROPERTY(AnglicizedName);
		REGISTER_PROPERTY(DerivesFrom);
		REGISTER_PROPERTY(Gender);
		REGISTER_PROPERTY(Language);
		REGISTER_PROPERTY(Meanings);
		REGISTER_PROPERTY(Type);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();

public:
	virtual void Initialize() override;
	bool HasMeaning(const String &meaning);
	String GetNounInflection(const int grammatical_number, const int grammatical_case, const int word_junction_type = -1);
	String GetVerbInflection(const int grammatical_number, const int grammatical_person, const int grammatical_tense, const int grammatical_mood);
	String GetAdjectiveInflection(const int comparison_degree, const int article_type = -1, int grammatical_case = -1, const int grammatical_number = -1, const CGrammaticalGender *grammatical_gender = nullptr);
	String GetParticiple(int grammatical_tense);
	void RemoveFromVector(std::vector<CWord *> &word_vector);
	
private:
	void SetLanguage(CLanguage *language);

public:
	Property<CLanguage *> Language {					/// the language the word belongs to
		Property<CLanguage *>::ValueType(nullptr),
		Property<CLanguage *>::SetterType([this](CLanguage *language) {
			this->SetLanguage(language);
		})
	};
	Property<String> AnglicizedName;				/// the anglicized version of the word
	ExposedProperty<const CWordType *> Type = nullptr;	/// word type
	ExposedProperty<const CGrammaticalGender *> Gender = nullptr;		/// what is the gender of the noun or article (Masculine, Feminine or Neuter)
	int GrammaticalNumber = -1;						/// grammatical number (i.e. whether the word is necessarily plural or not)
	bool Archaic = false;							/// whether the word is archaic (whether it is used in current speech)
	std::map<std::tuple<int, int>, String> NumberCaseInflections;	/// for nouns, mapped to grammatical number and grammatical case
	std::map<std::tuple<int, int, int, int>, String> NumberPersonTenseMoodInflections;	/// for verbs, mapped to grammatical number, grammatical person, grammatical tense and grammatical mood
	String ComparisonDegreeCaseInflections[MaxComparisonDegrees][MaxGrammaticalCases];	/// for adjectives
	String Participles[MaxGrammaticalTenses];	/// for verbs
	ExposedProperty<std::vector<String>> Meanings;		/// meanings of the word in English
	ExposedProperty<CWord *> DerivesFrom = nullptr;    	/// from which word does this word derive
	std::vector<CWord *> DerivesTo;				/// which words derive from this word
	CWord *CompoundElements[MaxAffixTypes];    	/// from which compound elements is this word formed
	std::vector<CWord *> CompoundElementOf[MaxAffixTypes];	/// which words are formed from this word as a compound element
	
	// noun-specific variables
	bool Uncountable = false;		/// whether the noun is uncountable or not.
	
	//pronoun and article-specific variables
	String Nominative;				/// nominative case for the pronoun (if any)
	String Accusative;				/// accusative case for the pronoun (if any)
	String Dative;					/// dative case for the pronoun (if any)
	String Genitive;				/// genitive case for the pronoun (if any)
	
	//article-specific variables
	int ArticleType = -1;			/// which article type this article belongs to
	
	//numeral-specific variables
	int Number = -1;
	
	String Mod;						/// to which mod (or map), if any, this word belongs
	
	friend int CclDefineLanguageWord(lua_State *l);

protected:
	static void _bind_methods();
};

#endif
