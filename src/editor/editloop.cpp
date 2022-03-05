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
/**@name editloop.cpp - The editor main loop. */
//
//      (c) Copyright 2002-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "editor.h"

#include "commands.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "engine_interface.h"
#include "game/game.h"
#include "guichan.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/tileset.h"
#include "menus.h"
#include "network.h"
#include "parameters.h"
#include "player/civilization.h"
#include "player/player.h"
#include "player/player_type.h"
#include "replay.h"
#include "script.h"
#include "settings.h"
#include "sound/game_sound_set.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/enum_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"
#include "video/font.h"
#include "video/render_context.h"
#include "video/video.h"
#include "widgets.h"

extern void DoScrollArea(int state, bool fast, bool isKeyboard, const Qt::KeyboardModifiers key_modifiers);
extern void DrawGuichanWidgets(std::vector<std::function<void(renderer *)>> &render_commands);

static int IconWidth;                       /// Icon width in panels
static int IconHeight;                      /// Icon height in panels

static int ButtonPanelWidth;
static int ButtonPanelHeight;

char TileToolRandom;      /// Tile tool draws random
static char TileToolDecoration;  /// Tile tool draws with decorations
static int TileCursorSize;       /// Tile cursor size 1x1 2x2 ... 4x4
static bool UnitPlacedThisPress = false;  /// Only allow one unit per press
static bool UpdateMinimap = false;        /// Update units on the minimap
static int MirrorEdit = 0;                /// Mirror editing enabled
static int VisibleUnitIcons;              /// Number of icons that are visible at a time
static int VisibleTileIcons;

enum _mode_buttons_ {
	SelectButton = 201,  /// Select mode button
	UnitButton,          /// Unit mode button
	TileButton,          /// Tile mode button
	StartButton
};

enum EditorActionType {
	EditorActionTypePlaceUnit,
	EditorActionTypeRemoveUnit
};

struct EditorAction {
	EditorActionType Type;
	Vec2i tilePos;
	const unit_type *UnitType;
	CPlayer *Player;
};

static std::deque<EditorAction> EditorUndoActions;
static std::deque<EditorAction> EditorRedoActions;

/// Unit mode icon
static int get_unit_icon_x()
{
	return IconWidth + (7 * preferences::get()->get_scale_factor()).to_int();
}

static int get_unit_icon_y()
{
	return 0;
}

/// Tile mode icon
static int get_tile_icon_x()
{
	return IconWidth * 2 + (16 * preferences::get()->get_scale_factor()).to_int();
}

static int get_tile_icon_y()
{
	return (2 * preferences::get()->get_scale_factor()).to_int();
}

/// Start mode icon
static int get_start_icon_x()
{
	return IconWidth * 3 + (16 * preferences::get()->get_scale_factor()).to_int();
}

static int get_start_icon_y()
{
	return (2 * preferences::get()->get_scale_factor()).to_int();
}

static void EditorUndoAction();
static void EditorRedoAction();
static void EditorAddUndoAction(EditorAction action);

static std::unique_ptr<gcn::Container> editorContainer;
static std::unique_ptr<gcn::Slider> editorUnitSlider;
static std::unique_ptr<gcn::Slider> editorSlider;

class EditorUnitSliderListener final : public gcn::ActionListener
{
public:
	virtual void action(const std::string &)
	{
		const int iconsPerStep = VisibleUnitIcons;
		//Wyrmgus start
//		const int steps = (CEditor::get()->ShownUnitTypes.size() + iconsPerStep - 1) / iconsPerStep;
		const int steps = (CEditor::get()->ShownUnitTypes.size() + 1 + iconsPerStep - 1) / iconsPerStep; // + 1 because of the button to create a new unit
		//Wyrmgus end
		const double value = editorUnitSlider->getValue();
		for (int i = 1; i <= steps; ++i) {
			if (value <= (double)i / steps) {
				CEditor::get()->UnitIndex = iconsPerStep * (i - 1);
				break;
			}
		}
	}
};

static std::unique_ptr<EditorUnitSliderListener> editorUnitSliderListener;

class EditorSliderListener final : public gcn::ActionListener
{
public:
	virtual void action(const std::string &)
	{
		const int iconsPerStep = VisibleTileIcons;
		const int steps = (CEditor::get()->ShownTileTypes.size() + iconsPerStep - 1) / iconsPerStep;
		const double value = editorSlider->getValue();
		for (int i = 1; i <= steps; ++i) {
			if (value <= (double)i / steps) {
				CEditor::get()->TileIndex = iconsPerStep * (i - 1);
				break;
			}
		}
	}
};

static std::unique_ptr<EditorSliderListener> editorSliderListener;

/*----------------------------------------------------------------------------
--  Edit
----------------------------------------------------------------------------*/

/**
**  Edit tile.
**
**  @param pos   map tile coordinate.
**  @param tile  Tile type to edit.
*/
//Wyrmgus start
//static void EditTile(const Vec2i &pos, int tile)
static void EditTile(const Vec2i &pos, const terrain_type *terrain, const Qt::KeyboardModifiers key_modifiers)
//Wyrmgus end
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, UI.CurrentMapLayer));
	
	tile &mf = *UI.CurrentMapLayer->Field(pos);
	
	//Wyrmgus start
	int value = 0;
	if (terrain->has_flag(tile_flag::tree) || terrain->has_flag(tile_flag::rock)) {
		value = terrain->get_resource()->get_default_amount();
	}
//	mf.setTileIndex(tileset, tileIndex, 0);
	mf.SetTerrain(terrain);
	if (!terrain->is_overlay() && !(key_modifiers & Qt::ShiftModifier)) { // don't remove overlay terrains if holding shift
		mf.RemoveOverlayTerrain();
	}
	mf.set_value(value);
//	mf.player_info->SeenTile = mf.getGraphicTile();
	mf.UpdateSeenTile();
	//Wyrmgus end
	

	//Wyrmgus start
	CUnitCache &unitcache = mf.UnitCache;
	std::vector<CUnit *> units_to_remove;

	for (CUnitCache::iterator it = unitcache.begin(); it != unitcache.end(); ++it) {
		CUnit *unit = *it;

		if (!CanBuildUnitType(unit, *unit->Type, pos, 1, true, UI.CurrentMapLayer->ID)) {
			units_to_remove.push_back(unit);
		}
	}
	
	for (size_t i = 0; i < units_to_remove.size(); ++i) {
		EditorActionRemoveUnit(*units_to_remove[i], false);
	}
	//Wyrmgus end

	UI.get_minimap()->UpdateSeenXY(pos);
	//Wyrmgus start
//	UI.get_minimap()->UpdateXY(pos);
	UI.get_minimap()->UpdateXY(pos, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	UpdateMinimap = true;
}

/**
**  Edit tiles (internal, used by EditTiles()).
**
**  @param pos   map tile coordinate.
**  @param tile  Tile type to edit.
**  @param size  Size of rectangle
**
**  @bug  This function does not support mirror editing!
*/
//Wyrmgus start
//static void EditTilesInternal(const Vec2i &pos, int tile, int size)
static void EditTilesInternal(const Vec2i &pos, const terrain_type *terrain, int size, const Qt::KeyboardModifiers key_modifiers)
//Wyrmgus end
{
	Vec2i minPos = pos;
	Vec2i maxPos(pos.x + size - 1, pos.y + size - 1);

	//Wyrmgus start
//	CMap::get()->FixSelectionArea(minPos, maxPos);
	CMap::get()->FixSelectionArea(minPos, maxPos, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	//Wyrmgus start
	std::vector<Vec2i> changed_tiles;
	//Wyrmgus end
	
	Vec2i itPos;
	for (itPos.y = minPos.y; itPos.y <= maxPos.y; ++itPos.y) {
		for (itPos.x = minPos.x; itPos.x <= maxPos.x; ++itPos.x) {
			//Wyrmgus start
			if (CMap::get()->GetTileTopTerrain(itPos, false, UI.CurrentMapLayer->ID) == terrain) {
				continue;
			}
//			EditTile(itPos, tile);
			EditTile(itPos, terrain, key_modifiers);
			changed_tiles.push_back(itPos);
			//Wyrmgus end
		}
	}
	
	//now, check if the tiles adjacent to the placed ones need correction
	//Wyrmgus start
	for (int i = (((int) changed_tiles.size()) - 1); i >= 0; --i) {
		const terrain_type *tile_terrain = CMap::get()->GetTileTerrain(changed_tiles[i], terrain->is_overlay(), UI.CurrentMapLayer->ID);
		
		CMap::get()->calculate_tile_transitions(changed_tiles[i], false, UI.CurrentMapLayer->ID);
		CMap::get()->calculate_tile_transitions(changed_tiles[i], true, UI.CurrentMapLayer->ID);

		bool has_transitions = terrain->is_overlay() ? (UI.CurrentMapLayer->Field(changed_tiles[i])->OverlayTransitionTiles.size() > 0) : (UI.CurrentMapLayer->Field(changed_tiles[i])->TransitionTiles.size() > 0);
		bool solid_tile = true;
		
		if (tile_terrain && !tile_terrain->allows_single()) {
			for (int x_offset = -1; x_offset <= 1; ++x_offset) {
				for (int y_offset = -1; y_offset <= 1; ++y_offset) {
					if (x_offset != 0 || y_offset != 0) {
						Vec2i adjacent_pos(changed_tiles[i].x + x_offset, changed_tiles[i].y + y_offset);
						if (CMap::get()->Info->IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
							const terrain_type *adjacent_terrain = CMap::get()->GetTileTerrain(adjacent_pos, tile_terrain->is_overlay(), UI.CurrentMapLayer->ID);
							if (tile_terrain->is_overlay() && adjacent_terrain && UI.CurrentMapLayer->Field(adjacent_pos)->OverlayTerrainDestroyed) {
								adjacent_terrain = nullptr;
							}
							if (tile_terrain != adjacent_terrain && !vector::contains(tile_terrain->get_outer_border_terrain_types(), adjacent_terrain)) { // also happens if terrain is null, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
								solid_tile = false;
								break;
							}
						}
					}
				}
			}
				
			if (!solid_tile && !has_transitions) {
				for (int x_offset = -1; x_offset <= 1; ++x_offset) {
					for (int y_offset = -1; y_offset <= 1; ++y_offset) {
						if (x_offset != 0 || y_offset != 0) {
							Vec2i adjacent_pos(changed_tiles[i].x + x_offset, changed_tiles[i].y + y_offset);
							if (std::find(changed_tiles.begin(), changed_tiles.end(), adjacent_pos) != changed_tiles.end()) {
								continue;
							}
							if (CMap::get()->Info->IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
								EditTile(adjacent_pos, terrain, key_modifiers);
								changed_tiles.push_back(adjacent_pos);
							}
						}
					}
				}
			}
		}
	}
	
	// now check if changing the tiles left any tiles in an incorrect position, and if so, change it accordingly
	for (size_t i = 0; i != changed_tiles.size(); ++i) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(changed_tiles[i].x + x_offset, changed_tiles[i].y + y_offset);
					
					if (std::find(changed_tiles.begin(), changed_tiles.end(), adjacent_pos) != changed_tiles.end()) {
						continue;
					}
					
					if (CMap::get()->Info->IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
						for (int overlay = 1; overlay >= 0; --overlay) {
							const terrain_type *adjacent_terrain = CMap::get()->GetTileTerrain(adjacent_pos, overlay > 0, UI.CurrentMapLayer->ID);
							if (!adjacent_terrain || adjacent_terrain == CMap::get()->GetTileTerrain(changed_tiles[i], overlay > 0, UI.CurrentMapLayer->ID)) {
								continue;
							}
							CMap::get()->calculate_tile_transitions(adjacent_pos, overlay == 1, UI.CurrentMapLayer->ID);
							bool has_transitions = overlay ? (UI.CurrentMapLayer->Field(adjacent_pos)->OverlayTransitionTiles.size() > 0) : (UI.CurrentMapLayer->Field(adjacent_pos)->TransitionTiles.size() > 0);
							bool solid_tile = true;
							
							if (!overlay && !adjacent_terrain->is_border_terrain_type(CMap::get()->GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID))) {
								const terrain_type *intermediate_terrain = adjacent_terrain->get_intermediate_terrain_type(CMap::get()->GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID));

								if (intermediate_terrain != nullptr) {
									//found a terrain type that can border both terrains
									CMap::get()->SetTileTerrain(adjacent_pos, intermediate_terrain, UI.CurrentMapLayer->ID);
									changed_tiles.push_back(adjacent_pos);
								}
							} else if (!adjacent_terrain->allows_single()) {
								for (int sub_x_offset = -1; sub_x_offset <= 1; ++sub_x_offset) {
									for (int sub_y_offset = -1; sub_y_offset <= 1; ++sub_y_offset) {
										if (sub_x_offset != 0 || sub_y_offset != 0) {
											Vec2i sub_adjacent_pos(adjacent_pos.x + sub_x_offset, adjacent_pos.y + sub_y_offset);
											if (CMap::get()->Info->IsPointOnMap(sub_adjacent_pos, UI.CurrentMapLayer)) {
												const terrain_type *sub_adjacent_terrain = CMap::get()->GetTileTerrain(sub_adjacent_pos, overlay > 0, UI.CurrentMapLayer->ID);
												if (adjacent_terrain->is_overlay() && sub_adjacent_terrain && UI.CurrentMapLayer->Field(sub_adjacent_pos)->OverlayTerrainDestroyed) {
													sub_adjacent_terrain = nullptr;
												}
												if (adjacent_terrain != sub_adjacent_terrain && !vector::contains(adjacent_terrain->get_outer_border_terrain_types(), sub_adjacent_terrain)) { // also happens if terrain is null, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
													solid_tile = false;
													break;
												}
											}
										}
									}
								}
									
								if (!solid_tile && !has_transitions) {
									if (overlay) {
										CMap::get()->RemoveTileOverlayTerrain(adjacent_pos, UI.CurrentMapLayer->ID);
									} else {
										CMap::get()->SetTileTerrain(adjacent_pos, CMap::get()->GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID), UI.CurrentMapLayer->ID);
									}
									changed_tiles.push_back(adjacent_pos);
								}
							}
						}
					}
				}
			}
		}
	}
	
	for (const Vec2i &tile_pos : changed_tiles) {
		if (CMap::get()->GetTileTerrain(tile_pos, terrain->is_overlay(), UI.CurrentMapLayer->ID) == terrain) {
			CMap::get()->calculate_tile_solid_tile(tile_pos, terrain->is_overlay(), UI.CurrentMapLayer->ID);
		}
		CMap::get()->calculate_tile_transitions(tile_pos, false, UI.CurrentMapLayer->ID);
		CMap::get()->calculate_tile_transitions(tile_pos, true, UI.CurrentMapLayer->ID);
		UI.get_minimap()->UpdateXY(tile_pos, UI.CurrentMapLayer->ID);

		const tile *tile = UI.CurrentMapLayer->Field(tile_pos);
		const player_color *player_color = tile->get_player_color();
		emit UI.CurrentMapLayer->tile_image_changed(tile_pos, tile->get_terrain(), tile->SolidTile, player_color);
		emit UI.CurrentMapLayer->tile_overlay_image_changed(tile_pos, tile->get_overlay_terrain(), tile->OverlaySolidTile, player_color);
		emit UI.CurrentMapLayer->tile_transition_images_changed(tile_pos, tile->TransitionTiles, player_color);
		emit UI.CurrentMapLayer->tile_overlay_transition_images_changed(tile_pos, tile->OverlayTransitionTiles, player_color);

		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					const QPoint adjacent_pos(tile_pos.x + x_offset, tile_pos.y + y_offset);
					
					if (std::find(changed_tiles.begin(), changed_tiles.end(), adjacent_pos) != changed_tiles.end()) {
						continue;
					}
					
					if (!CMap::get()->Info->IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
						continue;
					}

					const wyrmgus::tile *adjacent_tile = UI.CurrentMapLayer->Field(adjacent_pos);

					const size_t old_adjacent_base_transition_count = adjacent_tile->TransitionTiles.size();
					const size_t old_adjacent_overlay_transition_count = adjacent_tile->OverlayTransitionTiles.size();

					CMap::get()->calculate_tile_transitions(adjacent_pos, false, UI.CurrentMapLayer->ID);
					CMap::get()->calculate_tile_transitions(adjacent_pos, true, UI.CurrentMapLayer->ID);
					UI.get_minimap()->UpdateXY(adjacent_pos, UI.CurrentMapLayer->ID);

					const wyrmgus::player_color *adjacent_player_color = adjacent_tile->get_player_color();

					if (old_adjacent_base_transition_count != 0 || adjacent_tile->TransitionTiles.size() != 0) {
						emit UI.CurrentMapLayer->tile_transition_images_changed(adjacent_pos, adjacent_tile->TransitionTiles, adjacent_player_color);
					}

					if (old_adjacent_overlay_transition_count != 0 || adjacent_tile->OverlayTransitionTiles.size() != 0) {
						emit UI.CurrentMapLayer->tile_overlay_transition_images_changed(adjacent_pos, adjacent_tile->OverlayTransitionTiles, adjacent_player_color);
					}
				}
			}
		}
	}
	//Wyrmgus end
}

