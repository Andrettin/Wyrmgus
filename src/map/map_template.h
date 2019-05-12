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
//      (c) Copyright 2018-2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "map/tile.h"
#include "time/date.h"
#include "vec2i.h"

#include <set>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CFaction;
class CHistoricalLocation;
class CMapField;
class CPlane;
class CSite;
class CTerrainType;
class CUniqueItem;
class CUnitType;
class CWorld;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definitions
----------------------------------------------------------------------------*/

class CGeneratedTerrain
{
public:
	void ProcessConfigData(const CConfigData *config_data);
	
	bool CanUseTileAsSeed(const CMapField *tile) const;
	bool CanGenerateOnTile(const CMapField *tile) const;
	bool CanTileBePartOfExpansion(const CMapField *tile) const;
	bool CanRemoveTileOverlayTerrain(const CMapField *tile) const;

	CTerrainType *TerrainType = nullptr;
	int SeedCount = 0;
	int ExpansionChance = 50; //50% chance to expand to a tile by default
	int MaxPercent = 0; //the maximum percentage of tiles in the map subtemplate that should have the generated terrain type as their top tile
	bool UseExistingAsSeeds = false; //whether to use existing tiles of the given terrain in the map layer as seeds for this terrain generation
	bool UseSubtemplateBordersAsSeeds = false; //whether to use the border tiles of subtemplates that have the given terrain as seeds for this terrain generation
	std::vector<const CTerrainType *> TargetTerrainTypes; //the terrain types over which the terrain is to be generated
};

class CMapTemplate : public DataElement, public DataType<CMapTemplate>
{
	DATA_TYPE(CMapTemplate, DataElement)
	
public:
	~CMapTemplate();

public:	
	static constexpr const char *ClassIdentifier = "map_template";
	static constexpr int CMapTemplate::MinAdjacentTemplateDistance = 4;
	static constexpr int CMapTemplate::MaxAdjacentTemplateDistance = 16;
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	int GetWidth() const
	{
		return this->Width;
	}
	
	int GetHeight() const
	{
		return this->Height;
	}
	
	int GetScale() const
	{
		return this->Scale;
	}
	
	int GetPriority() const
	{
		return this->Priority;
	}
	
	int GetDependentTemplatesNorthOffset() const
	{
		int offset = 0;
		
		for (const CMapTemplate *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->NorthOfTemplates.begin(), map_template->NorthOfTemplates.end(), this) == map_template->NorthOfTemplates.end()) {
				continue;
			}
			
			offset += CMapTemplate::MinAdjacentTemplateDistance + map_template->Height + map_template->GetDependentTemplatesNorthOffset();
		}
		
		return offset;
	}
	
	int GetDependentTemplatesSouthOffset() const
	{
		int offset = 0;
		
		for (const CMapTemplate *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->SouthOfTemplates.begin(), map_template->SouthOfTemplates.end(), this) == map_template->SouthOfTemplates.end()) {
				continue;
			}
			
			offset += CMapTemplate::MinAdjacentTemplateDistance + map_template->Height + map_template->GetDependentTemplatesSouthOffset();
		}
		
		return offset;
	}
	
	int GetDependentTemplatesWestOffset() const
	{
		int offset = 0;
		
		for (const CMapTemplate *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->WestOfTemplates.begin(), map_template->WestOfTemplates.end(), this) == map_template->WestOfTemplates.end()) {
				continue;
			}
			
			offset += CMapTemplate::MinAdjacentTemplateDistance + map_template->Width + map_template->GetDependentTemplatesWestOffset();
		}
		
		return offset;
	}
	
	int GetDependentTemplatesEastOffset() const
	{
		int offset = 0;
		
		for (const CMapTemplate *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->EastOfTemplates.begin(), map_template->EastOfTemplates.end(), this) == map_template->EastOfTemplates.end()) {
				continue;
			}
			
			offset += CMapTemplate::MinAdjacentTemplateDistance + map_template->Width + map_template->GetDependentTemplatesEastOffset();
		}
		
		return offset;
	}
	
	void ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const;
	void ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const;
	void Apply(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z);
	void ApplySubtemplates(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random = false) const;
	void ApplySites(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random = false) const;
	void ApplyConnectors(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random = false) const;
	void ApplyUnits(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random = false) const;
	bool IsSubtemplateArea() const;
	const CMapTemplate *GetTopMapTemplate() const;
	Vec2i GetBestLocationMapPosition(const std::vector<const CHistoricalLocation *> &historical_location_list, bool &in_another_map_template, const Vec2i &template_start_pos, const Vec2i &map_start_pos, const bool random) const;
	
	std::string TerrainFile;
	std::string OverlayTerrainFile;
	std::string TerrainImage;
	std::string OverlayTerrainImage;
private:
	int Width = 0;
	int Height = 0;
	int Scale = 1;		/// 1 means a map template tile will be applied as one in-game tile, 2 means a 2x2 in-game tile
public:
	int SurfaceLayer = 0;							/// Surface layer of the map template (0 for surface, 1 and above for underground layers in succession)
private:
	int Priority = 0;	/// the priority of this map template, for the order of application of subtemplates
	bool OutputTerrainImage = false;
	bool GrowForSubtemplates = false;
public:
	Vec2i SubtemplatePosition = Vec2i(-1, -1);
	Vec2i MinPos = Vec2i(-1, -1);	/// the minimum position this (sub)template can be applied to (relative to the main template)
	Vec2i MaxPos = Vec2i(-1, -1);	/// the maximum position this (sub)template can be applied to (relative to the main template)
	Vec2i MinPosPercent = Vec2i(-1, -1);	/// the minimum position this (sub)template can be applied to (relative to the main template), as a percentage
	Vec2i MaxPosPercent = Vec2i(-1, -1);	/// the minimum position this (sub)template can be applied to (relative to the main template), as a percentage
	Vec2i CurrentStartPos = Vec2i(0, 0);
	PixelSize PixelTileSize = Vec2i(32, 32);
	CMapTemplate *MainTemplate = nullptr;					/// main template in which this one is located, if this is a subtemplate
	CMapTemplate *UpperTemplate = nullptr;					/// map template corresponding to this one in the upper layer
	CMapTemplate *LowerTemplate = nullptr;					/// map template corresponding to this one in the lower layer
	std::set<const CMapTemplate *> AdjacentToTemplates;		/// map templates to which this one is adjacent
	std::set<const CMapTemplate *> NorthOfTemplates;		/// map templates to which this one is to the north of
	std::set<const CMapTemplate *> SouthOfTemplates;		/// map templates to which this one is to the north of
	std::set<const CMapTemplate *> WestOfTemplates;			/// map templates to which this one is to the west of
	std::set<const CMapTemplate *> EastOfTemplates;			/// map templates to which this one is to the east of
	CPlane *Plane = nullptr;
	CWorld *World = nullptr;
	CTerrainType *BaseTerrainType = nullptr;
	CTerrainType *BaseOverlayTerrainType = nullptr;
	CTerrainType *BorderTerrainType = nullptr;
	CTerrainType *SurroundingTerrainType = nullptr;
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
	
	friend int CclDefineMapTemplate(lua_State *l);

protected:
	static void _bind_methods();
};

#endif
