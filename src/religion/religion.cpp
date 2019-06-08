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
/**@name religion.cpp - The religion source file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#include "religion/religion.h"

#include "config.h"
#include "religion/deity_domain.h"

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CReligion::ProcessConfigDataProperty(const String &key, String value)
{
	if (key == "domain") {
		CDeityDomain *deity_domain = CDeityDomain::Get(value);
		if (deity_domain) {
			this->Domains.push_back(deity_domain);
		}
	} else {
		return false;
	}
	
	return true;
}

void CReligion::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_cultural_deities", "cultural_deities"), +[](CReligion *religion, const bool cultural_deities){ religion->CulturalDeities = cultural_deities; });
	ClassDB::bind_method(D_METHOD("is_cultural_deities"), &CReligion::HasCulturalDeities);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cultural_deities"), "set_cultural_deities", "is_cultural_deities");
}
