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
/**@name unit_type_variation.h - The unit type variation header file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "ui/icon.h"
#include "unit/unit_type.h" //for the image layers enum
#include "upgrade/upgrade_structs.h" //for the costs enum

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAnimations;
class CConstruction;
class CGraphic;
class CPlayerColorGraphic;
class CUnitType;

namespace stratagus {
	class season;
	class terrain_type;
}

class CUnitTypeVariation
{
public:
	CUnitTypeVariation()
	{
		memset(LayerSprites, 0, sizeof(LayerSprites));
		memset(SpriteWhenLoaded, 0, sizeof(SpriteWhenLoaded));
		memset(SpriteWhenEmpty, 0, sizeof(SpriteWhenEmpty));
	}
	
	~CUnitTypeVariation();

	void ProcessConfigData(const CConfigData *config_data);
	
	int ID = -1;					/// The variation's index within the appropriate variation vector of its unit type
	int ImageLayer = -1;			/// The image layer to which the variation belongs (if any)
	std::string VariationId;		/// Variation's name.
	std::string TypeName;			/// Type name.
	std::string File;				/// Variation's graphics.
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
	CAnimations *Animations = nullptr;		/// Animation scripts
	CConstruction *Construction = nullptr;	/// What is shown in construction phase

	std::vector<const CUpgrade *> UpgradesRequired;		/// Upgrades required by variation
	std::vector<const CUpgrade *> UpgradesForbidden;	/// If the player has one of these upgrades, the unit can't have this variation
	std::vector<int> ItemClassesEquipped;
	std::vector<int> ItemClassesNotEquipped;
	std::vector<const CUnitType *> ItemsEquipped;
	std::vector<const CUnitType *> ItemsNotEquipped;
	std::vector<const stratagus::terrain_type *> Terrains;
	std::vector<const stratagus::terrain_type *> TerrainsForbidden;
	std::vector<const stratagus::season *> Seasons;
	std::vector<const stratagus::season *> ForbiddenSeasons;

	std::string LayerFiles[MaxImageLayers];	/// Variation's layer graphics.
	std::string FileWhenLoaded[MaxCosts];     /// Change the graphic when the unit is loaded.
	std::string FileWhenEmpty[MaxCosts];      /// Change the graphic when the unit is empty.
	CPlayerColorGraphic *LayerSprites[MaxImageLayers];	/// The graphics corresponding to LayerFiles.
	CPlayerColorGraphic *SpriteWhenLoaded[MaxCosts]; /// The graphic corresponding to FileWhenLoaded.
	CPlayerColorGraphic *SpriteWhenEmpty[MaxCosts];  /// The graphic corresponding to FileWhenEmpty
	
	std::map<ButtonCmd, IconConfig> ButtonIcons;				/// icons for button actions
};
