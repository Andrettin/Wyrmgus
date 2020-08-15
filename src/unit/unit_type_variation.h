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
//      (c) Copyright 2014-2020 by Andrettin
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

#pragma once

#include "ui/icon.h"
#include "unit/unit_type.h" //for the image layers enum
#include "upgrade/upgrade_structs.h" //for the costs enum

class CConstruction;
class CGraphic;
class CPlayerColorGraphic;

int CclDefineUnitType(lua_State *l);

namespace wyrmgus {

class animation_set;
class season;
class terrain_type;
class unit_type;
enum class item_class;

class unit_type_variation
{
public:
	unit_type_variation(const std::string &identifier, const unit_type *unit_type, const int image_layer = -1)
		: identifier(identifier), unit_type(unit_type), ImageLayer(image_layer)
	{
		if (image_layer == -1) {
			this->index = static_cast<int>(unit_type->get_variations().size());
		} else {
			this->index = unit_type->LayerVariations[image_layer].size();
		}

		memset(LayerSprites, 0, sizeof(LayerSprites));
		memset(SpriteWhenLoaded, 0, sizeof(SpriteWhenLoaded));
		memset(SpriteWhenEmpty, 0, sizeof(SpriteWhenEmpty));
	}
	
	~unit_type_variation();

	std::unique_ptr<unit_type_variation> duplicate(const unit_type *unit_type) const
	{
		auto variation = std::make_unique<unit_type_variation>(this->identifier, unit_type, this->ImageLayer);

		variation->TypeName = this->TypeName;
		variation->button_key = this->button_key;
		variation->image_file = this->image_file;
		for (unsigned int i = 0; i < MaxCosts; ++i) {
			variation->FileWhenLoaded[i] = this->FileWhenLoaded[i];
			variation->FileWhenEmpty[i] = this->FileWhenEmpty[i];
		}
		variation->ShadowFile = this->ShadowFile;
		variation->LightFile = this->LightFile;
		variation->FrameWidth = this->FrameWidth;
		variation->FrameHeight = this->FrameHeight;
		variation->ResourceMin = this->ResourceMin;
		variation->ResourceMax = this->ResourceMax;
		variation->Weight = this->Weight;
		variation->Icon = this->Icon;
		variation->Animations = this->Animations;
		variation->Construction = this->Construction;
		variation->UpgradesRequired = this->UpgradesRequired;
		variation->UpgradesForbidden = this->UpgradesForbidden;
		variation->item_classes_equipped = this->item_classes_equipped;
		variation->item_classes_not_equipped = this->item_classes_not_equipped;
		variation->ItemsEquipped = this->ItemsEquipped;
		variation->ItemsNotEquipped = this->ItemsNotEquipped;
		variation->Terrains = this->Terrains;
		for (int i = 0; i < MaxImageLayers; ++i) {
			variation->LayerFiles[i] = this->LayerFiles[i];
		}
		variation->ButtonIcons = this->ButtonIcons;

		return variation;
	}

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	void ProcessConfigData(const CConfigData *config_data);

	const std::string &get_identifier() const
	{
		return this->identifier;
	}

	int get_index() const
	{
		return this->index;
	}

	const wyrmgus::unit_type *get_unit_type() const
	{
		return this->unit_type;
	}
	
	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}
	
	const std::string &get_button_key() const
	{
		return this->button_key;
	}
	
private:
	std::string identifier;
	int index = -1;					//the variation's index within the appropriate variation vector of its unit type
	const wyrmgus::unit_type *unit_type = nullptr; //the unit type to which the variation belongs
public:
	int ImageLayer = -1;			/// The image layer to which the variation belongs (if any)
private:
	std::filesystem::path image_file;
public:
	std::string TypeName;			/// Type name.
private:
	std::string button_key;
public:
	std::string ShadowFile;			/// Variation's shadow graphics.
	std::string LightFile;			/// Variation's light graphics.
	int FrameWidth = 0;
	int FrameHeight = 0;
	int ResourceMin = 0;
	int ResourceMax = 0;
	int Weight = 1;							/// The weight for when randomly choosing a variation
	IconConfig Icon;						/// Icon to display for this unit
	CPlayerColorGraphic *Sprite = nullptr;	/// The graphic corresponding to File.
	CGraphic *ShadowSprite = nullptr;		/// The graphic corresponding to ShadowFile.
	CGraphic *LightSprite = nullptr;		/// The graphic corresponding to LightFile.
	animation_set *Animations = nullptr;		/// Animation scripts
	CConstruction *Construction = nullptr;	/// What is shown in construction phase

	std::vector<const CUpgrade *> UpgradesRequired;		/// Upgrades required by variation
	std::vector<const CUpgrade *> UpgradesForbidden;	/// If the player has one of these upgrades, the unit can't have this variation
	std::set<item_class> item_classes_equipped;
	std::set<item_class> item_classes_not_equipped;
	std::vector<const wyrmgus::unit_type *> ItemsEquipped;
	std::vector<const wyrmgus::unit_type *> ItemsNotEquipped;
	std::vector<const terrain_type *> Terrains;
	std::vector<const terrain_type *> TerrainsForbidden;
	std::vector<const season *> Seasons;
	std::vector<const season *> ForbiddenSeasons;

	std::string LayerFiles[MaxImageLayers];	/// Variation's layer graphics.
	std::string FileWhenLoaded[MaxCosts];     /// Change the graphic when the unit is loaded.
	std::string FileWhenEmpty[MaxCosts];      /// Change the graphic when the unit is empty.
	CPlayerColorGraphic *LayerSprites[MaxImageLayers];	/// The graphics corresponding to LayerFiles.
	CPlayerColorGraphic *SpriteWhenLoaded[MaxCosts]; /// The graphic corresponding to FileWhenLoaded.
	CPlayerColorGraphic *SpriteWhenEmpty[MaxCosts];  /// The graphic corresponding to FileWhenEmpty
	
	std::map<ButtonCmd, IconConfig> ButtonIcons;				/// icons for button actions

	friend int ::CclDefineUnitType(lua_State *l);
	friend class unit_type;
};

}
