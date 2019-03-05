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
/**@name player_color.cpp - The player color source file. */
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

#include "player_color.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CPlayerColor *> CPlayerColor::PlayerColors;
std::map<std::string, CPlayerColor *> CPlayerColor::PlayerColorsByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a player color
**
**	@param	ident		The player color's string identifier
**	@param	should_find	Whether it is an error if the player color could not be found; this is true by default
**
**	@return	The player color if found, or null otherwise
*/
CPlayerColor *CPlayerColor::GetPlayerColor(const std::string &ident, const bool should_find)
{
	std::map<std::string, CPlayerColor *>::const_iterator find_iterator = PlayerColorsByIdent.find(ident);
	
	if (find_iterator != PlayerColorsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid player color: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a player color
**
**	@param	ident	The player color's string identifier
**
**	@return	The player color if found, or a newly-created one otherwise
*/
CPlayerColor *CPlayerColor::GetOrAddPlayerColor(const std::string &ident)
{
	CPlayerColor *player_color = GetPlayerColor(ident, false);
	
	if (!player_color) {
		player_color = new CPlayerColor;
		player_color->Ident = ident;
		PlayerColors.push_back(player_color);
		PlayerColorsByIdent[ident] = player_color;
	}
	
	return player_color;
}

/**
**	@brief	Remove the existing player colors
*/
void CPlayerColor::ClearPlayerColors()
{
	for (size_t i = 0; i < PlayerColors.size(); ++i) {
		delete PlayerColors[i];
	}
	PlayerColors.clear();
}
