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
//      (c) Copyright 1998-2021 by Lutz Sammer, Vladi Shabanski,
//                                 Francois Beerten and Andrettin
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

#include "stratagus.h"

#include "map/map_info.h"

#include "map/map_layer.h"

namespace wyrmgus {

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	x	The x coordinate
**	@param	y	The y coordinate
**	@param	z	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool map_info::IsPointOnMap(const int x, const int y, const int z) const
{
	return (z >= 0 && z < (int) MapWidths.size() && z < (int) MapHeights.size() && x >= 0 && y >= 0 && x < MapWidths[z] && y < MapHeights[z]);
}

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	pos	The coordinate position
**	@param	z	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool map_info::IsPointOnMap(const Vec2i &pos, const int z) const
{
	return IsPointOnMap(pos.x, pos.y, z);
}

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	x			The x coordinate
**	@param	y			The y coordinate
**	@param	map_layer	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool map_info::IsPointOnMap(const int x, const int y, const CMapLayer *map_layer) const
{
	return (map_layer && x >= 0 && y >= 0 && x < map_layer->get_width() && y < map_layer->get_height());
}

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	pos			The coordinate position
**	@param	map_layer	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool map_info::IsPointOnMap(const Vec2i &pos, const CMapLayer *map_layer) const
{
	return IsPointOnMap(pos.x, pos.y, map_layer);
}

void map_info::Clear()
{
	this->name.clear();
	this->Filename.clear();
	this->MapWidth = this->MapHeight = 0;
	//Wyrmgus start
	this->MapWidths.clear();
	this->MapHeights.clear();
	//Wyrmgus end
	memset(this->PlayerSide, 0, sizeof(this->PlayerSide));
	memset(this->PlayerType, 0, sizeof(this->PlayerType));
	this->MapUID = 0;
}

}
