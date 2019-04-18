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
/**@name species_category.cpp - The species category source file. */
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

#include "species/species_category.h"

#include "config.h"
#include "species/species_category_rank.h"
#include "util.h"

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
bool CSpeciesCategory::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "lower_category") {
		value = FindAndReplaceString(value, "_", "-");
		CSpeciesCategory *category = CSpeciesCategory::Get(value);
		if (category) {
			this->LowerCategories.push_back(category);
			category->UpperCategory = this;
		}
	} else if (key == "upper_category") {
		value = FindAndReplaceString(value, "_", "-");
		CSpeciesCategory *category = CSpeciesCategory::Get(value);
		if (category) {
			this->UpperCategory = category;
			category->LowerCategories.push_back(this);
		}
	} else {
		return false;
	}
	
	return true;
}

void CSpeciesCategory::_bind_methods()
{
	BIND_PROPERTIES();
	
	ClassDB::bind_method(D_METHOD("get_upper_category"), [](const CSpeciesCategory *category){ return category->UpperCategory; });
	ClassDB::bind_method(D_METHOD("get_all_upper_categories"), [](const CSpeciesCategory *category){ return VectorToGodotArray(category->GetAllUpperCategories()); });
}
