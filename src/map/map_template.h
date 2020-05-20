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

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "data_type.h"
#include "map/tile.h"
#include "time/date.h"
#include "util/point_container.h"
#include "vec2i.h"

class CMapField;
class CUniqueItem;
struct lua_State;

int CclDefineMapTemplate(lua_State *l);

namespace stratagus {

class character;
class faction;
class historical_location;
class historical_unit;
class plane;
class site;
class terrain_type;
class unit_class;
class unit_type;
class world;

class generated_terrain
{
public:
	generated_terrain() {}

	generated_terrain(terrain_type *terrain_type) : TerrainType(terrain_type)
	{
	}

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);
	void ProcessConfigData(const CConfigData *config_data);
	
	bool CanUseTileAsSeed(const CMapField *tile) const;
	bool CanGenerateOnTile(const CMapField *tile) const;
	bool CanTileBePartOfExpansion(const CMapField *tile) const;
	bool CanRemoveTileOverlayTerrain(const CMapField *tile) const;

	terrain_type *TerrainType = nullptr;
	int SeedCount = 0;
	int ExpansionChance = 50; //50% chance to expand to a tile by default
	int MaxPercent = 0; //the maximum percentage of tiles in the map subtemplate that should have the generated terrain type as their top tile
	bool UseExistingAsSeeds = false; //whether to use existing tiles of the given terrain in the map layer as seeds for this terrain generation
	bool UseSubtemplateBordersAsSeeds = false; //whether to use the border tiles of subtemplates that have the given terrain as seeds for this terrain generation
	std::vector<const terrain_type *> TargetTerrainTypes; //the terrain types over which the terrain is to be generated
};

class map_template final : public named_data_entry, public data_type<map_template>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(QSize size MEMBER size READ get_size)
	Q_PROPERTY(bool circle MEMBER circle READ is_circle)
	Q_PROPERTY(bool optional MEMBER optional READ is_optional)
	Q_PROPERTY(QPoint start_pos MEMBER start_pos READ get_start_pos)
	Q_PROPERTY(QPoint end_pos MEMBER end_pos READ get_end_pos)
	Q_PROPERTY(QPoint subtemplate_top_left_pos MEMBER subtemplate_top_left_pos READ get_subtemplate_top_left_pos)
	Q_PROPERTY(QPoint subtemplate_center_pos MEMBER subtemplate_center_pos READ get_subtemplate_center_pos)
	Q_PROPERTY(QPoint min_subtemplate_pos MEMBER min_subtemplate_pos READ get_min_subtemplate_pos)
	Q_PROPERTY(QPoint max_subtemplate_pos MEMBER max_subtemplate_pos READ get_max_subtemplate_pos)
	Q_PROPERTY(stratagus::plane* plane MEMBER plane READ get_plane)
	Q_PROPERTY(stratagus::world* world READ get_world WRITE set_world)
	Q_PROPERTY(stratagus::map_template* main_template READ get_main_template WRITE set_main_template)
	Q_PROPERTY(QString terrain_file READ get_terrain_file_qstring)
	Q_PROPERTY(QString overlay_terrain_file READ get_overlay_terrain_file_qstring)
	Q_PROPERTY(QString terrain_image READ get_terrain_image_qstring)
	Q_PROPERTY(QString overlay_terrain_image READ get_overlay_terrain_image_qstring)
	Q_PROPERTY(stratagus::terrain_type* base_terrain_type MEMBER base_terrain_type READ get_base_terrain_type)
	Q_PROPERTY(stratagus::terrain_type* base_overlay_terrain_type MEMBER base_overlay_terrain_type READ get_base_overlay_terrain_type)
	Q_PROPERTY(stratagus::terrain_type* border_terrain_type MEMBER border_terrain_type READ get_border_terrain_type)
	Q_PROPERTY(stratagus::terrain_type* border_overlay_terrain_type MEMBER border_overlay_terrain_type READ get_border_overlay_terrain_type)
	Q_PROPERTY(stratagus::terrain_type* unusable_area_terrain_type MEMBER unusable_area_terrain_type READ get_unusable_area_terrain_type)
	Q_PROPERTY(stratagus::terrain_type* unusable_area_overlay_terrain_type MEMBER unusable_area_overlay_terrain_type READ get_unusable_area_overlay_terrain_type)
	Q_PROPERTY(stratagus::terrain_type* surrounding_terrain_type MEMBER surrounding_terrain_type READ get_surrounding_terrain_type)
	Q_PROPERTY(stratagus::terrain_type* surrounding_overlay_terrain_type MEMBER surrounding_overlay_terrain_type READ get_surrounding_overlay_terrain_type)
	Q_PROPERTY(bool output_terrain_image MEMBER output_terrain_image READ outputs_terrain_image)

