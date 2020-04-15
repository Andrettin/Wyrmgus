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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "data_type.h"
#include "map/tile.h"
#include "time/date.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CFaction;
class CHistoricalLocation;
class CMapField;
class CPlane;
class CSite;
class CUniqueItem;
class CUnitType;
struct lua_State;

namespace stratagus {
	class terrain_type;
}

int CclDefineMapTemplate(lua_State *l);

class CGeneratedTerrain
{
public:
	void ProcessConfigData(const CConfigData *config_data);
	
	bool CanUseTileAsSeed(const CMapField *tile) const;
	bool CanGenerateOnTile(const CMapField *tile) const;
	bool CanTileBePartOfExpansion(const CMapField *tile) const;
	bool CanRemoveTileOverlayTerrain(const CMapField *tile) const;

	stratagus::terrain_type *TerrainType = nullptr;
	int SeedCount = 0;
	int ExpansionChance = 50; //50% chance to expand to a tile by default
	int MaxPercent = 0; //the maximum percentage of tiles in the map subtemplate that should have the generated terrain type as their top tile
	bool UseExistingAsSeeds = false; //whether to use existing tiles of the given terrain in the map layer as seeds for this terrain generation
	bool UseSubtemplateBordersAsSeeds = false; //whether to use the border tiles of subtemplates that have the given terrain as seeds for this terrain generation
	std::vector<const stratagus::terrain_type *> TargetTerrainTypes; //the terrain types over which the terrain is to be generated
};

namespace stratagus {

class world;

class map_template final : public named_data_entry, public data_type<map_template>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(QSize size MEMBER size READ get_size)
	Q_PROPERTY(bool circle MEMBER circle READ is_circle)
	Q_PROPERTY(stratagus::world* world MEMBER world READ get_world)
	Q_PROPERTY(QString terrain_file READ get_terrain_file_qstring WRITE set_terrain_file_qstring)

public:
	static constexpr const char *class_identifier = "map_template";
	static constexpr const char *database_folder = "map_templates";
	static constexpr int MinAdjacentTemplateDistance = 4;
	static constexpr int MaxAdjacentTemplateDistance = 16;

	map_template(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
	{
	}
	
	~map_template();

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;

	void ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const;
	void ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const;
	void Apply(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z);
	void ApplySubtemplates(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random = false) const;
	void ApplySites(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random = false) const;
	void ApplyConnectors(Vec2i template_start_pos, Vec2i map_start_pos, int z, bool random = false) const;
	void ApplyUnits(const Vec2i &template_start_pos, const Vec2i &map_start_pos, const int z, const bool random = false) const;
	bool IsSubtemplateArea() const;
	const map_template *GetTopMapTemplate() const;

	const QSize &get_size() const
	{
		return this->size;
	}

	int get_width() const
	{
		return this->get_size().width();
	}

	int get_height() const
	{
		return this->get_size().height();
	}

	int get_area() const
	{
		return this->get_width() * this->get_height();
	}

	int GetDependentTemplatesNorthOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->NorthOfTemplates.begin(), map_template->NorthOfTemplates.end(), this) == map_template->NorthOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::MinAdjacentTemplateDistance + map_template->get_height() + map_template->GetDependentTemplatesNorthOffset());
		}

		return offset;
	}

	int GetDependentTemplatesSouthOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->SouthOfTemplates.begin(), map_template->SouthOfTemplates.end(), this) == map_template->SouthOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::MinAdjacentTemplateDistance + map_template->get_height() + map_template->GetDependentTemplatesSouthOffset());
		}

		return offset;
	}

	int GetDependentTemplatesWestOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->WestOfTemplates.begin(), map_template->WestOfTemplates.end(), this) == map_template->WestOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::MinAdjacentTemplateDistance + map_template->get_width() + map_template->GetDependentTemplatesWestOffset());
		}

		return offset;
	}

	int GetDependentTemplatesEastOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->MainTemplate->Subtemplates) {
			if (std::find(map_template->EastOfTemplates.begin(), map_template->EastOfTemplates.end(), this) == map_template->EastOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::MinAdjacentTemplateDistance + map_template->get_width() + map_template->GetDependentTemplatesEastOffset());
		}

		return offset;
	}

	bool is_circle() const
	{
		return this->circle;
	}

	bool contains_pos(const QPoint &pos) const
	{
		const QPoint &start_pos = this->get_current_start_pos();
		const QPoint end_pos = this->get_current_end_pos();
		return pos.x() >= start_pos.x() && pos.y() >= start_pos.y() && pos.x() <= end_pos.x() && pos.y() <= end_pos.y();
	}

	const QPoint &get_current_start_pos() const
	{
		return this->current_start_pos;
	}

	QPoint get_current_end_pos() const
	{
		const QPoint &start_pos = this->get_current_start_pos();
		return QPoint(start_pos.x() + this->get_width() - 1, start_pos.y() + this->get_height() - 1);
	}

	//whether a position relative to the entire map is a usable part of the map template
	bool is_map_pos_usable(const QPoint &pos) const
	{
		if (!this->contains_pos(pos)) {
			return false;
		}

		return this->is_pos_usable(pos - this->get_current_start_pos());
	}

	//whether a position relative to the map template itself is a usable part of it
	bool is_pos_usable(const QPoint &pos) const
	{
		if (this->is_circle()) {
			const QPoint start_pos(0, 0);
			const QPoint end_pos = QPoint(this->get_width() - 1, this->get_height() - 1);

			const double middle_x = (end_pos.x() + start_pos.x()) / 2;
			const double middle_y = (end_pos.y() + start_pos.y()) / 2;
			const double radius = ((middle_x - start_pos.x()) + (middle_y - start_pos.y())) / 2;

			const double rel_x = pos.x() - middle_x;
			const double rel_y = pos.y() - middle_y;
			const double my = radius * radius - rel_x * rel_x;
			if ((rel_y * rel_y) > my) {
				return false;
			}
		}

		return true;
	}

	world *get_world() const
	{
		return this->world;
	}

	const std::filesystem::path &get_terrain_file() const
	{
		return this->terrain_file;
	}

	void set_terrain_file(const std::filesystem::path &filepath);

	QString get_terrain_file_qstring() const
	{
		return QString::fromStdString(this->get_terrain_file().string());
	}

	void set_terrain_file_qstring(const QString &filepath)
	{
		this->set_terrain_file(filepath.toStdString());
	}

	const std::filesystem::path &get_overlay_terrain_file() const
	{
		return this->overlay_terrain_file;
	}

	const std::filesystem::path &get_terrain_image() const
	{
		return this->terrain_image;
	}

	const std::filesystem::path &get_overlay_terrain_image() const
	{
		return this->overlay_terrain_image;
	}

	Vec2i GetBestLocationMapPosition(const std::vector<CHistoricalLocation *> &historical_location_list, bool &in_another_map_template, const Vec2i &template_start_pos, const Vec2i &map_start_pos, const bool random) const;

	terrain_type *get_unusable_area_terrain_type() const
	{
		return this->unusable_area_terrain_type;
	}
	
	terrain_type *get_unusable_area_overlay_terrain_type() const
	{
		return this->unusable_area_overlay_terrain_type;
	}
	
