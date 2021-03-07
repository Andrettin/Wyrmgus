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

#include "ui/button_style.h"

#include "database/database.h"
#include "ui/button_state.h"
#include "ui/interface_style.h"
#include "video/video.h"

namespace wyrmgus {

void button_style::initialize()
{
	this->normal_graphics = CGraphic::New(this->normal_file.string());
	this->pressed_graphics = CGraphic::New(this->pressed_file.string());
	this->grayed_graphics = CGraphic::New(this->grayed_file.string());
}

void button_style::set_normal_file(const std::filesystem::path &filepath)
{
	this->normal_file = database::get()->get_graphics_path(this->interface->get_module()) / filepath;
}

void button_style::set_pressed_file(const std::filesystem::path &filepath)
{
	this->pressed_file = database::get()->get_graphics_path(this->interface->get_module()) / filepath;
}

void button_style::set_grayed_file(const std::filesystem::path &filepath)
{
	this->grayed_file = database::get()->get_graphics_path(this->interface->get_module()) / filepath;
}

const std::shared_ptr<CGraphic> &button_style::get_graphics(const button_state state) const
{
	switch (state) {
		case button_state::normal:
			return this->normal_graphics;
		case button_state::pressed:
			return this->pressed_graphics;
		case button_state::grayed:
			return this->grayed_graphics;
		default:
			throw std::runtime_error("Invalid button state: \"" + std::to_string(static_cast<int>(state)) + "\".");
	}
}

}
