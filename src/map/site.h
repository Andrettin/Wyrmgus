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

class CPlayer;
class CUnit;
class CUniqueItem;
struct lua_State;

int CclDefineSite(lua_State *l);

namespace stratagus {

class character;
class civilization;
class faction;
class map_template;
class region;
class unit_class;
class unit_type;

class site : public named_data_entry, public data_type<site>, public CDataType
{
	Q_OBJECT

	Q_PROPERTY(bool major MEMBER major READ is_major)
	Q_PROPERTY(stratagus::map_template* map_template MEMBER map_template READ get_map_template)
	Q_PROPERTY(QPoint pos MEMBER pos READ get_pos)
	Q_PROPERTY(stratagus::faction* owner_faction MEMBER owner_faction READ get_owner_faction)
	Q_PROPERTY(QVariantList building_classes READ get_building_classes_qvariant_list)
	Q_PROPERTY(QVariantList cores READ get_cores_qvariant_list)
	Q_PROPERTY(QVariantList regions READ get_regions_qvariant_list)

public:
	static constexpr const char *class_identifier = "site";
	static constexpr const char *database_folder = "sites";

	site(const std::string &identifier) : named_data_entry(identifier), CDataType(identifier)
	{
	}

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual void initialize() override;

	virtual void reset_history() override
	{
		this->owner_faction = nullptr;
		this->building_classes.clear();
	}

	std::string GetCulturalName(const civilization *civilization) const;

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

	faction *get_owner_faction() const
	{
		return this->owner_faction;
	}

	const std::vector<unit_class *> &get_building_classes() const
	{
		return this->building_classes;
	}

	QVariantList get_building_classes_qvariant_list() const;

	Q_INVOKABLE void add_building_class(unit_class *building_class)
	{
		this->building_classes.push_back(building_class);
	}

	Q_INVOKABLE void remove_building_class(unit_class *building_class);

	void add_border_tile(const QPoint &tile_pos)
	{
		this->border_tiles.push_back(tile_pos);
	}

	void clear_border_tiles()
	{
		this->border_tiles.clear();
	}

	void update_border_tiles(const bool minimap_only);

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

	const std::vector<character *> &get_characters() const
	{
		return this->characters;
	}

	void add_character(character *character)
	{
		this->characters.push_back(character);
	}

private:
	bool major = false; /// Whether the site is a major one; major sites have settlement sites, and as such can have town halls
	QPoint pos = QPoint(-1, -1); /// Position of the site in its map template
	map_template *map_template = nullptr; /// Map template where this site is located
	CPlayer *owner = nullptr;
	CUnit *site_unit = nullptr;									/// Unit which represents this site
	std::vector<region *> regions;								/// Regions where this site is located
	std::vector<faction *> cores;						/// Factions which have this site as a core
public:
	std::map<const civilization *, std::string> CulturalNames;	/// Names for the site for each different culture/civilization
private:
	std::vector<character *> characters; //characters which can be recruited at this site
	faction *owner_faction = nullptr; //used for the owner history of the site, and after game start is 	set to its player owner's faction
	std::vector<unit_class *> building_classes; //used by history; applied as buildings at scenario start
public:
	std::map<CDate, const faction *> HistoricalOwners;			/// Historical owners of the site
	std::map<CDate, int> HistoricalPopulation;					/// Historical population
	std::vector<std::tuple<CDate, CDate, const unit_type *, int, const faction *>> HistoricalUnits;	/// Historical quantity of a particular unit type (number of people for units representing a person)
	std::vector<std::tuple<CDate, CDate, const unit_class *, CUniqueItem *, const faction *>> HistoricalBuildings; /// Historical buildings, with start and end date
	std::vector<std::tuple<CDate, CDate, const unit_type *, CUniqueItem *, int>> HistoricalResources; /// Historical resources, with start and end date; the integer at the end is the resource quantity
private:
	std::vector<QPoint> border_tiles; //the tiles for this settlement which border the territory of another settlement

	friend int ::CclDefineSite(lua_State *l);
};

}
