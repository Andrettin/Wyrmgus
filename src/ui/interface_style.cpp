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

#include "stratagus.h"

#include "ui/interface_style.h"

#include "database/database.h"
#include "ui/interface_element_type.h"
#include "video/video.h"

namespace wyrmgus {

void interface_style::initialize()
{
	this->large_button_graphics = CGraphic::New(this->large_button_file.string());
	this->large_button_pressed_graphics = CGraphic::New(this->large_button_pressed_file.string());

	data_entry::initialize();
}

void interface_style::set_large_button_file(const std::filesystem::path &filepath)
{
	this->large_button_file = database::get()->get_graphics_path(this->get_module()) / filepath;
}

void interface_style::set_large_button_pressed_file(const std::filesystem::path &filepath)
{
	this->large_button_pressed_file = database::get()->get_graphics_path(this->get_module()) / filepath;
}

const std::shared_ptr<CGraphic> &interface_style::get_interface_element_graphics(const interface_element_type type) const
{
	switch (type) {
		case interface_element_type::large_button:
			return this->large_button_graphics;
			break;
		case interface_element_type::large_button_pressed:
			return this->large_button_pressed_graphics;
			break;
		default:
			throw std::runtime_error("Invalid interface element type: \"" + std::to_string(static_cast<int>(type)) + "\".");
	}
}

}