/**
**  Edit tiles
**
**  @param pos   map tile coordinate.
**  @param tile  Tile type to edit.
**  @param size  Size of rectangle
*/
//Wyrmgus start
//static void EditTiles(const Vec2i &pos, int tile, int size)
static void EditTiles(const Vec2i &pos, const terrain_type *terrain, int size, const Qt::KeyboardModifiers key_modifiers)
//Wyrmgus end
{
	//Wyrmgus start
//	EditTilesInternal(pos, tile, size);
	EditTilesInternal(pos, terrain, size, key_modifiers);
	//Wyrmgus end

	if (!MirrorEdit) {
		return;
	}
	const Vec2i mpos(UI.CurrentMapLayer->get_width() - size, UI.CurrentMapLayer->get_height() - size);
	const Vec2i mirror = mpos - pos;
	const Vec2i mirrorv(mirror.x, pos.y);

	EditTilesInternal(mirrorv, terrain, size, key_modifiers);
	if (MirrorEdit == 1) {
		return;
	}
	const Vec2i mirrorh(pos.x, mirror.y);

	EditTilesInternal(mirrorh, terrain, size, key_modifiers);
	EditTilesInternal(mirror, terrain, size, key_modifiers);
}

/*----------------------------------------------------------------------------
--  Actions
----------------------------------------------------------------------------*/

/**
**  Place unit.
**
**  @param pos     map tile coordinate.
**  @param type    Unit type to edit.
**  @param player  Player owning the unit.
**
**  @todo  FIXME: Check if the player has already a start-point.
**  @bug   This function does not support mirror editing!
*/
static void EditorActionPlaceUnit(const Vec2i &pos, const unit_type &type, CPlayer *player)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, UI.CurrentMapLayer));

	if (type.Neutral) {
		player = CPlayer::get_neutral_player();
	}

	// FIXME: vladi: should check place when mirror editing is enabled...?
	//Wyrmgus start
//	CUnit *unit = MakeUnitAndPlace(pos, type, player);
	CUnit *unit = MakeUnitAndPlace(pos, type, player, UI.CurrentMapLayer->ID);
	//Wyrmgus end
	if (unit == nullptr) {
		DebugPrint("Unable to allocate Unit");
		return;
	}

	//Wyrmgus start
//	const CBuildRestrictionOnTop *b = OnTopDetails(*unit, nullptr);
	const CBuildRestrictionOnTop *b = OnTopDetails(*unit->Type, nullptr);
	//Wyrmgus end
	if (b && b->ReplaceOnBuild) {
		CUnitCache &unitCache = UI.CurrentMapLayer->Field(pos)->UnitCache;

		CUnitCache::iterator it = std::find_if(unitCache.begin(), unitCache.end(), HasSameTypeAs(*b->Parent));

		if (it != unitCache.end()) {
			CUnit &replacedUnit = **it;
			unit->ReplaceOnTop(replacedUnit);
		}
	}
	if (unit != nullptr) {
		if (type.get_given_resource() != nullptr) {
			//Wyrmgus start
//			if (type.StartingResources != 0) {
			if (type.get_starting_resources().size() > 0) {
			//Wyrmgus end
				//Wyrmgus start
//				unit->ResourcesHeld = type.StartingResources;
//				unit->Variable[GIVERESOURCE_INDEX].Value = type.StartingResources;
//				unit->Variable[GIVERESOURCE_INDEX].Max = type.StartingResources;
				unit->SetResourcesHeld(vector::get_random(type.get_starting_resources()));
				unit->Variable[GIVERESOURCE_INDEX].Value = unit->ResourcesHeld;
				unit->Variable[GIVERESOURCE_INDEX].Max = unit->ResourcesHeld;
				//Wyrmgus end
			} else {
				unit->SetResourcesHeld(type.get_given_resource()->get_default_amount());
				unit->Variable[GIVERESOURCE_INDEX].Value = type.get_given_resource()->get_default_amount();
				unit->Variable[GIVERESOURCE_INDEX].Max = type.get_given_resource()->get_default_amount();
			}
			unit->Variable[GIVERESOURCE_INDEX].Enable = 1;
		}
	} else {
		DebugPrint("Unable to allocate Unit");
	}
	UpdateMinimap = true;
}

/**
**  Edit unit.
**
**  @param pos     map tile coordinate.
**  @param type    Unit type to edit.
**  @param player  Player owning the unit.
*/
static void EditorPlaceUnit(const Vec2i &pos, const unit_type &type, CPlayer *player)
{
	EditorAction editorAction;
	editorAction.Type = EditorActionTypePlaceUnit;
	editorAction.tilePos = pos;
	editorAction.UnitType = &type;
	editorAction.Player = player;

	EditorActionPlaceUnit(pos, type, player);
	EditorAddUndoAction(editorAction);
}

/**
**  Remove a unit
*/
//Wyrmgus start
//static void EditorActionRemoveUnit(CUnit &unit)
void EditorActionRemoveUnit(CUnit &unit, bool display)
//Wyrmgus end
{
	unit.Remove(nullptr);
	UnitLost(unit);
	unit.clear_orders();
	unit.Release();
	if (display) {
		UI.StatusLine.Set(_("Unit deleted"));
	}
	UpdateMinimap = true;
}

/**
**  Remove a unit
*/
static void EditorRemoveUnit(CUnit &unit)
{
	EditorAction editorAction;
	editorAction.Type = EditorActionTypeRemoveUnit;
	editorAction.tilePos = unit.tilePos;
	editorAction.UnitType = unit.Type;
	editorAction.Player = unit.Player;

	EditorActionRemoveUnit(unit);
	EditorAddUndoAction(editorAction);
}

/*----------------------------------------------------------------------------
--  Undo/Redo
----------------------------------------------------------------------------*/

static void EditorUndoAction()
{
	if (EditorUndoActions.empty()) {
		return;
	}

	EditorAction action = EditorUndoActions.back();
	EditorUndoActions.pop_back();

	switch (action.Type) {
		case EditorActionTypePlaceUnit: {
			CUnit *unit = UnitOnMapTile(action.tilePos, action.UnitType->get_domain(), UI.CurrentMapLayer->ID);

			EditorActionRemoveUnit(*unit);
			break;
		}

		case EditorActionTypeRemoveUnit:
			EditorActionPlaceUnit(action.tilePos, *action.UnitType, action.Player);
			break;
	}
	EditorRedoActions.push_back(action);
}

static void EditorRedoAction()
{
	if (EditorRedoActions.empty()) {
		return;
	}
	EditorAction action = EditorRedoActions.back();
	EditorRedoActions.pop_back();

	switch (action.Type) {
		case EditorActionTypePlaceUnit:
			EditorActionPlaceUnit(action.tilePos, *action.UnitType, action.Player);
			break;

		case EditorActionTypeRemoveUnit: {
			CUnit *unit = UnitOnMapTile(action.tilePos, action.UnitType->get_domain(), UI.CurrentMapLayer->ID);

			EditorActionRemoveUnit(*unit);
			break;
		}
	}
	EditorUndoActions.push_back(action);
}

static void EditorAddUndoAction(EditorAction action)
{
	EditorRedoActions.clear();
	EditorUndoActions.push_back(action);
}

/*----------------------------------------------------------------------------
--  Other
----------------------------------------------------------------------------*/

