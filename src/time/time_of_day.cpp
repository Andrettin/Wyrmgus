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
//

#include "stratagus.h"

#include "time/time_of_day.h"

#include "config.h"
#include "mod.h"
#include "util/string_util.h"
#include "video.h"

namespace stratagus {

void time_of_day::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "image") {
		std::filesystem::path filepath;
		Vec2i size(0, 0);

		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			if (key == "file") {
				filepath = database::get_graphics_path(this->get_module()) / value;
			} else if (key == "width") {
				size.x = std::stoi(value);
			} else if (key == "height") {
				size.y = std::stoi(value);
			} else {
				throw std::runtime_error("Invalid image property: \"" + key + "\".");
			}
		});

		if (filepath.empty()) {
			throw std::runtime_error("Image has no file.");
		}

		if (size.x == 0) {
			throw std::runtime_error("Image has no width.");
		}

		if (size.y == 0) {
			throw std::runtime_error("Image has no height.");
		}

		this->G = CGraphic::New(filepath.string(), size.x, size.y);
	} else if (tag == "color_modification") {
		const std::vector<std::string> &values = scope.get_values();
		this->ColorModification.R = std::stoi(values[0]);
		this->ColorModification.G = std::stoi(values[1]);
		this->ColorModification.B = std::stoi(values[2]);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void time_of_day::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "dawn") {
			this->dawn = string::to_bool(value);
		} else if (key == "day") {
			this->day = string::to_bool(value);
		} else if (key == "dusk") {
			this->dusk = string::to_bool(value);
		} else if (key == "night") {
			this->night = string::to_bool(value);
		} else {
			fprintf(stderr, "Invalid time of day property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "color_modification") {
			this->ColorModification.ProcessConfigData(child_config_data);
		} else if (child_config_data->Tag == "image") {
			std::string file;
			Vec2i size(0, 0);
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "file") {
					file = CMod::GetCurrentModPath() + value;
				} else if (key == "width") {
					size.x = std::stoi(value);
				} else if (key == "height") {
					size.y = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid image property: \"%s\".\n", key.c_str());
				}
			}
			
			if (file.empty()) {
				fprintf(stderr, "Image has no file.\n");
				continue;
			}
			
			if (size.x == 0) {
				fprintf(stderr, "Image has no width.\n");
				continue;
			}
			
			if (size.y == 0) {
				fprintf(stderr, "Image has no height.\n");
				continue;
			}
			
			this->G = CGraphic::New(file, size.x, size.y);
			this->G->Load();
			this->G->UseDisplayFormat();
		} else {
			fprintf(stderr, "Invalid time of day property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

/**
**	@brief	Gets whether the time of day modifies the color of graphics
**
**	@return	Whether the time of day modifies the color of graphics
*/
bool time_of_day::HasColorModification() const
{
	return this->ColorModification.R != 0 || this->ColorModification.G != 0 || this->ColorModification.B != 0;
}

}