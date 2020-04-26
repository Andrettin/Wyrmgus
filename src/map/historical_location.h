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
/**@name historical_location.h - The historical location header file. */
//
//      (c) Copyright 2018-2020 by Andrettin
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

#pragma once

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "time/date.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;

namespace stratagus {

class map_template;
class site;
class sml_data;
class sml_property;

class historical_location
{
public:
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	void ProcessConfigData(const CConfigData *config_data);
	void initialize();
	void check() const;
	
public:
	CDate Date; //the historical location's date
	stratagus::map_template *map_template = nullptr; //the historical location's map template (overwritten by the site's map template if the site is given)
	Vec2i Position = Vec2i(-1, -1); //the historical location's position in its map layer (overwritten by the site position if the site is given and has a valid position)
	stratagus::site *site = nullptr; //the historical location's site (if any)
};

}
