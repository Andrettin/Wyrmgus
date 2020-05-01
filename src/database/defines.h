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
//      (c) Copyright 2019-2020 by Andrettin
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

#include "util/singleton.h"

namespace stratagus {

class player_color;
class sml_data;
class sml_property;
class time_of_day;

class defines final : public QObject, public singleton<defines>
{
	Q_OBJECT

	Q_PROPERTY(QSize tile_size MEMBER tile_size READ get_tile_size)
	Q_PROPERTY(QSize icon_size MEMBER icon_size READ get_icon_size)
	Q_PROPERTY(stratagus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)
	Q_PROPERTY(stratagus::player_color* neutral_player_color MEMBER neutral_player_color READ get_neutral_player_color)
	Q_PROPERTY(stratagus::time_of_day* underground_time_of_day MEMBER underground_time_of_day READ get_underground_time_of_day)
	Q_PROPERTY(bool documents_modules_loading_enabled MEMBER documents_modules_loading_enabled READ is_documents_modules_loading_enabled)

public:
	void load(const std::filesystem::path &base_path);
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	const QSize &get_tile_size() const
	{
		return this->tile_size;
	}

	int get_tile_width() const
	{
		return this->get_tile_size().width();
	}

	int get_tile_height() const
	{
		return this->get_tile_size().height();
	}

	const QSize &get_icon_size() const
	{
		return this->icon_size;
	}

	int get_scale_factor() const
	{
		return this->scale_factor;
	}

	QSize get_scaled_tile_size() const
	{
		return this->get_tile_size() * this->get_scale_factor();
	}

	int get_scaled_tile_width() const
	{
		return this->get_tile_width() * this->get_scale_factor();
	}

	int get_scaled_tile_height() const
	{
		return this->get_tile_height() * this->get_scale_factor();
	}

	player_color *get_conversible_player_color() const
	{
		return this->conversible_player_color;
	}

	player_color *get_neutral_player_color() const
	{
		return this->neutral_player_color;
	}

	time_of_day *get_underground_time_of_day() const
	{
		return this->underground_time_of_day;
	}

	bool is_documents_modules_loading_enabled() const
	{
		return this->documents_modules_loading_enabled;
	}

private:
	QSize tile_size;
	QSize icon_size;
	int scale_factor = 1;
	player_color *conversible_player_color = nullptr;
	player_color *neutral_player_color = nullptr;
	time_of_day *underground_time_of_day = nullptr;
	bool documents_modules_loading_enabled = true;
};

}
