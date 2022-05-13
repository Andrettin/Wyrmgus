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
//      (c) Copyright 2014-2022 by Andrettin
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

#include "unit/unit_type_variation.h"

#include "animation/animation_set.h"
#include "config.h"
#include "item/item_class.h"
#include "map/terrain_type.h"
#include "mod.h"
#include "script/condition/and_condition.h"
#include "time/season.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "unit/construction.h"
#include "unit/variation_tag.h"
#include "util/util.h"
#include "video/video.h"

namespace wyrmgus {

unit_type_variation::unit_type_variation(const std::string &identifier, const wyrmgus::unit_type *unit_type, const int image_layer)
	: identifier(identifier), unit_type(unit_type), ImageLayer(image_layer)
{
	if (image_layer == -1) {
		this->index = static_cast<int>(unit_type->get_variations().size());
	} else {
		this->index = unit_type->LayerVariations[image_layer].size();
	}
}

unit_type_variation::~unit_type_variation()
{
}

void unit_type_variation::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type_name") {
		this->type_name = value;
	} else if (key == "image_file") {
		this->image_file = database::get()->get_graphics_filepath(value);
	} else if (key == "icon") {
		this->Icon.Name = value;
	} else if (key == "weight") {
		this->Weight = std::stoi(value);
	} else if (key == "animation_set") {
		this->animation_set = animation_set::get(value);
	} else if (key == "resource_min") {
		this->ResourceMin = std::stoi(value);
	} else if (key == "resource_max") {
		this->ResourceMax = std::stoi(value);
	} else {
		database::process_gsml_property_for_object(this, property);
	}
}

void unit_type_variation::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "frame_size") {
		this->frame_size = scope.to_size();
	} else if (tag == "resource_image_files") {
		scope.for_each_property([&](const gsml_property &property) {
			const resource *resource = resource::get(property.get_key());
			this->FileWhenEmpty[resource->get_index()] = database::get()->get_graphics_filepath(property.get_value());
		});
	} else if (tag == "resource_loaded_image_files") {
		scope.for_each_property([&](const gsml_property &property) {
			const resource *resource = resource::get(property.get_key());
			this->FileWhenLoaded[resource->get_index()] = database::get()->get_graphics_filepath(property.get_value());
		});
	} else if (tag == "player_conditions") {
		auto conditions = std::make_unique<and_condition>();
		database::process_gsml_data(conditions, scope);
		this->player_conditions = std::move(conditions);
		this->player_conditions_ptr = this->player_conditions.get();
	} else if (tag == "conditions" || tag == "unit_conditions") {
		auto conditions = std::make_unique<and_condition>();
		database::process_gsml_data(conditions, scope);
		this->unit_conditions = std::move(conditions);
		this->unit_conditions_ptr = this->unit_conditions.get();
	} else if (tag == "tags") {
		for (const std::string &value : values) {
			this->tags.insert(variation_tag::get(value));
		}
	} else {
		throw std::runtime_error("Invalid unit type variation scope: \"" + tag + "\".");
	}
}

