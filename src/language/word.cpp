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

#include "character.h"
#include "dependency/and_dependency.h"
#include "language/grammatical_gender.h"
#include "language/language.h"
#include "language/word_type.h"
#include "species/gender.h"
#include "species/species.h"
#include "species/species_category.h"
#include "unit/unit_class.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::map<const CGender *, std::vector<CWord *>> CWord::PersonalNameWords;
std::vector<CWord *> CWord::FamilyNameWords;
std::map<const CSpecies *, std::map<const CGender *, std::vector<CWord *>>> CWord::SpecimenNameWords;
std::map<const UnitClass *, std::vector<CWord *>> CWord::UnitNameWords;
std::vector<CWord *> CWord::ShipNameWords;
std::vector<CWord *> CWord::SettlementNameWords;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CWord::Clear()
{
	CWord::PersonalNameWords.clear();
	CWord::FamilyNameWords.clear();
	CWord::SpecimenNameWords.clear();
	CWord::UnitNameWords.clear();
	CWord::ShipNameWords.clear();
	CWord::SettlementNameWords.clear();

	DataType<CWord>::Clear();
}

const std::vector<CWord *> &CWord::GetPersonalNameWords(const CGender *gender)
{
	return CWord::PersonalNameWords[gender];
}

const std::vector<CWord *> &CWord::GetFamilyNameWords()
{
	return CWord::FamilyNameWords;
}

const std::vector<CWord *> &CWord::GetSpecimenNameWords(const CSpecies *species, const CGender *gender)
{
	return CWord::SpecimenNameWords[species][gender];
}

const std::vector<CWord *> &CWord::GetUnitNameWords(const UnitClass *unit_class)
{
	return CWord::UnitNameWords[unit_class];
}

const std::vector<CWord *> &CWord::GetShipNameWords()
{
	return CWord::ShipNameWords;
}

