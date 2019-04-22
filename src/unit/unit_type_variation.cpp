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
/**@name unit_type_variation.cpp - The unit type variation source file. */
//
//      (c) Copyright 2014-2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "unit/unit_type_variation.h"

#include "animation/animation.h"
#include "config.h"
#include "config_operator.h"
#include "construct.h"
#include "item/item_class.h"
#include "map/terrain_type.h"
#include "module.h"
#include "time/season.h"
#include "ui/button_action.h"
#include "ui/icon.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CUnitTypeVariation::~CUnitTypeVariation()
{
	if (this->Sprite) {
		CGraphic::Free(this->Sprite);
	}
	if (this->ShadowSprite) {
		CGraphic::Free(this->ShadowSprite);
	}
	if (this->LightSprite) {
		CGraphic::Free(this->LightSprite);
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (this->LayerSprites[i]) {
			CGraphic::Free(this->LayerSprites[i]);
		}
	}
	for (int res = 0; res < MaxCosts; ++res) {
		if (this->SpriteWhenLoaded[res]) {
			CGraphic::Free(this->SpriteWhenLoaded[res]);
		}
		if (this->SpriteWhenEmpty[res]) {
			CGraphic::Free(this->SpriteWhenEmpty[res]);
		}
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CUnitTypeVariation::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		std::string key = property.Key;
		std::string value = property.Value;
		
		if (key == "variation_id") {
			value = FindAndReplaceString(value, "_", "-");
			this->VariationId = value;
		} else if (key == "layer") {
			value = FindAndReplaceString(value, "_", "-");
			this->ImageLayer = GetImageLayerIdByName(value);
			if (this->ImageLayer == -1) {
				fprintf(stderr, "Invalid image layer: \"%s\".\n", value.c_str());
			}
		} else if (key == "type_name") {
			this->TypeName = value;
		} else if (key == "file") {
			this->File = CModule::GetCurrentPath() + value;
		} else if (key == "shadow_file") {
			this->ShadowFile = CModule::GetCurrentPath() + value;
		} else if (key == "light_file") {
			this->LightFile = CModule::GetCurrentPath() + value;
		} else if (key == "frame_width") {
			this->FrameWidth = std::stoi(value);
		} else if (key == "frame_height") {
			this->FrameHeight = std::stoi(value);
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
			this->Icon.Icon->Load();
		} else if (key == "animations") {
			value = FindAndReplaceString(value, "_", "-");
			this->Animations = AnimationsByIdent(value);
			if (!this->Animations) {
				fprintf(stderr, "Invalid animations: \"%s\".\n", value.c_str());
			}
		} else if (key == "construction") {
			value = FindAndReplaceString(value, "_", "-");
			this->Construction = ConstructionByIdent(value);
			if (!this->Construction) {
				fprintf(stderr, "Invalid construction: \"%s\".\n", value.c_str());
			}
		} else if (key == "required_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade != nullptr) {
				this->UpgradesRequired.push_back(upgrade);
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
			}
		} else if (key == "forbidden_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade != nullptr) {
				this->UpgradesForbidden.push_back(upgrade);
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
			}
		} else if (key == "item_class_equipped") {
			value = FindAndReplaceString(value, "_", "-");
			const ItemClass *item_class = ItemClass::Get(value);
			if (item_class != nullptr) {
				this->ItemClassesEquipped.push_back(item_class);
			}
		} else if (key == "item_class_not_equipped") {
			value = FindAndReplaceString(value, "_", "-");
			const ItemClass *item_class = ItemClass::Get(value);
			if (item_class != nullptr) {
				this->ItemClassesNotEquipped.push_back(item_class);
			}
		} else if (key == "item_equipped") {
			value = FindAndReplaceString(value, "_", "-");
			const CUnitType *unit_type = UnitTypeByIdent(value);
			if (unit_type != nullptr) {
				this->ItemsEquipped.push_back(unit_type);
			} else {
				fprintf(stderr, "Invalid unit type: \"%s\".\n", value.c_str());
			}
		} else if (key == "item_not_equipped") {
			value = FindAndReplaceString(value, "_", "-");
			const CUnitType *unit_type = UnitTypeByIdent(value);
			if (unit_type != nullptr) {
				this->ItemsNotEquipped.push_back(unit_type);
			} else {
				fprintf(stderr, "Invalid unit type: \"%s\".\n", value.c_str());
			}
		} else if (key == "terrain") {
			value = FindAndReplaceString(value, "_", "-");
			const CTerrainType *terrain_type = CTerrainType::Get(value);
			if (terrain_type != nullptr) {
				this->Terrains.push_back(terrain_type);
			}
		} else if (key == "forbidden_terrain") {
			value = FindAndReplaceString(value, "_", "-");
			const CTerrainType *terrain_type = CTerrainType::Get(value);
			if (terrain_type != nullptr) {
				this->TerrainsForbidden.push_back(terrain_type);
			}
		} else if (key == "season") {
			value = FindAndReplaceString(value, "_", "-");
			const CSeason *season = CSeason::Get(value);
			if (season != nullptr) {
				this->Seasons.push_back(season);
			}
		} else if (key == "forbidden_season") {
			value = FindAndReplaceString(value, "_", "-");
			const CSeason *season = CSeason::Get(value);
			if (season != nullptr) {
				this->ForbiddenSeasons.push_back(season);
			}
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
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "file_when_loaded") {
			std::string file;
			int resource = -1;
				
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				std::string key = property.Key;
				std::string value = property.Value;
				
				if (key == "file") {
					file = CModule::GetCurrentPath() + value;
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
		} else if (section->Tag == "file_when_empty") {
			std::string file;
			int resource = -1;
				
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				std::string key = property.Key;
				std::string value = property.Value;
				
				if (key == "file") {
					file = CModule::GetCurrentPath() + value;
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
		} else if (section->Tag == "layer_file") {
			std::string file;
			int image_layer = -1;
			
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				std::string key = property.Key;
				std::string value = property.Value;
				
				if (key == "file") {
					file = CModule::GetCurrentPath() + value;
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
		} else if (section->Tag == "button_icon") {
			std::string icon_ident;
			int button_action = -1;
				
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				std::string key = property.Key;
				std::string value = property.Value;
				
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
			
			if (button_action == -1) {
				fprintf(stderr, "Unit type variation button icon has no button action.\n");
				continue;
			}
			
			this->ButtonIcons[button_action].Name = icon_ident;
			this->ButtonIcons[button_action].Icon = nullptr;
			this->ButtonIcons[button_action].Load();
			this->ButtonIcons[button_action].Icon->Load();
		} else {
			fprintf(stderr, "Invalid unit type variation property: \"%s\".\n", section->Tag.c_str());
		}
	}
}
