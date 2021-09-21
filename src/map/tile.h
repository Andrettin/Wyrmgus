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
//      (c) Copyright 1998-2021 by Vladi Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Rafal Bursig and Andrettin
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

/**
**  tile_player_info::SeenTile
**
**    This is the tile number, that the player sitting on the computer
**    currently knows. Idea: Can be uses for illusions.
**
**  tile_player_info::Visible[]
**
**    Counter how many units of the player can see this field. 0 the
**    field is not explored, 1 explored, n-1 unit see it. Currently
**    no more than 253 units can see a field.
**
**  tile_player_info::VisCloak[]
**
**    Visiblity for cloaking.
**
**  tile_player_info::Radar[]
**
**    Visiblity for radar.
**
**  tile_player_info::RadarJammer[]
**
**    Jamming capabilities.
*/

/**
**  This class contains all information about a field on map.
**  It contains its look, properties and content.
**
**  The map-field class members:
**
**  tile::Tile
**
**    Tile is number defining the graphic image display for the
**    map-field. 65535 different tiles are supported. A tile is
**    currently 32x32 pixels. In the future is planned to support
**    animated tiles.
**
**  tile::Flags
**
**    Contains special information of that tile. What units are
**    on this field, what units could be placed on this field.
**
**    This is the list of all flags currently used:
**
**    ::MapFieldVisible field is visible.
**    ::MapFieldExplored field is explored.
**    ::tile_flag::land_allowed land units are allowed.
**    ::tile_flag::coast_allowed coast units (transporter) and coast buildings (shipyard) are allowed.
**    ::tile_flag::water_allowed water units allowed.
**    ::tile_flag::no_building no buildings allowed.
**    ::tile_flag::impassable field is movement blocked.
**    ::tile_flag::wall field contains wall.
**    ::tile_flag::rock field contains rocks.
**    ::tile_flag::tree field contains forest.
**    ::tile_flag::land_unit land unit on field.
**    ::tile_flag::air_unit air unit on field.
**    ::tile_flag::sea_unit water unit on field.
**    ::tile_flag::building building on field.
**
**    Note: We want to add support for more unit-types like under
**      ground units.
**
**  tile::Cost
**
**    Unit cost to move in this tile.
**
**  tile::value
**
**    Extra value for each tile. This currently only used for
**    walls, contains the remaining hit points of the wall and
**    for forest, contains the frames until they grow.
**
**  tile::UnitCache
**
**    Contains a vector of all units currently on this field.
**    Note: currently units are only inserted at the insert point.
**    This means units of the size of 2x2 fields are inserted at the
**    top and right most map coordinate.
*/

#include "map/tile_transition.h"
#include "player/player_container.h"
#include "unit/unit_cache.h"
#include "vec2i.h"

class CFile;
class CPlayer;
class CTileset;
//Wyrmgus start
class CGraphic;
//Wyrmgus end
struct lua_State;

namespace wyrmgus {

class landmass;
class player_color;
class resource;
class site;
class terrain_feature;
class terrain_type;
class world;
enum class tile_flag : uint32_t;

class tile_player_info final
{
public:
	/// Check if a field for the user is explored.
	bool is_explored(const int player_index) const
	{
		return this->Visible[player_index] != 0;
	}

	bool is_explored(const CPlayer &player) const;

	//Wyrmgus start
	bool IsTeamExplored(const CPlayer &player) const;
	//Wyrmgus end

	/// @note Manage CMap::get()->NoFogOfWar
	bool is_visible(const int player_index, const bool fog_of_war) const
	{
		return this->Visible[player_index] >= 2 || (!fog_of_war && this->is_explored(player_index));
	}

	bool is_visible(const CPlayer &player) const;
	bool IsTeamVisible(const CPlayer &player) const;

	/**
	**  Find out how a field is seen (By player, or by shared vision)
	**
	**  @param player   Player to check for.
	**  @note manage fogOfWar (using CMap::get()->NoFogOfWar)
	**
	**  @return        0 unexplored, 1 explored, 2 visible.
	*/
	unsigned char get_team_visibility_state(const int player_index, const player_index_set &mutual_shared_vision, const std::vector<int> &revealed_player_indexes, const bool fog_of_war) const
	{
		unsigned char max_vision = this->Visible[player_index];

		if (max_vision >= 2) {
			return 2;
		}

		for (const int p : mutual_shared_vision) {
			max_vision = std::max<unsigned char>(max_vision, this->Visible[p]);
			if (max_vision >= 2) {
				return 2;
			}
		}

		for (const int p : revealed_player_indexes) {
			if (this->Visible[p] < 2) { //don't show a revealed player's explored tiles, only the currently visible ones
				continue;
			}

			max_vision = std::max<unsigned char>(max_vision, this->Visible[p]);
			if (max_vision >= 2) {
				return 2;
			}
		}

		if (max_vision == 1 && !fog_of_war) {
			return 2;
		}

		return max_vision;
	}

	unsigned char get_team_visibility_state(const CPlayer &player) const;

public:
	//Wyrmgus start
//	unsigned short SeenTile = 0;              /// last seen tile (FOW)
	const wyrmgus::terrain_type *SeenTerrain = nullptr;	/// last seen tile (FOW)
	const wyrmgus::terrain_type *SeenOverlayTerrain = nullptr;
	short SeenSolidTile = 0;
	short SeenOverlaySolidTile = 0;
	std::vector<tile_transition> SeenTransitionTiles;			/// Transition tiles; the pair contains the terrain type and the tile index
	std::vector<tile_transition> SeenOverlayTransitionTiles;		/// Overlay transition tiles; the pair contains the terrain type and the tile index
	//Wyrmgus end
	std::array<unsigned short, PlayerMax> Visible = {};    /// Seen counter 0 unexplored
	std::array<unsigned char, PlayerMax> VisCloak = {};    /// Visiblity for cloaking.
	std::array<unsigned char, PlayerMax> VisEthereal = {};    /// Visiblity for ethereal.
	std::array<unsigned char, PlayerMax> Radar = {};       /// Visiblity for radar.
	std::array<unsigned char, PlayerMax> RadarJammer = {}; /// Jamming capabilities.
};

/// Describes a field of the map
class tile final
{
public:
	tile();

