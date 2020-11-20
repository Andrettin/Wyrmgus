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
#include "time/date.h"
#include "util/color_container.h"

class CMapLayer;
class CPlayer;
class CUnit;
struct lua_State;

static int CclDefineSite(lua_State *l);

namespace wyrmgus {

class character;
class civilization;
class faction;
class map_template;
class player_color;
class region;
class site_history;
class unique_item;
class unit_class;
class unit_type;

class site final : public named_data_entry, public data_type<site>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(bool major MEMBER major READ is_major)
	Q_PROPERTY(wyrmgus::map_template* map_template MEMBER map_template READ get_map_template)
	Q_PROPERTY(QPoint pos MEMBER pos READ get_pos)
	Q_PROPERTY(wyrmgus::site* pos_reference_site MEMBER pos_reference_site)
	Q_PROPERTY(QGeoCoordinate geocoordinate MEMBER geocoordinate READ get_geocoordinate)
	Q_PROPERTY(wyrmgus::site* geocoordinate_reference_site MEMBER geocoordinate_reference_site)
	Q_PROPERTY(int longitude_scale MEMBER longitude_scale)
	Q_PROPERTY(int latitude_scale MEMBER latitude_scale)
	Q_PROPERTY(wyrmgus::unit_type* base_unit_type MEMBER base_unit_type)
	Q_PROPERTY(wyrmgus::site* connection_destination MEMBER connection_destination)
	Q_PROPERTY(QVariantList cores READ get_cores_qvariant_list)
	Q_PROPERTY(QVariantList regions READ get_regions_qvariant_list)
	Q_PROPERTY(QColor color READ get_color WRITE set_color)

public:
	static constexpr const char *class_identifier = "site";
	static constexpr const char *database_folder = "sites";