/**
**  Calculate the number of icons that can be displayed
**
**  @return  Number of icons that can be displayed.
*/
static int CalculateVisibleIcons(bool tiles = false)
{
#if 0
	int w;
	int h;

	if (tiles) {
		w = defines::get()->get_scaled_tile_width();//+2,
		h = defines::get()->get_scaled_tile_height();//+2
	} else {
		w = IconWidth;
		h = IconHeight;
	}

	const int i = (ButtonPanelHeight - h - 24) / (h + 2);
	const int count = i * (ButtonPanelWidth - w - 10) / (w + 8);
	return count;
#endif
	//Wyrmgus start
	if (tiles) {
		return 12;
	}
	//Wyrmgus end

	return UI.ButtonPanel.Buttons.size();
}

/**
**  Calculate the max height and the max width of icons,
**  and assign them to IconHeight and IconWidth
*/
static void CalculateMaxIconSize()
{
	IconWidth = 0;
	IconHeight = 0;
	for (unsigned int i = 0; i < CEditor::get()->UnitTypes.size(); ++i) {
		const unit_type *type = unit_type::get(CEditor::get()->UnitTypes[i]);
		assert_throw(type->get_icon() != nullptr);
		const icon *icon = type->get_icon();

		IconWidth = std::max(IconWidth, icon->get_graphics()->Width);
		IconHeight = std::max(IconHeight, icon->get_graphics()->Height);
	}
}

/**
**  Recalculate the shown units.
*/
//Wyrmgus start
//static void RecalculateShownUnits()
void RecalculateShownUnits()
//Wyrmgus end
{
	CEditor::get()->ShownUnitTypes.clear();

	for (size_t i = 0; i != CEditor::get()->UnitTypes.size(); ++i) {
		const unit_type *type = unit_type::get(CEditor::get()->UnitTypes[i]);
		CEditor::get()->ShownUnitTypes.push_back(type);
	}

	if (CEditor::get()->UnitIndex >= (int) CEditor::get()->ShownUnitTypes.size()) {
		CEditor::get()->UnitIndex = CEditor::get()->ShownUnitTypes.size() / VisibleUnitIcons * VisibleUnitIcons;
	}
	// Quick & dirty make them invalid
	CEditor::get()->CursorUnitIndex = -1;
	CEditor::get()->SelectedUnitIndex = -1;
}

/*----------------------------------------------------------------------------
--  Display
----------------------------------------------------------------------------*/

/**
**  Draw a table with the players
*/
static void DrawPlayers(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	std::array<char, 256> buf{};
	CLabel label(defines::get()->get_small_font());

	//Wyrmgus start
//	int x = UI.InfoPanel.X + 8;
	int x = UI.InfoPanel.X + (26 * scale_factor).to_int();
	//Wyrmgus end
	int y = UI.InfoPanel.Y + (4 * scale_factor).to_int() + IconHeight + (10 * scale_factor).to_int();

	const int rectangle_size = (20 * scale_factor).to_int();

	for (int i = 0; i < PlayerMax; ++i) {
		//Wyrmgus start
		if (i >= 15 && i < PlayerNumNeutral) {
			continue;
		}
//		if (i == PlayerMax / 2) {
		if ((i % 8) == 0) {
		//Wyrmgus end
			y += rectangle_size;
		}

		const player_type player_type = CPlayer::Players[i]->get_type();

		if (i == CEditor::get()->CursorPlayer && player_type != player_type::nobody) {
			Video.DrawRectangle(ColorWhite, x + i % 8 * rectangle_size, y, rectangle_size, rectangle_size, render_commands);
		}
		Video.DrawRectangle(
			i == CEditor::get()->CursorPlayer && player_type != player_type::nobody ? ColorWhite : ColorGray,
			x + i % 8 * rectangle_size, y, rectangle_size - 1, rectangle_size - 1, render_commands);
		if (player_type != player_type::nobody) {
			Video.FillRectangle(CVideo::MapRGB(CPlayer::Players[i]->get_minimap_color()), x + 1 + i % 8 * rectangle_size, y + 1, rectangle_size - 1 - 2, rectangle_size - 1 - 2, render_commands);
		}
		if (i == CEditor::get()->SelectedPlayer) {
			Video.DrawRectangle(ColorGreen, x + 1 + i % 8 * rectangle_size, y + 1, rectangle_size - 1 - 2, rectangle_size - 1 - 2, render_commands);
		}
		//Wyrmgus start
//		sprintf(buf, "%d", i);
		sprintf(buf.data(), "%d", (i == PlayerNumNeutral) ? 16 : i + 1);
		//Wyrmgus end
		label.DrawCentered(x + i % 8 * rectangle_size + (10 * scale_factor).to_int(), y + (7 * scale_factor).to_int(), buf.data(), render_commands);
	}

	//Wyrmgus start
//	x = UI.InfoPanel.X + 4;
	x = UI.InfoPanel.X + (22 * scale_factor).to_int();
	//Wyrmgus end
	y += ((18 * 1 + 4) * scale_factor).to_int();
	if (CEditor::get()->SelectedPlayer != -1) {
		//Wyrmgus start
//		snprintf(buf.data(), buf.size(), "Plyr %d %s ", CEditor::get()->SelectedPlayer,
//				 PlayerRaces.Name[Players[CEditor::get()->SelectedPlayer].Race].c_str());
		std::string civ_str = civilization::get_all()[CPlayer::Players[CEditor::get()->SelectedPlayer]->Race]->get_identifier().c_str();
		civ_str[0] = toupper(civ_str[0]);
		snprintf(buf.data(), buf.size(), "Player %d %s ", (CEditor::get()->SelectedPlayer == PlayerNumNeutral) ? 16 : CEditor::get()->SelectedPlayer + 1, civ_str.c_str());
		//Wyrmgus end
		// Players[SelectedPlayer].RaceName);

		switch (CPlayer::Players[CEditor::get()->SelectedPlayer]->get_type()) {
			case player_type::neutral:
				strcat_s(buf.data(), buf.size(), "Neutral");
				break;
			case player_type::nobody:
			default:
				strcat_s(buf.data(), buf.size(), "Nobody");
				break;
			case player_type::person:
				strcat_s(buf.data(), buf.size(), "Person");
				break;
			case player_type::computer:
			case player_type::rescue_passive:
			case player_type::rescue_active:
				strcat_s(buf.data(), buf.size(), "Computer");
				break;
		}
		label.SetFont(defines::get()->get_game_font());
		label.Draw(x, y, buf.data(), render_commands);
	}
}

/**
**  Draw unit icons.
*/
static void DrawUnitIcons(std::vector<std::function<void(renderer *)>> &render_commands)
{
	int i = CEditor::get()->UnitIndex;

	for (size_t j = 0; j < UI.ButtonPanel.Buttons.size(); ++j) {
		const int x = UI.ButtonPanel.Buttons[j].X;
		const int y = UI.ButtonPanel.Buttons[j].Y;
		if (i >= (int) CEditor::get()->ShownUnitTypes.size()) {
			//Wyrmgus start
//			return;
			break;
			//Wyrmgus end
		}

		const icon *icon = CEditor::get()->ShownUnitTypes[i]->get_icon();
		const PixelPos pos(x, y);

		unsigned int flag = 0;
		if (i == CEditor::get()->CursorUnitIndex) {
			flag = IconActive;
			if (MouseButtons & LeftButton) {
				// Overwrite IconActive.
				flag = IconClicked;
			}
		}
		
		//Wyrmgus start
		flag |= IconCommandButton;
		//Wyrmgus end

		icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[CEditor::get()->SelectedPlayer]->get_player_color(), render_commands);

		//Wyrmgus start
//		Video.DrawRectangleClip(ColorGray, x, y, icon.G->Width, icon.G->Height);
		//Wyrmgus end
		if (i == CEditor::get()->SelectedUnitIndex) {
			//Wyrmgus start
//			Video.DrawRectangleClip(ColorGreen, x + 1, y + 1,
//									icon.G->Width - 2, icon.G->Height - 2);
			Video.DrawRectangleClip(ColorGreen, x, y,
									icon->get_graphics()->Width, icon->get_graphics()->Height, render_commands);
			//Wyrmgus end
		}
		if (i == CEditor::get()->CursorUnitIndex) {
			//Wyrmgus start
//			Video.DrawRectangleClip(ColorWhite, x - 1, y - 1,
//									icon.G->Width + 2, icon.G->Height + 2);
			//Wyrmgus end
			CEditor::get()->PopUpX = x;
			CEditor::get()->PopUpY = y;
		}
		++i;
	}
	
	//Wyrmgus start
	i = CEditor::get()->UnitIndex;
	for (size_t j = 0; j < UI.ButtonPanel.Buttons.size(); ++j) {
		if (i >= (int) CEditor::get()->ShownUnitTypes.size()) {
			break;
		}
		
		if (i == CEditor::get()->CursorUnitIndex) {
			DrawPopup(*CurrentButtons[j], UI.ButtonPanel.Buttons[j].X, UI.ButtonPanel.Buttons[j].Y, render_commands);
		}
		
		++i;
	}
	//Wyrmgus end
}

/**
**  Draw a tile icon
**
**  @param tilenum  Tile number to display
**  @param x        X display position
**  @param y        Y display position
**  @param flags    State of the icon (::IconActive,::IconClicked,...)
*/
static void DrawTileIcon(const terrain_type *terrain, unsigned x, unsigned y, unsigned flags, std::vector<std::function<void(renderer *)>> &render_commands)
{
	Video.DrawVLine(ColorGray, x + defines::get()->get_scaled_tile_width() + 4 - 1, y + 5 - 1, defines::get()->get_scaled_tile_height() - 1 - 1, render_commands); // _|
	Video.DrawVLine(ColorGray, x + defines::get()->get_scaled_tile_width() + 5 - 1, y + 5 - 1, defines::get()->get_scaled_tile_height() - 1 - 1, render_commands);
	Video.DrawHLine(ColorGray, x + 5 - 1, y + defines::get()->get_scaled_tile_height() + 4 - 1, defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);
	Video.DrawHLine(ColorGray, x + 5 - 1, y + defines::get()->get_scaled_tile_height() + 5 - 1, defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);

	uint32_t color = (flags & IconClicked) ? ColorGray : ColorWhite;
	Video.DrawHLine(color, x + 5 - 1, y + 3 - 1, defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);
	Video.DrawHLine(color, x + 5 - 1, y + 4 - 1, defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);
	Video.DrawVLine(color, x + 3 - 1, y + 3 - 1, defines::get()->get_scaled_tile_height() + 3 - 1, render_commands);
	Video.DrawVLine(color, x + 4 - 1, y + 3 - 1, defines::get()->get_scaled_tile_height() + 3 - 1, render_commands);
	
	color = (flags & IconActive) ? ColorGray : ColorBlack;

	Video.DrawRectangleClip(color, x, y, defines::get()->get_scaled_tile_width() + 7, defines::get()->get_scaled_tile_height() + 7, render_commands);
	Video.DrawRectangleClip(ColorBlack, x + 1, y + 1, defines::get()->get_scaled_tile_width() + 5, defines::get()->get_scaled_tile_height() + 5, render_commands);

	if (flags & IconClicked) {
		++x;
		++y;
	}

	x += 3;
	y += 3;

	const color_modification color_modification(terrain->get_hue_rotation(), terrain->get_colorization(), color_set(), nullptr, nullptr);
	terrain->get_graphics()->render_frame(terrain->get_solid_tiles().front(), QPoint(x, y), color_modification, render_commands);

	if (flags & IconSelected) {
		Video.DrawRectangleClip(ColorGreen, x, y, defines::get()->get_scaled_tile_width(), defines::get()->get_scaled_tile_height(), render_commands);
	}
}

