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
#include "dependency/and_dependency.h"
#include "dependency/dependency.h"
#include "economy/resource.h"
#include "hair_color.h"
#include "item/item_class.h"
#include "map/terrain_type.h"
#include "module.h"
#include "skin_color.h"
#include "time/season.h"
#include "ui/button_action.h"
#include "ui/icon.h"
#include "upgrade/upgrade.h"
#include "video/palette_image.h"
#include "video/video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

UnitTypeVariation::~UnitTypeVariation()
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
void UnitTypeVariation::ProcessConfigData(const CConfigData *config_data)
{
	this->Ident = config_data->Ident;
	
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.ProcessForObject(*this)) {
			continue;
		}
		
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
			continue;
		}
		
		String key = property.Key;
		String value = property.Value;
		
		if (key == "layer") {
			value = value.replace("_", "-");
			this->ImageLayer = GetImageLayerIdByName(value.utf8().get_data());
			if (this->ImageLayer == -1) {
				fprintf(stderr, "Invalid image layer: \"%s\".\n", value.utf8().get_data());
			}
		} else if (key == "type_name") {
			this->TypeName = value.utf8().get_data();
		} else if (key == "shadow_file") {
			this->ShadowFile = CModule::GetCurrentPath() + value.utf8().get_data();
		} else if (key == "light_file") {
			this->LightFile = CModule::GetCurrentPath() + value.utf8().get_data();
		} else if (key == "animations") {
			value = value.replace("_", "-");
			this->Animations = AnimationsByIdent(value.utf8().get_data());
			if (!this->Animations) {
				fprintf(stderr, "Invalid animations: \"%s\".\n", value.utf8().get_data());
			}
		} else if (key == "construction") {
			value = value.replace("_", "-");
			this->Construction = ConstructionByIdent(value.utf8().get_data());
			if (!this->Construction) {
				fprintf(stderr, "Invalid construction: \"%s\".\n", value.utf8().get_data());
			}
		} else if (key == "required_upgrade") {
			value = value.replace("_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value.utf8().get_data());
			if (upgrade != nullptr) {
				this->UpgradesRequired.push_back(upgrade);
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.utf8().get_data());
			}
		} else if (key == "forbidden_upgrade") {
			value = value.replace("_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value.utf8().get_data());
			if (upgrade != nullptr) {
				this->UpgradesForbidden.push_back(upgrade);
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.utf8().get_data());
			}
		} else if (key == "item_class_equipped") {
			const ItemClass *item_class = ItemClass::Get(value);
			if (item_class != nullptr) {
				this->ItemClassesEquipped.push_back(item_class);
			}
		} else if (key == "item_class_not_equipped") {
			const ItemClass *item_class = ItemClass::Get(value);
			if (item_class != nullptr) {
				this->ItemClassesNotEquipped.push_back(item_class);
			}
		} else if (key == "item_equipped") {
			const CUnitType *unit_type = CUnitType::Get(value);
			if (unit_type != nullptr) {
				this->ItemsEquipped.push_back(unit_type);
			} else {
				fprintf(stderr, "Invalid unit type: \"%s\".\n", value.utf8().get_data());
			}
		} else if (key == "item_not_equipped") {
			const CUnitType *unit_type = CUnitType::Get(value);
			if (unit_type != nullptr) {
				this->ItemsNotEquipped.push_back(unit_type);
			} else {
				fprintf(stderr, "Invalid unit type: \"%s\".\n", value.utf8().get_data());
			}
		} else if (key == "terrain") {
			const CTerrainType *terrain_type = CTerrainType::Get(value);
			if (terrain_type != nullptr) {
				this->Terrains.push_back(terrain_type);
			}
		} else if (key == "forbidden_terrain") {
			const CTerrainType *terrain_type = CTerrainType::Get(value);
			if (terrain_type != nullptr) {
				this->TerrainsForbidden.push_back(terrain_type);
			}
		} else if (key == "season") {
			const CSeason *season = CSeason::Get(value);
			if (season != nullptr) {
				this->Seasons.push_back(season);
			}
		} else if (key == "forbidden_season") {
			const CSeason *season = CSeason::Get(value);
			if (season != nullptr) {
				this->ForbiddenSeasons.push_back(season);
			}
		} else if (key == "resource_min") {
			this->ResourceMin = value.to_int();
		} else if (key == "resource_max") {
			this->ResourceMax = value.to_int();
		} else if (key == "weight") {
			this->Weight = value.to_int();
		} else if (key.find("_loaded_image") != -1) {
			String resource_ident = key.replace("_loaded_image", "");
			const CResource *resource = CResource::Get(resource_ident);
			this->ResourceLoadedImages[resource] = PaletteImage::Get(value);
		} else if (key.find("_empty_image") != -1) {
			String resource_ident = key.replace("_empty_image", "");
			const CResource *resource = CResource::Get(resource_ident);
			this->ResourceEmptyImages[resource] = PaletteImage::Get(value);
		} else {
			fprintf(stderr, "Invalid unit type variation property: \"%s\".\n", key.utf8().get_data());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "image") {
			String image_ident = this->Type->GetIdent() + "_" + this->GetIdent();
			image_ident = image_ident.replace("_", "-");
			PaletteImage *image = PaletteImage::GetOrAdd(image_ident.utf8().get_data());
			image->ProcessConfigData(section);
			this->Image = image;
		} else if (section->Tag.find("_loaded_image") != -1) {
			String resource_ident = section->Tag.replace("_loaded_image", "");
			const CResource *resource = CResource::Get(resource_ident);
			
			if (resource == nullptr) {
				print_error("Unit type variation file when loaded has no resource.");
				continue;
			}
			
			String image_ident = this->Type->GetIdent() + "_" + this->GetIdent() + "_" + resource->GetIdent() + "_loaded";
			image_ident = image_ident.replace("_", "-");
			PaletteImage *image = PaletteImage::GetOrAdd(image_ident.utf8().get_data());
			image->ProcessConfigData(section);
			this->ResourceLoadedImages[resource] = image;
		} else if (section->Tag.find("_empty_image") != -1) {
			String resource_ident = section->Tag.replace("_empty_image", "");
			const CResource *resource = CResource::Get(resource_ident);
			
			if (resource == nullptr) {
				print_error("Unit type variation file when loaded has no resource.");
				continue;
			}
			
			String image_ident = this->Type->GetIdent() + "_" + this->GetIdent() + "_" + resource->GetIdent() + "_empty";
			image_ident = image_ident.replace("_", "-");
			PaletteImage *image = PaletteImage::GetOrAdd(image_ident.utf8().get_data());
			image->ProcessConfigData(section);
			this->ResourceEmptyImages[resource] = image;
		} else if (section->Tag == "layer_file") {
			std::string file;
			int image_layer = -1;
			
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
					continue;
				}
				
				String key = property.Key;
				String value = property.Value;
				
				if (key == "file") {
					file = CModule::GetCurrentPath() + value.utf8().get_data();
				} else if (key == "image_layer") {
					value = value.replace("_", "-");
					image_layer = GetImageLayerIdByName(value.utf8().get_data());
				} else {
					fprintf(stderr, "Invalid unit type variation layer file property: \"%s\".\n", key.utf8().get_data());
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
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
					continue;
				}
				
				String key = property.Key;
				String value = property.Value;
				
				if (key == "icon") {
					value = value.replace("_", "-");
					icon_ident = value.utf8().get_data();
				} else if (key == "button_action") {
					value = value.replace("_", "-");
					button_action = GetButtonActionIdByName(value.utf8().get_data());
				} else {
					fprintf(stderr, "Invalid unit type variation button icon property: \"%s\".\n", key.utf8().get_data());
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
		} else if (section->Tag == "predependencies") {
			this->Predependency = new CAndDependency;
			this->Predependency->ProcessConfigData(section);
		} else if (section->Tag == "dependencies") {
			this->Dependency = new CAndDependency;
			this->Dependency->ProcessConfigData(section);
		} else {
			fprintf(stderr, "Invalid unit type variation property: \"%s\".\n", section->Tag.utf8().get_data());
		}
	}
}

