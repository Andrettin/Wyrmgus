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
//      (c) Copyright 2022 by Andrettin
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

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace wyrmgus {

class character;
class terrain_type;
class unit_type;

class dungeon_generation_settings final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::terrain_type* floor_terrain MEMBER floor_terrain)
	Q_PROPERTY(wyrmgus::terrain_type* wall_terrain MEMBER wall_terrain)
	Q_PROPERTY(wyrmgus::terrain_type* deep_wall_terrain MEMBER deep_wall_terrain)
	Q_PROPERTY(wyrmgus::terrain_type* water_terrain MEMBER water_terrain)
	Q_PROPERTY(wyrmgus::unit_type* glyph MEMBER glyph_unit_type)

public:
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const terrain_type *get_floor_terrain() const
	{
		return this->floor_terrain;
	}

	const terrain_type *get_wall_terrain() const
	{
		return this->wall_terrain;
	}

	const terrain_type *get_deep_wall_terrain() const
	{
		return this->deep_wall_terrain;
	}

	const terrain_type *get_water_terrain() const
	{
		return this->water_terrain;
	}

	const unit_type *get_glyph_unit_type() const
	{
		return this->glyph_unit_type;
	}

	const std::vector<const unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}

	const std::vector<const unit_type *> &get_item_unit_types() const
	{
		return this->item_unit_types;
	}

	const std::vector<const unit_type *> &get_trap_unit_types() const
	{
		return this->trap_unit_types;
	}

	const std::vector<character *> &get_heroes() const
	{
		return this->heroes;
	}

private:
	terrain_type *floor_terrain = nullptr;
	terrain_type *wall_terrain = nullptr;
	terrain_type *deep_wall_terrain = nullptr;
	terrain_type *water_terrain = nullptr;
	unit_type *glyph_unit_type = nullptr;
	std::vector<const unit_type *> unit_types;
	std::vector<const unit_type *> item_unit_types;
	std::vector<const unit_type *> trap_unit_types;
	std::vector<character *> heroes;
};

}