/**
**  Draw tile icons.
**
**  @todo for the start the solid tiles are hardcoded
**        If we have more solid tiles, than they fit into the panel, we need
**        some new ideas.
*/
static void DrawTileIcons(std::vector<std::function<void(renderer *)>> &render_commands)
{
	CLabel label(defines::get()->get_game_font());
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	int x = UI.InfoPanel.X + (46 * scale_factor).to_int();
	int y = UI.InfoPanel.Y + (4 * scale_factor).to_int() + IconHeight + (11 * scale_factor).to_int();

	if (CursorOn == cursor_on::button && 300 <= ButtonUnderCursor && ButtonUnderCursor < 306) {
		//Wyrmgus start
//		Video.DrawRectangle(ColorGray, x - 42, y - 3 + (ButtonUnderCursor - 300) * 20, 100, 20);
		if (ButtonUnderCursor <= 303) {
			Video.DrawRectangle(ColorGray, x - (42 * scale_factor).to_int(), y - (3 * scale_factor).to_int() + (ButtonUnderCursor - 300) * (20 * scale_factor).to_int(), (100 * scale_factor).to_int(), (20 * scale_factor).to_int(), render_commands);
		} else {
			Video.DrawRectangle(ColorGray, x + ((-42 + 100) * scale_factor).to_int(), y - (3 * scale_factor).to_int() + (ButtonUnderCursor - 304) * (20 * scale_factor).to_int(), (100 * scale_factor).to_int(), (20 * scale_factor).to_int(), render_commands);
		}
		//Wyrmgus end
	}

	if (TileCursorSize == 1) {
		label.DrawReverseCentered(x, y, "1x1", render_commands);
	} else {
		label.DrawCentered(x, y, "1x1", render_commands);
	}
	y += (20 * scale_factor).to_int();
	if (TileCursorSize == 2) {
		label.DrawReverseCentered(x, y, "2x2", render_commands);
	} else {
		label.DrawCentered(x, y, "2x2", render_commands);
	}
	y += (20 * scale_factor).to_int();
	if (TileCursorSize == 3) {
		label.DrawReverseCentered(x, y, "3x3", render_commands);
	} else {
		label.DrawCentered(x, y, "3x3", render_commands);
	}
	y += (20 * scale_factor).to_int();
	if (TileCursorSize == 4) {
		label.DrawReverseCentered(x, y, "4x4", render_commands);
	} else {
		label.DrawCentered(x, y, "4x4", render_commands);
	}
	//Wyrmgus start
//	y += 20 * scale_factor;
	x += (100 * scale_factor).to_int();
	y -= (20 * 3 * scale_factor).to_int();
	//Wyrmgus end
	//Wyrmgus start
	/*
	if (TileToolRandom) {
		label.DrawReverseCentered(x, y, "Random");
	} else {
		label.DrawCentered(x, y, "Random");
	}
	*/
	if (TileCursorSize == 5) {
		label.DrawReverseCentered(x, y, "5x5", render_commands);
	} else {
		label.DrawCentered(x, y, "5x5", render_commands);
	}
	//Wyrmgus end
	y += (20 * scale_factor).to_int();
	//Wyrmgus start
	/*
	if (TileToolDecoration) {
		label.DrawReverseCentered(x, y, "Filler");
	} else {
		label.DrawCentered(x, y, "Filler");
	}
	*/
	if (TileCursorSize == 10) {
		label.DrawReverseCentered(x, y, "10x10", render_commands);
	} else {
		label.DrawCentered(x, y, "10x10", render_commands);
	}
	//Wyrmgus end
	y += (20 * scale_factor).to_int();

	int i = CEditor::get()->TileIndex;
	assert_throw(CEditor::get()->TileIndex != -1);
	y = UI.ButtonPanel.Y + (24 * scale_factor).to_int();
	while (y < UI.ButtonPanel.Y + ButtonPanelHeight - defines::get()->get_scaled_tile_height()) {
		if (i >= (int) CEditor::get()->ShownTileTypes.size()) {
			break;
		}

		x = UI.ButtonPanel.X + (16 * scale_factor).to_int();

		while (x < UI.ButtonPanel.X + ButtonPanelWidth - defines::get()->get_scaled_tile_width()) {
			if (i >= (int) CEditor::get()->ShownTileTypes.size()) {
				break;
			}

			const terrain_type *terrain = CEditor::get()->ShownTileTypes[i];

			if (terrain->get_graphics() != nullptr && !terrain->get_solid_tiles().empty()) {
				const color_modification color_modification(terrain->get_hue_rotation(), terrain->get_colorization(), color_set(), nullptr, nullptr);
				terrain->get_graphics()->render_frame(terrain->get_solid_tiles().front(), QPoint(x, y), color_modification, render_commands);
			}

			Video.DrawRectangleClip(ColorGray, x, y, defines::get()->get_scaled_tile_width(), defines::get()->get_scaled_tile_height(), render_commands);

			if (i == CEditor::get()->SelectedTileIndex) {
				Video.DrawRectangleClip(ColorGreen, x + 1, y + 1,
					defines::get()->get_scaled_tile_width() - 2, defines::get()->get_scaled_tile_height() - 2, render_commands);
			}
			if (i == CEditor::get()->CursorTileIndex) {
				Video.DrawRectangleClip(ColorWhite, x - 1, y - 1,
					defines::get()->get_scaled_tile_width() + 2, defines::get()->get_scaled_tile_height() + 2, render_commands);
				CEditor::get()->PopUpX = x;
				CEditor::get()->PopUpY = y;
			}

			x += defines::get()->get_scaled_tile_width() + (30 * scale_factor).to_int();
			++i;
		}

		y += defines::get()->get_scaled_tile_height() + (18 * scale_factor).to_int();
	}
}

static void DrawEditorPanel_SelectIcon(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	//Wyrmgus start
//	const PixelPos pos(UI.InfoPanel.X + 4, UI.InfoPanel.Y + 4);
	const PixelPos pos(UI.InfoPanel.X + (11 * scale_factor).to_int(), UI.InfoPanel.Y + (7 * scale_factor).to_int());
	//Wyrmgus end
	icon *icon = CEditor::get()->Select.Icon;
	assert_throw(icon != nullptr);
	unsigned int flag = 0;
	if (ButtonUnderCursor == SelectButton) {
		flag = IconActive;
		if (MouseButtons & LeftButton) {
			// Overwrite IconActive.
			flag = IconClicked;
		}
	}
	
	//Wyrmgus start
	flag |= IconCommandButton;
	//Wyrmgus end
		
	// FIXME: wrong button style
	icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[CEditor::get()->SelectedPlayer]->get_player_color(), render_commands);
}

static void DrawEditorPanel_UnitsIcon(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	const PixelPos pos(UI.InfoPanel.X + (11 * scale_factor).to_int() + get_unit_icon_x(), UI.InfoPanel.Y + (7 * scale_factor).to_int() + get_unit_icon_y());
	icon *icon = CEditor::get()->Units.Icon;
	assert_throw(icon != nullptr);
	unsigned int flag = 0;
	if (ButtonUnderCursor == UnitButton) {
		flag = IconActive;
		if (MouseButtons & LeftButton) {
			// Overwrite IconActive.
			flag = IconClicked;
		}
	}
	
	//Wyrmgus start
	flag |= IconCommandButton;
	//Wyrmgus end
		
	// FIXME: wrong button style
	icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[CEditor::get()->SelectedPlayer]->get_player_color(), render_commands);
}

static void DrawEditorPanel_StartIcon(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	int x = UI.InfoPanel.X + (11 * scale_factor).to_int();
	int y = UI.InfoPanel.Y + (5 * scale_factor).to_int();

	if (CEditor::get()->StartUnit) {
		const icon *icon = CEditor::get()->StartUnit->get_icon();
		assert_throw(icon != nullptr);
		const PixelPos pos(x + get_start_icon_x(), y + get_start_icon_y());
		unsigned int flag = 0;
		if (ButtonUnderCursor == StartButton) {
			flag = IconActive;
			if (MouseButtons & LeftButton) {
				// Overwrite IconActive.
				flag = IconClicked;
			}
		}

		//Wyrmgus start
		flag |= IconCommandButton;
		//Wyrmgus end
		
		icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[CEditor::get()->SelectedPlayer]->get_player_color(), render_commands);
	} else {
		//  No unit specified : draw a cross.
		//  Todo : FIXME Should we just warn user to define Start unit ?
		PushClipping();
		x += get_start_icon_x() + (1 * scale_factor).to_int();
		y += get_start_icon_y() + (1 * scale_factor).to_int();
		if (ButtonUnderCursor == StartButton) {
			Video.DrawRectangleClip(ColorGray, x - (1 * scale_factor).to_int(), y - (1 * scale_factor).to_int(), IconHeight, IconHeight, render_commands);
		}
		Video.FillRectangleClip(ColorBlack, x, y, IconHeight - (2 * scale_factor).to_int(), IconHeight - (2 * scale_factor).to_int(), render_commands);

		const PixelPos lt(x, y);
		const PixelPos lb(x, y + IconHeight - (3 * scale_factor).to_int());
		const PixelPos rt(x + IconHeight - (3 * scale_factor).to_int(), y);
		const PixelPos rb(x + IconHeight - (2 * scale_factor).to_int(), y + IconHeight - (2 * scale_factor).to_int());
		const uint32_t color = CVideo::MapRGB(CPlayer::Players[CEditor::get()->SelectedPlayer]->get_minimap_color());

		Video.DrawLineClip(color, lt, rb, render_commands);
		Video.DrawLineClip(color, rt, lb, render_commands);
		PopClipping();
	}
}

/**
**  Draw the editor panels.
*/
static void DrawEditorPanel(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	DrawEditorPanel_SelectIcon(render_commands);
	DrawEditorPanel_UnitsIcon(render_commands);

	if (CEditor::get()->TerrainEditable) {
		const int x = UI.InfoPanel.X + get_tile_icon_x() + (11 * scale_factor).to_int();
		const int y = UI.InfoPanel.Y + get_tile_icon_y() + (4 * scale_factor).to_int();

		DrawTileIcon(CEditor::get()->ShownTileTypes[0], x, y,
					 (ButtonUnderCursor == TileButton ? IconActive : 0) |
					 (CEditor::get()->State == EditorEditTile ? IconSelected : 0), render_commands);
	}

	DrawEditorPanel_StartIcon(render_commands);

	switch (CEditor::get()->State) {
		case EditorSelecting:
			break;
		case EditorEditTile:
			DrawTileIcons(render_commands);
			break;
		case EditorSetStartLocation:
			DrawPlayers(render_commands);
			break;
		case EditorEditUnit:
			DrawPlayers(render_commands);
			DrawUnitIcons(render_commands);
			break;
	}
}

/**
**  Draw special cursor on map.
**
**  @todo support for bigger cursors (2x2, 3x3 ...)
*/
static void DrawMapCursor(std::vector<std::function<void(renderer *)>> &render_commands)
{
	//  Affect CursorBuilding if necessary.
	//  (Menu reset CursorBuilding)
	if (!CursorBuilding) {
		switch (CEditor::get()->State) {
			case EditorSelecting:
			case EditorEditTile:
				break;
			case EditorEditUnit:
				if (CEditor::get()->SelectedUnitIndex != -1) {
					CursorBuilding = CEditor::get()->ShownUnitTypes[CEditor::get()->SelectedUnitIndex];
				}
				break;
			case EditorSetStartLocation:
				if (CEditor::get()->StartUnit) {
					CursorBuilding = CEditor::get()->StartUnit;
				}
				break;
		}
	}

	// Draw map cursor
	if (UI.MouseViewport && !CursorBuilding) {
		const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
		const PixelPos screenPos = UI.MouseViewport->TilePosToScreen_TopLeft(tilePos);

		if (CEditor::get()->State == EditorEditTile && CEditor::get()->SelectedTileIndex != -1) {
			const terrain_type *terrain = CEditor::get()->ShownTileTypes[CEditor::get()->SelectedTileIndex];

			PushClipping();
			UI.MouseViewport->SetClipping();

			PixelPos screenPosIt;
			for (int j = 0; j < TileCursorSize; ++j) {
				screenPosIt.y = screenPos.y + j * defines::get()->get_scaled_tile_height();
				if (screenPosIt.y >= UI.MouseViewport->get_bottom_right_pos().y()) {
					break;
				}
				for (int i = 0; i < TileCursorSize; ++i) {
					screenPosIt.x = screenPos.x + i * defines::get()->get_scaled_tile_width();
					if (screenPosIt.x >= UI.MouseViewport->get_bottom_right_pos().x()) {
						break;
					}
					//Wyrmgus start
//					Map.TileGraphic->DrawFrameClip(tile, screenPosIt.x, screenPosIt.y);

					if (terrain->get_graphics() && !terrain->get_solid_tiles().empty()) {
						terrain->get_graphics()->DrawFrameClip(terrain->get_solid_tiles().front(), screenPosIt.x, screenPosIt.y, render_commands);
					}
					//Wyrmgus end
				}
			}
			Video.DrawRectangleClip(ColorWhite, screenPos.x, screenPos.y, defines::get()->get_scaled_tile_width() * TileCursorSize, defines::get()->get_scaled_tile_height() * TileCursorSize, render_commands);
			PopClipping();
		} else {
			// If there is an unit under the cursor, it's selection thing
			//  is drawn somewhere else (Check DrawUnitSelection.)
			if (UnitUnderCursor != nullptr) {
				PushClipping();
				UI.MouseViewport->SetClipping();
				Video.DrawRectangleClip(ColorWhite, screenPos.x, screenPos.y, defines::get()->get_scaled_tile_width(), defines::get()->get_scaled_tile_height(), render_commands);
				PopClipping();
			}
		}
	}
}