	void Save(CFile &file) const;
	void parse(lua_State *l);

	//Wyrmgus start
	void SetTerrain(const terrain_type *terrain_type);
	void RemoveOverlayTerrain();
	void SetOverlayTerrainDestroyed(bool destroyed);
	void SetOverlayTerrainDamaged(bool damaged);
	//Wyrmgus end
	void setTileIndex(const CTileset &tileset, unsigned int tileIndex, int value);
	//Wyrmgus start
	void UpdateSeenTile();
	//Wyrmgus end

	//Wyrmgus start
	/*
	unsigned int getGraphicTile() const { return tile; }

	unsigned int getTileIndex() const { return tilesetTile; }
	*/
	//Wyrmgus end

	/// Check if a field flags.
	bool CheckMask(const tile_flag mask) const;
	
	const terrain_type *get_terrain() const
	{
		return this->terrain;
	}

	const terrain_type *get_overlay_terrain() const
	{
		return this->overlay_terrain;
	}

	const terrain_type *get_top_terrain(const bool seen = false, const bool ignore_destroyed = false) const;

	bool is_water() const;
	bool is_non_coastal_water() const;
	bool is_coastal_water() const;
	bool is_river() const;
	bool is_space() const;

	/// Returns true, if water on the map tile field
	bool WaterOnMap() const;

	/// Returns true, if coast on the map tile field
	bool CoastOnMap() const;

	/// Returns true, if water on the map tile field
	bool ForestOnMap() const;

	/// Returns true, if coast on the map tile field
	bool RockOnMap() const;

	bool isAWall() const;

	bool IsSeenTileCorrect() const;
	
	const wyrmgus::terrain_feature *get_terrain_feature() const
	{
		return this->terrain_feature;
	}

	void set_terrain_feature(const wyrmgus::terrain_feature *terrain_feature)
	{
		this->terrain_feature = terrain_feature;
	}

	const resource *get_resource() const;

	bool is_destroyed_tree_tile() const;
	
	tile_flag get_flags() const
	{
		return this->Flags;
	}

	bool has_flag(const tile_flag flag) const;

	unsigned char get_movement_cost() const
	{
		return this->movement_cost;
	}

	void update_movement_cost();

	short get_value() const
	{
		return this->value;
	}

	void set_value(const short value)
	{
		this->value = value;
	}

	void change_value(const short change)
	{
		this->value += change;
	}

	void increment_value()
	{
		++this->value;
	}

	void decrement_value()
	{
		--this->value;
	}

	//Wyrmgus start
//	void setGraphicTile(unsigned int tile) { this->tile = tile; }
	//Wyrmgus end

	bool is_animated() const;

	CPlayer *get_owner() const;
	CPlayer *get_realm_owner() const;

	const player_color *get_player_color() const;

	bool is_border_tile() const
	{
		return this->get_ownership_border_tile() != -1;
	}

	int get_ownership_border_tile() const
	{
		return this->ownership_border_tile;
	}

	void set_ownership_border_tile(const int tile)
	{
		this->ownership_border_tile = tile;
	}

	wyrmgus::landmass *get_landmass() const
	{
		return this->landmass;
	}

	void set_landmass(wyrmgus::landmass *landmass)
	{
		this->landmass = landmass;
	}

	const world *get_world() const;

	const site *get_settlement() const
	{
		return this->settlement;
	}

	void set_settlement(const site *settlement)
	{
		if (settlement == this->get_settlement()) {
			return;
		}

		this->settlement = settlement;
	}

	bool is_on_trade_route() const;

	void bump_incompatible_units();
	void remove_incompatible_units();

public:
	tile_flag Flags;      /// field flags
	//Wyrmgus start
	unsigned char AnimationFrame = 0;		/// current frame of the tile's animation
	unsigned char OverlayAnimationFrame = 0;		/// current frame of the overlay tile's animation
private:
	const terrain_type *terrain = nullptr;
	const terrain_type *overlay_terrain = nullptr;
	const wyrmgus::terrain_feature *terrain_feature = nullptr;
public:
	short SolidTile = 0;
	short OverlaySolidTile = 0;
	bool OverlayTerrainDestroyed = false;
	bool OverlayTerrainDamaged = false;
	std::vector<tile_transition> TransitionTiles; //transition tiles; the pair contains the terrain type and the tile index
	std::vector<tile_transition> OverlayTransitionTiles; //overlay transition tiles; the pair contains the terrain type and the tile index
	//Wyrmgus end
private:
	unsigned char movement_cost = 0; //unit cost to move in this tile
	short value = 0; //HP for walls/resource quantity/forest regeneration/destroyed wall and rock decay
	wyrmgus::landmass *landmass = nullptr; //to which "landmass" (can also be water) does this map field belong (if any); a "landmass" is a collection of adjacent land tiles, or a collection of adjacent water tiles
	short ownership_border_tile = -1; //the transition type of the border between this tile's owner, and other players' tiles, if applicable)
	const site *settlement = nullptr;
public:
	CUnitCache UnitCache;      /// a unit on the map field.

	std::unique_ptr<tile_player_info> player_info;	/// stuff related to player
};

}
