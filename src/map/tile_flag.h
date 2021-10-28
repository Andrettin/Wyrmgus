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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

namespace wyrmgus {

enum class tile_flag : uint32_t {
	none = 0,

	land_allowed = 1 << 1,	/// Land units allowed
	coast_allowed = 1 << 2,	/// Coast (e.g. transporter) units allowed
	water_allowed = 1 << 3,	/// Water units allowed
	no_building = 1 << 4,	/// No buildings allowed

	impassable = 1 << 5,	/// Field is movement blocked
	air_impassable = 1 << 6,	/// Field is movement blocked for air units and missiles
	wall = 1 << 7,			/// Field contains wall
	rock = 1 << 8,			/// Field contains rocks
	tree = 1 << 9,		/// Field contains forest

	land_unit = 1 << 10,		/// Land unit on field
	sea_unit = 1 << 11,		/// Water unit on field
	air_unit = 1 << 12,		/// Air unit on field
	building = 1 << 13,		/// Building on field
	item = 1 << 14,			/// Item on field

	road = 1 << 15,			/// Road (moves faster)
	railroad = 1 << 16,		/// Railroad (moves faster)

	grass = 1 << 19,		/// Used for playing grass step sounds
	mud = 1 << 20,			/// Used for playing mud step sounds
	stone_floor = 1 << 21,	/// Used for playing stone step sounds
	dirt = 1 << 22,			/// Used for playing dirt step sounds
	desert = 1 << 23,		/// Used for identifying desert tiles for desertstalk and dehydration
	snow = 1 << 24,			/// Used for playing snow step sounds
	ice = 1 << 25,			/// Used for playing ice step sounds

	gravel = 1 << 26,		/// Used for playing gravel step sounds
	stumps = 1 << 27,		/// Used for playing stumps step sounds and identifying removed forests

	underground = 1 << 28,	/// The terrain is an underground one; this is used to make it always be "night" there, for graphical purposes as well as for unit sight
	space = 1 << 29, //the terrain is a space one, only being passable by space units
	space_cliff = 1 << 30, //used for the transition between land/water and space
	air_building = 1u << 31
};

inline tile_flag string_to_tile_flag(const std::string &tile_flag)
{
	if (tile_flag == "land") {
		return tile_flag::land_allowed;
	} else if (tile_flag == "coast") {
		return tile_flag::coast_allowed;
	} else if (tile_flag == "water") {
		return tile_flag::water_allowed;
	} else if (tile_flag == "no-building" || tile_flag == "no_building") {
		return tile_flag::no_building;
	} else if (tile_flag == "unpassable" || tile_flag == "impassable") {
		return tile_flag::impassable;
	} else if (tile_flag == "wall") {
		return tile_flag::wall;
	} else if (tile_flag == "rock") {
		return tile_flag::rock;
	} else if (tile_flag == "forest" || tile_flag == "tree") {
		return tile_flag::tree;
	} else if (tile_flag == "air-unpassable" || tile_flag == "air_unpassable" || tile_flag == "air_impassable") {
		return tile_flag::air_impassable;
	} else if (tile_flag == "desert") {
		return tile_flag::desert;
	} else if (tile_flag == "dirt") {
		return tile_flag::dirt;
	} else if (tile_flag == "grass") {
		return tile_flag::grass;
	} else if (tile_flag == "gravel") {
		return tile_flag::gravel;
	} else if (tile_flag == "ice") {
		return tile_flag::ice;
	} else if (tile_flag == "mud") {
		return tile_flag::mud;
	} else if (tile_flag == "railroad") {
		return tile_flag::railroad;
	} else if (tile_flag == "road") {
		return tile_flag::road;
	} else if (tile_flag == "snow") {
		return tile_flag::snow;
	} else if (tile_flag == "stone_floor") {
		return tile_flag::stone_floor;
	} else if (tile_flag == "stumps") {
		return tile_flag::stumps;
	} else if (tile_flag == "underground") {
		return tile_flag::underground;
	} else if (tile_flag == "space") {
		return tile_flag::space;
	} else if (tile_flag == "space_cliff") {
		return tile_flag::space_cliff;
	} else if (tile_flag == "land_unit" || tile_flag == "land-unit") {
		return tile_flag::land_unit;
	} else if (tile_flag == "sea_unit" || tile_flag == "sea-unit") {
		return tile_flag::sea_unit;
	} else if (tile_flag == "air_unit" || tile_flag == "air-unit") {
		return tile_flag::air_unit;
	} else if (tile_flag == "building") {
		return tile_flag::building;
	} else if (tile_flag == "air_building") {
		return tile_flag::air_building;
	} else if (tile_flag == "item") {
		return tile_flag::item;
	} else {
		throw std::runtime_error("Invalid tile flag: \"" + tile_flag + "\".");
	}
}

extern const tile_flag &operator &=(tile_flag &lhs, const tile_flag rhs);
extern tile_flag operator &(const tile_flag &lhs, const tile_flag rhs);

extern const tile_flag &operator |=(tile_flag &lhs, const tile_flag rhs);
extern tile_flag operator |(const tile_flag &lhs, const tile_flag rhs);

extern tile_flag operator ~(const tile_flag flag);

}