const std::vector<CWord *> &CWord::GetSettlementNameWords()
{
	return CWord::SettlementNameWords;
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CWord::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "personal_name") { //add as a personal name for all possible genders
		const bool value_bool = StringToBool(value);
		
		if (value_bool) {
			for (const CGender *gender : CGender::GetAll()) {
				this->PersonalNameWeights[gender] = 1;
				this->Language->AddPersonalNameWord(this, gender);
				CWord::PersonalNameWords[gender].push_back(this);
			}
		}
	} else if (key == "family_name") {
		const bool value_bool = StringToBool(value);
		
		if (value_bool) {
			this->FamilyNameWeight = 1;
			this->Language->AddFamilyNameWord(this);
			CWord::FamilyNameWords.push_back(this);
		}
	} else if (key == "ship_name") {
		const bool value_bool = StringToBool(value);
		
		if (value_bool) {
			this->ShipNameWeight = 1;
			this->Language->AddShipNameWord(this);
			CWord::ShipNameWords.push_back(this);
		}
	} else if (key == "settlement_name") {
		const bool value_bool = StringToBool(value);
		
		if (value_bool) {
			this->SettlementNameWeight = 1;
			this->Language->AddSettlementNameWord(this);
			CWord::SettlementNameWords.push_back(this);
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CWord::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "personal_name") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = FindAndReplaceString(property.Key, "_", "-");
			const CGender *gender = CGender::Get(key);
			
			if (gender == nullptr) {
				fprintf(stderr, "Invalid gender: \"%s\".\n", key.c_str());
				continue;
			}
			
			const bool value_bool = StringToBool(property.Value);
			if (value_bool) {
				this->PersonalNameWeights[gender] = 1;
				this->Language->AddPersonalNameWord(this, gender);
				CWord::PersonalNameWords[gender].push_back(this);
			}
		}
	} else if (section->Tag == "specimen_name") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = FindAndReplaceString(property.Key, "_", "-");
			CSpecies *species = CSpecies::Get(key);
			
			if (species == nullptr) {
				fprintf(stderr, "Invalid species: \"%s\".\n", key.c_str());
				continue;
			}
			
			const bool value_bool = StringToBool(property.Value);
			if (value_bool) {
				//add name for the species for all genders (we use all genders instead of only the species' gender itself because the name will be passed up to the species category as well, and names regardless of gender should be applied to other genders if used by a species with different genders; e.g. a species of sparrow with four genders should be able to use gender-agnostic names from other birds for all of their genders
				for (const CGender *gender : CGender::GetAll()) {
					this->SpecimenNameWeights[species][gender] = 1;
					
					const CSpeciesCategory *species_category = species->GetCategory();
					while (species_category != nullptr) {
						this->CategorySpecimenNameWeights[species_category][gender] = 1;
						species_category = species_category->GetUpperCategory();
					}
					
					this->Language->AddSpecimenNameWord(this, species, gender);
					species->AddSpecimenNameWord(this, gender);
					CWord::SpecimenNameWords[species][gender].push_back(this);
				}
			}
		}
		
		for (const CConfigData *sub_section : section->Sections) {
			CSpecies *species = CSpecies::Get(sub_section->Tag);
			
			if (species == nullptr) {
				continue;
			}
			
			for (const CConfigProperty &property : sub_section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				std::string key = FindAndReplaceString(property.Key, "_", "-");
				const CGender *gender = CGender::Get(key);
				
				if (gender == nullptr) {
					fprintf(stderr, "Invalid gender: \"%s\".\n", key.c_str());
					continue;
				}
				
				const bool value_bool = StringToBool(property.Value);
				if (value_bool) {
					this->SpecimenNameWeights[species][gender] = 1;
					
					const CSpeciesCategory *species_category = species->GetCategory();
					while (species_category != nullptr) {
						this->CategorySpecimenNameWeights[species_category][gender] = 1;
						species_category = species_category->GetUpperCategory();
					}
					
					this->Language->AddSpecimenNameWord(this, species, gender);
					species->AddSpecimenNameWord(this, gender);
					CWord::SpecimenNameWords[species][gender].push_back(this);
				}
			}
		}
	} else if (section->Tag == "unit_name") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = FindAndReplaceString(property.Key, "_", "-");
			const UnitClass *unit_class = UnitClass::Get(key);
			
			if (unit_class == nullptr) {
				fprintf(stderr, "Invalid unit class: \"%s\".\n", key.c_str());
				continue;
			}
			
			const bool value_bool = StringToBool(property.Value);
			if (value_bool) {
				this->UnitNameWeights[unit_class] = 1;
				this->Language->AddUnitNameWord(this, unit_class);
				CWord::UnitNameWords[unit_class].push_back(this);
			}
		}
	} else if (section->Tag == "dependencies") {
		this->Dependency = new CAndDependency;
		this->Dependency->ProcessConfigData(section);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the word
*/
void CWord::Initialize()
{
	if (this->Language == nullptr) {
		fprintf(stderr, "Word \"%s\" has not been assigned to any language.\n", this->GetIdent().utf8().get_data());
	}
	
	if (this->Type == nullptr && !this->Meanings.empty()) {
		fprintf(stderr, "Word \"%s\" has no type.\n", this->GetIdent().utf8().get_data());
	}
	
	this->Initialized = true;
}
	
void CWord::ChangePersonalNameWeight(const CGender *gender, const int change)
{
	std::map<const CGender *, int>::iterator find_iterator = this->PersonalNameWeights.find(gender);
	if (find_iterator != this->PersonalNameWeights.end()) {
		find_iterator->second += change;
	} else {
		fprintf(stderr, "Tried to increase personal name weight for gender \"%s\" for word \"%s\", but the word is not set to be a personal name for that gender.\n", gender->GetIdent().utf8().get_data(), this->GetIdent().utf8().get_data());
	}
}

void CWord::ChangeSpecimenNameWeight(const CSpecies *species, const CGender *gender, const int change)
{
	std::map<const CSpecies *, std::map<const CGender *, int>>::iterator find_iterator = this->SpecimenNameWeights.find(species);
	if (find_iterator != this->SpecimenNameWeights.end()) {
		std::map<const CGender *, int>::iterator sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end()) {
			sub_find_iterator->second += change;
			
			if (species->GetCategory() != nullptr) {
				this->ChangeCategorySpecimenNameWeight(species->GetCategory(), gender, change);
			}
		} else {
			fprintf(stderr, "Tried to increase personal name weight for species \"%s\" and gender \"%s\" for word \"%s\", but the word is not set to be a specimen name for that species and gender combination.\n", species->GetIdent().utf8().get_data(), gender->GetIdent().utf8().get_data(), this->GetIdent().utf8().get_data());
		}
	} else {
		fprintf(stderr, "Tried to increase personal name weight for species \"%s\" for word \"%s\", but the word is not set to be a specimen name for that species.\n", species->GetIdent().utf8().get_data(), this->GetIdent().utf8().get_data());
	}
}

