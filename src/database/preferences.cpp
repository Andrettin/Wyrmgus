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

#include "database/preferences.h"

#include "database/database.h"
#include "database/defines.h"
#include "database/sml_data.h"
#include "database/sml_parser.h"
#include "quest/campaign.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

std::filesystem::path preferences::get_path()
{
	return database::get_user_data_path() / "preferences.txt";
}

std::filesystem::path preferences::get_fallback_path()
{
	return database::get_documents_path() / "preferences.txt";
}

void preferences::load()
{
	std::filesystem::path preferences_path = preferences::get_path();

	if (!std::filesystem::exists(preferences_path)) {
		preferences_path = preferences::get_fallback_path();

		if (!std::filesystem::exists(preferences_path)) {
			return;
		}
	}

	sml_parser parser;
	const sml_data data = parser.parse(preferences_path);
	database::process_sml_data(this, data);
}

void preferences::save() const
{
	const std::filesystem::path preferences_path = preferences::get_path();

	sml_data data(preferences_path.filename().stem().string());

	data.add_property("scale_factor", std::to_string(this->get_scale_factor()));
	if (this->get_selected_campaign() != nullptr) {
		data.add_property("selected_campaign", this->get_selected_campaign()->get_identifier());
	}
	if (this->is_fullscreen()) {
		data.add_property("fullscreen", string::from_bool(this->is_fullscreen()));
	}

	try {
		data.print_to_file(preferences_path);
	} catch (const std::exception &exception) {
		exception::report(exception);
		log::log_error("Failed to save preferences file.");
	}
}

void preferences::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();

	if (key == "selected_campaign") {
		//use a try-catch for the selected campaign, as it could point to a campaign which no longer exists
		try {
			database::process_sml_property_for_object(this, property);
		} catch (const std::runtime_error &exception) {
			exception::report(exception);
		}
	} else {
		database::process_sml_property_for_object(this, property);
	}
}

void preferences::process_sml_scope(const sml_data &scope)
{
	database::process_sml_scope_for_object(this, scope);
}

void preferences::set_scale_factor(const int factor)
{
	if (factor == this->get_scale_factor()) {
		return;
	}

	this->scale_factor = factor;
}

}
