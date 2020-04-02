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
/**@name terrain_type.h - The terrain type headerfile. */
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

#ifndef __TERRAIN_TYPE_H__
#define __TERRAIN_TYPE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "color.h"
#include "data_type.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CGraphic;
class CPlayerColorGraphic;
class CSeason;
class CUnitType;

class CTerrainType : public CDataType
{
public:
	CTerrainType()
	{
		Color.R = 0;
		Color.G = 0;
		Color.B = 0;
		Color.A = 0;
	}
	
	~CTerrainType();
	
	static CTerrainType *GetTerrainType(const std::string &ident, const bool should_find = true);
	static CTerrainType *GetOrAddTerrainType(const std::string &ident);
	static void LoadTerrainTypeGraphics();
	static void ClearTerrainTypes();
	static unsigned long GetTerrainFlagByName(const std::string &flag_name);
	
	static std::vector<CTerrainType *>  TerrainTypes;
	static std::map<std::string, CTerrainType *> TerrainTypesByIdent;
	static std::map<std::string, CTerrainType *> TerrainTypesByCharacter;
	static std::map<std::tuple<int, int, int>, CTerrainType *> TerrainTypesByColor;

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	CGraphic *GetGraphics(const CSeason *season = nullptr) const;

	std::string Name;
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
	PixelSize PixelTileSize = PixelSize(32, 32);
	CUnitType *UnitType = nullptr;
//private:
	CGraphic *Graphics = nullptr;
	std::map<const CSeason *, CGraphic *> SeasonGraphics;		/// Graphics to be displayed instead of the normal ones during particular seasons
public:
	CGraphic *ElevationGraphics = nullptr;						/// Semi-transparent elevation graphics, separated so that borders look better
	CPlayerColorGraphic *PlayerColorGraphics = nullptr;
	std::vector<CTerrainType *> BaseTerrainTypes;				/// Possible base terrain types for this terrain type (if it is an overlay terrain)
	std::vector<CTerrainType *> BorderTerrains;					/// Terrain types which this one can border
	std::vector<CTerrainType *> InnerBorderTerrains;			/// Terrain types which this one can border, and which "enter" this tile type in transitions
	std::vector<CTerrainType *> OuterBorderTerrains;			/// Terrain types which this one can border, and which are "entered" by this tile type in transitions
	std::vector<int> SolidTiles;
	std::vector<int> DamagedTiles;
	std::vector<int> DestroyedTiles;
	std::map<std::tuple<int, int>, std::vector<int>> TransitionTiles;	/// Transition graphics, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)
	std::map<std::tuple<int, int>, std::vector<int>> AdjacentTransitionTiles;	/// Transition graphics for the tiles adjacent to this terrain type, mapped to the tile type (-1 means any tile) and the transition type (i.e. northeast outer)
};

#endif