private:
	std::filesystem::path terrain_file;
	std::filesystem::path overlay_terrain_file;
	std::filesystem::path terrain_image;
	std::filesystem::path overlay_terrain_image;
	QSize size = QSize(0, 0);
public:
	int Scale = 1; //1 means a map template tile will be applied as one in-game tile, 2 means a 2x2 in-game tile
	int SurfaceLayer = 0; //surface layer of the map template (0 for surface, 1 and above for underground layers in succession)
	int Priority = 0; //the priority of this map template, for the order of application of subtemplates
	bool OutputTerrainImage = false;
private:
	bool circle = false; //whether the template should be applied as a circle, i.e. it should apply no subtemplates and etc. or generate terrain outside the boundaries of the circle
public:
	Vec2i SubtemplatePosition = Vec2i(-1, -1);
	Vec2i MinPos = Vec2i(-1, -1); //the minimum position this (sub)template can be applied to (relative to the main template)
	Vec2i MaxPos = Vec2i(-1, -1); //the maximum position this (sub)template can be applied to (relative to the main template)
private:
	QPoint current_start_pos = QPoint(0, 0);
public:
	map_template *MainTemplate = nullptr; //main template in which this one is located, if this is a subtemplate
	map_template *UpperTemplate = nullptr; //map template corresponding to this one in the upper layer
	map_template *LowerTemplate = nullptr; //map template corresponding to this one in the lower layer
	std::vector<const map_template *> AdjacentTemplates; //map templates adjacent to this one
	std::vector<const map_template *> NorthOfTemplates; //map templates to which this one is to the north of
	std::vector<const map_template *> SouthOfTemplates; //map templates to which this one is to the north of
	std::vector<const map_template *> WestOfTemplates; //map templates to which this one is to the west of
	std::vector<const map_template *> EastOfTemplates; //map templates to which this one is to the east of
	CPlane *Plane = nullptr;
private:
	world *world = nullptr;
public:
	terrain_type *BaseTerrainType = nullptr;
	terrain_type *BaseOverlayTerrainType = nullptr;
	terrain_type *BorderTerrainType = nullptr;
	terrain_type *SurroundingTerrainType = nullptr;
private:
	terrain_type *unusable_area_terrain_type = nullptr; //the terrain type for the template's unusable area, e.g. the area outside its circle if the template is a circle
	terrain_type *unusable_area_overlay_terrain_type = nullptr;
public:
	std::vector<map_template *> Subtemplates;
	std::vector<CGeneratedTerrain *> GeneratedTerrains;				/// terrains generated in the map template
	std::vector<std::pair<CUnitType *, int>> GeneratedNeutralUnits; /// the first element of the pair is the resource's unit type, and the second is the quantity
	std::vector<std::pair<CUnitType *, int>> PlayerLocationGeneratedNeutralUnits;
	std::map<std::pair<int, int>, std::tuple<CUnitType *, int, CUniqueItem *>> Resources; /// Resources (with unit type, resources held, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, CFaction *, CDate, CDate, CUniqueItem *>> Units; /// Units; first value is the tile position, and the last ones are start date and end date
	std::vector<std::tuple<Vec2i, CCharacter *, CFaction *, CDate, CDate>> Heroes; /// Heroes; first value is the tile position, and the last ones are start year and end year
	std::vector<std::tuple<Vec2i, CUnitType *, CPlane *, CUniqueItem *>> PlaneConnectors; /// Layer connectors (with unit type, plane pointer, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, stratagus::world *, CUniqueItem *>> WorldConnectors; /// Layer connectors (with unit type, world pointer, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, CUnitType *, int, CUniqueItem *>> SurfaceLayerConnectors; /// Layer connectors (with unit type, surface/underground layer, and unique item pointer), mapped to the tile position
	std::map<std::pair<int, int>, std::string> TileLabels; /// labels to appear for certain tiles
	std::vector<CSite *> Sites;
	std::map<std::pair<int, int>, CSite *> SitesByPosition;
	std::vector<std::tuple<Vec2i, terrain_type *, CDate>> HistoricalTerrains; //terrain changes

	friend int ::CclDefineMapTemplate(lua_State *l);
};

}