static void DrawCross(const PixelPos &topleft_pos, const QSize &size, uint32_t color, std::vector<std::function<void(renderer *)>> &render_commands)
{
	const PixelPos lt = topleft_pos;
	const PixelPos lb(topleft_pos.x, topleft_pos.y + size.height());
	const PixelPos rt(topleft_pos.x + size.width(), topleft_pos.y);
	const PixelPos rb = topleft_pos + size;

	Video.DrawLineClip(color, lt, rb, render_commands);
	Video.DrawLineClip(color, lb, rt, render_commands);
}

/**
**  Draw the start locations of all active players on the map
*/
static void DrawStartLocations(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const unit_type *type = CEditor::get()->StartUnit;
	for (const CViewport *vp = UI.Viewports; vp < UI.Viewports + UI.NumViewports; ++vp) {
		PushClipping();
		vp->SetClipping();

		for (int i = 0; i < PlayerMax; i++) {
			const player_type player_type = CPlayer::Players[i]->get_type();

			if (player_type != player_type::nobody && player_type != player_type::neutral && CPlayer::Players[i]->StartMapLayer == UI.CurrentMapLayer->ID) {
				const PixelPos startScreenPos = vp->TilePosToScreen_TopLeft(CPlayer::Players[i]->StartPos);

				if (type) {
					DrawUnitType(*type, type->Sprite, i, 0, startScreenPos, nullptr, render_commands);
				} else { // Draw a cross
					DrawCross(startScreenPos, defines::get()->get_scaled_tile_size(), CVideo::MapRGB(CPlayer::Players[i]->get_minimap_color()), render_commands);
				}
			}
		}
		PopClipping();
	}
}

/**
**  Draw editor info.
**
**  If cursor is on map or minimap show information about the current tile.
*/
static void DrawEditorInfo(std::vector<std::function<void(renderer *)>> &render_commands)
{
	Vec2i pos(0, 0);

	if (UI.MouseViewport) {
		pos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	std::array<char, 256> buf{};
	snprintf(buf.data(), buf.size(), _("Editor (%d %d)"), pos.x, pos.y);
	CLabel(defines::get()->get_game_font()).Draw(UI.StatusLine.TextX + (2 * scale_factor).to_int(), UI.StatusLine.TextY - (12 * scale_factor).to_int(), buf.data(), render_commands);
	const tile &mf = *UI.CurrentMapLayer->Field(pos);
	//
	// Flags info
	//
	const tile_flag flag = mf.get_flags();
	sprintf(buf.data(), "%02X|%04X|%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
			mf.get_value(), enumeration::to_underlying(flag),
			mf.has_flag(tile_flag::impassable) ? 'u' : '-',
			mf.has_flag(tile_flag::air_impassable) ? 'A' : '-',
			mf.has_flag(tile_flag::no_building) ? 'n' : '-',
			mf.has_flag(tile_flag::wall) ? 'w' : '-',
			mf.has_flag(tile_flag::rock) ? 'r' : '-',
			mf.has_flag(tile_flag::tree) ? 'f' : '-',
			mf.has_flag(tile_flag::land_allowed) ? 'L' : '-',
			mf.has_flag(tile_flag::coast_allowed) ? 'C' : '-',
			mf.has_flag(tile_flag::water_allowed) ? 'W' : '-',
			mf.has_flag(tile_flag::ford) ? 'F' : '-',
			mf.has_flag(tile_flag::land_unit) ? 'l' : '-',
			mf.has_flag(tile_flag::air_unit) ? 'a' : '-',
			mf.has_flag(tile_flag::sea_unit) ? 's' : '-',
			mf.has_flag(tile_flag::building) ? 'b' : '-',
			mf.has_flag(tile_flag::air_building) ? 'I' : '-',
			mf.has_flag(tile_flag::item) ? 'i' : '-',
			mf.has_flag(tile_flag::stumps) ? 't' : '-',
			mf.has_flag(tile_flag::space) ? 'S' : '-');

	CLabel(defines::get()->get_game_font()).Draw(UI.StatusLine.TextX + (118 * scale_factor).to_int(), UI.StatusLine.TextY - (12 * scale_factor).to_int(), buf.data(), render_commands);

	//Wyrmgus start
//	const int index = tileset.findTileIndexByTile(mf.getGraphicTile());
//	assert_throw(index != -1);
	//Wyrmgus end
	//Wyrmgus start
	/*
	const int baseTerrainIdx = tileset.tiles[index].tileinfo.BaseTerrain;
	const char *baseTerrainStr = tileset.getTerrainName(baseTerrainIdx).c_str();
	const int mixTerrainIdx = tileset.tiles[index].tileinfo.MixTerrain;
	const char *mixTerrainStr = mixTerrainIdx ? tileset.getTerrainName(mixTerrainIdx).c_str() : "";
	snprintf(buf.data(), buf.size(), "%s %s", baseTerrainStr, mixTerrainStr);
	*/
	std::string terrain_name;
	if (mf.get_terrain() != nullptr) {
		if (mf.get_overlay_terrain() != nullptr) {
			terrain_name = mf.get_overlay_terrain()->get_name() + " (" + mf.get_terrain()->get_name() + ")";
		} else {
			terrain_name = mf.get_terrain()->get_name();
		}
	}
	snprintf(buf.data(), buf.size(), "%s", terrain_name.c_str());
	//Wyrmgus end

	CLabel(defines::get()->get_game_font()).Draw(UI.StatusLine.TextX + (298 * scale_factor).to_int(), UI.StatusLine.TextY - (12 * scale_factor).to_int(), buf.data(), render_commands);
}

/**
**  Show info about unit.
**
**  @param unit  Unit pointer.
*/
static void ShowUnitInfo(const CUnit &unit)
{
	std::string str = "#" + std::to_string(UnitNumber(unit)) + " '" + unit.get_type_name() + "' Player: #" + std::to_string((unit.Player->get_index() == PlayerNumNeutral) ? 16 : unit.Player->get_index() + 1) + " " + (unit.Active ? "active" : "passive");

	if (unit.Type->get_given_resource() != nullptr) {
		str += " Amount " + std::to_string(unit.ResourcesHeld);
	}
	UI.StatusLine.Set(str);
}

/**
**  Update editor display.
*/
void EditorUpdateDisplay()
{
	std::vector<std::function<void(renderer *)>> render_commands;

	DrawMapArea(render_commands); // draw the map area

	DrawStartLocations(render_commands);

	//Wyrmgus start
	/*
	// Fillers
	for (size_t i = 0; i != UI.Fillers.size(); ++i) {
		UI.Fillers[i].G->DrawClip(UI.Fillers[i].X, UI.Fillers[i].Y);
	}
	*/
	//Wyrmgus end

	if (CursorOn == cursor_on::map && Gui->getTop() == editorContainer.get() && !game::get()->is_paused()) {
		DrawMapCursor(render_commands); // cursor on map
	}

	//Wyrmgus start
	if (CursorBuilding && CursorOn == cursor_on::map) {
		DrawBuildingCursor(render_commands);
	}
	//Wyrmgus end
	
	//Wyrmgus start
	// Fillers
	for (size_t i = 0; i != UI.Fillers.size(); ++i) {
		UI.Fillers[i].G->DrawClip(UI.Fillers[i].X, UI.Fillers[i].Y, render_commands);
	}
	//Wyrmgus end
	
	// Menu button
	const int flag_active = ButtonAreaUnderCursor == ButtonAreaMenu
							&& ButtonUnderCursor == ButtonUnderMenu ? MI_FLAGS_ACTIVE : 0;

	// Minimap
	if (UI.SelectedViewport) {
		UI.get_minimap()->Draw(render_commands);
		UI.get_minimap()->DrawViewportArea(*UI.SelectedViewport, render_commands);
	}
	// Info panel
	if (UI.InfoPanel.G) {
		UI.InfoPanel.G->DrawClip(UI.InfoPanel.X, UI.InfoPanel.Y, render_commands);
	}
	// Button panel
	if (UI.ButtonPanel.G) {
		UI.ButtonPanel.G->DrawClip(UI.ButtonPanel.X, UI.ButtonPanel.Y, render_commands);
	}
	DrawEditorPanel(render_commands);

	if (CursorOn == cursor_on::map) {
		DrawEditorInfo(render_commands);
	}

	// Status line
	UI.StatusLine.Draw(render_commands);

	DrawGuichanWidgets(render_commands);

	// DrawPopup();

	DrawCursor(render_commands);

	render_context::get()->set_commands(std::move(render_commands));
}

/*----------------------------------------------------------------------------
--  Input / Keyboard / Mouse
----------------------------------------------------------------------------*/

/**
**  Callback for input.
*/
static void EditorCallbackButtonUp(unsigned button, const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(key_modifiers)

	if (cursor::get_current_cursor() == UI.get_cursor(cursor_type::scroll)) {
		// Move map.
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false); // Reset
		return;
	}

	if ((1 << button) == LeftButton) {
		UnitPlacedThisPress = false;
	}
}