	static site *get_by_color(const QColor &color)
	{
		site *site = site::try_get_by_color(color);

		if (site == nullptr) {
			throw std::runtime_error("No site found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return site;
	}

	static site *try_get_by_color(const QColor &color)
	{
		auto find_iterator = site::sites_by_color.find(color);
		if (find_iterator != site::sites_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();
		site::sites_by_color.clear();
	}

private:
	static inline color_map<site *> sites_by_color;

public:
	explicit site(const std::string &identifier);
	~site();

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;
	virtual data_entry_history *get_history_base() override;

	const site_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data()
	{
		this->owner = nullptr;
		this->site_unit = nullptr;
		this->clear_tiles();
		this->map_pos = QPoint(-1, -1);
		this->map_layer = nullptr;
	}

	const std::string &get_cultural_name(const civilization *civilization) const;
	const std::string &get_current_cultural_name() const;

	bool is_major() const
	{
		return this->major;
	}

	map_template *get_map_template() const
	{
		return this->map_template;
	}

	const QPoint &get_pos() const
	{
		return this->pos;
	}

	const QGeoCoordinate &get_geocoordinate() const
	{
		return this->geocoordinate;
	}

	const unit_type *get_base_unit_type() const
	{
		return this->base_unit_type;
	}

	const site *get_connection_destination() const
	{
		return this->connection_destination;
	}

	bool is_connector() const
	{
		return this->get_connection_destination() != nullptr;
	}

	CUnit *get_site_unit() const
	{
		return this->site_unit;
	}

	void set_site_unit(CUnit *unit);

	CPlayer *get_owner() const
	{
		return this->owner;
	}

	void set_owner(CPlayer *player);

	CPlayer *get_realm_owner() const;

	bool can_be_randomly_generated() const
	{
		return !this->get_name().empty() && !this->is_connector();
	}

	const QPoint &get_map_pos() const
	{
		return this->map_pos;
	}

	void set_map_pos(const QPoint &pos)
	{
		this->map_pos = pos;
	}

	const CMapLayer *get_map_layer() const
	{
		return this->map_layer;
	}

	void set_map_layer(const CMapLayer *map_layer)
	{
		this->map_layer = map_layer;
	}

	void clear_tiles()
	{
		this->clear_border_tiles();
		this->clear_trade_route_tiles();
	}

	void add_border_tile(const QPoint &tile_pos)
	{
		this->border_tiles.push_back(tile_pos);

		if (this->territory_rect.isNull()) {
			this->territory_rect = QRect(tile_pos, QSize(1, 1));
		} else {
			if (tile_pos.x() < this->territory_rect.x()) {
				this->territory_rect.setX(tile_pos.x());
			} else if (tile_pos.x() > this->territory_rect.right()) {
				this->territory_rect.setRight(tile_pos.x());
			}
			if (tile_pos.y() < this->territory_rect.y()) {
				this->territory_rect.setY(tile_pos.y());
			} else if (tile_pos.y() > this->territory_rect.bottom()) {
				this->territory_rect.setBottom(tile_pos.y());
			}
		}
	}

	void clear_border_tiles()
	{
		this->border_tiles.clear();
		this->territory_rect = QRect();
	}

	void update_border_tiles();
	void update_minimap_territory();

	const std::vector<QPoint> &get_trade_route_tiles() const
	{
		return this->trade_route_tiles;
	}

	void add_trade_route_tile(const QPoint &tile_pos)
	{
		this->trade_route_tiles.push_back(tile_pos);
	}

	void clear_trade_route_tiles()
	{
		this->trade_route_tiles.clear();
	}

	const std::vector<faction *> &get_cores() const
	{
		return this->cores;
	}

	QVariantList get_cores_qvariant_list() const;

	Q_INVOKABLE void add_core(faction *faction);
	Q_INVOKABLE void remove_core(faction *faction);

	const std::vector<region *> &get_regions() const
	{
		return this->regions;
	}

	QVariantList get_regions_qvariant_list() const;

	Q_INVOKABLE void add_region(region *region);
	Q_INVOKABLE void remove_region(region *region);

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color)
	{
		if (color == this->get_color()) {
			return;
		}

		if (site::try_get_by_color(color) != nullptr) {
			throw std::runtime_error("Color is already used by another site.");
		}

		this->color = color;
		site::sites_by_color[color] = this;
	}

	const std::vector<character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(character *character)
	{
		this->characters.push_back(character);
	}

private:
	bool major = false; //whether the site is a major one; major sites have settlement sites, and as such can have town halls
	wyrmgus::map_template *map_template = nullptr;
	QPoint pos = QPoint(-1, -1); /// Position of the site in its map template
	site *pos_reference_site = nullptr; //the site's reference position site, used as an offset for its position
	QGeoCoordinate geocoordinate; //the site's position as a geocoordinate
	site *geocoordinate_reference_site = nullptr; //the site's reference geocoordinate site, used as an offset for its geocoordinate
	int longitude_scale = 100;
	int latitude_scale = 100;
	unit_type *base_unit_type = nullptr;
	site *connection_destination = nullptr;
	CPlayer *owner = nullptr;
	CUnit *site_unit = nullptr; //unit which represents this site
	std::vector<region *> regions; //regions where this site is located
	std::vector<faction *> cores; //factions which have this site as a core
	std::map<const civilization *, std::string> cultural_names;	/// Names for the site for each different culture/civilization
	QColor color; //color used to represent the site on the minimap, and to identify its territory on territory images
	std::vector<character *> characters; //characters which can be recruited at this site
	std::unique_ptr<site_history> history;
public:
	std::map<CDate, const faction *> HistoricalOwners;			/// Historical owners of the site
	std::map<CDate, int> HistoricalPopulation;					/// Historical population
	std::vector<std::tuple<CDate, CDate, const unit_type *, int, const faction *>> HistoricalUnits;	/// Historical quantity of a particular unit type (number of people for units representing a person)
	std::vector<std::tuple<CDate, CDate, const unit_class *, unique_item *, const faction *>> HistoricalBuildings; /// Historical buildings, with start and end date
	std::vector<std::tuple<CDate, CDate, const unit_type *, unique_item *, int>> HistoricalResources; /// Historical resources, with start and end date; the integer at the end is the resource quantity
private:
	QPoint map_pos = QPoint(-1, -1);
	const CMapLayer *map_layer = nullptr;
	std::vector<QPoint> border_tiles; //the tiles for this settlement which border the territory of another settlement
	QRect territory_rect; //the territory rectangle of the site
	std::vector<QPoint> trade_route_tiles; //the tiles containing a trade route in this settlement's territory

	friend static int ::CclDefineSite(lua_State *l);
};

}
