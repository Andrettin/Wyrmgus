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
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "player/faction_type.h"
#include "sound/game_sound_set.h"
#include "sound/music.h"
#include "upgrade/upgrade_structs.h"
#include "util/container_util.h"
#include "util/path_util.h"
#include "util/size_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"
#include "video/video.h"

namespace wyrmgus {

defines::defines()
{
	connect(preferences::get(), &preferences::scale_factor_changed, this, &defines::scale_factor_changed);
}

defines::~defines()
{
}

void defines::load(const std::filesystem::path &data_path)
{
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
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "game_sound_set") {
		database::process_sml_data(game_sound_set::get(), scope);
	} else if (tag == "border_transition_tiles") {
		scope.for_each_child([&](const sml_data &child_scope) {
			std::string transition_type_str = child_scope.get_tag();
			string::replace(transition_type_str, "_", "-");
			const tile_transition_type transition_type = GetTransitionTypeIdByName(transition_type_str);
			std::vector<int> tiles;

			for (const std::string &value : child_scope.get_values()) {
				const int tile = std::stoi(value);

				this->border_transition_tiles[transition_type].push_back(tile);
			}
		});
	} else if (tag == "faction_type_upgrades") {
		scope.for_each_property([&](const sml_property &property) {
			const faction_type faction_type = string_to_faction_type(property.get_key());
			const CUpgrade *upgrade = CUpgrade::get(property.get_value());

			this->faction_type_upgrades[faction_type] = upgrade;
		});
	} else if (tag == "ignored_wesnoth_terrain_strings") {
		for (const std::string &value : values) {
			terrain_type::map_to_wesnoth_string(nullptr, value);
		}
	} else {
		database::process_sml_scope_for_object(this, scope);
	}
}

void defines::initialize()
{
	if (!this->border_image_file.empty()) {
		this->border_graphics = CPlayerColorGraphic::New(this->border_image_file, this->border_frame_size, this->get_conversible_player_color());
		this->border_graphics->Load(this->get_scale_factor());
	}

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

	if (this->get_population_per_unit() == 0) {
		throw std::runtime_error("No population per unit set in the defines.");
	}
}

const centesimal_int &defines::get_scale_factor() const
{
	return preferences::get()->get_scale_factor();
}

void defines::set_border_image_file(const std::filesystem::path &filepath)
{
	if (filepath == this->border_image_file) {
		return;
	}

	this->border_image_file = database::get()->get_graphics_filepath(filepath);
}

QPoint defines::get_border_offset() const
{
	return size::to_point((this->get_tile_size() - this->border_frame_size) * this->get_scale_factor() / 2);
}

QString defines::get_default_menu_background_file_qstring() const
{
	return path::to_qstring(this->default_menu_background_file);
}

void defines::set_default_menu_background_file(const std::filesystem::path &filepath)
{
	this->default_menu_background_file = database::get()->get_graphics_filepath(filepath);
}

QStringList defines::get_loading_background_files_qstring_list() const
{
	return container::to_qstring_list(this->loading_background_files);
}

void defines::add_loading_background_file(const std::string &filepath)
{
	this->loading_background_files.push_back(database::get()->get_graphics_filepath(filepath));
}

void defines::remove_loading_background_file(const std::string &filepath)
{
	vector::remove_one(this->loading_background_files, database::get()->get_graphics_filepath(filepath));
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
