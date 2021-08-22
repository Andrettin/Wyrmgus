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
#include "player.h" //for the player types enum
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/string_util.h"

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
	this->presentation_filepath.clear();
	this->MapWidth = this->MapHeight = 0;
	//Wyrmgus start
	this->MapWidths.clear();
	this->MapHeights.clear();
	//Wyrmgus end
	memset(this->PlayerSide, 0, sizeof(this->PlayerSide));
	memset(this->PlayerType, 0, sizeof(this->PlayerType));
	this->MapUID = 0;
	this->MapWorld = "Custom";
}

qunique_ptr<map_info> map_info::duplicate() const
{
	auto info = make_qunique<map_info>();

	if (QApplication::instance()->thread() != QThread::currentThread()) {
		info->moveToThread(QApplication::instance()->thread());
	}

	info->name = this->name;
	info->presentation_filepath = this->presentation_filepath;
	info->MapWidth = this->MapWidth;
	info->MapHeight = this->MapHeight;
	info->MapWidths = this->MapWidths;
	info->MapHeights = this->MapHeights;
	memcpy(info->PlayerType, this->PlayerType, sizeof(info->PlayerType));
	memcpy(info->PlayerSide, this->PlayerSide, sizeof(info->PlayerSide));
	info->MapUID = this->MapUID;
	info->MapWorld = this->MapWorld;

	return info;
}

QString map_info::get_presentation_filepath_qstring() const
{
	return path::to_qstring(this->get_presentation_filepath());
}

int map_info::get_player_count() const
{
	int count = 0;

	for (const int player_type : this->PlayerType) {
		if (player_type == PlayerPerson || player_type == PlayerComputer) {
			++count;
		}
	}

	return count;
}

int map_info::get_person_player_count() const
{
	int count = 0;

	for (const int player_type : this->PlayerType) {
		if (player_type == PlayerPerson) {
			++count;
		}
	}

	return count;
}

int map_info::get_person_player_index() const
{
	//get the index of the first person player
	for (size_t i = 0; i < (PlayerMax - 1); ++i) {
		if (PlayerType[i] == PlayerPerson) {
			return i;
		}
	}

	log::log_error("No person player found for map \"" + this->get_presentation_filepath().string() + "\".");

	return -1;
}

}