/**
**  Called if mouse button pressed down.
**
**  @param button  Mouse button number (0 left, 1 middle, 2 right)
*/
static void EditorCallbackButtonDown(unsigned button, const Qt::KeyboardModifiers key_modifiers)
{
	if (game::get()->is_paused()) {
		return;
	}

	if ((button >> MouseHoldShift) != 0) {
		// Ignore repeated events when holding down a button
		return;
	}

	// Click on minimap
	if (CursorOn == cursor_on::minimap) {
		if (MouseButtons & LeftButton) { // enter move mini-mode
			const Vec2i tilePos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);
			UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(tilePos));
		}
		return;
	}

	// Click on mode area
	if (CursorOn == cursor_on::button) {
		CursorBuilding = nullptr;
		switch (ButtonUnderCursor) {
			case SelectButton :
				CEditor::get()->State = EditorSelecting;
				editorUnitSlider->setVisible(false);
				editorSlider->setVisible(false);
				return;
			case UnitButton:
				CEditor::get()->State = EditorEditUnit;
				if (VisibleUnitIcons < (int) CEditor::get()->ShownUnitTypes.size()) {
					editorUnitSlider->setVisible(true);
				}
				editorSlider->setVisible(false);
				return;
			case TileButton :
				CEditor::get()->State = EditorEditTile;
				editorUnitSlider->setVisible(false);
				if (VisibleTileIcons < (int) CEditor::get()->ShownTileTypes.size()) {
					editorSlider->setVisible(true);
				}
				return;
			case StartButton:
				CEditor::get()->State = EditorSetStartLocation;
				editorUnitSlider->setVisible(false);
				editorSlider->setVisible(false);
				return;
			default:
				break;
		}
	}
	// Click on tile area
	if (CEditor::get()->State == EditorEditTile) {
		if (CursorOn == cursor_on::button && ButtonUnderCursor >= 100) {
			switch (ButtonUnderCursor) {
				case 300: TileCursorSize = 1; return;
				case 301: TileCursorSize = 2; return;
				case 302: TileCursorSize = 3; return;
				case 303: TileCursorSize = 4; return;
				//Wyrmgus start
//				case 304: TileToolRandom ^= 1; return;
//				case 305: TileToolDecoration ^= 1; return;
				case 304: TileCursorSize = 5; return;
				case 305: TileCursorSize = 10; return;
				//Wyrmgus end
			}
		}
		if (CEditor::get()->CursorTileIndex != -1) {
			CEditor::get()->SelectedTileIndex = CEditor::get()->CursorTileIndex;
			return;
		}
	}

	// Click on player area
	if (CEditor::get()->State == EditorEditUnit || CEditor::get()->State == EditorSetStartLocation) {
		// Cursor on player icons
		if (CEditor::get()->CursorPlayer != -1) {
			const player_type player_type = CPlayer::Players[CEditor::get()->CursorPlayer]->get_type();

			if (player_type != player_type::nobody) {
				CEditor::get()->SelectedPlayer = CEditor::get()->CursorPlayer;
				CPlayer::SetThisPlayer(CPlayer::Players[CEditor::get()->SelectedPlayer].get());
			}
			return;
		}
	}

	// Click on unit area
	if (CEditor::get()->State == EditorEditUnit) {
		// Cursor on unit icons
		if (CEditor::get()->CursorUnitIndex != -1) {
			if (CEditor::get()->CursorUnitIndex != (int) CEditor::get()->ShownUnitTypes.size()) {
				if (MouseButtons & LeftButton) {
					CEditor::get()->SelectedUnitIndex = CEditor::get()->CursorUnitIndex;
					CursorBuilding = CEditor::get()->ShownUnitTypes[CEditor::get()->CursorUnitIndex];
					return;
				}
			}
		}
	}

	// Right click on a resource
	if (CEditor::get()->State == EditorSelecting) {
		if ((MouseButtons & RightButton) && UnitUnderCursor != nullptr) {
			CclCommand("if (EditUnitProperties ~= nil) then EditUnitProperties() end;");
			return;
		}
	}

	// Click on map area
	if (CursorOn == cursor_on::map) {
		if (MouseButtons & RightButton) {
			if (CEditor::get()->State == EditorEditUnit && CEditor::get()->SelectedUnitIndex != -1) {
				CEditor::get()->SelectedUnitIndex = -1;
				CursorBuilding = nullptr;
				return;
			} else if (CEditor::get()->State == EditorEditTile && CEditor::get()->SelectedTileIndex != -1) {
				CEditor::get()->SelectedTileIndex = -1;
				CursorBuilding = nullptr;
				return;
			}
		}

		CViewport *vp = GetViewport(CursorScreenPos);
		assert_throw(vp != nullptr);
		if ((MouseButtons & LeftButton) && UI.SelectedViewport != vp) {
			// viewport changed
			UI.SelectedViewport = vp;
		}
		if (MouseButtons & LeftButton) {
			const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);

			if (CEditor::get()->State == EditorEditTile &&
				CEditor::get()->SelectedTileIndex != -1) {
				EditTiles(tilePos, CEditor::get()->ShownTileTypes[CEditor::get()->SelectedTileIndex], TileCursorSize, key_modifiers);
			} else if (CEditor::get()->State == EditorEditUnit) {
				if (!UnitPlacedThisPress && CursorBuilding) {
					if (CanBuildUnitType(nullptr, *CursorBuilding, tilePos, 1, true, UI.CurrentMapLayer->ID)) {
						PlayGameSound(game_sound_set::get()->get_placement_success_sound(), MaxSampleVolume);
						EditorPlaceUnit(tilePos, *CursorBuilding, CPlayer::Players[CEditor::get()->SelectedPlayer].get());
						UnitPlacedThisPress = true;
						UI.StatusLine.Clear();
					} else {
						UI.StatusLine.Set(_("Unit cannot be placed here."));
						PlayGameSound(game_sound_set::get()->get_placement_error_sound(), MaxSampleVolume);
					}
				}
			} else if (CEditor::get()->State == EditorSetStartLocation) {
				CPlayer::Players[CEditor::get()->SelectedPlayer]->StartPos = tilePos;
				CPlayer::Players[CEditor::get()->SelectedPlayer]->StartMapLayer = UI.CurrentMapLayer->ID;
			}
		} else if (MouseButtons & MiddleButton) {
			// enter move map mode
			CursorStartScreenPos = CursorScreenPos;
			cursor::set_current_cursor(UI.get_cursor(cursor_type::scroll), false);
			//Wyrmgus start
			UnitUnderCursor = nullptr;
			//Wyrmgus end
		}
	}
}

/**
**  Handle key down.
**
**  @param key      Key scancode.
**  @param keychar  Character code.
*/
static void EditorCallbackKeyDown(unsigned key, unsigned keychar, const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(keychar)

	switch (key) {
		case SDLK_SYSREQ:
		case SDLK_PRINTSCREEN:
		case SDLK_F11:
			Screenshot();
			return;
		default:
			break;
	}

	if (HandleKeyModifiersDown(key)) {
		return;
	}

	// FIXME: don't handle unicode well. Should work on all latin keyboard.
	const char *ptr = strchr(UiGroupKeys.c_str(), key);

	if (ptr) {
		key = '0' + ptr - UiGroupKeys.c_str();
		if (key > '9') {
			key = SDLK_BACKQUOTE;
		}
	}
	switch (key) {
		// FIXME: move to lua
		case 'm': // CTRL+M Mirror edit
			if (key_modifiers & Qt::ControlModifier)  {
				++MirrorEdit;
				if (MirrorEdit == 3) {
					MirrorEdit = 0;
				}
				switch (MirrorEdit) {
					case 1:
						UI.StatusLine.Set(_("Mirror editing enabled: 2-side"));
						break;
					case 2:
						UI.StatusLine.Set(_("Mirror editing enabled: 4-side"));
						break;
					default:
						UI.StatusLine.Set(_("Mirror editing disabled"));
						break;
				}
			//Wyrmgus start
			} else {
				HandleCommandKey(key, key_modifiers);
				return;
			//Wyrmgus end
			}
			break;
		case 'x': // ALT+X, CTRL+X: Exit editor
			if (!(key_modifiers & (Qt::AltModifier | Qt::ControlModifier))) {
				break;
			}
			Exit(0);

		case 'z':
			if (key_modifiers & Qt::ControlModifier) {
				EditorUndoAction();
			}
			break;
		case 'y':
			if (key_modifiers & Qt::ControlModifier) {
				EditorRedoAction();
			}
			break;

		case SDLK_BACKSPACE:
		case SDLK_DELETE: // Delete
			if (UnitUnderCursor != nullptr) {
				EditorRemoveUnit(*UnitUnderCursor);
			}
			break;

		case SDLK_UP: // Keyboard scrolling
		case SDLK_KP_8:
			KeyScrollState |= ScrollUp;
			break;
		case SDLK_DOWN:
		case SDLK_KP_2:
			KeyScrollState |= ScrollDown;
			break;
		case SDLK_LEFT:
		case SDLK_KP_4:
			KeyScrollState |= ScrollLeft;
			break;
		case SDLK_RIGHT:
		case SDLK_KP_6:
			KeyScrollState |= ScrollRight;
			break;
		case '0':
			if (UnitUnderCursor != nullptr) {
				UnitUnderCursor->ChangeOwner(*CPlayer::get_neutral_player());
				UI.StatusLine.Set(_("Unit owner modified"));
			}
			break;
		case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9': {
			CPlayer *player = CPlayer::Players[key - '1'].get();
			const player_type player_type = player->get_type();

			if (UnitUnderCursor != nullptr && player_type != player_type::nobody) {
				UnitUnderCursor->ChangeOwner(*player);
				UI.StatusLine.Set(_("Unit owner modified"));
				UpdateMinimap = true;
			}
			break;
		}
		default:
			HandleCommandKey(key, key_modifiers);
			return;
	}
}

/**
**  Handle key up.
**
**  @param key      Key scancode.
**  @param keychar  Character code.
*/
static void EditorCallbackKeyUp(unsigned key, unsigned keychar, const Qt::KeyboardModifiers key_modifiers)
{
	Q_UNUSED(keychar)
	Q_UNUSED(key_modifiers)

	if (HandleKeyModifiersUp(key)) {
		return;
	}

	switch (key) {
		case SDLK_UP: // Keyboard scrolling
		case SDLK_KP_8:
			KeyScrollState &= ~ScrollUp;
			break;
		case SDLK_DOWN:
		case SDLK_KP_2:
			KeyScrollState &= ~ScrollDown;
			break;
		case SDLK_LEFT:
		case SDLK_KP_4:
			KeyScrollState &= ~ScrollLeft;
			break;
		case SDLK_RIGHT:
		case SDLK_KP_6:
			KeyScrollState &= ~ScrollRight;
			break;
		default:
			break;
	}
}

/**
**  Callback for input.
*/
static void EditorCallbackKeyRepeated(unsigned key, unsigned, const Qt::KeyboardModifiers key_modifiers)
{
	switch (key) {
		case 'z':
			if (key_modifiers & Qt::ControlModifier) {
				EditorUndoAction();
			}
			break;
		case 'y':
			if (key_modifiers & Qt::ControlModifier) {
				EditorRedoAction();
			}
			break;
	}
}

static bool EditorCallbackMouse_EditUnitArea(const PixelPos &screenPos)
{
	assert_throw(CEditor::get()->State == EditorEditUnit || CEditor::get()->State == EditorSetStartLocation);
	
	//Wyrmgus start
	LastDrawnButtonPopup = nullptr;
	//Wyrmgus end

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	// Scrollbar
	if (UI.ButtonPanel.X + (4 * scale_factor).to_int() < CursorScreenPos.x
		&& CursorScreenPos.x < UI.ButtonPanel.X + ((176 - 4) * scale_factor).to_int()
		&& UI.ButtonPanel.Y + (4 * scale_factor).to_int() < CursorScreenPos.y
		//Wyrmgus start
//		&& CursorScreenPos.y < UI.ButtonPanel.Y + 24) {
		&& CursorScreenPos.y < UI.ButtonPanel.Y) {
		//Wyrmgus end
		return true;
	}
	//Wyrmgus start
//	int bx = UI.InfoPanel.X + 8;
	int bx = UI.InfoPanel.X + (26 * scale_factor).to_int();
	//Wyrmgus end
	int by = UI.InfoPanel.Y + (4 * scale_factor).to_int() + IconHeight + (10 * scale_factor).to_int();
	for (int i = 0; i < PlayerMax; ++i) {
		//Wyrmgus start
		if (i >= 15 && i < PlayerNumNeutral) {
			continue;
		}
//		if (i == PlayerMax / 2) {
		if ((i % 8) == 0) {
		//Wyrmgus end
			//Wyrmgus start
//			bx = UI.InfoPanel.X + 8;
			bx = UI.InfoPanel.X + (26 * scale_factor).to_int();
			//Wyrmgus end
			by += (20 * scale_factor).to_int();
		}
		if (bx < screenPos.x && screenPos.x < bx + (20 * scale_factor).to_int() && by < screenPos.y && screenPos.y < by + (20 * scale_factor).to_int()) {
			const player_type player_type = CPlayer::Players[i]->get_type();
			if (player_type != player_type::nobody) {
				std::array<char, 256> buf{};
				//Wyrmgus start
//				snprintf(buf.data(), buf.size(), _("Select Player #%d"), i);
				snprintf(buf.data(), buf.size(), _("Select Player #%d"), (i == PlayerNumNeutral) ? 16 : i + 1);
				//Wyrmgus end
				UI.StatusLine.Set(buf.data());
			} else {
				UI.StatusLine.Clear();
			}
			CEditor::get()->CursorPlayer = i;
#if 0
			ButtonUnderCursor = i + 100;
			CursorOn = cursor_on::button;
#endif
			return true;
		}
		bx += (20 * scale_factor).to_int();
	}

	int i = CEditor::get()->UnitIndex;
	by = UI.ButtonPanel.Y + (24 * scale_factor).to_int();
	for (size_t j = 0; j < UI.ButtonPanel.Buttons.size(); ++j) {
		const int x = UI.ButtonPanel.Buttons[j].X;
		const int y = UI.ButtonPanel.Buttons[j].Y;
		//Wyrmgus start
//		if (i >= (int) CEditor::get()->ShownUnitTypes.size()) {
		if (i >= (int) CEditor::get()->ShownUnitTypes.size() + 1) {
		//Wyrmgus end
			break;
		}
		//Wyrmgus start
		if (i < (int) CEditor::get()->ShownUnitTypes.size()) {
			if (j >= CurrentButtons.size()) {
				CurrentButtons.push_back(std::make_unique<button>());
			}
			CurrentButtons[j]->Hint = CEditor::get()->ShownUnitTypes[i]->get_name();
			CurrentButtons[j]->pos = j;
			CurrentButtons[j]->level = nullptr;
			CurrentButtons[j]->Action = ButtonCmd::EditorUnit;
			CurrentButtons[j]->ValueStr = CEditor::get()->ShownUnitTypes[i]->Ident;
			CurrentButtons[j]->Value = CEditor::get()->ShownUnitTypes[i]->Slot;
			CurrentButtons[j]->Popup = "popup_unit";
		}
		//Wyrmgus end
		if (x < screenPos.x && screenPos.x < x + IconWidth
			&& y < screenPos.y && screenPos.y < y + IconHeight) {
			CEditor::get()->CursorUnitIndex = i;
			return true;
		}
		++i;
	}
	return false;
}

