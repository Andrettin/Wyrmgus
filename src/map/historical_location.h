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
//      (c) Copyright 2018-2022 by Andrettin
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

#pragma once

#include "time/date.h"
#include "util/geocoordinate.h"

class CConfigData;
struct lua_State;

static int CclDefineCharacter(lua_State *l);

namespace wyrmgus {

class gsml_data;
class gsml_property;
class map_template;
class site;

class historical_location final
{
public:
	historical_location()
	{
	}

	explicit historical_location(const wyrmgus::site *site) : site(site)
	{
		this->initialize();
		this->check();
	}

	explicit historical_location(const gsml_data &data);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void ProcessConfigData(const CConfigData *config_data);
	void initialize();
	void check() const;

	const wyrmgus::map_template *get_map_template() const
	{
		return this->map_template;
	}
	
	const QPoint &get_pos() const
	{
		return this->pos;
	}

	const wyrmgus::site *get_site() const
	{
		return this->site;
	}
	
public:
	CDate Date; //the historical location's date
private:
	const wyrmgus::map_template *map_template = nullptr; //the historical location's map template (overwritten by the site's map template if the site is given)
	QPoint pos = QPoint(-1, -1); //the historical location's position in its map layer, or as an offset from its pos reference site
	const wyrmgus::site *pos_reference_site = nullptr;
	wyrmgus::geocoordinate geocoordinate; //the historical location's position as a geocoordinate
	const wyrmgus::site *site = nullptr; //the historical location's site (if any)

	friend int ::CclDefineCharacter(lua_State *l);
};

}
