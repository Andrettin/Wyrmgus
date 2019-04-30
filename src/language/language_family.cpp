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
/**@name language_family.cpp - The language family source file. */
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

#include "language/language_family.h"
#include "language/word.h"
#include "species/gender.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CLanguageFamily::AddPersonalNameWord(CWord *word, const CGender *gender)
{
	this->PersonalNameWords[gender].push_back(word);
	
	if (this->Family != nullptr) {
		this->Family->AddPersonalNameWord(word, gender);
	}
}

const std::vector<CWord *> &CLanguageFamily::GetPersonalNameWords(const CGender *gender) const
{
	auto find_iterator = this->PersonalNameWords.find(gender);
	if (find_iterator != this->PersonalNameWords.end() && find_iterator->second.size() >= CWord::MinimumWordsForNameGeneration) {
		return find_iterator->second;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetPersonalNameWords(gender);
	}
	
	return CWord::GetPersonalNameWords(gender);
}

void CLanguageFamily::AddFamilyNameWord(CWord *word)
{
	this->FamilyNameWords.push_back(word);
	
	if (this->Family != nullptr) {
		this->Family->AddFamilyNameWord(word);
	}
}

const std::vector<CWord *> &CLanguageFamily::GetFamilyNameWords() const
{
	if (this->FamilyNameWords.size() >= CWord::MinimumWordsForNameGeneration) {
		return this->FamilyNameWords;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetFamilyNameWords();
	}
	
	return CWord::GetFamilyNameWords();
}

void CLanguageFamily::AddSpecimenNameWord(CWord *word, const CSpecies *species, const CGender *gender)
{
	this->SpecimenNameWords[species][gender].push_back(word);
	
	if (this->Family != nullptr) {
		this->Family->AddSpecimenNameWord(word, species, gender);
	}
}

const std::vector<CWord *> &CLanguageFamily::GetSpecimenNameWords(const CSpecies *species, const CGender *gender) const
{
	auto find_iterator = this->SpecimenNameWords.find(species);
	if (find_iterator != this->SpecimenNameWords.end()) {
		auto sub_find_iterator = find_iterator->second.find(gender);
		if (sub_find_iterator != find_iterator->second.end() && sub_find_iterator->second.size() >= CWord::MinimumWordsForNameGeneration) {
			return sub_find_iterator->second;
		}
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetSpecimenNameWords(species, gender);
	}
	
	return CWord::GetSpecimenNameWords(species, gender);
}

void CLanguageFamily::AddUnitNameWord(CWord *word, const UnitClass *unit_class)
{
	this->UnitNameWords[unit_class].push_back(word);
	
	if (this->Family != nullptr) {
		this->Family->AddUnitNameWord(word, unit_class);
	}
}

const std::vector<CWord *> &CLanguageFamily::GetUnitNameWords(const UnitClass *unit_class) const
{
	auto find_iterator = this->UnitNameWords.find(unit_class);
	if (find_iterator != this->UnitNameWords.end() && find_iterator->second.size() >= CWord::MinimumWordsForNameGeneration) {
		return find_iterator->second;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetUnitNameWords(unit_class);
	}
	
	return CWord::GetUnitNameWords(unit_class);
}

void CLanguageFamily::AddShipNameWord(CWord *word)
{
	this->ShipNameWords.push_back(word);
	
	if (this->Family != nullptr) {
		this->Family->AddShipNameWord(word);
	}
}

const std::vector<CWord *> &CLanguageFamily::GetShipNameWords() const
{
	if (this->ShipNameWords.size() >= CWord::MinimumWordsForNameGeneration) {
		return this->ShipNameWords;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetShipNameWords();
	}
	
	return CWord::GetShipNameWords();
}

void CLanguageFamily::AddSettlementNameWord(CWord *word)
{
	this->SettlementNameWords.push_back(word);
	
	if (this->Family != nullptr) {
		this->Family->AddSettlementNameWord(word);
	}
}

const std::vector<CWord *> &CLanguageFamily::GetSettlementNameWords() const
{
	if (this->SettlementNameWords.size() >= CWord::MinimumWordsForNameGeneration) {
		return this->SettlementNameWords;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetSettlementNameWords();
	}
	
	return CWord::GetSettlementNameWords();
}

void CLanguageFamily::_bind_methods()
{
	BIND_PROPERTIES();
}
