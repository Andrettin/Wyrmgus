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
#include "species/species.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
	if (key == "family_name") {
		const bool value_bool = StringToBool(value);
		
		if (value_bool) {
			this->FamilyNameWeight = 1;
			this->Language->AddFamilyNameWord(this);
		}
	} else if (key == "ship_name") {
		const bool value_bool = StringToBool(value);
		
		if (value_bool) {
			this->ShipNameWeight = 1;
			this->Language->AddShipNameWord(this);
		}
	} else if (key == "settlement_name") {
		const bool value_bool = StringToBool(value);
		
		if (value_bool) {
			this->SettlementNameWeight = 1;
			this->Language->AddSettlementNameWord(this);
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
			const int gender_index = GetGenderIdByName(key);
			
			if (gender_index == -1) {
				fprintf(stderr, "Invalid gender: \"%s\".\n", key.c_str());
				continue;
			}
			
			const bool value_bool = StringToBool(property.Value);
			if (value_bool) {
				this->PersonalNameWeights[gender_index] = 1;
				this->Language->AddPersonalNameWord(this, gender_index);
			}
		}
	} else if (section->Tag == "specimen_name") {
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
				const int gender_index = GetGenderIdByName(key);
				
				if (gender_index == -1) {
					fprintf(stderr, "Invalid gender: \"%s\".\n", key.c_str());
					continue;
				}
				
				const bool value_bool = StringToBool(property.Value);
				if (value_bool) {
					this->SpecimenNameWeights[species][gender_index] = 1;
					this->Language->AddSpecimenNameWord(this, species, gender_index);
					species->AddSpecimenNameWord(this, gender_index);
				}
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
	
	if (this->Type == nullptr && !this->IsProperName()) {
		fprintf(stderr, "Word \"%s\" has no type.\n", this->GetIdent().utf8().get_data());
	}
	
	this->Initialized = true;
}
	
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
	
	this->Language.Value = language;
	
	if (this->Language != nullptr) {
		this->Language->Words.push_back(this);
		for (CLanguage *dialect : this->Language->Dialects) {
			dialect->Words.push_back(this); //copy the word over for dialects
		}
	}
}

void CWord::ChangePersonalNameWeight(const int gender, const int change)
{
	std::map<int, int>::iterator find_iterator = this->PersonalNameWeights.find(gender);
	if (find_iterator != this->PersonalNameWeights.end()) {
		find_iterator->second += change;
	} else {
		fprintf(stderr, "Tried to increase personal name weight for gender \"%s\" for word \"%s\", but the word is not set to be a personal name for that gender.\n", GetGenderNameById(gender).c_str(), this->GetIdent().utf8().get_data());
	}
}

void CWord::ChangeSpecimenNameWeight(const CSpecies *species, const int gender, const int change)
{
	std::map<const CSpecies *, std::map<int, int>>::iterator find_iterator = this->SpecimenNameWeights.find(species);
	if (find_iterator != this->SpecimenNameWeights.end()) {
		std::map<int, int>::iterator sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end()) {
			sub_find_iterator->second += change;
		} else {
			fprintf(stderr, "Tried to increase personal name weight for species \"%s\" and gender \"%s\" for word \"%s\", but the word is not set to be a specimen name for that species and gender combination.\n", species->GetIdent().utf8().get_data(), GetGenderNameById(gender).c_str(), this->GetIdent().utf8().get_data());
		}
	} else {
		fprintf(stderr, "Tried to increase personal name weight for species \"%s\" for word \"%s\", but the word is not set to be a specimen name for that species.\n", species->GetIdent().utf8().get_data(), this->GetIdent().utf8().get_data());
	}
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
	BIND_PROPERTIES();
	
	ClassDB::bind_method(D_METHOD("set_anglicized_name", "anglicized_name"), [](CWord *word, const String &anglicized_name){ word->AnglicizedName = anglicized_name; });
	ClassDB::bind_method(D_METHOD("get_anglicized_name"), &CWord::GetAnglicizedName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "anglicized_name"), "set_anglicized_name", "get_anglicized_name");
	
	ClassDB::bind_method(D_METHOD("set_type", "type_ident"), [](CWord *word, const String &type_ident){ word->Type = CWordType::Get(type_ident); });
	ClassDB::bind_method(D_METHOD("get_type"), [](const CWord *word){ return const_cast<CWordType *>(word->GetType()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "type"), "set_type", "get_type");
	
	ClassDB::bind_method(D_METHOD("set_gender", "gender_ident"), [](CWord *word, const String &gender_ident){ word->Gender = CGrammaticalGender::Get(gender_ident); });
	ClassDB::bind_method(D_METHOD("get_gender"), [](const CWord *word){ return const_cast<CGrammaticalGender *>(word->GetGender()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gender"), "set_gender", "get_gender");
	
	ClassDB::bind_method(D_METHOD("get_derives_to"), [](const CWord *word){ return VectorToGodotArray(word->DerivesTo); });
	
	ClassDB::bind_method(D_METHOD("is_personal_name"), [](const CWord *word){ return word->PersonalNameWeights.empty() == false; });
	ClassDB::bind_method(D_METHOD("is_family_name"), [](const CWord *word){ return word->FamilyNameWeight == 0; });
	ClassDB::bind_method(D_METHOD("get_specimen_name_species"), [](const CWord *word){
		Array specimen_name_species;
		for (const auto &element : word->SpecimenNameWeights) {
			specimen_name_species.push_back(const_cast<CSpecies *>(element.first));
		}
		return specimen_name_species;
	});
	ClassDB::bind_method(D_METHOD("is_ship_name"), [](const CWord *word){ return word->ShipNameWeight == 0; });
	ClassDB::bind_method(D_METHOD("is_settlement_name"), [](const CWord *word){ return word->SettlementNameWeight == 0; });
}
