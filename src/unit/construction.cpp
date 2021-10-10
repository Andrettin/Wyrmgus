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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "unit/construction.h"

#include "database/preferences.h"
#include "script.h"
#include "translate.h"
#include "ui/ui.h"
#include "video/video.h"

namespace wyrmgus {

void construction_frame::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "percent") {
		this->percent = std::stoi(value);
	} else if (key == "image_type") {
		if (value == "construction") {
			this->image_type = construction_image_type::construction;
		} else if (value == "main") {
			this->image_type = construction_image_type::main;
		} else {
			throw std::runtime_error("Invalid construction image type: \"" + value + "\".");
		}
	} else if (key == "frame") {
		this->frame = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid construction frame property: \"" + key + "\".");
	}
}

construction::~construction()
{
}

void construction::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "image_file") {
		this->image_file = database::get()->get_graphics_filepath(value);
	} else {
		data_entry::process_sml_property(property);
	}
}

void construction::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "frames") {
		scope.for_each_child([&](const sml_data &child_scope) {
			auto cframe = std::make_unique<construction_frame>();
			database::process_sml_data(cframe, child_scope);
			if (!this->frames.empty()) {
				this->frames.back()->next = cframe.get();
			}
			this->frames.push_back(std::move(cframe));
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void construction::load()
{
	if (!this->image_file.empty()) {
		this->graphics = CPlayerColorGraphic::New(this->image_file, this->get_frame_size(), nullptr);
		this->graphics->Load(preferences::get()->get_scale_factor());
		IncItemsLoaded();
	}
}

}

/**
**  Return the amount of constructions.
*/
int GetConstructionsCount()
{
	int count = 0;
	for (const wyrmgus::construction *construction : wyrmgus::construction::get_all()) {
		if (!construction->get_image_file().empty()) count++;
	}
	return count;
}

/**
**  Load the graphics for the constructions.
**
**  HELPME: who make this better terrain depended and extendable
**  HELPME: filename construction.
*/
void LoadConstructions()
{
	ShowLoadProgress("%s", _("Loading Construction Graphics..."));
		
	for (wyrmgus::construction *construction : wyrmgus::construction::get_all()) {
		construction->load();
	}
}
