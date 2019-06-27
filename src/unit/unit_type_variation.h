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
class CDependency;
class CGraphic;
class CHairColor;
class CPlayerColorGraphic;
class CResource;
class CSeason;
class CSkinColor;
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
	
	const CSkinColor *GetSkinColor() const
	{
		return this->SkinColor;
	}
	
	const CHairColor *GetHairColor() const
	{
		return this->HairColor;
	}
	
	const CConstruction *GetConstruction() const
	{
		return this->Construction;
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
private:
	const CSkinColor *SkinColor = nullptr;	/// the skin color of the variation
	const CHairColor *HairColor = nullptr;	/// the hair color of the variation
public:
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
private:
	CConstruction *Construction = nullptr;	/// what is shown in construction phase

public:
	std::vector<const CUpgrade *> UpgradesRequired;		/// upgrades required by variation
	std::vector<const CUpgrade *> UpgradesForbidden;	/// if the player has one of these upgrades, the unit can't have this variation
	std::vector<const ItemClass *> ItemClassesEquipped;
	std::vector<const ItemClass *> ItemClassesNotEquipped;
	std::vector<const CUnitType *> ItemsEquipped;
	std::vector<const CUnitType *> ItemsNotEquipped;
	std::vector<const CTerrainType *> Terrains;
	std::vector<const CTerrainType *> TerrainsForbidden;
	std::vector<const CSeason *> Seasons;
	std::vector<const CSeason *> ForbiddenSeasons;

	std::string LayerFiles[MaxImageLayers];				/// the variation's layer graphics
	std::map<const CResource *, PaletteImage *> ResourceLoadedImages;	/// change the graphic when the unit is loaded with a given resource
	std::map<const CResource *, PaletteImage *> ResourceEmptyImages;	/// change the graphic when the unit is harvesting a given resource but is empty
	CPlayerColorGraphic *LayerSprites[MaxImageLayers];	/// the graphics corresponding to LayerFiles
	CPlayerColorGraphic *SpriteWhenLoaded[MaxCosts];	/// the graphic corresponding to FileWhenLoaded
	CPlayerColorGraphic *SpriteWhenEmpty[MaxCosts];		/// the graphic corresponding to FileWhenEmpty
	
	std::map<int, IconConfig> ButtonIcons;				/// icons for button actions
	CDependency *Predependency = nullptr;
	CDependency *Dependency = nullptr;
	
	friend class CUnitType;
	friend int CclDefineUnitType(lua_State *l);
	
protected:
	static void _bind_methods();
};

#endif
