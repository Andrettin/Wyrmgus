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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon, Rafal Bursig
//		and Andrettin
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

#include "stratagus.h"

#include "unit/unit_type.h"

#include "actions.h"
#include "editor.h"
#include "game/game.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/map_settings.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "player/player.h"
#include "unit/build_restriction/and_build_restriction.h"
#include "unit/build_restriction/on_top_build_restriction.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_find.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

/**
**  Find the building restriction that gives me this unit built on top
**  Allows you to define how the restriction is effecting the build
**
**  @param unit    the unit that is "OnTop"
**  @param parent  the parent unit if known. (guess otherwise)
**
**  @return        the BuildingRestrictionDetails
*/
const on_top_build_restriction *OnTopDetails(const wyrmgus::unit_type &type, const wyrmgus::unit_type *parent)
{
	if (type.get_build_restrictions() != nullptr) {
		for (const std::unique_ptr<build_restriction> &b : type.get_build_restrictions()->get_restrictions()) {
			const on_top_build_restriction *ontopb = dynamic_cast<on_top_build_restriction *>(b.get());

			if (ontopb) {
				if (CMap::get()->get_settings()->is_unit_type_disabled(ontopb->Parent)) {
					continue;
				}

				if (!parent) {
					// Guess this is right
					return ontopb;
				}

				if (parent == ontopb->Parent) {
					return ontopb;
				}

				continue;
			}

			and_build_restriction *andb = dynamic_cast<and_build_restriction *>(b.get());

			if (andb) {
				for (const auto &sub_b : andb->get_restrictions()) {
					ontopb = dynamic_cast<on_top_build_restriction *>(sub_b.get());

					if (ontopb) {
						if (CMap::get()->get_settings()->is_unit_type_disabled(ontopb->Parent)) {
							continue;
						}

						if (!parent) {
							// Guess this is right
							return ontopb;
						}

						if (parent == ontopb->Parent) {
							return ontopb;
						}
					}
				}
			}
		}
	}

	return nullptr;
}

/**
**  Can build unit here.
**  Hall too near to goldmine.
**  Refinery or shipyard too near to oil patch.
**
**  @param unit  Unit doing the building
**  @param type  unit-type to be checked.
**  @param pos   Map position.
**
**  @return      OnTop, parent unit, builder on true or 1 if unit==nullptr, null false.
*/
CUnit *CanBuildHere(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int z, const bool no_bordering_impassable)
{
	//  Can't build outside the map
	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		return nullptr;
	}

	if ((pos.x() + type.get_tile_width()) > CMap::get()->Info->MapWidths[z]) {
		return nullptr;
	}
	if ((pos.y() + type.get_tile_height()) > CMap::get()->Info->MapHeights[z]) {
		return nullptr;
	}
	
	if (no_bordering_impassable && !OnTopDetails(type, nullptr)) {
		//if a game is starting, only place buildings with a certain space from other buildings
		if (type.pos_borders_impassable(pos, z)) {
			return nullptr;
		}
	}

	if (GameEstablishing) {
		//do not allow buildings to be placed on borders at start
		const QRect rect(pos, type.get_tile_size());

		if (CMap::get()->is_rect_on_settlement_borders(rect, z)) {
			return nullptr;
		}
	}

	// Must be checked before oil!
	if (type.BoolFlag[SHOREBUILDING_INDEX].value) {
		const int width = type.get_tile_width();
		int h = type.get_tile_height();
		bool success = false;

		// Need at least one coast tile
		//Wyrmgus start
//		unsigned int index = CMap::get()->get_pos_index(pos);
		unsigned int index = CMap::get()->get_pos_index(pos, z);
		//Wyrmgus end
		do {
			//Wyrmgus start
//			const wyrmgus::tile *mf = CMap::get()->Field(index);
			const wyrmgus::tile *mf = CMap::get()->Field(index, z);
			//Wyrmgus end
			int w = width;
			do {
				if (mf->CoastOnMap()) {
					success = true;
				}
				//Wyrmgus start
				if (GameCycle == 0 && mf->WaterOnMap() && CMap::get()->TileBordersFlag(pos, z, tile_flag::water_allowed, true)) {
					//if the game hasn't started, it is possible that coast map fields haven't been applied yet, so we have to check if the tile is a water tile with an adjacent non-water tile (which is what a coastal tile is)
					success = true;
				}
				//Wyrmgus end
				++mf;
			} while (!success && --w);
			//Wyrmgus start
//			index += CMap::get()->Info->MapWidth;
			index += CMap::get()->Info->MapWidths[z];
			//Wyrmgus end
		} while (!success && --h);
		if (!success) {
			return nullptr;
		}
	}

	// Check special rules for AI players
	if (unit && unit->Player->AiEnabled) {
		if (type.get_ai_build_restrictions() != nullptr) {
			CUnit *ontoptarget = nullptr;
			//all checks processed, did we really have success
			if (!type.get_ai_build_restrictions()->Check(unit, type, pos, ontoptarget, z)) {
				//we passed a full ruleset
				return nullptr;
			}
		}
	}

	if (type.get_build_restrictions() != nullptr) {
		CUnit *ontoptarget = nullptr;

		//all checks processed, did we really have success
		if (type.get_build_restrictions()->Check(unit, type, pos, ontoptarget, z)) {
			//we passed a full ruleset return
			if (unit == nullptr) {
				return ontoptarget ? ontoptarget : (CUnit *)1;
			} else {
				return ontoptarget ? ontoptarget : const_cast<CUnit *>(unit);
			}
		} else {
			return nullptr;
		}
	}
	
	if (unit && z != unit->Player->StartMapLayer && CMap::get()->MapLayers[z]->world != CMap::get()->MapLayers[unit->Player->StartMapLayer]->world) {
		return nullptr;
	}

	return (unit == nullptr) ? (CUnit *)1 : const_cast<CUnit *>(unit);
}

