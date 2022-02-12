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

#include "stratagus.h"

#include "video/color_modification.h"

#include "util/colorization_type.h"
#include "time/time_of_day.h"

namespace wyrmgus {

color_modification::color_modification() : colorization(colorization_type::none)
{
}

color_modification::color_modification(const double hue_rotation, const colorization_type colorization, const color_set &hue_ignored_colors, const wyrmgus::player_color *player_color, const short red_change, const short green_change, const short blue_change) : hue_rotation(hue_rotation), colorization(colorization), hue_ignored_colors(hue_ignored_colors), player_color(player_color), red_change(red_change), green_change(green_change), blue_change(blue_change)
{
	if (this->get_hue_rotation() != 0 && this->get_colorization() != colorization_type::none) {
		throw std::runtime_error("A color modification cannot both have a hue rotation and a colorization.");
	}
}

color_modification::color_modification(const double hue_rotation, const colorization_type colorization, const color_set &hue_ignored_colors, const wyrmgus::player_color *player_color, const time_of_day *time_of_day) : color_modification(hue_rotation, colorization, hue_ignored_colors, player_color, time_of_day ? time_of_day->ColorModification.R : 0, time_of_day ? time_of_day->ColorModification.G : 0, time_of_day ? time_of_day->ColorModification.B : 0)
{
}

bool color_modification::is_null() const
{
	return this->get_hue_rotation() == 0 && this->get_colorization() == colorization_type::none && this->get_player_color() == nullptr && !this->has_rgb_change();
}

}