int CWord::GetSpecimenNameWeight(const CSpecies *species, const CGender *gender) const
{
	std::map<const CSpecies *, std::map<const CGender *, int>>::const_iterator find_iterator = this->SpecimenNameWeights.find(species);
	if (find_iterator != this->SpecimenNameWeights.end()) {
		std::map<const CGender *, int>::const_iterator sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}
	
	if (species->GetCategory() != nullptr) {
		return this->GetCategorySpecimenNameWeight(species->GetCategory(), gender);
	}
	
	return 0;
}

void CWord::ChangeCategorySpecimenNameWeight(const CSpeciesCategory *species_category, const CGender *gender, const int change)
{
	std::map<const CSpeciesCategory *, std::map<const CGender *, int>>::iterator find_iterator = this->CategorySpecimenNameWeights.find(species_category);
	if (find_iterator != this->CategorySpecimenNameWeights.end()) {
		std::map<const CGender *, int>::iterator sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end()) {
			sub_find_iterator->second += change;
			
			if (species_category->GetUpperCategory() != nullptr) {
				this->ChangeCategorySpecimenNameWeight(species_category->GetUpperCategory(), gender, change);
			}
		} else {
			fprintf(stderr, "Tried to increase personal name weight for species category \"%s\" and gender \"%s\" for word \"%s\", but the word is not set to be a specimen name for that species category and gender combination.\n", species_category->GetIdent().utf8().get_data(), gender->GetIdent().utf8().get_data(), this->GetIdent().utf8().get_data());
		}
	} else {
		fprintf(stderr, "Tried to increase personal name weight for species category \"%s\" for word \"%s\", but the word is not set to be a specimen name for that species category.\n", species_category->GetIdent().utf8().get_data(), this->GetIdent().utf8().get_data());
	}
}

int CWord::GetCategorySpecimenNameWeight(const CSpeciesCategory *species_category, const CGender *gender) const
{
	std::map<const CSpeciesCategory *, std::map<const CGender *, int>>::const_iterator find_iterator = this->CategorySpecimenNameWeights.find(species_category);
	if (find_iterator != this->CategorySpecimenNameWeights.end()) {
		std::map<const CGender *, int>::const_iterator sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}
	
	if (species_category->GetUpperCategory() != nullptr) {
		return this->GetCategorySpecimenNameWeight(species_category->GetUpperCategory(), gender);
	}
	
	return 0;
}

String CWord::GetNounInflection(const int grammatical_number, const int grammatical_case)
{
	if (this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case)) != this->NumberCaseInflections.end()) {
		return this->NumberCaseInflections.find(std::tuple<int, int>(grammatical_number, grammatical_case))->second;
	}
	
	return this->Name;
}

String CWord::GetVerbInflection(const int grammatical_number, const int grammatical_person, const int grammatical_tense, const int grammatical_mood)
{
	if (this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood)) != this->NumberPersonTenseMoodInflections.end()) {
		return this->NumberPersonTenseMoodInflections.find(std::tuple<int, int, int, int>(grammatical_number, grammatical_person, grammatical_tense, grammatical_mood))->second;
	}

	return this->Name;
}

String CWord::GetAdjectiveInflection(const int comparison_degree, const int article_type, int grammatical_case, const int grammatical_number, const CGrammaticalGender *grammatical_gender)
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
	
	return inflected_word;
}

String CWord::GetParticiple(const int grammatical_tense)
{
	if (!this->Participles[grammatical_tense].empty()) {
		return this->Participles[grammatical_tense];
	}
	
	return this->Name;
}

