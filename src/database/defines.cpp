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
//      (c) Copyright 2019-2021 by Andrettin
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

#include "database/defines.h"

#include "database/database.h"
#include "database/preferences.h"
#include "database/sml_data.h"
#include "database/sml_parser.h"
#include "faction_type.h"
#include "sound/game_sound_set.h"
#include "sound/music.h"
#include "upgrade/upgrade_structs.h"
#include "util/container_util.h"
#include "util/vector_util.h"
#include "video/video.h"

namespace wyrmgus {

defines::~defines()
{
}

void defines::load(const std::filesystem::path &data_path)
{
	//set the scale factor to that of the preferences on load
	if (preferences::get()->get_scale_factor() != this->get_scale_factor()) {
		this->scale_factor = preferences::get()->get_scale_factor();
	}

	std::filesystem::path defines_path(data_path / "defines.txt");

	if (!std::filesystem::exists(defines_path)) {
		return;
	}

	sml_parser parser;
	const sml_data data = parser.parse(defines_path);
	database::process_sml_data(this, data);
}

void defines::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "icon_frame_file") {
		this->icon_frame_graphics = CGraphic::New(value);
	} else if (key == "pressed_icon_frame_file") {
		this->pressed_icon_frame_graphics = CGraphic::New(value);
	} else if (key == "command_button_frame_file") {
		this->command_button_frame_graphics = CGraphic::New(value);
	} else if (key == "bar_frame_file") {
		this->bar_frame_graphics = CGraphic::New(value);
	} else if (key == "infopanel_frame_file") {
		this->infopanel_frame_graphics = CGraphic::New(value);
	} else if (key == "progress_bar_file") {
		this->progress_bar_graphics = CGraphic::New(value);
	} else {
		database::process_sml_property_for_object(this, property);
	}
}

void defines::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "game_sound_set") {
		database::process_sml_data(game_sound_set::get(), scope);
	} else if (tag == "faction_type_upgrades") {
		scope.for_each_property([&](const sml_property &property) {
			const faction_type faction_type = string_to_faction_type(property.get_key());
			const CUpgrade *upgrade = CUpgrade::get(property.get_value());

			this->faction_type_upgrades[faction_type] = upgrade;
		});
	} else {
		database::process_sml_scope_for_object(this, scope);
	}
}

void defines::initialize()
{
	if (this->icon_frame_graphics != nullptr) {
		this->icon_frame_graphics->Load(this->get_scale_factor());
	}

	if (this->pressed_icon_frame_graphics != nullptr) {
		this->pressed_icon_frame_graphics->Load(this->get_scale_factor());
	}

	if (this->command_button_frame_graphics != nullptr) {
		this->command_button_frame_graphics->Load(this->get_scale_factor());
	}

	if (this->bar_frame_graphics != nullptr) {
		this->bar_frame_graphics->Load(this->get_scale_factor());
	}

	if (this->infopanel_frame_graphics != nullptr) {
		this->infopanel_frame_graphics->Load(this->get_scale_factor());
	}

	if (this->progress_bar_graphics != nullptr) {
		this->progress_bar_graphics->Load(this->get_scale_factor());
	}
}

void defines::set_default_menu_background_file(const std::filesystem::path &filepath)
{
	this->default_menu_background_file = database::get()->get_graphics_path(nullptr) / filepath;
}

QStringList defines::get_loading_background_files_qstring_list() const
{
	return container::to_qstring_list(this->loading_background_files);
}

void defines::add_loading_background_file(const std::string &filepath)
{
	this->loading_background_files.push_back(database::get()->get_graphics_path(nullptr) / filepath);
}

void defines::remove_loading_background_file(const std::string &filepath)
{
	vector::remove_one(this->loading_background_files, database::get()->get_graphics_path(nullptr) / filepath);
}

QStringList defines::get_tips_qstring_list() const
{
	return container::to_qstring_list(this->get_tips());
}

void defines::remove_tip(const std::string &tip)
{
	vector::remove_one(this->tips, tip);
}

}
