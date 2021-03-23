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

#include "ui/checkbox_style.h"

#include "database/database.h"
#include "ui/button_state.h"
#include "ui/checkbox_state.h"
#include "ui/interface_style.h"
#include "video/video.h"

namespace wyrmgus {

void checkbox_style::initialize()
{
	this->checked_graphics = CGraphic::New(this->checked_file.string());
	this->checked_pressed_graphics = CGraphic::New(this->checked_pressed_file.string());
	this->unchecked_graphics = CGraphic::New(this->unchecked_file.string());
	this->unchecked_pressed_graphics = CGraphic::New(this->unchecked_pressed_file.string());
}

void checkbox_style::set_checked_file(const std::filesystem::path &filepath)
{
	this->checked_file = database::get()->get_graphics_path(this->interface->get_module()) / filepath;
}

void checkbox_style::set_checked_pressed_file(const std::filesystem::path &filepath)
{
	this->checked_pressed_file = database::get()->get_graphics_path(this->interface->get_module()) / filepath;
}

void checkbox_style::set_unchecked_file(const std::filesystem::path &filepath)
{
	this->unchecked_file = database::get()->get_graphics_path(this->interface->get_module()) / filepath;
}

void checkbox_style::set_unchecked_pressed_file(const std::filesystem::path &filepath)
{
	this->unchecked_pressed_file = database::get()->get_graphics_path(this->interface->get_module()) / filepath;
}

const std::shared_ptr<CGraphic> &checkbox_style::get_graphics(const checkbox_state checkbox_state, const button_state button_state) const
{
	switch (checkbox_state) {
		case checkbox_state::checked:
			switch (button_state) {
				case button_state::normal:
					return this->checked_graphics;
				case button_state::pressed:
					return this->checked_pressed_graphics;
				default:
					break;
			}
			break;
		case checkbox_state::unchecked:
			switch (button_state) {
				case button_state::normal:
					return this->unchecked_graphics;
				case button_state::pressed:
					return this->unchecked_pressed_graphics;
				default:
					break;
			}
			break;
		default:
			throw std::runtime_error("Invalid checkbox state: \"" + std::to_string(static_cast<int>(checkbox_state)) + "\".");
	}

	throw std::runtime_error("Invalid checkbox-button state combination: \"" + std::to_string(static_cast<int>(checkbox_state)) + "\" and \"" + std::to_string(static_cast<int>(button_state)) + "\".");
}

}
