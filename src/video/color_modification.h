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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "util/color_container.h"

namespace wyrmgus {

class player_color;
class time_of_day;
enum class colorization_type;

//a color modification to be applied to a texture
class color_modification final
{
public:
	color_modification();

	explicit color_modification(const int hue_rotation, const colorization_type colorization, const color_set &hue_ignored_colors, const wyrmgus::player_color *player_color, const short red_change, const short green_change, const short blue_change);

	explicit color_modification(const int hue_rotation, const colorization_type colorization, const color_set &hue_ignored_colors, const wyrmgus::player_color *player_color)
		: color_modification(hue_rotation, colorization, hue_ignored_colors, player_color, 0, 0, 0)
	{
	}

	explicit color_modification(const int hue_rotation, const colorization_type colorization, const color_set &hue_ignored_colors, const wyrmgus::player_color *player_color, const time_of_day *time_of_day);

	int get_hue_rotation() const
	{
		return this->hue_rotation;
	}

	colorization_type get_colorization() const
	{
		return this->colorization;
	}

	const color_set &get_hue_ignored_colors() const
	{
		return this->hue_ignored_colors;
	}

	const wyrmgus::player_color *get_player_color() const
	{
		return this->player_color;
	}

	short get_red_change() const
	{
		return this->red_change;
	}

	short get_green_change() const
	{
		return this->green_change;
	}

	short get_blue_change() const
	{
		return this->blue_change;
	}

	bool has_rgb_change() const
	{
		return this->get_red_change() != 0 || this->get_green_change() != 0 || this->get_blue_change() != 0;
	}

	bool is_null() const;

	bool operator <(const color_modification &other) const {
		if (this->get_hue_rotation() != other.get_hue_rotation()) {
			return this->get_hue_rotation() < other.get_hue_rotation();
		}

		if (this->get_colorization() != other.get_colorization()) {
			return this->get_colorization() < other.get_colorization();
		}

		if (this->get_hue_ignored_colors() != other.get_hue_ignored_colors()) {
			return this->get_hue_ignored_colors() < other.get_hue_ignored_colors();
		}

		if (this->get_player_color() != other.get_player_color()) {
			return this->get_player_color() < other.get_player_color();
		}

		if (this->get_red_change() != other.get_red_change()) {
			return this->get_red_change() < other.get_red_change();
		}

		if (this->get_green_change() != other.get_green_change()) {
			return this->get_green_change() < other.get_green_change();
		}

		if (this->get_blue_change() != other.get_blue_change()) {
			return this->get_blue_change() < other.get_blue_change();
		}

		return false;
	}


private:
	int hue_rotation = 0; //rotation in degrees to the hue
	colorization_type colorization;
	color_set hue_ignored_colors; //ignored colors for the hue rotation and desaturation

	const wyrmgus::player_color *player_color = nullptr; //the player color to be applied to the texture

	//the RGB value changes to be applied to the texture
	short red_change = 0;
	short green_change = 0;
	short blue_change = 0;
};

}