static bool EditorCallbackMouse_EditTileArea(const PixelPos &screenPos)
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	//Wyrmgus start
//	int bx = UI.InfoPanel.X + 4;
	int bx = UI.InfoPanel.X + (11 * scale_factor).to_int();
	//Wyrmgus end
	int by = UI.InfoPanel.Y + (4 * scale_factor).to_int() + IconHeight + (10 * scale_factor).to_int();

	for (int i = 0; i < 6; ++i) {
		if (bx < screenPos.x && screenPos.x < bx + (100 * scale_factor).to_int() && by < screenPos.y && screenPos.y < by + (18 * scale_factor).to_int()) {
			ButtonUnderCursor = i + 300;
			CursorOn = cursor_on::button;
			return true;
		}
		by += (20 * scale_factor).to_int();
		//Wyrmgus start
		if (i == 3) {
			by = UI.InfoPanel.Y + (4 * scale_factor).to_int() + IconHeight + (10 * scale_factor).to_int();
			bx += (100 * scale_factor).to_int();
		}
		//Wyrmgus end
	}

	int i = CEditor::get()->TileIndex;
	by = UI.ButtonPanel.Y + (24 * scale_factor).to_int();
	while (by < UI.ButtonPanel.Y + ButtonPanelHeight - defines::get()->get_scaled_tile_height()) {
		if (i >= (int) CEditor::get()->ShownTileTypes.size()) {
			break;
		}
		//Wyrmgus start
//		bx = UI.ButtonPanel.X + 10;
		bx = UI.ButtonPanel.X + ((10 + 6) * scale_factor).to_int();
		//Wyrmgus end
		while (bx < UI.ButtonPanel.X + ButtonPanelWidth - defines::get()->get_scaled_tile_width()) {
			if (i >= (int) CEditor::get()->ShownTileTypes.size()) {
				break;
			}
			if (bx < screenPos.x && screenPos.x < bx + defines::get()->get_scaled_tile_width()
				&& by < screenPos.y && screenPos.y < by + defines::get()->get_scaled_tile_height()) {
				//Wyrmgus start
//				const int tile = CEditor::get()->ShownTileTypes[i];
//				const int tileindex = Map.Tileset->findTileIndexByTile(tile);
//				const int base = Map.Tileset->tiles[tileindex].tileinfo.BaseTerrain;
//				UI.StatusLine.Set(Map.Tileset->getTerrainName(base));
				UI.StatusLine.Set(CEditor::get()->ShownTileTypes[i]->get_name());
				//Wyrmgus end
				CEditor::get()->CursorTileIndex = i;
				return true;
			}

			bx += defines::get()->get_scaled_tile_width() + (30 * scale_factor).to_int();
			i++;
		}

		by += defines::get()->get_scaled_tile_height() + (18 * scale_factor).to_int();
	}
	return false;
}

/**
**  Callback for input movement of the cursor.
**
**  @param pos  Screen position.
*/
static void EditorCallbackMouse(const PixelPos &pos, const Qt::KeyboardModifiers key_modifiers)
{
	static int LastMapX = 0;
	static int LastMapY = 0;

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	PixelPos restrictPos = pos;
	HandleCursorMove(&restrictPos.x, &restrictPos.y); // Reduce to screen
	const PixelPos screenPos = pos;

	// Move map.
	if (cursor::get_current_cursor() == UI.get_cursor(cursor_type::scroll)) {
		MouseScrollMap(screenPos, key_modifiers);
		return;
	}

	// Automatically unpress when map tile has changed
	const Vec2i cursorTilePos = UI.SelectedViewport->ScreenToTilePos(CursorScreenPos);

	if (LastMapX != cursorTilePos.x || LastMapY != cursorTilePos.y) {
		LastMapX = cursorTilePos.x;
		LastMapY = cursorTilePos.y;
		UnitPlacedThisPress = false;
	}
	// Drawing tiles on map.
	if (CursorOn == cursor_on::map && (MouseButtons & LeftButton)
		&& (CEditor::get()->State == EditorEditTile || CEditor::get()->State == EditorEditUnit)) {
		Vec2i vpTilePos = UI.SelectedViewport->MapPos;
		// Scroll the map
		if (CursorScreenPos.x <= UI.SelectedViewport->get_top_left_pos().x()) {
			vpTilePos.x--;
			UI.SelectedViewport->Set(vpTilePos, defines::get()->get_scaled_tile_size() / 2);
		} else if (CursorScreenPos.x >= UI.SelectedViewport->get_bottom_right_pos().x()) {
			vpTilePos.x++;
			UI.SelectedViewport->Set(vpTilePos, defines::get()->get_scaled_tile_size() / 2);
		}

		if (CursorScreenPos.y <= UI.SelectedViewport->get_top_left_pos().y()) {
			vpTilePos.y--;
			UI.SelectedViewport->Set(vpTilePos, defines::get()->get_scaled_tile_size() / 2);
		} else if (CursorScreenPos.y >= UI.SelectedViewport->get_bottom_right_pos().y()) {
			vpTilePos.y++;
			UI.SelectedViewport->Set(vpTilePos, defines::get()->get_scaled_tile_size() / 2);
		}

		// Scroll the map, if cursor moves outside the viewport.
		RestrictCursorToViewport();
		const Vec2i tilePos = UI.SelectedViewport->ScreenToTilePos(CursorScreenPos);

		if (CEditor::get()->State == EditorEditTile && CEditor::get()->SelectedTileIndex != -1) {
			EditTiles(tilePos, CEditor::get()->ShownTileTypes[CEditor::get()->SelectedTileIndex], TileCursorSize, key_modifiers);
		} else if (CEditor::get()->State == EditorEditUnit && CursorBuilding) {
			if (!UnitPlacedThisPress) {
				if (CanBuildUnitType(nullptr, *CursorBuilding, tilePos, 1, true, UI.CurrentMapLayer->ID)) {
					EditorPlaceUnit(tilePos, *CursorBuilding, CPlayer::Players[CEditor::get()->SelectedPlayer].get());
					UnitPlacedThisPress = true;
					UI.StatusLine.Clear();
				}
			}
		}
		return;
	}

	// Minimap move viewpoint
	if (CursorOn == cursor_on::minimap && (MouseButtons & LeftButton)) {
		RestrictCursorToMinimap();
		const Vec2i tilePos = UI.get_minimap()->screen_to_tile_pos(CursorScreenPos);

		UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(tilePos));
		return;
	}

	MouseScrollState = ScrollNone;
	cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false);
	CursorOn = cursor_on::unknown;
	CEditor::get()->CursorPlayer = -1;
	CEditor::get()->CursorUnitIndex = -1;
	CEditor::get()->CursorTileIndex = -1;
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;

	// Minimap
	if (UI.get_minimap()->Contains(screenPos)) {
		CursorOn = cursor_on::minimap;
	}
	// Handle edit unit area
	if (CEditor::get()->State == EditorEditUnit || CEditor::get()->State == EditorSetStartLocation) {
		if (EditorCallbackMouse_EditUnitArea(screenPos) == true) {
			return;
		}
	}

	// Handle tile area
	if (CEditor::get()->State == EditorEditTile) {
		if (EditorCallbackMouse_EditTileArea(screenPos) == true) {
			return;
		}
	}

	// Handle buttons
	if (UI.InfoPanel.X + (11 * scale_factor).to_int() < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + (11 * scale_factor).to_int() + CEditor::get()->Select.Icon->get_graphics()->Width
		&& UI.InfoPanel.Y + (7 * scale_factor).to_int() < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + (7 * scale_factor).to_int() + CEditor::get()->Select.Icon->get_graphics()->Height) {
		// FIXME: what is this button?
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = SelectButton;
		CursorOn = cursor_on::button;
		UI.StatusLine.Set(_("Select Mode"));
		return;
	}
	if (UI.InfoPanel.X + (11 * scale_factor).to_int() + get_unit_icon_x() < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + (11 * scale_factor).to_int() + get_unit_icon_x() + CEditor::get()->Units.Icon->get_graphics()->Width
		&& UI.InfoPanel.Y + (7 * scale_factor).to_int() + get_unit_icon_y() < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + (7 * scale_factor).to_int() + get_unit_icon_y() + CEditor::get()->Units.Icon->get_graphics()->Height) {
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = UnitButton;
		CursorOn = cursor_on::button;
		UI.StatusLine.Set(_("Unit Mode"));
		return;
	}
	if (CEditor::get()->TerrainEditable) {
		if (UI.InfoPanel.X + (11 * scale_factor).to_int() + get_tile_icon_x() < CursorScreenPos.x
			&& CursorScreenPos.x < UI.InfoPanel.X + (11 * scale_factor).to_int() + get_tile_icon_x() + defines::get()->get_scaled_tile_width() + (7 * scale_factor).to_int()
			&& UI.InfoPanel.Y + (4 * scale_factor).to_int() + get_tile_icon_y() < CursorScreenPos.y
			&& CursorScreenPos.y < UI.InfoPanel.Y + (4 * scale_factor).to_int() + get_tile_icon_y() + defines::get()->get_scaled_tile_height() + (7 * scale_factor).to_int()) {
			ButtonAreaUnderCursor = -1;
			ButtonUnderCursor = TileButton;
			CursorOn = cursor_on::button;
			UI.StatusLine.Set(_("Tile Mode"));
			return;
		}
	}

	int StartUnitWidth = CEditor::get()->StartUnit ? CEditor::get()->StartUnit->get_icon()->get_graphics()->Width : defines::get()->get_scaled_tile_width() + (7 * scale_factor).to_int();
	int StartUnitHeight = CEditor::get()->StartUnit ? CEditor::get()->StartUnit->get_icon()->get_graphics()->Height : defines::get()->get_scaled_tile_height() + (7 * scale_factor).to_int();
	if (UI.InfoPanel.X + (11 * scale_factor).to_int() + get_start_icon_x() < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + (11 * scale_factor).to_int() + get_start_icon_x() + StartUnitWidth
		&& UI.InfoPanel.Y + (5 * scale_factor).to_int() + get_start_icon_y() < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + (5 * scale_factor).to_int() + get_start_icon_y() + StartUnitHeight) {
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = StartButton;
		CursorOn = cursor_on::button;
		UI.StatusLine.Set(_("Set Start Location"));
		return;
	}

	// Minimap
	if (UI.get_minimap()->Contains(screenPos)) {
		CursorOn = cursor_on::minimap;
		return;
	}

	// Map
	UnitUnderCursor = nullptr;
	if (UI.MapArea.contains(screenPos)) {
		CViewport *vp = GetViewport(screenPos);

		if (vp == nullptr) {
			log::log_error("Screen position " + point::to_string(screenPos) + " is contained in the map area, but has no viewport.");
		}

		if (UI.MouseViewport != vp) { // viewport changed
			UI.MouseViewport = vp;
			DebugPrint("active viewport changed to %ld.\n" _C_
					   static_cast<long int>(UI.Viewports - vp));
		}
		CursorOn = cursor_on::map;

		if (UI.MouseViewport) {
			// Look if there is an unit under the cursor.
			const PixelPos cursorMapPos = UI.MouseViewport->screen_to_scaled_map_pixel_pos(CursorScreenPos);
			UnitUnderCursor = UnitOnScreen(cursorMapPos.x, cursorMapPos.y);

			if (UnitUnderCursor != nullptr) {
				ShowUnitInfo(*UnitUnderCursor);
				return;
			}
		}
	}
	// Scrolling Region Handling
	if (HandleMouseScrollArea(screenPos)) {
		return;
	}

	// Not reached if cursor is inside the scroll area

	UI.StatusLine.Clear();
}

/**
**  Callback for exit.
*/
static void EditorCallbackExit()
{
}