public:
	static constexpr const char *class_identifier = "map_template";
	static constexpr const char *database_folder = "map_templates";
	static constexpr QPoint min_adjacent_template_distance = QPoint(4, 4);
	static constexpr QPoint max_adjacent_template_distance = QPoint(16, 16);

	map_template(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
	{
	}
	
	~map_template();

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;

	void ApplyTerrainFile(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const;
	void ApplyTerrainImage(bool overlay, Vec2i template_start_pos, Vec2i map_start_pos, int z) const;
	void Apply(const QPoint &template_start_pos, const QPoint &map_start_pos, const int z);
	void apply_subtemplates(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random = false) const;
	void apply_subtemplate(map_template *subtemplate, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random = false) const;
	void apply_sites(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random = false) const;
	void ApplyConnectors(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random = false) const;
	void ApplyUnits(const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random = false) const;
	void apply_historical_unit(const historical_unit *historical_unit, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const bool random) const;

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

	const QPoint &get_start_pos() const
	{
		return this->start_pos;
	}

	const QPoint &get_end_pos() const
	{
		return this->end_pos;
	}

	bool contains_pos(const QPoint &pos) const
	{
		if (this->get_end_pos().x() != -1 && pos.x() > this->get_end_pos().x()) {
			return false;
		}

		if (this->get_end_pos().y() != -1 && pos.y() > this->get_end_pos().y()) {
			return false;
		}

		return pos.x() >= this->get_start_pos().x() && pos.y() >= this->get_start_pos().y();
	}

	QSize get_applied_size() const;

	int get_applied_width() const
	{
		return this->get_applied_size().width();
	}

	int get_applied_height() const
	{
		return this->get_applied_size().height();
	}

	QSize get_applied_size_with_dependent_template_offsets() const
	{
		QSize applied_size = this->get_applied_size();
		applied_size += QSize(this->GetDependentTemplatesWestOffset() + this->GetDependentTemplatesEastOffset(), this->GetDependentTemplatesNorthOffset() + this->GetDependentTemplatesSouthOffset());
		return applied_size;
	}

	int get_area() const
	{
		return this->get_width() * this->get_height();
	}

	int get_applied_area_with_dependent_template_offsets() const
	{
		const QSize applied_size = this->get_applied_size_with_dependent_template_offsets();
		return applied_size.width() * applied_size.height();
	}

	int GetDependentTemplatesNorthOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->get_main_template()->get_subtemplates()) {
			if (std::find(map_template->NorthOfTemplates.begin(), map_template->NorthOfTemplates.end(), this) == map_template->NorthOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::min_adjacent_template_distance.y() + map_template->get_height() + map_template->GetDependentTemplatesNorthOffset());
		}

		return offset;
	}

	int GetDependentTemplatesSouthOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->get_main_template()->get_subtemplates()) {
			if (std::find(map_template->SouthOfTemplates.begin(), map_template->SouthOfTemplates.end(), this) == map_template->SouthOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::min_adjacent_template_distance.y() + map_template->get_height() + map_template->GetDependentTemplatesSouthOffset());
		}

		return offset;
	}

	int GetDependentTemplatesWestOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->get_main_template()->get_subtemplates()) {
			if (std::find(map_template->WestOfTemplates.begin(), map_template->WestOfTemplates.end(), this) == map_template->WestOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::min_adjacent_template_distance.x() + map_template->get_width() + map_template->GetDependentTemplatesWestOffset());
		}

		return offset;
	}

	int GetDependentTemplatesEastOffset() const
	{
		int offset = 0;

		for (const map_template *map_template : this->get_main_template()->get_subtemplates()) {
			if (std::find(map_template->EastOfTemplates.begin(), map_template->EastOfTemplates.end(), this) == map_template->EastOfTemplates.end()) {
				continue;
			}

			offset = std::max(offset, map_template::min_adjacent_template_distance.x() + map_template->get_width() + map_template->GetDependentTemplatesEastOffset());
		}

		return offset;
	}

	bool is_circle() const
	{
		return this->circle;
	}

	bool is_optional() const
	{
		return this->optional;
	}

	bool contains_map_pos(const QPoint &pos) const
	{
		const QPoint &start_pos = this->get_current_map_start_pos();
		const QPoint end_pos = this->get_current_map_end_pos();
		return pos.x() >= start_pos.x() && pos.y() >= start_pos.y() && pos.x() <= end_pos.x() && pos.y() <= end_pos.y();
	}

	const QPoint &get_current_map_start_pos() const
	{
		return this->current_map_start_pos;
	}

	QPoint get_current_map_end_pos() const
	{
		const QPoint &start_pos = this->get_current_map_start_pos();
		return QPoint(start_pos.x() + this->get_applied_width() - 1, start_pos.y() + this->get_applied_height() - 1);
	}

	//whether a position relative to the entire map is a usable part of the map template
	bool is_map_pos_usable(const QPoint &pos) const
	{
		if (!this->contains_map_pos(pos)) {
			return false;
		}

		return this->is_pos_usable(pos - this->get_current_map_start_pos());
	}

	//whether a position relative to the map template itself is a usable part of it
	bool is_pos_usable(const QPoint &pos) const
	{
		if (this->is_circle()) {
			const QPoint start_pos(0, 0);
			const QPoint end_pos = QPoint(this->get_applied_width() - 1, this->get_applied_height() - 1);

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

	plane *get_plane() const
	{
		return this->plane;
	}

	world *get_world() const
	{
		return this->world;
	}

	void set_world(world *world);

	map_template *get_main_template() const
	{
		return this->main_template;
	}

	void set_main_template(map_template *map_template) 
	{
		if (map_template == this->get_main_template()) {
			return;
		}

		this->main_template = map_template;
		main_template->subtemplates.push_back(this);
	}

	const std::vector<map_template *> &get_subtemplates() const
	{
		return this->subtemplates;
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

	Q_INVOKABLE void set_terrain_file(const std::string &filepath)
	{
		this->set_terrain_file(std::filesystem::path(filepath));
	}

	const std::filesystem::path &get_overlay_terrain_file() const
	{
		return this->overlay_terrain_file;
	}

	void set_overlay_terrain_file(const std::filesystem::path &filepath);

	QString get_overlay_terrain_file_qstring() const
	{
		return QString::fromStdString(this->get_overlay_terrain_file().string());
	}

	Q_INVOKABLE void set_overlay_terrain_file(const std::string &filepath)
	{
		this->set_overlay_terrain_file(std::filesystem::path(filepath));
	}

	const std::filesystem::path &get_terrain_image() const
	{
		return this->terrain_image;
	}

	void set_terrain_image(const std::filesystem::path &filepath);

	QString get_terrain_image_qstring() const
	{
		return QString::fromStdString(this->get_terrain_image().string());
	}

	Q_INVOKABLE void set_terrain_image(const std::string &filepath)
	{
		this->set_terrain_image(std::filesystem::path(filepath));
	}

	const std::filesystem::path &get_overlay_terrain_image() const
	{
		return this->overlay_terrain_image;
	}

	void set_overlay_terrain_image(const std::filesystem::path &filepath);

	QString get_overlay_terrain_image_qstring() const
	{
		return QString::fromStdString(this->get_overlay_terrain_image().string());
	}

	Q_INVOKABLE void set_overlay_terrain_image(const std::string &filepath)
	{
		this->set_overlay_terrain_image(std::filesystem::path(filepath));
	}

	const QPoint &get_subtemplate_top_left_pos() const
	{
		return this->subtemplate_top_left_pos;
	}

	const QPoint &get_subtemplate_center_pos() const
	{
		return this->subtemplate_center_pos;
	}

	const QPoint &get_min_subtemplate_pos() const
	{
		return this->min_subtemplate_pos;
	}

	const QPoint &get_max_subtemplate_pos() const
	{
		return this->max_subtemplate_pos;
	}

	bool is_dependent_on(const map_template *other_template) const;
	void add_dependency_template(const map_template *other_template);

	size_t get_total_adjacent_template_count() const
	{
		//return the total number of adjacent templates on which this template ultimately depends
		size_t count = 0;
		for (const map_template *map_template : this->AdjacentTemplates) {
			count++;
			count += map_template->get_total_adjacent_template_count();
		}
		return count;
	}

	QPoint generate_subtemplate_position(const map_template *subtemplate, const QPoint &template_start_pos, const QPoint &map_start_pos, const QPoint &map_end, const int z, const QPoint &max_adjacent_template_distance, bool &adjacency_restriction_occurred) const;

	Vec2i get_best_location_map_position(const std::vector<std::unique_ptr<historical_location>> &historical_location_list, bool &in_another_map_template, const Vec2i &template_start_pos, const Vec2i &map_start_pos, const bool random) const;

	QPoint get_location_map_position(const std::unique_ptr<historical_location> &historical_location, const QPoint &template_start_pos, const QPoint &map_start_pos, const bool random) const;

	terrain_type *get_base_terrain_type() const
	{
		return this->base_terrain_type;
	}
	
	terrain_type *get_base_overlay_terrain_type() const
	{
		return this->base_overlay_terrain_type;
	}

	terrain_type *get_border_terrain_type() const
	{
		return this->border_terrain_type;
	}
	
	terrain_type *get_border_overlay_terrain_type() const
	{
		return this->border_overlay_terrain_type;
	}
	
	terrain_type *get_unusable_area_terrain_type() const
	{
		return this->unusable_area_terrain_type;
	}
	
	terrain_type *get_unusable_area_overlay_terrain_type() const
	{
		return this->unusable_area_overlay_terrain_type;
	}

	terrain_type *get_surrounding_terrain_type() const
	{
		return this->surrounding_terrain_type;
	}

	terrain_type *get_surrounding_overlay_terrain_type() const
	{
		return this->surrounding_overlay_terrain_type;
	}

	bool outputs_terrain_image() const
	{
		return this->output_terrain_image;
	}

	const point_map<terrain_type *> &get_tile_terrains() const
	{
		return this->tile_terrains;
	}

	void set_tile_terrain(const QPoint &tile_pos, terrain_type *terrain)
	{
		this->tile_terrains[tile_pos] = terrain;
	}
	
private:
	std::filesystem::path terrain_file;
	std::filesystem::path overlay_terrain_file;
	std::filesystem::path terrain_image;
	std::filesystem::path overlay_terrain_image;
	QSize size = QSize(0, 0);
public:
	int Priority = 100; //the priority of this map template, for the order of application of subtemplates
private:
	bool circle = false; //whether the template should be applied as a circle, i.e. it should apply no subtemplates and etc. or generate terrain outside the boundaries of the circle
	bool optional = false;
	bool output_terrain_image = false;
private:
	QPoint subtemplate_top_left_pos = QPoint(-1, -1); //this template's position as a subtemplate in its main template; the position is relative to the subtemplate's top left
	QPoint subtemplate_center_pos = QPoint(-1, -1); //this template's position as a subtemplate in its main template; the position is relative to the subtemplate's center
	QPoint min_subtemplate_pos = QPoint(-1, -1); //the minimum position this subtemplate can be applied to in its main template; the position is relative to the subtemplate's center
	QPoint max_subtemplate_pos = QPoint(-1, -1); //the maximum position this subtemplate can be applied to in its main template; the position is relative to the subtemplate's center
	QPoint start_pos = QPoint(0, 0); //the start position within the map template to be applied when it is used
	QPoint end_pos = QPoint(-1, -1); //the end position within the map template to be applied when it is used
private:
	QPoint current_map_start_pos = QPoint(0, 0);
public:
	QPoint current_start_pos = QPoint(0, 0);
	map_template *main_template = nullptr; //main template in which this one is located, if this is a subtemplate
	map_template *UpperTemplate = nullptr; //map template corresponding to this one in the upper layer
	map_template *LowerTemplate = nullptr; //map template corresponding to this one in the lower layer
	std::vector<const map_template *> AdjacentTemplates; //map templates adjacent to this one
private:
	std::vector<map_template *> dependent_adjacent_templates;
public:
	std::vector<const map_template *> NorthOfTemplates; //map templates to which this one is to the north of
	std::vector<const map_template *> SouthOfTemplates; //map templates to which this one is to the north of
	std::vector<const map_template *> WestOfTemplates; //map templates to which this one is to the west of
	std::vector<const map_template *> EastOfTemplates; //map templates to which this one is to the east of
private:
	std::vector<const map_template *> dependency_templates; //the other templates on which this one depends on to be applied as a subtemplate, e.g. its adjacent templates, north of templates, etc.
	plane *plane = nullptr;
	world *world = nullptr;
	terrain_type *base_terrain_type = nullptr;
	terrain_type *base_overlay_terrain_type = nullptr;
	terrain_type *border_terrain_type = nullptr;
	terrain_type *border_overlay_terrain_type = nullptr;
	terrain_type *surrounding_terrain_type = nullptr;
	terrain_type *surrounding_overlay_terrain_type = nullptr;
	terrain_type *unusable_area_terrain_type = nullptr; //the terrain type for the template's unusable area, e.g. the area outside its circle if the template is a circle
	terrain_type *unusable_area_overlay_terrain_type = nullptr;
	std::vector<map_template *> subtemplates;
	std::vector<std::unique_ptr<generated_terrain>> generated_terrains; //terrains generated in the map template
public:
	std::vector<std::pair<unit_type *, int>> GeneratedNeutralUnits; /// the first element of the pair is the resource's unit type, and the second is the quantity
	std::vector<std::pair<unit_type *, int>> PlayerLocationGeneratedNeutralUnits;
	std::map<std::pair<int, int>, std::tuple<unit_type *, int, CUniqueItem *>> Resources; /// Resources (with unit type, resources held, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, unit_type *, faction *, CDate, CDate, CUniqueItem *>> Units; /// Units; first value is the tile position, and the last ones are start date and end date
	std::vector<std::tuple<Vec2i, character *, faction *, CDate, CDate>> Heroes; /// Heroes; first value is the tile position, and the last ones are start year and end year
	std::vector<std::tuple<Vec2i, unit_type *, stratagus::plane *, CUniqueItem *>> PlaneConnectors; /// Layer connectors (with unit type, plane pointer, and unique item pointer), mapped to the tile position
	std::vector<std::tuple<Vec2i, unit_type *, stratagus::world *, CUniqueItem *>> WorldConnectors; /// Layer connectors (with unit type, world pointer, and unique item pointer), mapped to the tile position
	std::map<std::pair<int, int>, std::string> TileLabels; /// labels to appear for certain tiles
	std::vector<site *> sites;
	point_map<site *> sites_by_position;
private:
	point_map<terrain_type *> tile_terrains;
public:
	std::vector<std::tuple<Vec2i, terrain_type *, CDate>> HistoricalTerrains; //terrain changes

	friend int ::CclDefineMapTemplate(lua_State *l);
};

}