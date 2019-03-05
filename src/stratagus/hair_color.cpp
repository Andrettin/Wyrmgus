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
/**@name hair_color.cpp - The hair color source file. */
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

#include "hair_color.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CHairColor *> CHairColor::HairColors;
std::map<std::string, CHairColor *> CHairColor::HairColorsByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a hair color
**
**	@param	ident		The hair color's string identifier
**	@param	should_find	Whether it is an error if the hair color could not be found; this is true by default
**
**	@return	The hair color if found, or null otherwise
*/
CHairColor *CHairColor::GetHairColor(const std::string &ident, const bool should_find)
{
	std::map<std::string, CHairColor *>::const_iterator find_iterator = HairColorsByIdent.find(ident);
	
	if (find_iterator != HairColorsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid hair color: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a hair color
**
**	@param	ident	The hair color's string identifier
**
**	@return	The hair color if found, or a newly-created one otherwise
*/
CHairColor *CHairColor::GetOrAddHairColor(const std::string &ident)
{
	CHairColor *hair_color = GetHairColor(ident, false);
	
	if (!hair_color) {
		hair_color = new CHairColor;
		hair_color->Ident = ident;
		HairColors.push_back(hair_color);
		HairColorsByIdent[ident] = hair_color;
	}
	
	return hair_color;
}

/**
**	@brief	Remove the existing hair colors
*/
void CHairColor::ClearHairColors()
{
	for (size_t i = 0; i < HairColors.size(); ++i) {
		delete HairColors[i];
	}
	HairColors.clear();
}