const Vector2i &UnitTypeVariation::GetFrameSize() const
{
	if (this->GetImage() != nullptr) {
		return this->GetImage()->GetFrameSize();
	}
	
	return PaletteImage::EmptyFrameSize;
}

void UnitTypeVariation::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_image", "ident"), +[](UnitTypeVariation *variation, const String &ident){ variation->Image = PaletteImage::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_image"), +[](const UnitTypeVariation *variation){ return const_cast<PaletteImage *>(variation->GetImage()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "image"), "set_image", "get_image");
	
	ClassDB::bind_method(D_METHOD("set_icon", "ident"), +[](UnitTypeVariation *variation, const String &ident){
		CIcon *icon = CIcon::Get(ident);
		variation->Icon = icon;
		
		if (icon != nullptr) {
			icon->Load();
		}
	});
	ClassDB::bind_method(D_METHOD("get_icon"), +[](const UnitTypeVariation *variation){ return const_cast<CIcon *>(variation->GetIcon()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon"), "set_icon", "get_icon");
	
	ClassDB::bind_method(D_METHOD("set_skin_color", "ident"), +[](UnitTypeVariation *variation, const String &ident){ variation->SkinColor = CSkinColor::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_skin_color"), +[](const UnitTypeVariation *variation){ return const_cast<CSkinColor *>(variation->GetSkinColor()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "skin_color"), "set_skin_color", "get_skin_color");
	
	ClassDB::bind_method(D_METHOD("set_hair_color", "ident"), +[](UnitTypeVariation *variation, const String &ident){ variation->HairColor = CHairColor::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_hair_color"), +[](const UnitTypeVariation *variation){ return const_cast<CHairColor *>(variation->GetHairColor()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "hair_color"), "set_hair_color", "get_hair_color");
}