/**
**  Can build on this point.
**
**  @param pos   tile map position.
**  @param  mask terrain mask
**
**  @return true if we can build on this point.
*/
bool CanBuildOn(const QPoint &pos, const tile_flag mask, const int z, const CPlayer *player, const wyrmgus::unit_type *unit_type)
{
	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		return false;
	}

	const wyrmgus::tile *tile = CMap::get()->Field(pos, z);

	if (tile->CheckMask(mask)) {
		return false;
	}

	if (player != nullptr && tile->get_owner() != nullptr && tile->get_owner() != player) {
		return false;
	}

	const wyrmgus::terrain_type *built_terrain = unit_type->get_terrain_type();

	//cannot build anything other than pathways on trade routes
	if (tile->is_on_trade_route() && (built_terrain == nullptr || !built_terrain->is_pathway())) {
		return false;
	}

	const wyrmgus::terrain_type *overlay_terrain = tile->get_overlay_terrain();

	//can only build a pathway on top of another pathway if the latter has a smaller movement bonus, or if the former is a railroad and the latter isn't
	if (built_terrain != nullptr && built_terrain->is_pathway() && overlay_terrain != nullptr && overlay_terrain->is_pathway() && built_terrain->get_movement_bonus() <= overlay_terrain->get_movement_bonus() && (!built_terrain->has_flag(tile_flag::railroad) || tile->has_flag(tile_flag::railroad))) {
		return false;
	}

	return true;
}

/**
**  Can build unit-type at this point.
**
**  @param unit  Worker that want to build the building or null.
**  @param type  Building unit-type.
**  @param pos   tile map position.
**  @param real  Really build, or just placement
**
**  @return      OnTop, parent unit, builder on true, null false.
**
*/
CUnit *CanBuildUnitType(const CUnit *unit, const wyrmgus::unit_type &type, const QPoint &pos, const int real, const bool ignore_exploration, const int z, const bool no_bordering_impassable)
{
	// Terrain Flags don't matter if building on top of a unit.
	CUnit *ontop = CanBuildHere(unit, type, pos, z, no_bordering_impassable);
	if (ontop == nullptr) {
		return nullptr;
	}
	if (ontop != (CUnit *)1 && ontop != unit) {
		return ontop;
	}

	//  Remove unit that is building!
	if (unit) {
		UnmarkUnitFieldFlags(*unit);
	}

	CPlayer *player = nullptr;

	//Wyrmgus start
//	if (unit && unit->Player->get_type() == player_type::person) {
	if (unit) {
	//Wyrmgus end
		player = unit->Player;
	}
	tile_flag testmask = tile_flag::none;
	unsigned int index = pos.y() * CMap::get()->Info->MapWidths[z];
	for (int h = 0; h < type.get_tile_height(); ++h) {
		for (int w = type.get_tile_width(); w--;) {
			/* first part of if (!CanBuildOn(x + w, y + h, testmask)) */
			if (!CMap::get()->Info->IsPointOnMap(pos.x() + w, pos.y() + h, z)) {
				ontop = nullptr;
				break;
			}
			if (player && !real) {
				//testmask = MapFogFilterFlags(player, x + w, y + h, type.MovementMask);
				testmask = MapFogFilterFlags(*player,
											 index + pos.x() + w, type.MovementMask, z);
			} else {
				testmask = type.MovementMask;
			}
			/*second part of if (!CanBuildOn(x + w, y + h, testmask)) */
			const wyrmgus::tile *tile = CMap::get()->Field(index + pos.x() + w, z);
			if (tile->CheckMask(testmask)) {
				ontop = nullptr;
				break;
			}
			//Wyrmgus start
//			if (player && !mf.player_info->is_explored(*player)) {
			if (player && !ignore_exploration && !tile->player_info->IsTeamExplored(*player)) {
			//Wyrmgus end
				ontop = nullptr;
				break;
			}

			if (player != nullptr && tile->get_owner() != nullptr && tile->get_owner() != player) {
				ontop = nullptr;
				break;
			}

			//cannot build anything other than pathways on trade routes
			if (tile->is_on_trade_route() && (type.get_terrain_type() == nullptr || !type.get_terrain_type()->is_pathway())) {
				ontop = nullptr;
				break;
			}

			//can only build a pathway on top of another pathway if the latter has a smaller movement bonus
			if (type.get_terrain_type() != nullptr && type.get_terrain_type()->is_pathway() && tile->get_overlay_terrain() != nullptr && tile->get_overlay_terrain()->is_pathway() && type.get_terrain_type()->get_movement_bonus() <= tile->get_overlay_terrain()->get_movement_bonus()) {
				ontop = nullptr;
				break;
			}
		}

		if (ontop == nullptr) {
			break;
		}

		index += CMap::get()->Info->MapWidths[z];
	}
	if (unit) {
		MarkUnitFieldFlags(*unit);
	}
	// We can build here: check distance to gold mine/oil patch!
	return ontop;
}