/**
**  Create editor.
*/
void CEditor::Init()
{
	// Load and evaluate the editor configuration file
	const std::string filename = LibraryFileName(parameters::get()->luaEditorStartFilename.c_str());
	if (!CanAccessFile(filename.c_str())) {
		throw std::runtime_error("Editor configuration file \"" + parameters::get()->luaEditorStartFilename  + "\" was not found, specify another with '-E file.lua'");
	}

	ShowLoadProgress(_("Loading Script \"%s\"..."), filename.c_str());
	LuaLoadFile(filename);
	LuaGarbageCollect();
	
	//Wyrmgus start
	if (this->UnitTypes.size() == 0) {
		//if editor's unit types vector is still empty after loading the editor's lua file, then fill it automatically
		for (const unit_type *unit_type : unit_type::get_all()) {
			if (unit_type->is_template()) {
				continue;
			}

			if (unit_type->get_icon() == nullptr || unit_type->BoolFlag[VANISHES_INDEX].value || unit_type->BoolFlag[HIDDENINEDITOR_INDEX].value) {
				continue;
			}
			
			this->UnitTypes.push_back(unit_type->Ident);
		}
	}
	//Wyrmgus end

	CPlayer::SetThisPlayer(CPlayer::Players[0].get());

	FlagRevealMap = 1; // editor without fog and all visible
	CMap::get()->NoFogOfWar = true;

	//Wyrmgus start
//	if (!*CurrentMapPath) { // new map!
	if (CurrentMapPath.empty()) { // new map or is a mod
	//Wyrmgus end
		InitUnitTypes(1);
		//
		// Inititialize Map / Players.
		//
		InitPlayers();
		for (int i = 0; i < PlayerMax; ++i) {
			if (i == PlayerNumNeutral) {
				CreatePlayer(player_type::neutral);
				CPlayer::Players[i]->set_type(player_type::neutral);
				CPlayer::Players[i]->set_civilization(defines::get()->get_neutral_civilization());
			} else {
				CreatePlayer(player_type::nobody);
				CPlayer::Players[i]->set_type(player_type::nobody);
			}
		}

		//Wyrmgus start
//		Map.Fields = new tile[Map.Info.MapWidth * Map.Info.MapHeight];
		CMap::get()->ClearMapLayers();
		auto new_map_layer = std::make_unique<CMapLayer>(CMap::get()->Info->get_map_width(), CMap::get()->Info->get_map_height());

		if (QApplication::instance()->thread() != QThread::currentThread()) {
			new_map_layer->moveToThread(QApplication::instance()->thread());
		}

		new_map_layer->ID = CMap::get()->MapLayers.size();
		CMap::get()->Info->MapWidths.clear();
		CMap::get()->Info->MapWidths.push_back(CMap::get()->Info->get_map_width());
		CMap::get()->Info->MapHeights.clear();
		CMap::get()->Info->MapHeights.push_back(CMap::get()->Info->get_map_height());
		CMap::get()->MapLayers.push_back(std::move(new_map_layer));
		//Wyrmgus end

		const int defaultTile = CMap::get()->Tileset->getDefaultTileIndex();
		//Wyrmgus start
		const CTileset &tileset = *CMap::get()->Tileset;
		//Wyrmgus end

		//Wyrmgus start
		for (const std::unique_ptr<CMapLayer> &map_layer : CMap::get()->MapLayers) {
			int max_tile_index = map_layer->get_width() * map_layer->get_height();
			for (int i = 0; i < max_tile_index; ++i) {
				//Wyrmgus start
	//			Map.Fields[i].setTileIndex(*Map.Tileset, defaultTile, 0);
				map_layer->Field(i)->setTileIndex(*CMap::get()->Tileset, tileset.getTileNumber(defaultTile, true, false), 0);
				//Wyrmgus end
			}
		}
		//Wyrmgus start
		GameSettings.Resources = SettingsPresetMapDefault;
		//Wyrmgus start
//		CreateGame("", &Map);
		//Wyrmgus end
	//Wyrmgus start
	}
	if (CurrentMapPath.empty()) {
		CreateGame("", CMap::get());
	//Wyrmgus end
	} else {
		CreateGame(CurrentMapPath, CMap::get());
	}

	ReplayRevealMap = 1;
	FlagRevealMap = 0;
	CEditor::get()->SelectedPlayer = PlayerNumNeutral;

	// Place the start points, which the loader discarded.
	for (int i = 0; i < PlayerMax; ++i) {
		const player_type player_type = CPlayer::Players[i]->get_type();
		if (player_type != player_type::nobody) {
			// Set SelectedPlayer to a valid player
			if (CEditor::get()->SelectedPlayer == PlayerNumNeutral) {
				CEditor::get()->SelectedPlayer = i;
				break;
			}
		}
	}
	//Wyrmgus start
//	ButtonPanelWidth = 170;//200;
//	ButtonPanelHeight = 160 + (Video.Height - 480);
	// adapt to new UI size, should make this more scriptable
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	ButtonPanelWidth = (243 * scale_factor).to_int();
	ButtonPanelHeight = (186 * scale_factor).to_int();
	//Wyrmgus end

	CalculateMaxIconSize();
	VisibleUnitIcons = CalculateVisibleIcons();

	if (!StartUnitName.empty()) {
		StartUnit = unit_type::get(StartUnitName);
	}
	Select.Icon = nullptr;
	Select.Load();
	Units.Icon = nullptr;
	Units.Load();

	//Wyrmgus start
//	Map.Tileset->fillSolidTiles(&CEditor::get()->ShownTileTypes);
	CEditor::get()->ShownTileTypes.clear();
	for (terrain_type *terrain_type : terrain_type::get_all()) {
		if (!terrain_type->is_hidden()) {
			CEditor::get()->ShownTileTypes.push_back(terrain_type);
		}
	}
	//Wyrmgus end
	VisibleTileIcons = CalculateVisibleIcons(true);

	RecalculateShownUnits();

	EditorUndoActions.clear();
	EditorRedoActions.clear();

	EditorCallbacks.ButtonPressed = EditorCallbackButtonDown;
	EditorCallbacks.ButtonReleased = EditorCallbackButtonUp;
	EditorCallbacks.MouseMoved = EditorCallbackMouse;
	EditorCallbacks.MouseExit = EditorCallbackExit;
	EditorCallbacks.KeyPressed = EditorCallbackKeyDown;
	EditorCallbacks.KeyReleased = EditorCallbackKeyUp;
	EditorCallbacks.KeyRepeated = EditorCallbackKeyRepeated;
	EditorCallbacks.NetworkEvent = NetworkEvent;
	SetCallbacks(&EditorCallbacks);
}

/**
**  Save a map from editor.
**
**  @param file  Save the level to this file.
**
**  @return      0 for success, -1 for error
**
**  @todo  FIXME: Check if the map is valid, contains no failures.
**         At least two players, one human slot, every player a startpoint
**         ...
*/
int EditorSaveMap(const std::string &file)
{
	if (SaveStratagusMap(file, *CMap::get(), CEditor::get()->TerrainEditable) == -1) {
		log::log_error("Cannot save map.");
		return -1;
	}
	return 0;
}

std::string get_user_maps_path()
{
	std::filesystem::path path = database::get_user_maps_path();
	path.make_preferred();
	return path.string();
}

/*----------------------------------------------------------------------------
--  Editor main loop
----------------------------------------------------------------------------*/

/**
**  Editor main event loop.
*/
void EditorMainLoop()
{
	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	bool OldCommandLogDisabled = CommandLogDisabled;
	const EventCallback *old_callbacks = GetCallbacks();
	bool first_init = true;

	CommandLogDisabled = true;

	gcn::Widget *oldTop = Gui->getTop();

	editorContainer = std::make_unique<gcn::Container>();
	editorContainer->setDimension(gcn::Rectangle(0, 0, Video.Width, Video.Height));
	editorContainer->setOpaque(false);
	Gui->setTop(editorContainer.get());

	editorUnitSliderListener = std::make_unique<EditorUnitSliderListener>();
	editorSliderListener = std::make_unique<EditorSliderListener>();

	editorUnitSlider = std::make_unique<gcn::Slider>();
	editorUnitSlider->setBaseColor(gcn::Color(38, 38, 78));
	editorUnitSlider->setForegroundColor(gcn::Color(200, 200, 120));
	editorUnitSlider->setBackgroundColor(gcn::Color(200, 200, 120));
	editorUnitSlider->setVisible(false);
	editorUnitSlider->addActionListener(editorUnitSliderListener.get());

	editorSlider = std::make_unique<gcn::Slider>();
	editorSlider->setBaseColor(gcn::Color(38, 38, 78));
	editorSlider->setForegroundColor(gcn::Color(200, 200, 120));
	editorSlider->setBackgroundColor(gcn::Color(200, 200, 120));
	editorSlider->setVisible(false);
	editorSlider->addActionListener(editorSliderListener.get());

	UpdateMinimap = true;

	CEditor::get()->set_running(true);

	CEditor::get()->Init();

	if (first_init) {
		first_init = false;
		//Wyrmgus start
//			editorUnitSlider->setSize(ButtonPanelWidth/*176*/, 16);
//			editorSlider->setSize(ButtonPanelWidth/*176*/, 16);
		editorUnitSlider->setSize(((218 - 24 - 6) * scale_factor).to_int(), (16 * scale_factor).to_int()); // adapt to new UI size, should make this more scriptable
		editorSlider->setSize(((218 - 24 - 6) * scale_factor).to_int(), (16 * scale_factor).to_int());
		//Wyrmgus end
		//Wyrmgus start
//			editorContainer->add(editorUnitSlider.get(), UI.ButtonPanel.X + 2, UI.ButtonPanel.Y - 16);
//			editorContainer->add(editorSlider.get(), UI.ButtonPanel.X + 2, UI.ButtonPanel.Y - 16);
		editorContainer->add(editorUnitSlider.get(), UI.InfoPanel.X + (12 * scale_factor).to_int(), UI.InfoPanel.Y + ((160 - 24) * scale_factor).to_int());
		editorContainer->add(editorSlider.get(), UI.InfoPanel.X + (12 * scale_factor).to_int(), UI.InfoPanel.Y + ((160 - 24) * scale_factor).to_int());
		//Wyrmgus end
	}
	//ProcessMenu("menu-editor-tips", 1);
	current_interface_state = interface_state::normal;

	SetVideoSync();

	cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);
	current_interface_state = interface_state::normal;
	CEditor::get()->State = EditorSelecting;
	UI.SelectedViewport = UI.Viewports;
	TileCursorSize = 1;

	game::get()->set_running(true); //should use something different instead?

	engine_interface::get()->set_waiting_for_interface(true);
	engine_interface::get()->get_map_view_created_future().wait();
	engine_interface::get()->reset_map_view_created_promise();
	engine_interface::get()->set_waiting_for_interface(false);

	engine_interface::get()->set_loading_message("");

	while (CEditor::get()->is_running()) {
		engine_interface::get()->run_event_loop();

		CheckMusicFinished();

		if (FrameCounter % FRAMES_PER_SECOND == 0) {
			if (UpdateMinimap) {
				UI.get_minimap()->Update();
				UpdateMinimap = false;
			}
		}

		EditorUpdateDisplay();

		//
		// Map scrolling
		//
		if (UI.MouseScroll) {
			DoScrollArea(MouseScrollState, 0, MouseScrollState == 0 && KeyScrollState > 0, stored_key_modifiers);
		}
		if (UI.KeyScroll) {
			DoScrollArea(KeyScrollState, (stored_key_modifiers & Qt::ControlModifier) != 0, MouseScrollState == 0 && KeyScrollState > 0, stored_key_modifiers);
			if (CursorOn == cursor_on::map && (MouseButtons & LeftButton) &&
				(CEditor::get()->State == EditorEditTile ||
					CEditor::get()->State == EditorEditUnit)) {
				EditorCallbackButtonDown(0, stored_key_modifiers);
			}
		}

		WaitEventsOneFrame();
	}

	game::get()->clear_results();
	game::get()->set_running(false); //should use something different instead?

	CursorBuilding = nullptr;

	CommandLogDisabled = OldCommandLogDisabled;
	SetCallbacks(old_callbacks);
	Gui->setTop(oldTop);
	editorContainer.reset();
	editorUnitSliderListener.reset();
	editorSliderListener.reset();
	editorUnitSlider.reset();
	editorSlider.reset();
}
