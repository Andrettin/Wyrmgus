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

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CLanguageFamily::AddPersonalNameWord(CWord *word, const int gender)
{
	this->PersonalNameWords[gender].push_back(word);
	
	if (this->Family != nullptr) {
		this->Family->AddPersonalNameWord(word, gender);
	}
}

const std::vector<CWord *> &CLanguageFamily::GetPersonalNameWords(const int gender)
{
	if (!this->PersonalNameWords[gender].empty()) {
		return this->PersonalNameWords[gender];
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetPersonalNameWords(gender);
	}
	
	return this->PersonalNameWords[gender];
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
	if (!this->FamilyNameWords.empty()) {
		return this->FamilyNameWords;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetFamilyNameWords();
	}
	
	return this->FamilyNameWords;
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
	if (!this->ShipNameWords.empty()) {
		return this->ShipNameWords;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetShipNameWords();
	}
	
	return this->ShipNameWords;
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
	if (!this->SettlementNameWords.empty()) {
		return this->SettlementNameWords;
	}
	
	if (this->Family != nullptr) {
		return this->Family->GetSettlementNameWords();
	}
	
	return this->SettlementNameWords;
}

void CLanguageFamily::_bind_methods()
{
	BIND_PROPERTIES();
}
