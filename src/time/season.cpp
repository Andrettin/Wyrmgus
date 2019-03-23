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
/**@name season.cpp - The season source file. */
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

#include "time/season.h"

#include "config.h"
#include "config_operator.h"
#include "mod.h"
#include "video/video.h"

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
bool CSeason::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "name") {
		this->Name = value;
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CSeason::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "image") {
		std::string file;
		Vec2i size(0, 0);
			
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			std::string value = property.Value;
			
			if (key == "file") {
				file = CMod::GetCurrentModPath() + value;
			} else if (key == "width") {
				size.x = std::stoi(value);
			} else if (key == "height") {
				size.y = std::stoi(value);
			} else {
				fprintf(stderr, "Invalid image property: \"%s\".\n", key.c_str());
			}
		}
		
		if (file.empty()) {
			fprintf(stderr, "Image has no file.\n");
			return true;
		}
		
		if (size.x == 0) {
			fprintf(stderr, "Image has no width.\n");
			return true;
		}
		
		if (size.y == 0) {
			fprintf(stderr, "Image has no height.\n");
			return true;
		}
		
		this->G = CGraphic::New(file, size.x, size.y);
		this->G->Load();
		this->G->UseDisplayFormat();
	} else {
		return false;
	}
	
	return true;
}
