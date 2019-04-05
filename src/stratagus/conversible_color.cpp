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
/**@name conversible_color.cpp - The conversible color source file. */
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

#include "conversible_color.h"

#include "config.h"
#include "video/color.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CConversibleColor::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "color") {
		Color color = section->ProcessColor();
		this->Colors.push_back(color);
	} else {
		return false;
	}
	
	return true;
}

void CConversibleColor::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_colors"), &CConversibleColor::GetColors);
}