void CWord::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_anglicized_name", "anglicized_name"), [](CWord *word, const String &anglicized_name){ word->AnglicizedName = anglicized_name; });
	ClassDB::bind_method(D_METHOD("get_anglicized_name"), &CWord::GetAnglicizedName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "anglicized_name"), "set_anglicized_name", "get_anglicized_name");
	
	ClassDB::bind_method(D_METHOD("set_language", "language_ident"), [](CWord *word, const String &language_ident){
		if (word->Language != nullptr) {
			word->Language->Words.erase(std::remove(word->Language->Words.begin(), word->Language->Words.end(), word), word->Language->Words.end());
			for (CLanguage *dialect : word->Language->Dialects) {
				dialect->Words.erase(std::remove(dialect->Words.begin(), dialect->Words.end(), word), dialect->Words.end());
			}
		}
		
		word->Language = CLanguage::Get(language_ident);
		
		if (word->Language != nullptr) {
			word->Language->Words.push_back(word);
			for (CLanguage *dialect : word->Language->Dialects) {
				dialect->Words.push_back(word); //copy the word over for dialects
			}
		}
	});
	ClassDB::bind_method(D_METHOD("get_language"), [](const CWord *word){ return const_cast<CLanguage *>(word->GetLanguage()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "language"), "set_language", "get_language");
	
	ClassDB::bind_method(D_METHOD("set_type", "type_ident"), [](CWord *word, const String &type_ident){ word->Type = CWordType::Get(type_ident); });
	ClassDB::bind_method(D_METHOD("get_type"), [](const CWord *word){ return const_cast<CWordType *>(word->GetType()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "type"), "set_type", "get_type");
	
	ClassDB::bind_method(D_METHOD("set_gender", "gender_ident"), [](CWord *word, const String &gender_ident){ word->Gender = CGrammaticalGender::Get(gender_ident); });
	ClassDB::bind_method(D_METHOD("get_gender"), [](const CWord *word){ return const_cast<CGrammaticalGender *>(word->GetGender()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gender"), "set_gender", "get_gender");
	
	ClassDB::bind_method(D_METHOD("set_derives_from", "derives_from_ident"), [](CWord *word, const String &derives_from_ident){
		if (word->DerivesFrom != nullptr) {
			word->DerivesFrom->DerivesTo.erase(std::remove(word->DerivesFrom->DerivesTo.begin(), word->DerivesFrom->DerivesTo.end(), word), word->DerivesFrom->DerivesTo.end());
		}
		word->DerivesFrom = word;
		if (word != nullptr) {
			word->DerivesTo.push_back(word);
		}
	});
	ClassDB::bind_method(D_METHOD("get_derives_from"), &CWord::GetDerivesFrom);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "derives_from"), "set_derives_from", "get_derives_from");
	
	ClassDB::bind_method(D_METHOD("get_derives_to"), [](const CWord *word){ return VectorToGodotArray(word->DerivesTo); });
	
	ClassDB::bind_method(D_METHOD("add_to_meanings", "meaning"), [](CWord *word, const String &meaning){ word->Meanings.push_back(meaning); });
	ClassDB::bind_method(D_METHOD("remove_from_meanings", "meaning"), [](CWord *word, const String &meaning){ word->Meanings.erase(std::remove(word->Meanings.begin(), word->Meanings.end(), meaning), word->Meanings.end()); });
	ClassDB::bind_method(D_METHOD("get_meanings"), [](const CWord *word){ return VectorToGodotArray(word->Meanings); });
	
	ClassDB::bind_method(D_METHOD("add_to_compound_elements", "compound_element"), [](CWord *word, const String &compound_element){ word->CompoundElements.push_back(CWord::Get(compound_element)); });
	ClassDB::bind_method(D_METHOD("remove_from_compound_elements", "compound_element"), [](CWord *word, const String &compound_element){ word->CompoundElements.erase(std::remove(word->CompoundElements.begin(), word->CompoundElements.end(), CWord::Get(compound_element)), word->CompoundElements.end()); });
	ClassDB::bind_method(D_METHOD("get_compound_elements"), [](const CWord *word){ return VectorToGodotArray(word->CompoundElements); });
	
	ClassDB::bind_method(D_METHOD("get_personal_name_genders"), [](const CWord *word){
		Array genders;
		for (const auto &element : word->PersonalNameWeights) {
			genders.push_back(const_cast<CGender *>(element.first));
		}
		return genders;
	});
	ClassDB::bind_method(D_METHOD("is_family_name"), [](const CWord *word){ return word->FamilyNameWeight != 0; });
	ClassDB::bind_method(D_METHOD("get_specimen_name_species"), [](const CWord *word){
		Array species;
		for (const auto &element : word->SpecimenNameWeights) {
			species.push_back(const_cast<CSpecies *>(element.first));
		}
		return species;
	});
	ClassDB::bind_method(D_METHOD("get_specimen_name_genders", "species"), [](const CWord *word, Object *species){
		Array genders;
		auto find_iterator = word->SpecimenNameWeights.find(static_cast<CSpecies *>(species));
		if (find_iterator != word->SpecimenNameWeights.end()) {
			for (const auto &element : find_iterator->second) {
				genders.push_back(const_cast<CGender *>(element.first));
			}
		}
		return genders;
	});
	ClassDB::bind_method(D_METHOD("is_ship_name"), [](const CWord *word){ return word->ShipNameWeight != 0; });
	ClassDB::bind_method(D_METHOD("is_settlement_name"), [](const CWord *word){ return word->SettlementNameWeight != 0; });
}
