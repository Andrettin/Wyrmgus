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
/**@name button_level.cpp - The button level source file. */
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

#include "ui/button_level.h"

#include "config.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CButtonLevel *> CButtonLevel::ButtonLevels;
std::map<std::string, CButtonLevel *> CButtonLevel::ButtonLevelsByIdent;
CButtonLevel *CButtonLevel::CancelButtonLevel = nullptr;
CButtonLevel *CButtonLevel::InventoryButtonLevel = nullptr;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a button level
**
**	@param	ident		The button level's string identifier
**	@param	should_find	Whether it is an error if the button level could not be found; this is true by default
**
**	@return	The button level if found, or null otherwise
*/
CButtonLevel *CButtonLevel::GetButtonLevel(const std::string &ident, const bool should_find)
{
	if (ident.empty()) {
		return nullptr;
	}
	
	std::map<std::string, CButtonLevel *>::const_iterator find_iterator = ButtonLevelsByIdent.find(ident);
	
	if (find_iterator != ButtonLevelsByIdent.end()) {
		return ButtonLevelsByIdent.find(ident)->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid button level: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a button level
**
**	@param	ident	The button level's string identifier
**
**	@return	The button level if found, or a newly-created one otherwise
*/
CButtonLevel *CButtonLevel::GetOrAddButtonLevel(const std::string &ident)
{
	CButtonLevel *button_level = GetButtonLevel(ident, false);
	
	if (!button_level) {
		button_level = new CButtonLevel;
		button_level->Ident = ident;
		ButtonLevels.push_back(button_level);
		button_level->Index = ButtonLevels.size(); //index starts at 1, so that buttons with a null button level have a value of 0
		ButtonLevelsByIdent[ident] = button_level;
	}
	
	return button_level;
}

/**
**	@brief	Remove the existing button levels
*/
void CButtonLevel::ClearButtonLevels()
{
	for (size_t i = 0; i < ButtonLevels.size(); ++i) {
		delete ButtonLevels[i];
	}
	ButtonLevels.clear();
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CButtonLevel::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "cancel_button_level") {
		const bool is_cancel_button_level = StringToBool(value);
		if (is_cancel_button_level) {
			CButtonLevel::CancelButtonLevel = this;
		}
	} else if (key == "inventory_button_level") {
		const bool is_inventory_button_level = StringToBool(value);
		if (is_inventory_button_level) {
			CButtonLevel::InventoryButtonLevel = this;
		}
	} else {
		return false;
	}
	
	return true;
}
