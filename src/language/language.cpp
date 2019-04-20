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

#include "language/grammatical_gender.h"
#include "language/language_family.h"
#include "language/word.h"
#include "language/word_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CWord *CLanguage::GetWord(const String &name, const CWordType *word_type, const std::vector<String> &word_meanings) const
{
	for (CWord *word : this->Words) {
		if (
			word->Name == name
			&& (word_type == nullptr || word->Type == word_type)
			&& (word_meanings.size() == 0 || word->Meanings == word_meanings)
		) {
			return word;
		}
	}

	return nullptr;
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
String CLanguage::TranslateName(const String &name) const
{
	String new_name;
	
	if (name.empty()) {
		return new_name;
	}

	// try to translate the entire name, as a particular translation for it may exist
	if (this->NameTranslations.find(name) != this->NameTranslations.end()) {
		return this->NameTranslations.find(name)->second[SyncRand(this->NameTranslations.find(name)->second.size())];
	}
	
	//if adapting the entire name failed, try to match prefixes and suffixes
	if (name.size() > 1) {
		if (name.find(" ") == -1) {
			for (size_t i = 0; i < name.size(); ++i) {
				String name_prefix = name.substr(0, i + 1);
				String name_suffix = CapitalizeString(name.substr(i + 1, name.size() - (i + 1)).utf8().get_data()).c_str();
			
	//			fprintf(stdout, "Trying to match prefix \"%s\" and suffix \"%s\" for translating name \"%s\" to the \"%s\" language.\n", name_prefix.c_str(), name_suffix.c_str(), name.c_str(), this->Ident.c_str());
			
				if (this->NameTranslations.find(name_prefix) != this->NameTranslations.end() && this->NameTranslations.find(name_suffix) != this->NameTranslations.end()) { // if both a prefix and suffix have been matched
					name_prefix = this->NameTranslations.find(name_prefix)->second[SyncRand(this->NameTranslations.find(name_prefix)->second.size())];
					name_suffix = this->NameTranslations.find(name_suffix)->second[SyncRand(this->NameTranslations.find(name_suffix)->second.size())];
					name_suffix = DecapitalizeString(name_suffix.utf8().get_data()).c_str();
					if (name_prefix.substr(name_prefix.size() - 2, 2) == "gs" && name_suffix.substr(0, 1) == "g") { //if the last two characters of the prefix are "gs", and the first character of the suffix is "g", then remove the final "s" from the prefix (as in "Königgrätz")
						name_prefix = FindAndReplaceStringEnding(name_prefix.utf8().get_data(), "gs", "g").c_str();
					}
					if (name_prefix.substr(name_prefix.size() - 1, 1) == "s" && name_suffix.substr(0, 1) == "s") { //if the prefix ends in "s" and the suffix begins in "s" as well, then remove the final "s" from the prefix (as in "Josefstadt", "Kronstadt" and "Leopoldstadt")
						name_prefix = FindAndReplaceStringEnding(name_prefix.utf8().get_data(), "s", "").c_str();
					}

					return name_prefix + name_suffix;
				}
			}
		} else { // if the name contains a space, try to translate each of its elements separately
			size_t previous_pos = 0;
			new_name = name;
			for (size_t i = 0; i < name.size(); ++i) {
				if ((i + 1) == name.size() || name[i + 1] == ' ') {
					String name_element = this->TranslateName(name.substr(previous_pos, i + 1 - previous_pos));
				
					if (name_element.empty()) {
						new_name = "";
						break;
					}
				
					new_name = FindAndReplaceString(new_name.utf8().get_data(), name.substr(previous_pos, i + 1 - previous_pos).utf8().get_data(), name_element.utf8().get_data()).c_str();
				
					previous_pos = i + 2;
				}
			}
		}
	}
	
	return new_name;
}

void CLanguage::_bind_methods()
{
	BIND_PROPERTIES();
}
