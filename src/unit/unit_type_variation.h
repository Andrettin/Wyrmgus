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

#ifndef __UNIT_TYPE_VARIATION_H__
#define __UNIT_TYPE_VARIATION_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "economy/resource.h" //for the costs enum
#include "ui/icon_config.h"
#include "unit/unit_type.h" //for the image layers enum

#include <core/object.h>

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CAnimations;
class CConstruction;
class CGraphic;
class CPlayerColorGraphic;
class CSeason;
class CTerrainType;
class CUnitType;
class ItemClass;
class PaletteImage;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class UnitTypeVariation : public Object
{
	GDCLASS(UnitTypeVariation, Object)

public:
	UnitTypeVariation(const CUnitType *type = nullptr) : Type(type)
	{
		memset(LayerSprites, 0, sizeof(LayerSprites));
		memset(SpriteWhenLoaded, 0, sizeof(SpriteWhenLoaded));
		memset(SpriteWhenEmpty, 0, sizeof(SpriteWhenEmpty));
	}
	
	~UnitTypeVariation();

	void ProcessConfigData(const CConfigData *config_data);
	
	int GetIndex() const
	{
		return this->Index;
	}
	
	const String &GetIdent() const
	{
		return this->Ident;
	}
	
	const PaletteImage *GetImage() const
	{
		return this->Image;
	}
	
	const Vector2i &GetFrameSize() const;
	
	const CIcon *GetIcon() const
	{
		return this->Icon;
	}
	
private:
	int Index = -1;					/// the variation's index within the appropriate variation vector of its unit type
	String Ident;					/// the variation's string identifier
public:
	int ImageLayer = -1;			/// the image layer to which the variation belongs (if any)
	std::string TypeName;			/// type name.
private:
	const CUnitType *Type = nullptr;	/// the unit type to which the variation belongs
	PaletteImage *Image = nullptr;	/// the variation's sprite
public:
	std::string ShadowFile;			/// variation's shadow graphics.
	std::string LightFile;			/// variation's light graphics.
	int ResourceMin = 0;
	int ResourceMax = 0;
	int Weight = 1;							/// the weight for when randomly choosing a variation
private:
	const CIcon *Icon = nullptr;			/// icon to display for this variation
public:
	CPlayerColorGraphic *Sprite = nullptr;	/// the graphic corresponding to File.
	CGraphic *ShadowSprite = nullptr;		/// the graphic corresponding to ShadowFile.
	CGraphic *LightSprite = nullptr;		/// the graphic corresponding to LightFile.
	CAnimations *Animations = nullptr;		/// animation scripts
	CConstruction *Construction = nullptr;	/// what is shown in construction phase

	std::vector<const CUpgrade *> UpgradesRequired;		/// Upgrades required by variation
	std::vector<const CUpgrade *> UpgradesForbidden;	/// If the player has one of these upgrades, the unit can't have this variation
	std::vector<const ItemClass *> ItemClassesEquipped;
	std::vector<const ItemClass *> ItemClassesNotEquipped;
	std::vector<const CUnitType *> ItemsEquipped;
	std::vector<const CUnitType *> ItemsNotEquipped;
	std::vector<const CTerrainType *> Terrains;
	std::vector<const CTerrainType *> TerrainsForbidden;
	std::vector<const CSeason *> Seasons;
	std::vector<const CSeason *> ForbiddenSeasons;

	std::string LayerFiles[MaxImageLayers];		/// Variation's layer graphics.
	std::string FileWhenLoaded[MaxCosts];		/// Change the graphic when the unit is loaded.
	std::string FileWhenEmpty[MaxCosts];		/// Change the graphic when the unit is empty.
	CPlayerColorGraphic *LayerSprites[MaxImageLayers];	/// The graphics corresponding to LayerFiles.
	CPlayerColorGraphic *SpriteWhenLoaded[MaxCosts];	/// The graphic corresponding to FileWhenLoaded.
	CPlayerColorGraphic *SpriteWhenEmpty[MaxCosts];		/// The graphic corresponding to FileWhenEmpty
	
	std::map<int, IconConfig> ButtonIcons;				/// icons for button actions
	
	friend class CUnitType;
	friend int CclDefineUnitType(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
