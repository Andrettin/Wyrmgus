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
//      (c) Copyright 2018-2020 by Andrettin
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

#include "color.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "data_type.h"

class CGraphic;
class CPlayerColorGraphic;
class CSeason;
class CUnitType;

namespace stratagus {

class terrain_type : public named_data_entry, public data_type<terrain_type>, public CDataType
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "terrain_type";
	static constexpr const char *database_folder = "terrain_types";

	static terrain_type *add(const std::string &identifier, const stratagus::module *module)
	{
		terrain_type *terrain_type = data_type::add(identifier, module);
		terrain_type->ID = terrain_type::get_all().size() - 1;
		return terrain_type;
	}

	static void clear()
	{
		data_type::clear();

		TerrainTypesByCharacter.clear();
		TerrainTypesByColor.clear();
	}

	terrain_type(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
	{
		Color.R = 0;
		Color.G = 0;
		Color.B = 0;
		Color.A = 0;
	}
	
	~terrain_type();
	
	static void LoadTerrainTypeGraphics();
	static unsigned long GetTerrainFlagByName(const std::string &flag_name);
	
	static std::map<std::string, terrain_type *> TerrainTypesByCharacter;
	static std::map<std::tuple<int, int, int>, terrain_type *> TerrainTypesByColor;

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	CGraphic *GetGraphics(const CSeason *season = nullptr) const;

	std::string Character;
	CColor Color;
	int ID = -1;
	int SolidAnimationFrames = 0;
	int Resource = -1;
	unsigned long Flags = 0;
	bool Overlay = false;										/// Whether this terrain type belongs to the overlay layer
	bool Buildable = false;										/// Whether structures can be built upon this terrain type
	bool AllowSingle = false;									/// Whether this terrain type has transitions for single tiles
	bool Hidden = false;
	CUnitType *UnitType = nullptr;
//private:
	CGraphic *Graphics = nullptr;
	std::map<const CSeason *, CGraphic *> SeasonGraphics;		/// Graphics to be displayed instead of the normal ones during particular seasons
public:
	CGraphic *ElevationGraphics = nullptr;						/// Semi-transparent elevation graphics, separated so that borders look better
	CPlayerColorGraphic *PlayerColorGraphics = nullptr;
	std::vector<terrain_type *> BaseTerrainTypes;				/// Possible base terrain types for this terrain type (if it is an overlay terrain)
	std::vector<terrain_type *> BorderTerrains;					/// Terrain types which this one can border
	std::vector<terrain_type *> InnerBorderTerrains;			/// Terrain types which this one can border, and which "enter" this tile type in transitions
	std::vector<terrain_type *> OuterBorderTerrains;			/// Terrain types which this one can border, and which are "entered" by this tile type in transitions
	std::vector<int> SolidTiles;
	std::vector<int> DamagedTiles;
	std::vector<int> DestroyedTiles;
	std::map<std::tuple<int, int>, std::vector<int>> TransitionTiles;	/// Transition graphics, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)
	std::map<std::tuple<int, int>, std::vector<int>> AdjacentTransitionTiles;	/// Transition graphics for the tiles adjacent to this terrain type, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)
};

}
