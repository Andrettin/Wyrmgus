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

class CDependency;
class CGender;
class CGrammaticalGender;
class CLanguage;
class CSpecies;
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
	static constexpr int MinimumWordsForNameGeneration = 10;	/// the minimum quantity of words desired for name generation
	
public:
	static void Clear();
	
	static const std::vector<CWord *> &GetPersonalNameWords(const CGender *gender);
	static const std::vector<CWord *> &GetFamilyNameWords();
	static const std::vector<CWord *> &GetSpecimenNameWords(const CSpecies *species, const CGender *gender);
	static const std::vector<CWord *> &GetShipNameWords();
	static const std::vector<CWord *> &GetSettlementNameWords();

private:
	static std::map<const CGender *, std::vector<CWord *>> PersonalNameWords;	/// the words used for personal name generation, mapped to the gender for which they can be used
	static std::vector<CWord *> FamilyNameWords;
	static std::map<const CSpecies *, std::map<const CGender *, std::vector<CWord *>>> SpecimenNameWords;	/// the words used for specimen name generation, mapped to the species and gender for which they can be used
	static std::vector<CWord *> ShipNameWords;
	static std::vector<CWord *> SettlementNameWords;

public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value);
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	
	virtual void Initialize() override;
	
	const String &GetAnglicizedName() const
	{
		if (!this->AnglicizedName.empty()) {
			return this->AnglicizedName;
		} else {
			return this->GetName();
		}
	}
	
	const CLanguage *GetLanguage() const
	{
		return this->Language;
	}
	
	const CWordType *GetType() const
	{
		return this->Type;
	}
	
	const CGrammaticalGender *GetGender() const
	{
		return this->Gender;
	}
	
	const std::vector<String> &GetMeanings() const
	{
		return this->Meanings;
	}
	
	CWord *GetDerivesFrom() const
	{
		return this->DerivesFrom;
	}
	
	void ChangePersonalNameWeight(const CGender *gender, const int change);
	
	int GetPersonalNameWeight(const CGender *gender) const
	{
		std::map<const CGender *, int>::const_iterator find_iterator = this->PersonalNameWeights.find(gender);
		if (find_iterator != this->PersonalNameWeights.end()) {
			return find_iterator->second;
		}
		
		return 0;
	}
	
	void ChangeFamilyNameWeight(const int change)
	{
		if (this->FamilyNameWeight != 0) {
			this->FamilyNameWeight += change;
		} else {
			fprintf(stderr, "Tried to increase family name weight for word \"%s\", but the word is not set to be a family name.\n", this->GetIdent().utf8().get_data());
		}
	}
	
	int GetFamilyNameWeight() const
	{
		return this->FamilyNameWeight;
	}
	
	void ChangeSpecimenNameWeight(const CSpecies *species, const CGender *gender, const int change);
	
	int GetSpecimenNameWeight(const CSpecies *species, const CGender *gender) const
	{
		std::map<const CSpecies *, std::map<const CGender *, int>>::const_iterator find_iterator = this->SpecimenNameWeights.find(species);
		if (find_iterator != this->SpecimenNameWeights.end()) {
			std::map<const CGender *, int>::const_iterator sub_find_iterator = find_iterator->second.find(gender);
			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}
		
		return 0;
	}
	
	int GetShipNameWeight() const
	{
		return this->ShipNameWeight;
	}
	
	int GetSettlementNameWeight() const
	{
		return this->SettlementNameWeight;
	}
	
	String GetNounInflection(const int grammatical_number, const int grammatical_case);
	String GetVerbInflection(const int grammatical_number, const int grammatical_person, const int grammatical_tense, const int grammatical_mood);
	String GetAdjectiveInflection(const int comparison_degree, const int article_type = -1, int grammatical_case = -1, const int grammatical_number = -1, const CGrammaticalGender *grammatical_gender = nullptr);
	String GetParticiple(int grammatical_tense);
	
	bool IsProperName() const
	{
		return !this->PersonalNameWeights.empty() || this->FamilyNameWeight != 0 || !this->SpecimenNameWeights.empty() || this->ShipNameWeight != 0 || this->SettlementNameWeight != 0;
	}

private:
	CLanguage *Language = nullptr;		/// the language the word belongs to
	String AnglicizedName;				/// the anglicized version of the word
	const CWordType *Type = nullptr;	/// word type
	const CGrammaticalGender *Gender = nullptr;		/// the grammatical gender of the noun or article
public:
	int GrammaticalNumber = -1;						/// grammatical number (i.e. whether the word is necessarily plural or not)
	bool Archaic = false;							/// whether the word is archaic (whether it is used in current speech)
	std::map<std::tuple<int, int>, String> NumberCaseInflections;	/// for nouns, mapped to grammatical number and grammatical case
	std::map<std::tuple<int, int, int, int>, String> NumberPersonTenseMoodInflections;	/// for verbs, mapped to grammatical number, grammatical person, grammatical tense and grammatical mood
	String ComparisonDegreeCaseInflections[MaxComparisonDegrees][MaxGrammaticalCases];	/// for adjectives
	String Participles[MaxGrammaticalTenses];	/// for verbs
private:
	std::vector<String> Meanings;		/// meanings of the word in English
	CWord *DerivesFrom = nullptr;		/// from which word does this word derive
public:
	std::vector<CWord *> DerivesTo;				/// which words derive from this word
	CWord *CompoundElements[MaxAffixTypes];    	/// from which compound elements is this word formed
	std::vector<CWord *> CompoundElementOf[MaxAffixTypes];	/// which words are formed from this word as a compound element
	
	// noun-specific variables
	bool Uncountable = false;		/// whether the noun is uncountable or not
	
	//pronoun and article-specific variables
	String Nominative;				/// nominative case for the pronoun (if any)
	String Accusative;				/// accusative case for the pronoun (if any)
	String Dative;					/// dative case for the pronoun (if any)
	String Genitive;				/// genitive case for the pronoun (if any)
	
	//article-specific variables
	int ArticleType = -1;			/// which article type this article belongs to
	
	//numeral-specific variables
	int Number = -1;
	
private:
	std::map<const CGender *, int> PersonalNameWeights;	/// the weight of this word for personal name generation, mapped to each possible gender for the name generation
	int FamilyNameWeight = 0;
	std::map<const CSpecies *, std::map<const CGender *, int>> SpecimenNameWeights;	/// the weight of this word for name generation for species individuals, mapped to each possible gender for the name generation
	int ShipNameWeight = 0;					/// the weight of this word for ship name generation
	int SettlementNameWeight = 0;
	
public:
	CDependency *Predependency = nullptr;	/// the predependency for the word to be used as a personal name
	CDependency *Dependency = nullptr;		/// the dependency for the word to be used as a personal name
	
	String Mod;						/// to which mod (or map), if any, this word belongs
	
	friend int CclDefineLanguageWord(lua_State *l);

protected:
	static void _bind_methods();
};

#endif
