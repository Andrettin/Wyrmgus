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

#include "ui/interface_style.h"

#include "database/database.h"
#include "database/defines.h"
#include "ui/button_state.h"
#include "ui/button_style.h"
#include "ui/checkbox_state.h"
#include "ui/checkbox_style.h"
#include "ui/interface_element_type.h"
#include "video/video.h"

namespace wyrmgus {

interface_style::interface_style(const std::string &identifier) : data_entry(identifier)
{
}

interface_style::~interface_style()
{
}

void interface_style::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "panel_files") {
		scope.for_each_property([&](const gsml_property &property) {
			const int panel = std::stoi(property.get_key());
			const std::filesystem::path filepath = property.get_value();
			this->panel_files[panel] = database::get()->get_graphics_filepath(filepath);
		});
	} else if (tag == "large_button") {
		this->large_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->large_button, scope);
	} else if (tag == "small_button") {
		this->small_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->small_button, scope);
	} else if (tag == "thin_button") {
		this->thin_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->thin_button, scope);
	} else if (tag == "small_thin_button") {
		this->small_thin_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->small_thin_button, scope);
	} else if (tag == "thinnest_button") {
		this->thinnest_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->thinnest_button, scope);
	} else if (tag == "radio_button") {
		this->radio_button = std::make_unique<checkbox_style>(this);
		database::process_gsml_data(this->radio_button, scope);
	} else if (tag == "up_arrow_button") {
		this->up_arrow_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->up_arrow_button, scope);
	} else if (tag == "down_arrow_button") {
		this->down_arrow_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->down_arrow_button, scope);
	} else if (tag == "left_arrow_button") {
		this->left_arrow_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->left_arrow_button, scope);
	} else if (tag == "right_arrow_button") {
		this->right_arrow_button = std::make_unique<button_style>(this);
		database::process_gsml_data(this->right_arrow_button, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void interface_style::initialize()
{
	for (const auto &[panel, filepath] : this->panel_files) {
		this->panel_graphics[panel] = CGraphic::New(filepath.string());
	}

	if (!this->top_bar_file.empty()) {
		this->top_bar_graphics = CGraphic::New(this->top_bar_file.string());
	}

	if (!this->dropdown_bar_file.empty()) {
		this->dropdown_bar_graphics = CGraphic::New(this->dropdown_bar_file.string());
	}

	if (!this->slider_bar_file.empty()) {
		this->slider_bar_graphics = CGraphic::New(this->slider_bar_file.string());
	}

	if (!this->slider_marker_file.empty()) {
		this->slider_marker_graphics = CGraphic::New(this->slider_marker_file.string());
	}

	if (this->large_button != nullptr) {
		this->large_button->initialize();
	}

	if (this->small_button != nullptr) {
		this->small_button->initialize();
	}

	if (this->thin_button != nullptr) {
		this->thin_button->initialize();
	}

	if (this->small_thin_button != nullptr) {
		this->small_thin_button->initialize();
	}

	if (this->thinnest_button != nullptr) {
		this->thinnest_button->initialize();
	}

	if (this->radio_button != nullptr) {
		this->radio_button->initialize();
	}

	if (this->up_arrow_button != nullptr) {
		this->up_arrow_button->initialize();
	}

	if (this->down_arrow_button != nullptr) {
		this->down_arrow_button->initialize();
	}

	if (this->left_arrow_button != nullptr) {
		this->left_arrow_button->initialize();
	}

	if (this->right_arrow_button != nullptr) {
		this->right_arrow_button->initialize();
	}

	data_entry::initialize();
}

void interface_style::set_top_bar_file(const std::filesystem::path &filepath)
{
	this->top_bar_file = database::get()->get_graphics_filepath(filepath);
}

void interface_style::set_dropdown_bar_file(const std::filesystem::path &filepath)
{
	this->dropdown_bar_file = database::get()->get_graphics_filepath(filepath);
}

void interface_style::set_slider_bar_file(const std::filesystem::path &filepath)
{
	this->slider_bar_file = database::get()->get_graphics_filepath(filepath);
}

void interface_style::set_slider_marker_file(const std::filesystem::path &filepath)
{
	this->slider_marker_file = database::get()->get_graphics_filepath(filepath);
}

const std::shared_ptr<CGraphic> &interface_style::get_interface_element_graphics(const interface_element_type type, const std::vector<std::string> &qualifiers) const
{
	switch (type) {
		case interface_element_type::top_bar:
			return this->top_bar_graphics;
		case interface_element_type::panel: {
			const int panel = std::stoi(qualifiers.at(0));
			return this->get_panel_graphics(panel);
		}
		case interface_element_type::dropdown_bar:
			return this->dropdown_bar_graphics;
		case interface_element_type::slider_bar:
			return this->slider_bar_graphics;
		case interface_element_type::slider_marker:
			return this->slider_marker_graphics;
		case interface_element_type::large_button:
		case interface_element_type::small_button:
		case interface_element_type::thin_button:
		case interface_element_type::small_thin_button:
		case interface_element_type::thinnest_button:
		case interface_element_type::up_arrow_button:
		case interface_element_type::down_arrow_button:
		case interface_element_type::left_arrow_button:
		case interface_element_type::right_arrow_button: {
			const button_style *button = this->get_button(type);
			const button_state state = string_to_button_state(qualifiers.front());
			return button->get_graphics(state);
		}
		case interface_element_type::radio_button: {
			const checkbox_style *checkbox = this->radio_button.get();
			const checkbox_state checkbox_state = string_to_checkbox_state(qualifiers.at(0));
			const button_state button_state = string_to_button_state(qualifiers.at(1));
			return checkbox->get_graphics(checkbox_state, button_state);
		}
		case interface_element_type::icon_frame:
			return defines::get()->get_icon_frame_graphics();
		case interface_element_type::pressed_icon_frame:
			return defines::get()->get_pressed_icon_frame_graphics();
		case interface_element_type::command_button_frame:
			return defines::get()->get_command_button_frame_graphics();
		default:
			throw std::runtime_error("Invalid interface element type: \"" + std::to_string(static_cast<int>(type)) + "\".");
	}
}

const button_style *interface_style::get_button(const interface_element_type type) const
{
	switch (type) {
		case interface_element_type::large_button:
			return this->large_button.get();
		case interface_element_type::small_button:
			return this->small_button.get();
		case interface_element_type::thin_button:
			return this->thin_button.get();
		case interface_element_type::small_thin_button:
			return this->small_thin_button.get();
		case interface_element_type::thinnest_button:
			return this->thinnest_button.get();
		case interface_element_type::up_arrow_button:
			return this->up_arrow_button.get();
		case interface_element_type::down_arrow_button:
			return this->down_arrow_button.get();
		case interface_element_type::left_arrow_button:
			return this->left_arrow_button.get();
		case interface_element_type::right_arrow_button:
			return this->right_arrow_button.get();
		default:
			throw std::runtime_error("Invalid button interface element type: \"" + std::to_string(static_cast<int>(type)) + "\".");
	}
}

}