void unit_type_variation::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "type_name") {
			this->type_name = value;
		} else if (key == "button_key") {
			this->button_key = value;
		} else if (key == "shadow_file") {
			this->ShadowFile = CMod::GetCurrentModPath() + value;
		} else if (key == "light_file") {
			this->LightFile = CMod::GetCurrentModPath() + value;
		} else if (key == "frame_width") {
			this->frame_size.setWidth(std::stoi(value));
		} else if (key == "frame_height") {
			this->frame_size.setHeight(std::stoi(value));
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
		} else if (key == "animations") {
			this->animation_set = animation_set::get(value);
		} else if (key == "construction") {
			this->construction = construction::get(value);
		} else if (key == "required_upgrade") {
			const CUpgrade *upgrade = CUpgrade::get(value);
			this->UpgradesRequired.push_back(upgrade);
		} else if (key == "forbidden_upgrade") {
			const CUpgrade *upgrade = CUpgrade::get(value);
			this->UpgradesForbidden.push_back(upgrade);
		} else if (key == "item_class_equipped") {
			this->item_classes_equipped.insert(string_to_item_class(value));
		} else if (key == "item_class_not_equipped") {
			this->item_classes_not_equipped.insert(string_to_item_class(value));
		} else if (key == "item_equipped") {
			const wyrmgus::unit_type *unit_type = unit_type::get(value);
			this->ItemsEquipped.push_back(unit_type);
		} else if (key == "item_not_equipped") {
			const wyrmgus::unit_type *unit_type = unit_type::get(value);
			this->ItemsNotEquipped.push_back(unit_type);
		} else if (key == "terrain") {
			const terrain_type *terrain_type = terrain_type::get(value);
			this->Terrains.push_back(terrain_type);
		} else if (key == "forbidden_terrain") {
			const terrain_type *terrain_type = terrain_type::get(value);
			this->TerrainsForbidden.push_back(terrain_type);
		} else if (key == "season") {
			const season *season = season::get(value);
			this->Seasons.push_back(season);
		} else if (key == "forbidden_season") {
			const season *season = season::get(value);
			this->ForbiddenSeasons.push_back(season);
		} else if (key == "resource_min") {
			this->ResourceMin = std::stoi(value);
		} else if (key == "resource_max") {
			this->ResourceMax = std::stoi(value);
		} else if (key == "weight") {
			this->Weight = std::stoi(value);
		} else {
			fprintf(stderr, "Invalid unit type variation property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "file_when_loaded") {
			std::string file;
			int resource = -1;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "file") {
					file = CMod::GetCurrentModPath() + value;
				} else if (key == "resource") {
					value = FindAndReplaceString(value, "_", "-");
					resource = GetResourceIdByName(value.c_str());
				} else {
					fprintf(stderr, "Invalid unit type variation file when loaded property: \"%s\".\n", key.c_str());
				}
			}
			
			if (file.empty()) {
				fprintf(stderr, "Unit type variation file when loaded has no file.\n");
				continue;
			}
			
			if (resource == -1) {
				fprintf(stderr, "Unit type variation file when loaded has no resource.\n");
				continue;
			}
			
			this->FileWhenLoaded[resource] = file;
		} else if (child_config_data->Tag == "file_when_empty") {
			std::string file;
			int resource = -1;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "file") {
					file = CMod::GetCurrentModPath() + value;
				} else if (key == "resource") {
					value = FindAndReplaceString(value, "_", "-");
					resource = GetResourceIdByName(value.c_str());
				} else {
					fprintf(stderr, "Invalid unit type variation file when empty property: \"%s\".\n", key.c_str());
				}
			}
			
			if (file.empty()) {
				fprintf(stderr, "Unit type variation file when empty has no file.\n");
				continue;
			}
			
			if (resource == -1) {
				fprintf(stderr, "Unit type variation file when empty has no resource.\n");
				continue;
			}
			
			this->FileWhenEmpty[resource] = file;
		} else if (child_config_data->Tag == "layer_file") {
			std::string file;
			int image_layer = -1;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "file") {
					file = CMod::GetCurrentModPath() + value;
				} else if (key == "image_layer") {
					value = FindAndReplaceString(value, "_", "-");
					image_layer = GetImageLayerIdByName(value);
				} else {
					fprintf(stderr, "Invalid unit type variation layer file property: \"%s\".\n", key.c_str());
				}
			}
			
			if (file.empty()) {
				fprintf(stderr, "Unit type variation layer file has no file.\n");
				continue;
			}
			
			if (image_layer == -1) {
				fprintf(stderr, "Unit type variation layer file has no image layer.\n");
				continue;
			}
			
			this->LayerFiles[image_layer] = file;
		} else if (child_config_data->Tag == "button_icon") {
			std::string icon_ident;
			ButtonCmd button_action = ButtonCmd::None;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "icon") {
					value = FindAndReplaceString(value, "_", "-");
					icon_ident = value;
				} else if (key == "button_action") {
					value = FindAndReplaceString(value, "_", "-");
					button_action = GetButtonActionIdByName(value);
				} else {
					fprintf(stderr, "Invalid unit type variation button icon property: \"%s\".\n", key.c_str());
				}
			}
			
			if (icon_ident.empty()) {
				fprintf(stderr, "Unit type variation button icon has no icon.\n");
				continue;
			}
			
			if (button_action == ButtonCmd::None) {
				fprintf(stderr, "Unit type variation button icon has no button action.\n");
				continue;
			}
			
			this->ButtonIcons[button_action].Name = icon_ident;
			this->ButtonIcons[button_action].Icon = nullptr;
			this->ButtonIcons[button_action].Load();
		} else {
			fprintf(stderr, "Invalid unit type variation property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void unit_type_variation::check() const
{
	if (this->get_player_conditions() != nullptr) {
		this->get_player_conditions()->check_validity();
	}

	if (this->get_unit_conditions() != nullptr) {
		this->get_unit_conditions()->check_validity();
	}
}

}
