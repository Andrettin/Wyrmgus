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
//      (c) Copyright 1998-2022 by Lutz Sammer, Vladi Shabanski,
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
#include "map/map_presets.h"
#include "map/map_settings.h"
#include "player/player.h" //for the PlayerNumNeutral constexpr
#include "player/player_type.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/string_util.h"

namespace wyrmgus {

map_info::map_info()
{
	this->reset();
}

map_info::~map_info()
{
}

void map_info::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "settings") {
		this->set_presets(map_presets::get(value));
	} else {
		database::get()->process_gsml_property_for_object(this, property);
	}
}

void map_info::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "player_types") {
		for (size_t i = 0; i < values.size(); ++i) {
			const std::string &value = values[i];
			this->player_types[i] = string_to_player_type(value);
		}
	} else if (tag == "settings") {
		this->presets = nullptr;

		this->settings = make_qunique<map_settings>();
		if (QApplication::instance()->thread() != QThread::currentThread()) {
			this->settings->moveToThread(QApplication::instance()->thread());
		}
		database::process_gsml_data(this->settings, scope);
	} else {
		database::process_gsml_scope_for_object(this, scope);
	}
}

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

void map_info::reset()
{
	this->name.clear();
	this->description.clear();
	this->author.clear();
	this->presentation_filepath.clear();
	this->map_size = QSize(0, 0);
	//Wyrmgus start
	this->MapWidths.clear();
	this->MapHeights.clear();
	//Wyrmgus end
	std::fill(this->PlayerSide.begin(), this->PlayerSide.end(), 0);

	std::fill(this->player_types.begin(), this->player_types.end(), player_type::nobody);
	this->player_types[PlayerNumNeutral] = player_type::neutral;

	this->MapUID = 0;
	this->MapWorld = "Custom";

	this->settings = make_qunique<map_settings>();
	if (QApplication::instance()->thread() != QThread::currentThread()) {
		this->settings->moveToThread(QApplication::instance()->thread());
	}

	this->presets = nullptr;

	this->hidden = false;
}

qunique_ptr<map_info> map_info::duplicate() const
{
	auto info = make_qunique<map_info>();

	if (QApplication::instance()->thread() != QThread::currentThread()) {
		info->moveToThread(QApplication::instance()->thread());
	}

	info->name = this->name;
	info->description = this->description;
	info->author = this->author;
	info->presentation_filepath = this->presentation_filepath;
	info->map_size = this->map_size;
	info->MapWidths = this->MapWidths;
	info->MapHeights = this->MapHeights;
	info->player_types = this->player_types;
	info->PlayerSide = this->PlayerSide;
	info->MapUID = this->MapUID;
	info->MapWorld = this->MapWorld;
	info->settings = this->settings->duplicate();
	info->presets = this->presets;

	return info;
}

QString map_info::get_presentation_filepath_qstring() const
{
	return path::to_qstring(this->get_presentation_filepath());
}

QString map_info::get_presentation_filename_qstring() const
{
	return path::to_qstring(this->get_presentation_filepath().filename());
}

int map_info::get_player_count() const
{
	int count = 0;

	for (const player_type player_type : this->player_types) {
		if (player_type == player_type::person || player_type == player_type::computer) {
			++count;
		}
	}

	return count;
}

int map_info::get_person_player_count() const
{
	int count = 0;

	for (const player_type player_type : this->player_types) {
		if (player_type == player_type::person) {
			++count;
		}
	}

	return count;
}

int map_info::get_person_player_index() const
{
	//get the index of the first person player
	for (size_t i = 0; i < PlayerNumNeutral; ++i) {
		if (this->player_types[i] == player_type::person) {
			return i;
		}
	}

	log::log_error("No person player found for map \"" + this->get_presentation_filepath().string() + "\".");

	return -1;
}

void map_info::set_settings(qunique_ptr<map_settings> &&settings)
{
	this->settings = std::move(settings);
}

void map_info::set_presets(const map_presets *presets)
{
	if (presets == this->get_presets()) {
		return;
	}

	this->presets = presets;
	this->settings = this->presets->get_settings()->duplicate();
}

std::string map_info::get_text() const
{
	std::string str = "Map: " + this->get_name() + " (" + std::to_string(this->get_map_width()) + "x" + std::to_string(this->get_map_height()) + ")";

	if (!this->description.empty()) {
		str += "\n\nDescription: " + this->description;
	}

	if (!this->author.empty()) {
		str += "\n\nAuthor: " + this->author;
	}

	std::string settings_str = this->get_settings()->get_string();
	if (!settings_str.empty()) {
		str += "\n\n" + std::move(settings_str);
	}

	return str;
}

}
