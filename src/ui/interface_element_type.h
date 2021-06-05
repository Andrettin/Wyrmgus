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
//      (c) Copyright 2021 by Andrettin
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

namespace wyrmgus {

enum class interface_element_type {
	top_bar,
	panel,
	large_button,
	small_button,
	radio_button,
	up_arrow_button,
	down_arrow_button,
	left_arrow_button,
	right_arrow_button,
	dropdown_bar,
	slider_bar,
	slider_marker,
	icon_frame,
	pressed_icon_frame
};

inline interface_element_type string_to_interface_element_type(const std::string &str)
{
	if (str == "top_bar") {
		return interface_element_type::top_bar;
	} else if (str == "panel") {
		return interface_element_type::panel;
	} else if (str == "large_button") {
		return interface_element_type::large_button;
	} else if (str == "small_button") {
		return interface_element_type::small_button;
	} else if (str == "radio_button") {
		return interface_element_type::radio_button;
	} else if (str == "up_arrow_button") {
		return interface_element_type::up_arrow_button;
	} else if (str == "down_arrow_button") {
		return interface_element_type::down_arrow_button;
	} else if (str == "left_arrow_button") {
		return interface_element_type::left_arrow_button;
	} else if (str == "right_arrow_button") {
		return interface_element_type::right_arrow_button;
	} else if (str == "dropdown_bar") {
		return interface_element_type::dropdown_bar;
	} else if (str == "slider_bar") {
		return interface_element_type::slider_bar;
	} else if (str == "slider_marker") {
		return interface_element_type::slider_marker;
	} else if (str == "icon_frame") {
		return interface_element_type::icon_frame;
	} else if (str == "pressed_icon_frame") {
		return interface_element_type::pressed_icon_frame;
	}

	throw std::runtime_error("Invalid interface element type: \"" + str + "\".");
}

}
