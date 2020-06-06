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
#include "util/color_container.h"

struct lua_State;

int CclDefineTerrainFeature(lua_State *l);

namespace stratagus {

class civilization;
class plane;
class terrain_type;
class world;

class terrain_feature final : public named_data_entry, public data_type<terrain_feature>
{
	Q_OBJECT

	Q_PROPERTY(QColor color READ get_color WRITE set_color)
	Q_PROPERTY(stratagus::terrain_type* terrain_type MEMBER terrain_type READ get_terrain_type)

public:
	static constexpr const char *class_identifier = "terrain_feature";
	static constexpr const char *database_folder = "terrain_features";

	static terrain_feature *get_by_color(const QColor &color)
	{
		terrain_feature *feature = terrain_feature::try_get_by_color(color);

		if (feature == nullptr) {
			throw std::runtime_error("No terrain feature found for color: (" + std::to_string(color.red()) + ", " + std::to_string(color.green()) + ", " + std::to_string(color.blue()) + ").");
		}

		return feature;
	}

	static terrain_feature *try_get_by_color(const QColor &color)
	{
		auto find_iterator = terrain_feature::terrain_features_by_color.find(color);
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
	static inline color_map<terrain_feature *> terrain_features_by_color;

public:
	explicit terrain_feature(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;

	const QColor &get_color() const
	{
		return this->color;
	}

	void set_color(const QColor &color);

	terrain_type *get_terrain_type() const
	{
		return this->terrain_type;
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

private:
	QColor color;
	stratagus::terrain_type *terrain_type = nullptr;
	std::map<const civilization *, std::string> cultural_names; //names for the terrain feature for each different culture/civilization

	friend int ::CclDefineTerrainFeature(lua_State *l);
};

}
