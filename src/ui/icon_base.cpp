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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "ui/icon_base.h"

#include "database/database.h"
#include "database/defines.h"
#include "video/video.h"

namespace wyrmgus {

icon_base::~icon_base()
{
}

void icon_base::initialize()
{
	if (!this->get_file().empty() && this->graphics == nullptr) {
		const QSize &icon_size = this->get_size();
		this->graphics = CGraphic::New(this->get_file().string(), icon_size);
	}

	if (this->graphics == nullptr) {
		throw std::runtime_error("Icon \"" + this->get_identifier() + "\" has no graphics.");
	}

	data_entry::initialize();
}

bool icon_base::is_loaded() const
{
	return this->graphics->IsLoaded();
}

void icon_base::load() const
{
	if (this->is_loaded()) {
		return;
	}

	this->graphics->Load(defines::get()->get_scale_factor());

	if (this->get_frame() >= this->graphics->NumFrames) {
		throw std::runtime_error("Invalid icon frame: \"" + this->get_identifier() + "\" - " + std::to_string(this->get_frame()));
	}
}

void icon_base::set_file(const std::filesystem::path &filepath)
{
	if (filepath == this->get_file()) {
		return;
	}

	this->file = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
