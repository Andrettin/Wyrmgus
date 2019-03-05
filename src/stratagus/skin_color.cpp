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
/**@name skin_color.cpp - The skin color source file. */
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

#include "skin_color.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CSkinColor *> CSkinColor::SkinColors;
std::map<std::string, CSkinColor *> CSkinColor::SkinColorsByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a skin color
**
**	@param	ident		The skin color's string identifier
**	@param	should_find	Whether it is an error if the skin color could not be found; this is true by default
**
**	@return	The skin color if found, or null otherwise
*/
CSkinColor *CSkinColor::GetSkinColor(const std::string &ident, const bool should_find)
{
	std::map<std::string, CSkinColor *>::const_iterator find_iterator = SkinColorsByIdent.find(ident);
	
	if (find_iterator != SkinColorsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid skin color: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a skin color
**
**	@param	ident	The skin color's string identifier
**
**	@return	The skin color if found, or a newly-created one otherwise
*/
CSkinColor *CSkinColor::GetOrAddSkinColor(const std::string &ident)
{
	CSkinColor *skin_color = GetSkinColor(ident, false);
	
	if (!skin_color) {
		skin_color = new CSkinColor;
		skin_color->Ident = ident;
		SkinColors.push_back(skin_color);
		SkinColorsByIdent[ident] = skin_color;
	}
	
	return skin_color;
}

/**
**	@brief	Remove the existing skin colors
*/
void CSkinColor::ClearSkinColors()
{
	for (size_t i = 0; i < SkinColors.size(); ++i) {
		delete SkinColors[i];
	}
	SkinColors.clear();
}
