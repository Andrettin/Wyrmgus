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
/**@name species_category.h - The species category header file. */
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

#ifndef __SPECIES_CATEGORY_H__
#define __SPECIES_CATEGORY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "property.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CSpeciesCategoryRank;
class CWord;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CSpeciesCategory : public DataElement, public DataType<CSpeciesCategory>
{
	DATA_TYPE(CSpeciesCategory, DataElement)
	
public:
	CSpeciesCategory(const std::string &ident = "", const int index = -1) : DataElement(ident, index)
	{
	}
	
private:
	/**
	**	@brief	Initialize the class
	*/
	static inline bool InitializeClass()
	{
		REGISTER_PROPERTY(CommonName);
		REGISTER_PROPERTY(CommonNamePlural);
		REGISTER_PROPERTY(Rank);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();
	
public:
	static constexpr const char *ClassIdentifier = "species_category";
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	const std::vector<CSpeciesCategory *> &GetLowerCategory() const
	{
		return this->LowerCategories;
	}
	
	CSpeciesCategory *GetUpperCategory() const
	{
		return this->UpperCategory;
	}
	
	std::vector<CSpeciesCategory *> GetAllUpperCategories() const
	{
		if (this->UpperCategory == nullptr) {
			return std::vector<CSpeciesCategory *>();
		}
		
		std::vector<CSpeciesCategory *> upper_categories = this->UpperCategory->GetAllUpperCategories();
		upper_categories.push_back(this->UpperCategory);
		
		return upper_categories;
	}
	
	void AddSpecimenNameWord(CWord *word, const int gender);
	const std::vector<CWord *> &GetSpecimenNameWords(const int gender);
	
	Property<String> CommonName;	/// the common name of members of the species category
	Property<String> CommonNamePlural;	/// the plural of the common name of members of the species category
	Property<CSpeciesCategoryRank *> Rank = nullptr;	/// the rank of the species category
	
private:
	std::vector<CSpeciesCategory *> LowerCategories;	/// the categories directly below this one
	CSpeciesCategory *UpperCategory = nullptr;	/// the category directly above this one
	std::map<int, std::vector<CWord *>> SpecimenNameWords;	/// the words used for specimen name generation, mapped to the gender for which they can be used

protected:
	static void _bind_methods();
};

#endif
