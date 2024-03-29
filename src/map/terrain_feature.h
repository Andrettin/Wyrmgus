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
//      (c) Copyright 2018-2022 by Andrettin
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

#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/color_container.h"
#include "util/geocoordinate.h"

struct lua_State;

extern int CclDefineTerrainFeature(lua_State *l);

namespace wyrmgus {

class civilization;
class region;
class terrain_type;

class terrain_feature final : public named_data_entry, public data_type<terrain_feature>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color)
	Q_PROPERTY(wyrmgus::terrain_type* terrain_type MEMBER terrain_type)
	Q_PROPERTY(bool trade_route MEMBER trade_route READ is_trade_route)
	Q_PROPERTY(bool major_river MEMBER major_river READ is_major_river)
	Q_PROPERTY(bool minor_river MEMBER minor_river READ is_minor_river)
	Q_PROPERTY(bool terrain_generation_seed MEMBER terrain_generation_seed READ is_terrain_generation_seed)
	Q_PROPERTY(bool hidden MEMBER hidden READ is_hidden)
	Q_PROPERTY(archimedes::geocoordinate geocoordinate MEMBER geocoordinate READ get_geocoordinate)

public:
	static constexpr const char *class_identifier = "terrain_feature";
	static constexpr const char property_class_identifier[] = "wyrmgus::terrain_feature*";
	static constexpr const char *database_folder = "terrain_features";

	static const terrain_feature *get_by_color(const QColor &color)
	{
		const terrain_feature *feature = terrain_feature::try_get_by_color(color);

		if (feature == nullptr) {
			throw std::runtime_error("No terrain feature found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return feature;
	}

	static const terrain_feature *try_get_by_color(const QColor &color)
	{
		const auto find_iterator = terrain_feature::terrain_features_by_color.find(color);
		if (find_iterator != terrain_feature::terrain_features_by_color.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void clear()
	{
		data_type::clear();
		terrain_feature::terrain_features_by_color.clear();
	}

private:
	static inline color_map<const terrain_feature *> terrain_features_by_color;

public:
	explicit terrain_feature(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	virtual void check() const override
	{
		if (this->get_terrain_type() == nullptr && !this->is_trade_route()) {
			throw std::runtime_error("Terrain feature \"" + this->get_identifier() + "\" has no terrain type, and isn't a trade route.");
		}
	}

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color);

	const wyrmgus::terrain_type *get_terrain_type() const
	{
		return this->terrain_type;
	}

	bool is_trade_route() const
	{
		return this->trade_route;
	}

	bool is_river() const
	{
		return this->is_major_river() || this->is_minor_river();
	}

	bool is_major_river() const
	{
		return this->major_river;
	}

	bool is_minor_river() const
	{
		return this->minor_river;
	}

	const std::string &get_cultural_name(const civilization *civilization) const
	{
		if (civilization != nullptr) {
			const auto find_iterator = this->cultural_names.find(civilization);
			if (find_iterator != this->cultural_names.end()) {
				return find_iterator->second;
			}
		}

		return this->get_name();
	}

	int get_geopath_width() const
	{
		if (this->is_major_river()) {
			return 25000;
		} else if (this->is_minor_river()) {
			return 10000;
		}

		return 0;
	}

	bool is_terrain_generation_seed() const
	{
		return this->terrain_generation_seed;
	}

	bool is_hidden() const
	{
		return this->hidden;
	}

	const archimedes::geocoordinate &get_geocoordinate() const
	{
		return this->geocoordinate;
	}

	const std::vector<region *> &get_regions() const
	{
		return this->regions;
	}

	void add_region(region *region);
	void remove_region(region *region);

private:
	QColor color;
	wyrmgus::terrain_type *terrain_type = nullptr;
	bool trade_route = false;
	bool major_river = false;
	bool minor_river = false;
	bool terrain_generation_seed = false;
	bool hidden = false;
	archimedes::geocoordinate geocoordinate;
	std::map<const civilization *, std::string> cultural_names; //names for the terrain feature for each different culture/civilization
	std::vector<region *> regions; //regions where this terrain feature is located

	friend int ::CclDefineTerrainFeature(lua_State *l);
};

}
