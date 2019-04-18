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
/**@name property.cpp - The property source file. */
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

#include "property.h"

#include "data_type.h"
#include "item/item_slot.h"
#include "language/language.h"
#include "literary_text.h"
#include "map/terrain_type.h"
#include "species/species.h"
#include "species/species_category.h"
#include "species/species_category_rank.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "ui/icon.h"
#include "unit/unit_type.h"
#include "world/plane.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Convert to a type from a string
**
**	@param	str	The string to be converted
**
**	@return	The converted variable
*/
template <typename T>
T ConvertFromString(const std::string &str)
{
	if constexpr(std::is_same_v<T, int>) {
		return std::stoi(str);
	} else if constexpr(std::is_same_v<T, bool>) {
		return StringToBool(str);
	} else if constexpr(std::is_same_v<T, String>) {
		return String(str.c_str());
	} else if constexpr(std::is_pointer_v<T> && std::is_base_of_v<DataType<std::remove_const_t<std::remove_pointer_t<T>>>, std::remove_const_t<std::remove_pointer_t<T>>>) {
		std::string ident = FindAndReplaceString(str, "_", "-");
		return std::remove_const_t<std::remove_pointer_t<T>>::Get(ident);
	} else {
		fprintf(stderr, "Cannot convert from a string to the designated type.\n");
		return T();
	}
}

template int ConvertFromString(const std::string &str);
template bool ConvertFromString(const std::string &str);
template String ConvertFromString(const std::string &str);
template CIcon *ConvertFromString(const std::string &str);
template CLanguage *ConvertFromString(const std::string &str);
template CLiteraryText *ConvertFromString(const std::string &str);
template CPlane *ConvertFromString(const std::string &str);
template CSeasonSchedule *ConvertFromString(const std::string &str);
template CSpecies *ConvertFromString(const std::string &str);
template CSpeciesCategory *ConvertFromString(const std::string &str);
template CSpeciesCategoryRank *ConvertFromString(const std::string &str);
template CTerrainType *ConvertFromString(const std::string &str);
template CTimeOfDaySchedule *ConvertFromString(const std::string &str);
template CUnitType *ConvertFromString(const std::string &str);
template ItemSlot *ConvertFromString(const std::string &str);
template const ItemSlot *ConvertFromString(const std::string &str);
