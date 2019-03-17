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
/**@name language.cpp - The language source file. */
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

#include "language/language.h"

#include "config.h"
#include "language/word.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CLanguage::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid language property: \"%s\".\n", key.c_str());
		}
	}
}

CWord *CLanguage::GetWord(const std::string &name, int word_type, std::vector<std::string>& word_meanings) const
{
	for (CWord *word : this->Words) {
		if (
			word->Name == name
			&& (word_type == -1 || word->Type == word_type)
			&& (word_meanings.size() == 0 || word->Meanings == word_meanings)
		) {
			return word;
		}
	}

	return nullptr;
}

std::string CLanguage::GetArticle(int gender, int grammatical_case, int article_type, int grammatical_number)
{
	for (const CWord *word : this->Words) {
		if (word->Type != WordTypeArticle || word->ArticleType != article_type) {
			continue;
		}
		
		if (grammatical_number != -1 && word->GrammaticalNumber != -1 && word->GrammaticalNumber != grammatical_number) {
			continue;
		}
		
		if (gender == -1 || word->Gender == -1 || gender == word->Gender) {
			if (grammatical_case == GrammaticalCaseNominative && !word->Nominative.empty()) {
				return word->Nominative;
			} else if (grammatical_case == GrammaticalCaseAccusative && !word->Accusative.empty()) {
				return word->Accusative;
			} else if (grammatical_case == GrammaticalCaseDative && !word->Dative.empty()) {
				return word->Dative;
			} else if (grammatical_case == GrammaticalCaseGenitive && !word->Genitive.empty()) {
				return word->Genitive;
			}
		}
	}
	return "";
}

std::string CLanguage::GetNounEnding(int grammatical_number, int grammatical_case, int word_junction_type)
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

std::string CLanguage::GetAdjectiveEnding(int article_type, int grammatical_case, int grammatical_number, int grammatical_gender)
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

void CLanguage::RemoveWord(CWord *word)
{
	if (std::find(this->Words.begin(), this->Words.end(), word) != this->Words.end()) {
		this->Words.erase(std::remove(this->Words.begin(), this->Words.end(), word), this->Words.end());
	}
}

/**
**  "Translate" (that is, adapt) a proper name from one culture (civilization) to another.
*/
std::string CLanguage::TranslateName(const std::string &name) const
{
	std::string new_name;
	
	if (name.empty()) {
		return new_name;
	}

	// try to translate the entire name, as a particular translation for it may exist
	if (this->NameTranslations.find(name) != this->NameTranslations.end()) {
		return this->NameTranslations.find(name)->second[SyncRand(this->NameTranslations.find(name)->second.size())];
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (name.size() > 1) {
		if (name.find(" ") == std::string::npos) {
			for (size_t i = 0; i < name.size(); ++i) {
				std::string name_prefix = name.substr(0, i + 1);
				std::string name_suffix = CapitalizeString(name.substr(i + 1, name.size() - (i + 1)));
			
	//			fprintf(stdout, "Trying to match prefix \"%s\" and suffix \"%s\" for translating name \"%s\" to the \"%s\" language.\n", name_prefix.c_str(), name_suffix.c_str(), name.c_str(), this->Ident.c_str());
			
				if (this->NameTranslations.find(name_prefix) != this->NameTranslations.end() && this->NameTranslations.find(name_suffix) != this->NameTranslations.end()) { // if both a prefix and suffix have been matched
					name_prefix = this->NameTranslations.find(name_prefix)->second[SyncRand(this->NameTranslations.find(name_prefix)->second.size())];
					name_suffix = this->NameTranslations.find(name_suffix)->second[SyncRand(this->NameTranslations.find(name_suffix)->second.size())];
					name_suffix = DecapitalizeString(name_suffix);
					if (name_prefix.substr(name_prefix.size() - 2, 2) == "gs" && name_suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
						name_prefix = FindAndReplaceStringEnding(name_prefix, "gs", "g");
					}
					if (name_prefix.substr(name_prefix.size() - 1, 1) == "s" && name_suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
						name_prefix = FindAndReplaceStringEnding(name_prefix, "s", "");
					}

					return name_prefix + name_suffix;
				}
			}
		} else { // if the name contains a space, try to translate each of its elements separately
			size_t previous_pos = 0;
			new_name = name;
			for (size_t i = 0; i < name.size(); ++i) {
				if ((i + 1) == name.size() || name[i + 1] == ' ') {
					std::string name_element = this->TranslateName(name.substr(previous_pos, i + 1 - previous_pos));
				
					if (name_element.empty()) {
						new_name = "";
						break;
					}
				
					new_name = FindAndReplaceString(new_name, name.substr(previous_pos, i + 1 - previous_pos), name_element);
				
					previous_pos = i + 2;
				}
			}
		}
	}
	
	return new_name;
}
