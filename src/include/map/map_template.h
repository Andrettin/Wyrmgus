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
/**@name map_template.h - The map template header file. */
//
//      (c) Copyright 2018 by Andrettin
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

#ifndef __MAP_TEMPLATE_H__
#define __MAP_TEMPLATE_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>
#include <vector>

#include "map/tile.h"
#include "time/date.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CConfigData;
class CFaction;
class CPlane;
class CSite;
class CTerrainType;
class CUniqueItem;
class CUnitType;
class CWorld;

class CGeneratedTerrain
{
public:
	CTerrainType *TerrainType = nullptr;
	int SeedCount = 0;
	int ExpansionChance = 50; //50% chance to expand to a tile by default
	bool UseExistingAsSeeds = false; //whether to use existing tiles of the given terrain in the map layer as seeds for this terrain generation
};

class CMapTemplate
{
public:
	CMapTemplate() :
		Width(0), Height(0), Scale(1), SurfaceLayer(0),
		Overland(false), OutputTerrainImage(false),
		SubtemplatePosition(-1, -1), CurrentStartPos(0, 0), PixelTileSize(32, 32),
		MainTemplate(nullptr), Plane(nullptr), World(nullptr),
		BaseTerrainType(nullptr), BaseOverlayTerrainType(nullptr), BorderTerrainType(nullptr), SurroundingTerrainType(nullptr)
	{
	}
	
	~CMapTemplate();

	static CMapTemplate *GetMapTemplate(const std::string &ident);
	static CMapTemplate *GetOrAddMapTemplate(const std::string &ident);
	static void ClearMapTemplates();
	
	static std::vector<CMapTemplate *> MapTemplates;								/// Map templates
	static std::map<std::string, CMapTemplate *> MapTemplatesByIdent;

	void ProcessConfigData(const CConfigData *config_data);
	void ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z);
	void ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z);
	void Apply(Vec2i template_start_pos, Vec2i map_start_pos, int z);
	void ApplySites(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random = false);
	void ApplyConnectors(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random = false);
	void ApplyUnits(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random = false);
	bool IsSubtemplateArea() const;
	CMapTemplate *GetTopMapTemplate();
	
	std::string Name;
	std::string Ident;
	std::string TerrainFile;
	std::string OverlayTerrainFile;
	std::string TerrainImage;
	std::string OverlayTerrainImage;
	int Width;
	int Height;
	int Scale;													/// 1 means a map template tile will be applied as one in-game tile, 2 means a 2x2 in-game tile
	int SurfaceLayer;											/// Surface layer of the map template (0 for surface, 1 and above for underground layers in succession)
	bool Overland;												/// Whether this is an overland map
	bool OutputTerrainImage;
	Vec2i SubtemplatePosition;
	Vec2i CurrentStartPos;
	PixelSize PixelTileSize;
	CMapTemplate *MainTemplate;									/// Main template in which this one is located
	CPlane *Plane;
	CWorld *World;
	CTerrainType *BaseTerrainType;
	CTerrainType *BaseOverlayTerrainType;
	CTerrainType *BorderTerrainType;
	CTerrainType *SurroundingTerrainType;
	std::vector<CMapTemplate *> Subtemplates;
	std::vector<CGeneratedTerrain *> GeneratedTerrains;				/// terrains generated in the map template
	std::vector<std::pair<CUnitType *, int>> GeneratedNeutralUnits; /// the first element of the pair is the resource's unit type, and the second is the quantity
	std::vector<std::pair<CUnitType *, int>> PlayerLocationGeneratedNeutralUnits;
	std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>> Resources; /// Resources (with unit type, resources held, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, CFaction *, CDate, CDate, CUniqueItem *>> Units; /// Units; first value is the tile position, and the last ones are start date and end date
	std::vector<std::tuple<Vec2i, CCharacter *, CFaction *, CDate, CDate>> Heroes; /// Heroes; first value is the tile position, and the last ones are start year and end year
	std::vector<std::tuple<Vec2i, CUnitType *, CPlane *, CUniqueItem *>> PlaneConnectors; /// Layer connectors (with unit type, plane pointer, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, CWorld *, CUniqueItem *>> WorldConnectors; /// Layer connectors (with unit type, world pointer, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, int, CUniqueItem *>> SurfaceLayerConnectors; /// Layer connectors (with unit type, surface/underground layer, and unique item pointer), mapped to the tile position
	std::map<std::pair<int, int>, std::string> TileLabels; /// labels to appear for certain tiles
	std::vector<CSite *> Sites;
	std::map<std::pair<int, int>, CSite *> SitesByPosition;
	std::vector<std::tuple<Vec2i, CTerrainType *, CDate>> HistoricalTerrains;	/// Terrain changes
};

//@}

#endif // !__MAP_TEMPLATE_H__
