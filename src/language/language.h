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
/**@name language.h - The language header file. */
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

#ifndef __LANGUAGE_H__
#define __LANGUAGE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "word.h" // for certain enums

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGender;
class CGrammaticalGender;
class CLanguageFamily;
class CSpecies;
class CWord;
class CWordType;
class UnitClass;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CLanguage : public DataElement, public DataType<CLanguage>
{
	DATA_TYPE(CLanguage, DataElement)

public:
	static constexpr const char *ClassIdentifier = "language";
	
private:
	static inline bool InitializeClass()
	{
		REGISTER_PROPERTY(DialectOf);
		REGISTER_PROPERTY(Family);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();

public:
	static void Clear();
	static const CLanguage *GetEnglishLanguage();

private:
	static const CLanguage *EnglishLanguage;

public:	
	CWord *GetWord(const String &name, const CWordType *word_type, const std::vector<String> &word_meanings) const;
	void RemoveWord(CWord *word);
	
	void AddPersonalNameWord(CWord *word, const CGender *gender);
	const std::vector<CWord *> &GetPersonalNameWords(const CGender *gender) const;
	void AddFamilyNameWord(CWord *word);
	const std::vector<CWord *> &GetFamilyNameWords() const;
	void AddSpecimenNameWord(CWord *word, const CSpecies *species, const CGender *gender);
	const std::vector<CWord *> &GetSpecimenNameWords(const CSpecies *species, const CGender *gender) const;
	void AddUnitNameWord(CWord *word, const UnitClass *unit_class);
	const std::vector<CWord *> &GetUnitNameWords(const UnitClass *unit_class) const;
	void AddShipNameWord(CWord *word);
	const std::vector<CWord *> &GetShipNameWords() const;
	void AddSettlementNameWord(CWord *word);
	const std::vector<CWord *> &GetSettlementNameWords() const;
	
	String TranslateName(const String &name) const;
	
public:
	Property<CLanguageFamily *> Family = nullptr;	/// the family to which the language belongs
	Property<CLanguage *> DialectOf {	/// of which language this is a dialect of (if at all); dialects inherit the words from the parent language unless specified otherwise
		Property<CLanguage *>::ValueType(nullptr),
		Property<CLanguage *>::SetterType([this](CLanguage *language) {
			if (this->DialectOf.Value != nullptr) {
				this->DialectOf->Dialects.erase(std::remove(this->DialectOf->Dialects.begin(), this->DialectOf->Dialects.end(), this), this->DialectOf->Dialects.end());
			}
			this->DialectOf.Value = language;
			this->DialectOf->Dialects.push_back(this);
		})
	};
	std::vector<CLanguage *> Dialects;				/// dialects of this language
	std::vector<CWord *> Words;						/// words of the language
	std::map<String, std::vector<String>> NameTranslations;	/// name translations; possible translations mapped to the name to be translated
	
private:
	std::map<const CGender *, std::vector<CWord *>> PersonalNameWords;	/// the words used for personal name generation, mapped to the gender for which they can be used
	std::vector<CWord *> FamilyNameWords;
	std::map<const CSpecies *, std::map<const CGender *, std::vector<CWord *>>> SpecimenNameWords;	/// the words used for specimen name generation, mapped to the species and gender for which they can be used
	std::map<const UnitClass *, std::vector<CWord *>> UnitNameWords;	/// the words used for unit name generation, mapped to the unit class for which they can be used
	std::vector<CWord *> ShipNameWords;
	std::vector<CWord *> SettlementNameWords;

	friend int CclDefineLanguage(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
