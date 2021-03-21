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
//      (c) Copyright 2002-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "civilization.h"
#include "commands.h"
#include "database/defines.h"
#include "engine_interface.h"
#include "game.h"
#include "guichan.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/tileset.h"
#include "menus.h"
#include "network.h"
#include "parameters.h"
#include "player.h"
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
#include "util/enum_util.h"
#include "util/util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"
#include "video/font.h"
#include "video/render_context.h"
#include "video/video.h"
#include "widgets.h"

extern void DoScrollArea(int state, bool fast, bool isKeyboard);
extern void DrawGuichanWidgets(std::vector<std::function<void(renderer *)>> &render_commands);
extern void CleanGame();

static int IconWidth;                       /// Icon width in panels
static int IconHeight;                      /// Icon height in panels

static int ButtonPanelWidth;
static int ButtonPanelHeight;

char TileToolRandom;      /// Tile tool draws random
static char TileToolDecoration;  /// Tile tool draws with decorations
static int TileCursorSize;       /// Tile cursor size 1x1 2x2 ... 4x4
static bool UnitPlacedThisPress = false;  /// Only allow one unit per press
static bool UpdateMinimap = false;        /// Update units on the minimap
//Wyrmgus start
static bool IsMod = false;				  /// Whether the current "map" is a mod
//Wyrmgus end
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
	const wyrmgus::unit_type *UnitType;
	CPlayer *Player;
};

static std::deque<EditorAction> EditorUndoActions;
static std::deque<EditorAction> EditorRedoActions;

/// Unit mode icon
static int get_unit_icon_x()
{
	return IconWidth + 7 * wyrmgus::defines::get()->get_scale_factor();
}

static int get_unit_icon_y()
{
	return 0;
}

/// Tile mode icon
static int get_tile_icon_x()
{
	return IconWidth * 2 + 16 * wyrmgus::defines::get()->get_scale_factor();
}

static int get_tile_icon_y()
{
	return 2 * wyrmgus::defines::get()->get_scale_factor();
}

/// Start mode icon
static int get_start_icon_x()
{
	return IconWidth * 3 + 16 * wyrmgus::defines::get()->get_scale_factor();
}

static int get_start_icon_y()
{
	return 2 * wyrmgus::defines::get()->get_scale_factor();
}

static void EditorUndoAction();
static void EditorRedoAction();
static void EditorAddUndoAction(EditorAction action);

static std::unique_ptr<gcn::Container> editorContainer;
static std::unique_ptr<gcn::Slider> editorUnitSlider;
static std::unique_ptr<gcn::Slider> editorSlider;

class EditorUnitSliderListener : public gcn::ActionListener
{
public:
	virtual void action(const std::string &)
	{
		const int iconsPerStep = VisibleUnitIcons;
		//Wyrmgus start
//		const int steps = (Editor.ShownUnitTypes.size() + iconsPerStep - 1) / iconsPerStep;
		const int steps = (Editor.ShownUnitTypes.size() + 1 + iconsPerStep - 1) / iconsPerStep; // + 1 because of the button to create a new unit
		//Wyrmgus end
		const double value = editorUnitSlider->getValue();
		for (int i = 1; i <= steps; ++i) {
			if (value <= (double)i / steps) {
				Editor.UnitIndex = iconsPerStep * (i - 1);
				break;
			}
		}
	}
};

static std::unique_ptr<EditorUnitSliderListener> editorUnitSliderListener;

class EditorSliderListener : public gcn::ActionListener
{
public:
	virtual void action(const std::string &)
	{
		const int iconsPerStep = VisibleTileIcons;
		const int steps = (Editor.ShownTileTypes.size() + iconsPerStep - 1) / iconsPerStep;
		const double value = editorSlider->getValue();
		for (int i = 1; i <= steps; ++i) {
			if (value <= (double)i / steps) {
				Editor.TileIndex = iconsPerStep * (i - 1);
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
static void EditTile(const Vec2i &pos, const terrain_type *terrain)
//Wyrmgus end
{
	Assert(CMap::get()->Info.IsPointOnMap(pos, UI.CurrentMapLayer));
	
	wyrmgus::tile &mf = *UI.CurrentMapLayer->Field(pos);
	
	//Wyrmgus start
	int value = 0;
	if (terrain->has_flag(tile_flag::tree) || terrain->has_flag(tile_flag::rock)) {
		value = terrain->get_resource()->get_default_amount();
	}
//	mf.setTileIndex(tileset, tileIndex, 0);
	mf.SetTerrain(terrain);
	if (!terrain->is_overlay() && !(KeyModifiers & ModifierShift)) { // don't remove overlay terrains if holding shift
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
static void EditTilesInternal(const Vec2i &pos, const terrain_type *terrain, int size)
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
			EditTile(itPos, terrain);
			changed_tiles.push_back(itPos);
			//Wyrmgus end
		}
	}
	
	//now, check if the tiles adjacent to the placed ones need correction
	//Wyrmgus start
	for (int i = (((int) changed_tiles.size()) - 1); i >= 0; --i) {
		const terrain_type *tile_terrain = CMap::get()->GetTileTerrain(changed_tiles[i], terrain->is_overlay(), UI.CurrentMapLayer->ID);
		
		CMap::get()->CalculateTileTransitions(changed_tiles[i], false, UI.CurrentMapLayer->ID);
		CMap::get()->CalculateTileTransitions(changed_tiles[i], true, UI.CurrentMapLayer->ID);

		bool has_transitions = terrain->is_overlay() ? (UI.CurrentMapLayer->Field(changed_tiles[i])->OverlayTransitionTiles.size() > 0) : (UI.CurrentMapLayer->Field(changed_tiles[i])->TransitionTiles.size() > 0);
		bool solid_tile = true;
		
		if (tile_terrain && !tile_terrain->allows_single()) {
			for (int x_offset = -1; x_offset <= 1; ++x_offset) {
				for (int y_offset = -1; y_offset <= 1; ++y_offset) {
					if (x_offset != 0 || y_offset != 0) {
						Vec2i adjacent_pos(changed_tiles[i].x + x_offset, changed_tiles[i].y + y_offset);
						if (CMap::get()->Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
							const wyrmgus::terrain_type *adjacent_terrain = CMap::get()->GetTileTerrain(adjacent_pos, tile_terrain->is_overlay(), UI.CurrentMapLayer->ID);
							if (tile_terrain->is_overlay() && adjacent_terrain && UI.CurrentMapLayer->Field(adjacent_pos)->OverlayTerrainDestroyed) {
								adjacent_terrain = nullptr;
							}
							if (tile_terrain != adjacent_terrain && !wyrmgus::vector::contains(tile_terrain->get_outer_border_terrain_types(), adjacent_terrain)) { // also happens if terrain is null, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
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
							if (CMap::get()->Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
								EditTile(adjacent_pos, terrain);
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
					
					if (CMap::get()->Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
						for (int overlay = 1; overlay >= 0; --overlay) {
							const wyrmgus::terrain_type *adjacent_terrain = CMap::get()->GetTileTerrain(adjacent_pos, overlay > 0, UI.CurrentMapLayer->ID);
							if (!adjacent_terrain || adjacent_terrain == CMap::get()->GetTileTerrain(changed_tiles[i], overlay > 0, UI.CurrentMapLayer->ID)) {
								continue;
							}
							CMap::get()->CalculateTileTransitions(adjacent_pos, overlay == 1, UI.CurrentMapLayer->ID);
							bool has_transitions = overlay ? (UI.CurrentMapLayer->Field(adjacent_pos)->OverlayTransitionTiles.size() > 0) : (UI.CurrentMapLayer->Field(adjacent_pos)->TransitionTiles.size() > 0);
							bool solid_tile = true;
							
							if (!overlay && std::find(adjacent_terrain->BorderTerrains.begin(), adjacent_terrain->BorderTerrains.end(), CMap::get()->GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID)) == adjacent_terrain->BorderTerrains.end()) {
								for (size_t j = 0; j != adjacent_terrain->BorderTerrains.size(); ++j) {
									wyrmgus::terrain_type *border_terrain = adjacent_terrain->BorderTerrains[j];
									if (std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), adjacent_terrain) != border_terrain->BorderTerrains.end() && std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), CMap::get()->GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID)) != border_terrain->BorderTerrains.end()) { // found a terrain type that can border both terrains
										CMap::get()->SetTileTerrain(adjacent_pos, border_terrain, UI.CurrentMapLayer->ID);
										changed_tiles.push_back(adjacent_pos);
										break;
									}
								}
							} else if (!adjacent_terrain->allows_single()) {
								for (int sub_x_offset = -1; sub_x_offset <= 1; ++sub_x_offset) {
									for (int sub_y_offset = -1; sub_y_offset <= 1; ++sub_y_offset) {
										if (sub_x_offset != 0 || sub_y_offset != 0) {
											Vec2i sub_adjacent_pos(adjacent_pos.x + sub_x_offset, adjacent_pos.y + sub_y_offset);
											if (CMap::get()->Info.IsPointOnMap(sub_adjacent_pos, UI.CurrentMapLayer)) {
												const wyrmgus::terrain_type *sub_adjacent_terrain = CMap::get()->GetTileTerrain(sub_adjacent_pos, overlay > 0, UI.CurrentMapLayer->ID);
												if (adjacent_terrain->is_overlay() && sub_adjacent_terrain && UI.CurrentMapLayer->Field(sub_adjacent_pos)->OverlayTerrainDestroyed) {
													sub_adjacent_terrain = nullptr;
												}
												if (adjacent_terrain != sub_adjacent_terrain && !wyrmgus::vector::contains(adjacent_terrain->get_outer_border_terrain_types(), sub_adjacent_terrain)) { // also happens if terrain is null, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
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
		CMap::get()->CalculateTileTransitions(tile_pos, false, UI.CurrentMapLayer->ID);
		CMap::get()->CalculateTileTransitions(tile_pos, true, UI.CurrentMapLayer->ID);
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
					
					if (!CMap::get()->Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
						continue;
					}

					const wyrmgus::tile *adjacent_tile = UI.CurrentMapLayer->Field(adjacent_pos);

					const size_t old_adjacent_base_transition_count = adjacent_tile->TransitionTiles.size();
					const size_t old_adjacent_overlay_transition_count = adjacent_tile->OverlayTransitionTiles.size();

					CMap::get()->CalculateTileTransitions(adjacent_pos, false, UI.CurrentMapLayer->ID);
					CMap::get()->CalculateTileTransitions(adjacent_pos, true, UI.CurrentMapLayer->ID);
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
static void EditTiles(const Vec2i &pos, const terrain_type *terrain, int size)
//Wyrmgus end
{
	//Wyrmgus start
//	EditTilesInternal(pos, tile, size);
	EditTilesInternal(pos, terrain, size);
	//Wyrmgus end

	if (!MirrorEdit) {
		return;
	}
	const Vec2i mpos(UI.CurrentMapLayer->get_width() - size, UI.CurrentMapLayer->get_height() - size);
	const Vec2i mirror = mpos - pos;
	const Vec2i mirrorv(mirror.x, pos.y);

	EditTilesInternal(mirrorv, terrain, size);
	if (MirrorEdit == 1) {
		return;
	}
	const Vec2i mirrorh(pos.x, mirror.y);

	EditTilesInternal(mirrorh, terrain, size);
	EditTilesInternal(mirror, terrain, size);
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
static void EditorActionPlaceUnit(const Vec2i &pos, const wyrmgus::unit_type &type, CPlayer *player)
{
	Assert(CMap::get()->Info.IsPointOnMap(pos, UI.CurrentMapLayer));

	if (type.Neutral) {
		player = CPlayer::Players[PlayerNumNeutral];
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
static void EditorPlaceUnit(const Vec2i &pos, const wyrmgus::unit_type &type, CPlayer *player)
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
			CUnit *unit = UnitOnMapTile(action.tilePos, action.UnitType->UnitType, UI.CurrentMapLayer->ID);

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
			CUnit *unit = UnitOnMapTile(action.tilePos, action.UnitType->UnitType, UI.CurrentMapLayer->ID);

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
		w = wyrmgus::defines::get()->get_scaled_tile_width();//+2,
		h = wyrmgus::defines::get()->get_scaled_tile_height();//+2
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
	for (unsigned int i = 0; i < Editor.UnitTypes.size(); ++i) {
		const wyrmgus::unit_type *type = wyrmgus::unit_type::get(Editor.UnitTypes[i].c_str());
		Assert(type->Icon.Icon);
		const wyrmgus::icon &icon = *type->Icon.Icon;

		IconWidth = std::max(IconWidth, icon.get_graphics()->Width);
		IconHeight = std::max(IconHeight, icon.get_graphics()->Height);
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
	Editor.ShownUnitTypes.clear();

	for (size_t i = 0; i != Editor.UnitTypes.size(); ++i) {
		const wyrmgus::unit_type *type = wyrmgus::unit_type::get(Editor.UnitTypes[i].c_str());
		Editor.ShownUnitTypes.push_back(type);
	}

	if (Editor.UnitIndex >= (int)Editor.ShownUnitTypes.size()) {
		Editor.UnitIndex = Editor.ShownUnitTypes.size() / VisibleUnitIcons * VisibleUnitIcons;
	}
	// Quick & dirty make them invalid
	Editor.CursorUnitIndex = -1;
	Editor.SelectedUnitIndex = -1;
}

/*----------------------------------------------------------------------------
--  Display
----------------------------------------------------------------------------*/

/**
**  Draw a table with the players
*/
static void DrawPlayers(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();
	std::array<char, 256> buf{};
	CLabel label(wyrmgus::defines::get()->get_small_font());

	//Wyrmgus start
//	int x = UI.InfoPanel.X + 8;
	int x = UI.InfoPanel.X + 26 * scale_factor;
	//Wyrmgus end
	int y = UI.InfoPanel.Y + 4 * scale_factor + IconHeight + 10 * scale_factor;

	const int rectangle_size = 20 * scale_factor;

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
		if (i == Editor.CursorPlayer && CMap::get()->Info.PlayerType[i] != PlayerNobody) {
			Video.DrawRectangle(ColorWhite, x + i % 8 * rectangle_size, y, rectangle_size, rectangle_size);
		}
		Video.DrawRectangle(
			i == Editor.CursorPlayer && CMap::get()->Info.PlayerType[i] != PlayerNobody ? ColorWhite : ColorGray,
			x + i % 8 * rectangle_size, y, rectangle_size - 1, rectangle_size - 1);
		if (CMap::get()->Info.PlayerType[i] != PlayerNobody) {
			Video.FillRectangle(CVideo::MapRGB(CPlayer::Players[i]->get_minimap_color()), x + 1 + i % 8 * rectangle_size, y + 1, rectangle_size - 1 - 2, rectangle_size - 1 - 2, render_commands);
		}
		if (i == Editor.SelectedPlayer) {
			Video.DrawRectangle(ColorGreen, x + 1 + i % 8 * rectangle_size, y + 1, rectangle_size - 1 - 2, rectangle_size - 1 - 2);
		}
		//Wyrmgus start
//		sprintf(buf, "%d", i);
		sprintf(buf.data(), "%d", (i == PlayerNumNeutral) ? 16 : i + 1);
		//Wyrmgus end
		label.DrawCentered(x + i % 8 * rectangle_size + 10 * scale_factor, y + 7 * scale_factor, buf.data(), render_commands);
	}

	//Wyrmgus start
//	x = UI.InfoPanel.X + 4;
	x = UI.InfoPanel.X + 22 * scale_factor;
	//Wyrmgus end
	y += (18 * 1 + 4) * scale_factor;
	if (Editor.SelectedPlayer != -1) {
		//Wyrmgus start
//		snprintf(buf.data(), buf.size(), "Plyr %d %s ", Editor.SelectedPlayer,
//				 PlayerRaces.Name[Players[Editor.SelectedPlayer].Race].c_str());
		std::string civ_str = wyrmgus::civilization::get_all()[CPlayer::Players[Editor.SelectedPlayer]->Race]->get_identifier().c_str();
		civ_str[0] = toupper(civ_str[0]);
		snprintf(buf.data(), buf.size(), "Player %d %s ", (Editor.SelectedPlayer == PlayerNumNeutral) ? 16 : Editor.SelectedPlayer + 1, civ_str.c_str());
		//Wyrmgus end
		// Players[SelectedPlayer].RaceName);

		switch (CMap::get()->Info.PlayerType[Editor.SelectedPlayer]) {
			case PlayerNeutral:
				strcat_s(buf.data(), buf.size(), "Neutral");
				break;
			case PlayerNobody:
			default:
				strcat_s(buf.data(), buf.size(), "Nobody");
				break;
			case PlayerPerson:
				strcat_s(buf.data(), buf.size(), "Person");
				break;
			case PlayerComputer:
			case PlayerRescuePassive:
			case PlayerRescueActive:
				strcat_s(buf.data(), buf.size(), "Computer");
				break;
		}
		label.SetFont(wyrmgus::defines::get()->get_game_font());
		label.Draw(x, y, buf.data(), render_commands);
	}
}

#if 0
extern void DrawPopupUnitInfo(const wyrmgus::unit_type *type,
							  int player_index, wyrmgus::font *font,
							  uint32_t backgroundColor, int buttonX, int buttonY);

static void DrawPopup()
{
	if (Editor.State == EditorEditUnit && Editor.CursorUnitIndex != -1) {
		DrawPopupUnitInfo(Editor.ShownUnitTypes[Editor.CursorUnitIndex],
						  Editor.SelectedPlayer, GetSmallFont(),
						  CVideo::MapRGB(38, 38, 78),
						  Editor.PopUpX, Editor.PopUpY);
	}
}
#endif

/**
**  Draw unit icons.
*/
static void DrawUnitIcons(std::vector<std::function<void(renderer *)>> &render_commands)
{
	int i = Editor.UnitIndex;

	for (size_t j = 0; j < UI.ButtonPanel.Buttons.size(); ++j) {
		const int x = UI.ButtonPanel.Buttons[j].X;
		const int y = UI.ButtonPanel.Buttons[j].Y;
		//Wyrmgus start
//		if (i >= (int) Editor.ShownUnitTypes.size()) {
		if (i >= (int) Editor.ShownUnitTypes.size() + 1) {
		//Wyrmgus end
			//Wyrmgus start
//			return;
			break;
			//Wyrmgus emd
		}
		//Wyrmgus start
//		wyrmgus::icon &icon = *Editor.ShownUnitTypes[i]->Icon.Icon;
		wyrmgus::icon &icon = (i != (int) Editor.ShownUnitTypes.size()) ? *Editor.ShownUnitTypes[i]->Icon.Icon : *wyrmgus::icon::get("icon-level-up");
		//Wyrmgus end
		const PixelPos pos(x, y);

		unsigned int flag = 0;
		if (i == Editor.CursorUnitIndex) {
			flag = IconActive;
			if (MouseButtons & LeftButton) {
				// Overwrite IconActive.
				flag = IconClicked;
			}
		}
		
		//Wyrmgus start
		flag |= IconCommandButton;
		//Wyrmgus end

		icon.DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[Editor.SelectedPlayer]->get_player_color(), render_commands);

		//Wyrmgus start
//		Video.DrawRectangleClip(ColorGray, x, y, icon.G->Width, icon.G->Height);
		//Wyrmgus end
		if (i == Editor.SelectedUnitIndex) {
			//Wyrmgus start
//			Video.DrawRectangleClip(ColorGreen, x + 1, y + 1,
//									icon.G->Width - 2, icon.G->Height - 2);
			Video.DrawRectangleClip(ColorGreen, x, y,
									icon.get_graphics()->Width, icon.get_graphics()->Height, render_commands);
			//Wyrmgus end
		}
		if (i == Editor.CursorUnitIndex) {
			//Wyrmgus start
//			Video.DrawRectangleClip(ColorWhite, x - 1, y - 1,
//									icon.G->Width + 2, icon.G->Height + 2);
			//Wyrmgus end
			Editor.PopUpX = x;
			Editor.PopUpY = y;
		}
		++i;
	}
	
	//Wyrmgus start
	i = Editor.UnitIndex;
	for (size_t j = 0; j < UI.ButtonPanel.Buttons.size(); ++j) {
		if (i >= (int) Editor.ShownUnitTypes.size() + 1) {
			break;
		}
		
		if (i == Editor.CursorUnitIndex) {
			if (i == (int) Editor.ShownUnitTypes.size()) {
				DrawGenericPopup("Create Unit Type", UI.ButtonPanel.Buttons[j].X, UI.ButtonPanel.Buttons[j].Y, render_commands);
			} else {
				DrawPopup(*CurrentButtons[j], UI.ButtonPanel.Buttons[j].X, UI.ButtonPanel.Buttons[j].Y, render_commands);
			}
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
static void DrawTileIcon(const wyrmgus::terrain_type *terrain, unsigned x, unsigned y, unsigned flags, std::vector<std::function<void(renderer *)>> &render_commands)
{
	//Wyrmgus start
	/*
	uint32_t color = (flags & IconActive) ? ColorGray : ColorBlack;

	Video.DrawRectangleClip(color, x, y, wyrmgus::defines::get()->get_scaled_tile_width() + 7, wyrmgus::defines::get()->get_scaled_tile_height() + 7);
	Video.DrawRectangleClip(ColorBlack, x + 1, y + 1, wyrmgus::defines::get()->get_scaled_tile_width() + 5, wyrmgus::defines::get()->get_scaled_tile_height() + 5);

	Video.DrawVLine(ColorGray, x + wyrmgus::defines::get()->get_scaled_tile_width() + 4, y + 5, wyrmgus::defines::get()->get_scaled_tile_height() - 1); // _|
	Video.DrawVLine(ColorGray, x + wyrmgus::defines::get()->get_scaled_tile_width() + 5, y + 5, wyrmgus::defines::get()->get_scaled_tile_height() - 1);
	Video.DrawHLine(ColorGray, x + 5, y + wyrmgus::defines::get()->get_scaled_tile_height() + 4, wyrmgus::defines::get()->get_scaled_tile_width() + 1);
	Video.DrawHLine(ColorGray, x + 5, y + wyrmgus::defines::get()->get_scaled_tile_height() + 5, wyrmgus::defines::get()->get_scaled_tile_width() + 1);

	color = (flags & IconClicked) ? ColorGray : ColorWhite;
	Video.DrawHLine(color, x + 5, y + 3, wyrmgus::defines::get()->get_scaled_tile_width() + 1);
	Video.DrawHLine(color, x + 5, y + 4, wyrmgus::defines::get()->get_scaled_tile_width() + 1);
	Video.DrawVLine(color, x + 3, y + 3, wyrmgus::defines::get()->get_scaled_tile_height() + 3);
	Video.DrawVLine(color, x + 4, y + 3, wyrmgus::defines::get()->get_scaled_tile_height() + 3);
	*/
	Video.DrawVLine(ColorGray, x + wyrmgus::defines::get()->get_scaled_tile_width() + 4 - 1, y + 5 - 1, wyrmgus::defines::get()->get_scaled_tile_height() - 1 - 1, render_commands); // _|
	Video.DrawVLine(ColorGray, x + wyrmgus::defines::get()->get_scaled_tile_width() + 5 - 1, y + 5 - 1, wyrmgus::defines::get()->get_scaled_tile_height() - 1 - 1, render_commands);
	Video.DrawHLine(ColorGray, x + 5 - 1, y + defines::get()->get_scaled_tile_height() + 4 - 1, wyrmgus::defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);
	Video.DrawHLine(ColorGray, x + 5 - 1, y + wyrmgus::defines::get()->get_scaled_tile_height() + 5 - 1, wyrmgus::defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);

	uint32_t color = (flags & IconClicked) ? ColorGray : ColorWhite;
	Video.DrawHLine(color, x + 5 - 1, y + 3 - 1, wyrmgus::defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);
	Video.DrawHLine(color, x + 5 - 1, y + 4 - 1, wyrmgus::defines::get()->get_scaled_tile_width() + 1 - 1, render_commands);
	Video.DrawVLine(color, x + 3 - 1, y + 3 - 1, wyrmgus::defines::get()->get_scaled_tile_height() + 3 - 1, render_commands);
	Video.DrawVLine(color, x + 4 - 1, y + 3 - 1, wyrmgus::defines::get()->get_scaled_tile_height() + 3 - 1, render_commands);
	
	color = (flags & IconActive) ? ColorGray : ColorBlack;

	Video.DrawRectangleClip(color, x, y, wyrmgus::defines::get()->get_scaled_tile_width() + 7, wyrmgus::defines::get()->get_scaled_tile_height() + 7, render_commands);
	Video.DrawRectangleClip(ColorBlack, x + 1, y + 1, wyrmgus::defines::get()->get_scaled_tile_width() + 5, wyrmgus::defines::get()->get_scaled_tile_height() + 5, render_commands);
	//Wyrmgus end

	if (flags & IconClicked) {
		++x;
		++y;
	}

	x += 4;
	y += 4;
	//Wyrmgus start
	x -= 1;
	y -= 1;
//	Map.TileGraphic->DrawFrameClip(Map.Tileset->tiles[tilenum].tile, x, y);
	terrain->get_graphics()->DrawFrameClip(terrain->get_solid_tiles().front(), x, y, render_commands);
	//Wyrmgus end

	if (flags & IconSelected) {
		Video.DrawRectangleClip(ColorGreen, x, y, wyrmgus::defines::get()->get_scaled_tile_width(), wyrmgus::defines::get()->get_scaled_tile_height(), render_commands);
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
	CLabel label(wyrmgus::defines::get()->get_game_font());
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();
	int x = UI.InfoPanel.X + 46 * scale_factor;
	int y = UI.InfoPanel.Y + 4 * scale_factor + IconHeight + 11 * scale_factor;

	if (CursorOn == cursor_on::button && 300 <= ButtonUnderCursor && ButtonUnderCursor < 306) {
		//Wyrmgus start
//		Video.DrawRectangle(ColorGray, x - 42, y - 3 + (ButtonUnderCursor - 300) * 20, 100, 20);
		if (ButtonUnderCursor <= 303) {
			Video.DrawRectangle(ColorGray, x - 42 * scale_factor, y - 3 * scale_factor + (ButtonUnderCursor - 300) * 20 * scale_factor, 100 * scale_factor, 20 * scale_factor);
		} else {
			Video.DrawRectangle(ColorGray, x + (-42 + 100) * scale_factor, y - 3 * scale_factor + (ButtonUnderCursor - 304) * 20 * scale_factor, 100 * scale_factor, 20 * scale_factor);
		}
		//Wyrmgus end
	}

	if (TileCursorSize == 1) {
		label.DrawReverseCentered(x, y, "1x1", render_commands);
	} else {
		label.DrawCentered(x, y, "1x1", render_commands);
	}
	y += 20 * scale_factor;
	if (TileCursorSize == 2) {
		label.DrawReverseCentered(x, y, "2x2", render_commands);
	} else {
		label.DrawCentered(x, y, "2x2", render_commands);
	}
	y += 20 * scale_factor;
	if (TileCursorSize == 3) {
		label.DrawReverseCentered(x, y, "3x3", render_commands);
	} else {
		label.DrawCentered(x, y, "3x3", render_commands);
	}
	y += 20 * scale_factor;
	if (TileCursorSize == 4) {
		label.DrawReverseCentered(x, y, "4x4", render_commands);
	} else {
		label.DrawCentered(x, y, "4x4", render_commands);
	}
	//Wyrmgus start
//	y += 20 * scale_factor;
	x += 100 * scale_factor;
	y -= 20 * 3 * scale_factor;
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
	y += 20 * scale_factor;
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
	y += 20 * scale_factor;

	int i = Editor.TileIndex;
	Assert(Editor.TileIndex != -1);
	y = UI.ButtonPanel.Y + 24 * scale_factor;
	while (y < UI.ButtonPanel.Y + ButtonPanelHeight - wyrmgus::defines::get()->get_scaled_tile_height()) {
		if (i >= (int)Editor.ShownTileTypes.size()) {
			break;
		}
		//Wyrmgus start
//		x = UI.ButtonPanel.X + 10 * scale_factor;
		x = UI.ButtonPanel.X + (10 + 6) * scale_factor;
		//Wyrmgus end
		while (x < UI.ButtonPanel.X + ButtonPanelWidth - wyrmgus::defines::get()->get_scaled_tile_width()) {
			if (i >= (int) Editor.ShownTileTypes.size()) {
				break;
			}
			//Wyrmgus start
//			const unsigned int tile = Editor.ShownTileTypes[i];

//			Map.TileGraphic->DrawFrameClip(tile, x, y);

			const wyrmgus::terrain_type *terrain = Editor.ShownTileTypes[i];

			if (terrain->get_graphics() && terrain->get_solid_tiles().size() > 0) {
				terrain->get_graphics()->DrawFrameClip(terrain->get_solid_tiles()[0], x, y, render_commands);
			}
			//Wyrmgus end
			Video.DrawRectangleClip(ColorGray, x, y, wyrmgus::defines::get()->get_scaled_tile_width(), wyrmgus::defines::get()->get_scaled_tile_height(), render_commands);

			if (i == Editor.SelectedTileIndex) {
				Video.DrawRectangleClip(ColorGreen, x + 1, y + 1,
					wyrmgus::defines::get()->get_scaled_tile_width() - 2, wyrmgus::defines::get()->get_scaled_tile_height() - 2, render_commands);
			}
			if (i == Editor.CursorTileIndex) {
				Video.DrawRectangleClip(ColorWhite, x - 1, y - 1,
					wyrmgus::defines::get()->get_scaled_tile_width() + 2, wyrmgus::defines::get()->get_scaled_tile_height() + 2, render_commands);
				Editor.PopUpX = x;
				Editor.PopUpY = y;
			}

			//Wyrmgus start
//			x += wyrmgus::defines::get()->get_scaled_tile_width() + 8 * scale_factor;
			x += wyrmgus::defines::get()->get_scaled_tile_width() + 30 * scale_factor; // to allow 5 tile types per row with the new UI
			//Wyrmgus end
			++i;
		}
		//Wyrmgus start
//		y += wyrmgus::defines::get()->get_scaled_tile_height() + 2 * scale_factor;
		y += wyrmgus::defines::get()->get_scaled_tile_height() + 18 * scale_factor; // make this space a little larger (as large as the space between the top of the panel and the first icon, minus the parts of the panel which are "lower" so to speak)
		//Wyrmgus end
	}
}

static void DrawEditorPanel_SelectIcon(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	//Wyrmgus start
//	const PixelPos pos(UI.InfoPanel.X + 4, UI.InfoPanel.Y + 4);
	const PixelPos pos(UI.InfoPanel.X + 11 * scale_factor, UI.InfoPanel.Y + 7 * scale_factor);
	//Wyrmgus end
	wyrmgus::icon *icon = Editor.Select.Icon;
	Assert(icon);
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
	icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[Editor.SelectedPlayer]->get_player_color(), render_commands);
}

static void DrawEditorPanel_UnitsIcon(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	const PixelPos pos(UI.InfoPanel.X + 11 * scale_factor + get_unit_icon_x(), UI.InfoPanel.Y + 7 * scale_factor + get_unit_icon_y());
	wyrmgus::icon *icon = Editor.Units.Icon;
	Assert(icon);
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
	icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[Editor.SelectedPlayer]->get_player_color(), render_commands);
}

static void DrawEditorPanel_StartIcon(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	int x = UI.InfoPanel.X + 11 * scale_factor;
	int y = UI.InfoPanel.Y + 5 * scale_factor;

	if (Editor.StartUnit) {
		wyrmgus::icon *icon = Editor.StartUnit->Icon.Icon;
		Assert(icon);
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
		
		icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[Editor.SelectedPlayer]->get_player_color(), render_commands);
	} else {
		//  No unit specified : draw a cross.
		//  Todo : FIXME Should we just warn user to define Start unit ?
		PushClipping();
		x += get_start_icon_x() + 1 * scale_factor;
		y += get_start_icon_y() + 1 * scale_factor;
		if (ButtonUnderCursor == StartButton) {
			Video.DrawRectangleClip(ColorGray, x - 1 * scale_factor, y - 1 * scale_factor, IconHeight, IconHeight, render_commands);
		}
		Video.FillRectangleClip(ColorBlack, x, y, IconHeight - 2 * scale_factor, IconHeight - 2 * scale_factor, render_commands);

		const PixelPos lt(x, y);
		const PixelPos lb(x, y + IconHeight - 3 * scale_factor);
		const PixelPos rt(x + IconHeight - 3 * scale_factor, y);
		const PixelPos rb(x + IconHeight - 2 * scale_factor, y + IconHeight - 2 * scale_factor);
		const uint32_t color = CVideo::MapRGB(CPlayer::Players[Editor.SelectedPlayer]->get_minimap_color());

		Video.DrawLineClip(color, lt, rb);
		Video.DrawLineClip(color, rt, lb);
		PopClipping();
	}
}

/**
**  Draw the editor panels.
*/
static void DrawEditorPanel(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	DrawEditorPanel_SelectIcon(render_commands);
	DrawEditorPanel_UnitsIcon(render_commands);

	if (Editor.TerrainEditable) {
		const int x = UI.InfoPanel.X + get_tile_icon_x() + 11 * scale_factor;
		const int y = UI.InfoPanel.Y + get_tile_icon_y() + 4 * scale_factor;

		DrawTileIcon(Editor.ShownTileTypes[0], x, y,
					 (ButtonUnderCursor == TileButton ? IconActive : 0) |
					 (Editor.State == EditorEditTile ? IconSelected : 0), render_commands);
	}
	DrawEditorPanel_StartIcon(render_commands);

	switch (Editor.State) {
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
		switch (Editor.State) {
			case EditorSelecting:
			case EditorEditTile:
				break;
			case EditorEditUnit:
				if (Editor.SelectedUnitIndex != -1) {
					CursorBuilding = Editor.ShownUnitTypes[Editor.SelectedUnitIndex];
				}
				break;
			case EditorSetStartLocation:
				if (Editor.StartUnit) {
					CursorBuilding = Editor.StartUnit;
				}
				break;
		}
	}

	// Draw map cursor
	if (UI.MouseViewport && !CursorBuilding) {
		const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
		const PixelPos screenPos = UI.MouseViewport->TilePosToScreen_TopLeft(tilePos);

		if (Editor.State == EditorEditTile && Editor.SelectedTileIndex != -1) {
			//Wyrmgus start
//			const unsigned short tile = Editor.ShownTileTypes[Editor.SelectedTileIndex];
			const wyrmgus::terrain_type *terrain = Editor.ShownTileTypes[Editor.SelectedTileIndex];
			//Wyrmgus end
			PushClipping();
			UI.MouseViewport->SetClipping();

			PixelPos screenPosIt;
			for (int j = 0; j < TileCursorSize; ++j) {
				screenPosIt.y = screenPos.y + j * wyrmgus::defines::get()->get_scaled_tile_height();
				if (screenPosIt.y >= UI.MouseViewport->GetBottomRightPos().y) {
					break;
				}
				for (int i = 0; i < TileCursorSize; ++i) {
					screenPosIt.x = screenPos.x + i * wyrmgus::defines::get()->get_scaled_tile_width();
					if (screenPosIt.x >= UI.MouseViewport->GetBottomRightPos().x) {
						break;
					}
					//Wyrmgus start
//					Map.TileGraphic->DrawFrameClip(tile, screenPosIt.x, screenPosIt.y);

					if (terrain->get_graphics() && terrain->get_solid_tiles().size() > 0) {
						terrain->get_graphics()->DrawFrameClip(terrain->get_solid_tiles()[0], screenPosIt.x, screenPosIt.y, render_commands);
					}
					//Wyrmgus end
				}
			}
			Video.DrawRectangleClip(ColorWhite, screenPos.x, screenPos.y, wyrmgus::defines::get()->get_scaled_tile_width() * TileCursorSize, wyrmgus::defines::get()->get_scaled_tile_height() * TileCursorSize, render_commands);
			PopClipping();
		} else {
			// If there is an unit under the cursor, it's selection thing
			//  is drawn somewhere else (Check DrawUnitSelection.)
			if (UnitUnderCursor != nullptr) {
				PushClipping();
				UI.MouseViewport->SetClipping();
				Video.DrawRectangleClip(ColorWhite, screenPos.x, screenPos.y, wyrmgus::defines::get()->get_scaled_tile_width(), wyrmgus::defines::get()->get_scaled_tile_height(), render_commands);
				PopClipping();
			}
		}
	}
}

static void DrawCross(const PixelPos &topleft_pos, const QSize &size, uint32_t color)
{
	const PixelPos lt = topleft_pos;
	const PixelPos lb(topleft_pos.x, topleft_pos.y + size.height());
	const PixelPos rt(topleft_pos.x + size.width(), topleft_pos.y);
	const PixelPos rb = topleft_pos + size;

	Video.DrawLineClip(color, lt, rb);
	Video.DrawLineClip(color, lb, rt);
}

/**
**  Draw the start locations of all active players on the map
*/
static void DrawStartLocations(std::vector<std::function<void(renderer *)>> &render_commands)
{
	const wyrmgus::unit_type *type = Editor.StartUnit;
	for (const CViewport *vp = UI.Viewports; vp < UI.Viewports + UI.NumViewports; ++vp) {
		PushClipping();
		vp->SetClipping();

		for (int i = 0; i < PlayerMax; i++) {
			if (CMap::get()->Info.PlayerType[i] != PlayerNobody && CMap::get()->Info.PlayerType[i] != PlayerNeutral && CPlayer::Players[i]->StartMapLayer == UI.CurrentMapLayer->ID) {
				const PixelPos startScreenPos = vp->TilePosToScreen_TopLeft(CPlayer::Players[i]->StartPos);

				if (type) {
					DrawUnitType(*type, type->Sprite, i, 0, startScreenPos, nullptr, render_commands);
				} else { // Draw a cross
					DrawCross(startScreenPos, wyrmgus::defines::get()->get_scaled_tile_size(), CVideo::MapRGB(CPlayer::Players[i]->get_minimap_color()));
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
#if 1
	Vec2i pos(0, 0);

	if (UI.MouseViewport) {
		pos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
	}

	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	std::array<char, 256> buf{};
	snprintf(buf.data(), buf.size(), _("Editor (%d %d)"), pos.x, pos.y);
	CLabel(wyrmgus::defines::get()->get_game_font()).Draw(UI.StatusLine.TextX + 2 * scale_factor, UI.StatusLine.TextY - 12 * scale_factor, buf.data(), render_commands);
	const wyrmgus::tile &mf = *UI.CurrentMapLayer->Field(pos);
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
			mf.has_flag(tile_flag::land_unit) ? 'l' : '-',
			mf.has_flag(tile_flag::air_unit) ? 'a' : '-',
			mf.has_flag(tile_flag::sea_unit) ? 's' : '-',
			mf.has_flag(tile_flag::building) ? 'b' : '-',
			mf.has_flag(tile_flag::air_building) ? 'I' : '-',
			mf.has_flag(tile_flag::item) ? 'i' : '-',
			mf.has_flag(tile_flag::stumps) ? 't' : '-',
			mf.has_flag(tile_flag::bridge) ? 'B' : '-',
			mf.has_flag(tile_flag::space) ? 'S' : '-');

	CLabel(defines::get()->get_game_font()).Draw(UI.StatusLine.TextX + 118 * scale_factor, UI.StatusLine.TextY - 12 * scale_factor, buf.data(), render_commands);

	//Wyrmgus start
//	const int index = tileset.findTileIndexByTile(mf.getGraphicTile());
//	Assert(index != -1);
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
	//Wyrmgus start
//	CLabel(wyrmgus::defines::get()->get_game_font()).Draw(UI.StatusLine.TextX + 250, UI.StatusLine.TextY - 16, buf);
	CLabel(wyrmgus::defines::get()->get_game_font()).Draw(UI.StatusLine.TextX + 298 * scale_factor, UI.StatusLine.TextY - 12 * scale_factor, buf.data(), render_commands);
	//Wyrmgus end
#endif
}

/**
**  Show info about unit.
**
**  @param unit  Unit pointer.
*/
static void ShowUnitInfo(const CUnit &unit)
{
	std::string str = "#" + std::to_string(UnitNumber(unit)) + " '" + unit.get_type_name() + "' Player: #" + std::to_string((unit.Player->Index == PlayerNumNeutral) ? 16 : unit.Player->Index + 1) + " " + (unit.Active ? "active" : "passive");

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

	if (CursorOn == cursor_on::map && Gui->getTop() == editorContainer.get() && !GamePaused) {
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
	//Wyrmgus start
//	const int flag_clicked = GameMenuButtonClicked ? MI_FLAGS_CLICKED : 0;
	const int flag_clicked = UI.MenuButton.Clicked ? MI_FLAGS_CLICKED : 0;
	//Wyrmgus end
	DrawUIButton(UI.MenuButton.Style,
				 flag_active | flag_clicked,
				 UI.MenuButton.X, UI.MenuButton.Y,
				 UI.MenuButton.Text, render_commands);


	// Minimap
	if (UI.SelectedViewport) {
		UI.get_minimap()->Draw(render_commands);
		UI.get_minimap()->DrawViewportArea(*UI.SelectedViewport);
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

	// refresh entire screen, so no further invalidate needed
	RealizeVideoMemory();
}

/*----------------------------------------------------------------------------
--  Input / Keyboard / Mouse
----------------------------------------------------------------------------*/

/**
**  Callback for input.
*/
static void EditorCallbackButtonUp(unsigned button)
{
	if (cursor::get_current_cursor() == UI.get_cursor(cursor_type::scroll)) {
		// Move map.
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), false); // Reset
		return;
	}

	//Wyrmgus start
//	if ((1 << button) == LeftButton && GameMenuButtonClicked) {
	if ((1 << button) == LeftButton && UI.MenuButton.Clicked) {
	//Wyrmgus end
		//Wyrmgus start
//		GameMenuButtonClicked = false;
		UI.MenuButton.Clicked = false;
		//Wyrmgus end
		if (ButtonUnderCursor == ButtonUnderMenu) {
			if (UI.MenuButton.Callback) {
				UI.MenuButton.Callback->action("");
			}
		}
	}
	if ((1 << button) == LeftButton) {
		UnitPlacedThisPress = false;
	}
	//Wyrmgus start
	if (Editor.State == EditorEditUnit) {
		if (Editor.CursorUnitIndex != -1) {
			if (Editor.CursorUnitIndex == (int) Editor.ShownUnitTypes.size()) {
				if ((1 << button) == LeftButton) {
					Editor.SelectedUnitIndex = -1;
					Editor.CursorUnitIndex = -1;
					CursorBuilding = nullptr;
					std::array<char, 256> buf{};
					snprintf(buf.data(), buf.size(), "if (EditorCreateUnitType() ~= nil) then EditorCreateUnitType() end;");
					CclCommand(buf.data());
					return;
				}
			}
		}
	}
	//Wyrmgus end
}

/**
**  Called if mouse button pressed down.
**
**  @param button  Mouse button number (0 left, 1 middle, 2 right)
*/
static void EditorCallbackButtonDown(unsigned button)
{
	if (GamePaused) {
		return;
	}
	if ((button >> MouseHoldShift) != 0) {
		// Ignore repeated events when holding down a button
		return;
	}
	// Click on menu button
	if (CursorOn == cursor_on::button && ButtonAreaUnderCursor == ButtonAreaMenu &&
		//Wyrmgus start
//		(MouseButtons & LeftButton) && !GameMenuButtonClicked) {
		(MouseButtons & LeftButton) && !UI.MenuButton.Clicked) {
		//Wyrmgus end
		PlayGameSound(wyrmgus::game_sound_set::get()->get_click_sound(), MaxSampleVolume);
		//Wyrmgus start
//		GameMenuButtonClicked = true;
		UI.MenuButton.Clicked = true;
		//Wyrmgus end
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
				Editor.State = EditorSelecting;
				editorUnitSlider->setVisible(false);
				editorSlider->setVisible(false);
				return;
			case UnitButton:
				Editor.State = EditorEditUnit;
				if (VisibleUnitIcons < (int)Editor.ShownUnitTypes.size()) {
					editorUnitSlider->setVisible(true);
				}
				editorSlider->setVisible(false);
				return;
			case TileButton :
				Editor.State = EditorEditTile;
				editorUnitSlider->setVisible(false);
				if (VisibleTileIcons < (int)Editor.ShownTileTypes.size()) {
					editorSlider->setVisible(true);
				}
				return;
			case StartButton:
				Editor.State = EditorSetStartLocation;
				editorUnitSlider->setVisible(false);
				editorSlider->setVisible(false);
				return;
			default:
				break;
		}
	}
	// Click on tile area
	if (Editor.State == EditorEditTile) {
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
		if (Editor.CursorTileIndex != -1) {
			Editor.SelectedTileIndex = Editor.CursorTileIndex;
			return;
		}
	}

	// Click on player area
	if (Editor.State == EditorEditUnit || Editor.State == EditorSetStartLocation) {
		// Cursor on player icons
		if (Editor.CursorPlayer != -1) {
			if (CMap::get()->Info.PlayerType[Editor.CursorPlayer] != PlayerNobody) {
				Editor.SelectedPlayer = Editor.CursorPlayer;
				CPlayer::SetThisPlayer(CPlayer::Players[Editor.SelectedPlayer]);
			}
			return;
		}
	}

	// Click on unit area
	if (Editor.State == EditorEditUnit) {
		// Cursor on unit icons
		if (Editor.CursorUnitIndex != -1) {
			if (Editor.CursorUnitIndex != (int) Editor.ShownUnitTypes.size()) {
				if (MouseButtons & LeftButton) {
					Editor.SelectedUnitIndex = Editor.CursorUnitIndex;
					CursorBuilding = Editor.ShownUnitTypes[Editor.CursorUnitIndex];
					return;
				} else if (MouseButtons & RightButton) {
					std::array<char, 256> buf{};
					snprintf(buf.data(), buf.size(), "if (EditUnitTypeProperties ~= nil) then EditUnitTypeProperties(\"%s\") end;", Editor.ShownUnitTypes[Editor.CursorUnitIndex]->Ident.c_str());
					Editor.CursorUnitIndex = -1;
					CclCommand(buf.data());
					return;
				}
			}
			//Wyrmgus end
		}
	}

	// Right click on a resource
	if (Editor.State == EditorSelecting) {
		if ((MouseButtons & RightButton) && UnitUnderCursor != nullptr) {
			CclCommand("if (EditUnitProperties ~= nil) then EditUnitProperties() end;");
			return;
		}
	}

	// Click on map area
	if (CursorOn == cursor_on::map) {
		if (MouseButtons & RightButton) {
			if (Editor.State == EditorEditUnit && Editor.SelectedUnitIndex != -1) {
				Editor.SelectedUnitIndex = -1;
				CursorBuilding = nullptr;
				return;
			} else if (Editor.State == EditorEditTile && Editor.SelectedTileIndex != -1) {
				Editor.SelectedTileIndex = -1;
				CursorBuilding = nullptr;
				return;
			}
		}

		CViewport *vp = GetViewport(CursorScreenPos);
		Assert(vp);
		if ((MouseButtons & LeftButton) && UI.SelectedViewport != vp) {
			// viewport changed
			UI.SelectedViewport = vp;
		}
		if (MouseButtons & LeftButton) {
			const Vec2i tilePos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);

			if (Editor.State == EditorEditTile &&
				Editor.SelectedTileIndex != -1) {
				EditTiles(tilePos, Editor.ShownTileTypes[Editor.SelectedTileIndex], TileCursorSize);
			} else if (Editor.State == EditorEditUnit) {
				if (!UnitPlacedThisPress && CursorBuilding) {
					if (CanBuildUnitType(nullptr, *CursorBuilding, tilePos, 1, true, UI.CurrentMapLayer->ID)) {
						PlayGameSound(wyrmgus::game_sound_set::get()->get_placement_success_sound(), MaxSampleVolume);
						EditorPlaceUnit(tilePos, *CursorBuilding, CPlayer::Players[Editor.SelectedPlayer]);
						UnitPlacedThisPress = true;
						UI.StatusLine.Clear();
					} else {
						UI.StatusLine.Set(_("Unit cannot be placed here."));
						PlayGameSound(wyrmgus::game_sound_set::get()->get_placement_error_sound(), MaxSampleVolume);
					}
				}
			} else if (Editor.State == EditorSetStartLocation) {
				CPlayer::Players[Editor.SelectedPlayer]->StartPos = tilePos;
				CPlayer::Players[Editor.SelectedPlayer]->StartMapLayer = UI.CurrentMapLayer->ID;
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
static void EditorCallbackKeyDown(unsigned key, unsigned keychar)
{
	if (HandleKeyModifiersDown(key, keychar)) {
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
		case 'f': // ALT+F, CTRL+F toggle fullscreen
			if (!(KeyModifiers & (ModifierAlt | ModifierControl))) {
				break;
			}
			ToggleFullScreen();
			break;

		case 'v': // 'v' Viewport
			if (KeyModifiers & ModifierControl) {
				CycleViewportMode(-1);
			} else {
				CycleViewportMode(1);
			}
			break;

		// FIXME: move to lua
		case 'm': // CTRL+M Mirror edit
			if (KeyModifiers & ModifierControl)  {
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
				HandleCommandKey(key);
				return;
			//Wyrmgus end
			}
			break;
		case 'x': // ALT+X, CTRL+X: Exit editor
			if (!(KeyModifiers & (ModifierAlt | ModifierControl))) {
				break;
			}
			Exit(0);

		case 'z':
			if (KeyModifiers & ModifierControl) {
				EditorUndoAction();
			}
			break;
		case 'y':
			if (KeyModifiers & ModifierControl) {
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
		case SDLK_KP8:
			KeyScrollState |= ScrollUp;
			break;
		case SDLK_DOWN:
		case SDLK_KP2:
			KeyScrollState |= ScrollDown;
			break;
		case SDLK_LEFT:
		case SDLK_KP4:
			KeyScrollState |= ScrollLeft;
			break;
		case SDLK_RIGHT:
		case SDLK_KP6:
			KeyScrollState |= ScrollRight;
			break;
		case '0':
			if (UnitUnderCursor != nullptr) {
				UnitUnderCursor->ChangeOwner(*CPlayer::Players[PlayerNumNeutral]);
				UI.StatusLine.Set(_("Unit owner modified"));
			}
			break;
		case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9':
			if (UnitUnderCursor != nullptr && CMap::get()->Info.PlayerType[(int) key - '1'] != PlayerNobody) {
				UnitUnderCursor->ChangeOwner(*CPlayer::Players[(int) key - '1']);
				UI.StatusLine.Set(_("Unit owner modified"));
				UpdateMinimap = true;
			}
			break;

		default:
			HandleCommandKey(key);
			return;
	}
}

/**
**  Handle key up.
**
**  @param key      Key scancode.
**  @param keychar  Character code.
*/
static void EditorCallbackKeyUp(unsigned key, unsigned keychar)
{
	if (HandleKeyModifiersUp(key, keychar)) {
		return;
	}

	switch (key) {
		case SDLK_UP: // Keyboard scrolling
		case SDLK_KP8:
			KeyScrollState &= ~ScrollUp;
			break;
		case SDLK_DOWN:
		case SDLK_KP2:
			KeyScrollState &= ~ScrollDown;
			break;
		case SDLK_LEFT:
		case SDLK_KP4:
			KeyScrollState &= ~ScrollLeft;
			break;
		case SDLK_RIGHT:
		case SDLK_KP6:
			KeyScrollState &= ~ScrollRight;
			break;
		default:
			break;
	}
}

/**
**  Callback for input.
*/
static void EditorCallbackKeyRepeated(unsigned key, unsigned)
{
	switch (key) {
		case 'z':
			if (KeyModifiers & ModifierControl) {
				EditorUndoAction();
			}
			break;
		case 'y':
			if (KeyModifiers & ModifierControl) {
				EditorRedoAction();
			}
			break;
	}
}

static bool EditorCallbackMouse_EditUnitArea(const PixelPos &screenPos)
{
	Assert(Editor.State == EditorEditUnit || Editor.State == EditorSetStartLocation);
	
	//Wyrmgus start
	LastDrawnButtonPopup = nullptr;
	//Wyrmgus end

	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	// Scrollbar
	if (UI.ButtonPanel.X + 4 * scale_factor < CursorScreenPos.x
		&& CursorScreenPos.x < UI.ButtonPanel.X + (176 - 4) * scale_factor
		&& UI.ButtonPanel.Y + 4 * scale_factor < CursorScreenPos.y
		//Wyrmgus start
//		&& CursorScreenPos.y < UI.ButtonPanel.Y + 24) {
		&& CursorScreenPos.y < UI.ButtonPanel.Y) {
		//Wyrmgus end
		return true;
	}
	//Wyrmgus start
//	int bx = UI.InfoPanel.X + 8;
	int bx = UI.InfoPanel.X + 26 * scale_factor;
	//Wyrmgus end
	int by = UI.InfoPanel.Y + 4 * scale_factor + IconHeight + 10 * scale_factor;
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
			bx = UI.InfoPanel.X + 26 * scale_factor;
			//Wyrmgus end
			by += 20 * scale_factor;
		}
		if (bx < screenPos.x && screenPos.x < bx + 20 * scale_factor && by < screenPos.y && screenPos.y < by + 20 * scale_factor) {
			if (CMap::get()->Info.PlayerType[i] != PlayerNobody) {
				std::array<char, 256> buf{};
				//Wyrmgus start
//				snprintf(buf.data(), buf.size(), _("Select Player #%d"), i);
				snprintf(buf.data(), buf.size(), _("Select Player #%d"), (i == PlayerNumNeutral) ? 16 : i + 1);
				//Wyrmgus end
				UI.StatusLine.Set(buf.data());
			} else {
				UI.StatusLine.Clear();
			}
			Editor.CursorPlayer = i;
#if 0
			ButtonUnderCursor = i + 100;
			CursorOn = cursor_on::button;
#endif
			return true;
		}
		bx += 20 * scale_factor;
	}

	int i = Editor.UnitIndex;
	by = UI.ButtonPanel.Y + 24 * scale_factor;
	for (size_t j = 0; j < UI.ButtonPanel.Buttons.size(); ++j) {
		const int x = UI.ButtonPanel.Buttons[j].X;
		const int y = UI.ButtonPanel.Buttons[j].Y;
		//Wyrmgus start
//		if (i >= (int) Editor.ShownUnitTypes.size()) {
		if (i >= (int) Editor.ShownUnitTypes.size() + 1) {
		//Wyrmgus end
			break;
		}
		//Wyrmgus start
		if (i < (int) Editor.ShownUnitTypes.size()) {
			if (j >= CurrentButtons.size()) {
				CurrentButtons.push_back(std::make_unique<wyrmgus::button>());
			}
			CurrentButtons[j]->Hint = Editor.ShownUnitTypes[i]->get_name();
			CurrentButtons[j]->pos = j;
			CurrentButtons[j]->level = nullptr;
			CurrentButtons[j]->Action = ButtonCmd::EditorUnit;
			CurrentButtons[j]->ValueStr = Editor.ShownUnitTypes[i]->Ident;
			CurrentButtons[j]->Value = Editor.ShownUnitTypes[i]->Slot;
			CurrentButtons[j]->Popup = "popup_unit";
		}
		//Wyrmgus end
		if (x < screenPos.x && screenPos.x < x + IconWidth
			&& y < screenPos.y && screenPos.y < y + IconHeight) {
			Editor.CursorUnitIndex = i;
			return true;
		}
		++i;
	}
	return false;
}

static bool EditorCallbackMouse_EditTileArea(const PixelPos &screenPos)
{
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	//Wyrmgus start
//	int bx = UI.InfoPanel.X + 4;
	int bx = UI.InfoPanel.X + 11 * scale_factor;
	//Wyrmgus end
	int by = UI.InfoPanel.Y + 4 * scale_factor + IconHeight + 10 * scale_factor;

	for (int i = 0; i < 6; ++i) {
		if (bx < screenPos.x && screenPos.x < bx + 100 * scale_factor && by < screenPos.y && screenPos.y < by + 18 * scale_factor) {
			ButtonUnderCursor = i + 300;
			CursorOn = cursor_on::button;
			return true;
		}
		by += 20 * scale_factor;
		//Wyrmgus start
		if (i == 3) {
			by = UI.InfoPanel.Y + 4 * scale_factor + IconHeight + 10 * scale_factor;
			bx += 100 * scale_factor;
		}
		//Wyrmgus end
	}

	int i = Editor.TileIndex;
	by = UI.ButtonPanel.Y + 24 * scale_factor;
	while (by < UI.ButtonPanel.Y + ButtonPanelHeight - wyrmgus::defines::get()->get_scaled_tile_height()) {
		if (i >= (int)Editor.ShownTileTypes.size()) {
			break;
		}
		//Wyrmgus start
//		bx = UI.ButtonPanel.X + 10;
		bx = UI.ButtonPanel.X + (10 + 6) * scale_factor;
		//Wyrmgus end
		while (bx < UI.ButtonPanel.X + ButtonPanelWidth - wyrmgus::defines::get()->get_scaled_tile_width()) {
			if (i >= (int)Editor.ShownTileTypes.size()) {
				break;
			}
			if (bx < screenPos.x && screenPos.x < bx + wyrmgus::defines::get()->get_scaled_tile_width()
				&& by < screenPos.y && screenPos.y < by + wyrmgus::defines::get()->get_scaled_tile_height()) {
				//Wyrmgus start
//				const int tile = Editor.ShownTileTypes[i];
//				const int tileindex = Map.Tileset->findTileIndexByTile(tile);
//				const int base = Map.Tileset->tiles[tileindex].tileinfo.BaseTerrain;
//				UI.StatusLine.Set(Map.Tileset->getTerrainName(base));
				UI.StatusLine.Set(Editor.ShownTileTypes[i]->get_name());
				//Wyrmgus end
				Editor.CursorTileIndex = i;
				return true;
			}
			//Wyrmgus start
//			bx += wyrmgus::defines::get()->get_scaled_tile_width() + 8;
			bx += wyrmgus::defines::get()->get_scaled_tile_width() + 30 * scale_factor;
			//Wyrmgus end
			i++;
		}
		//Wyrmgus start
//		by += wyrmgus::defines::get()->get_scaled_tile_height() + 2;
		by += wyrmgus::defines::get()->get_scaled_tile_height() + 18 * scale_factor;
		//Wyrmgus end
	}
	return false;
}

/**
**  Callback for input movement of the cursor.
**
**  @param pos  Screen position.
*/
static void EditorCallbackMouse(const PixelPos &pos)
{
	static int LastMapX = 0;
	static int LastMapY = 0;

	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

	PixelPos restrictPos = pos;
	HandleCursorMove(&restrictPos.x, &restrictPos.y); // Reduce to screen
	const PixelPos screenPos = pos;

	// Move map.
	if (cursor::get_current_cursor() == UI.get_cursor(cursor_type::scroll)) {
		Vec2i tilePos = UI.MouseViewport->MapPos;

		// FIXME: Support with CTRL for faster scrolling.
		// FIXME: code duplication, see ../ui/mouse.c
		if (UI.MouseScrollSpeedDefault < 0) {
			if (screenPos.x < CursorStartScreenPos.x) {
				tilePos.x++;
			} else if (screenPos.x > CursorStartScreenPos.x) {
				tilePos.x--;
			}
			if (screenPos.y < CursorStartScreenPos.y) {
				tilePos.y++;
			} else if (screenPos.y > CursorStartScreenPos.y) {
				tilePos.y--;
			}
		} else {
			if (screenPos.x < CursorStartScreenPos.x) {
				tilePos.x--;
			} else if (screenPos.x > CursorStartScreenPos.x) {
				tilePos.x++;
			}
			if (screenPos.y < CursorStartScreenPos.y) {
				tilePos.y--;
			} else if (screenPos.y > CursorStartScreenPos.y) {
				tilePos.y++;
			}
		}
		UI.MouseWarpPos = CursorStartScreenPos;
		UI.MouseViewport->Set(tilePos, wyrmgus::defines::get()->get_scaled_tile_size() / 2);
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
		&& (Editor.State == EditorEditTile || Editor.State == EditorEditUnit)) {
		Vec2i vpTilePos = UI.SelectedViewport->MapPos;
		// Scroll the map
		if (CursorScreenPos.x <= UI.SelectedViewport->GetTopLeftPos().x) {
			vpTilePos.x--;
			UI.SelectedViewport->Set(vpTilePos, wyrmgus::defines::get()->get_scaled_tile_size() / 2);
		} else if (CursorScreenPos.x >= UI.SelectedViewport->GetBottomRightPos().x) {
			vpTilePos.x++;
			UI.SelectedViewport->Set(vpTilePos, wyrmgus::defines::get()->get_scaled_tile_size() / 2);
		}

		if (CursorScreenPos.y <= UI.SelectedViewport->GetTopLeftPos().y) {
			vpTilePos.y--;
			UI.SelectedViewport->Set(vpTilePos, wyrmgus::defines::get()->get_scaled_tile_size() / 2);
		} else if (CursorScreenPos.y >= UI.SelectedViewport->GetBottomRightPos().y) {
			vpTilePos.y++;
			UI.SelectedViewport->Set(vpTilePos, wyrmgus::defines::get()->get_scaled_tile_size() / 2);
		}

		// Scroll the map, if cursor moves outside the viewport.
		RestrictCursorToViewport();
		const Vec2i tilePos = UI.SelectedViewport->ScreenToTilePos(CursorScreenPos);

		if (Editor.State == EditorEditTile && Editor.SelectedTileIndex != -1) {
			EditTiles(tilePos, Editor.ShownTileTypes[Editor.SelectedTileIndex], TileCursorSize);
		} else if (Editor.State == EditorEditUnit && CursorBuilding) {
			if (!UnitPlacedThisPress) {
				if (CanBuildUnitType(nullptr, *CursorBuilding, tilePos, 1, true, UI.CurrentMapLayer->ID)) {
					EditorPlaceUnit(tilePos, *CursorBuilding, CPlayer::Players[Editor.SelectedPlayer]);
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
	Editor.CursorPlayer = -1;
	Editor.CursorUnitIndex = -1;
	Editor.CursorTileIndex = -1;
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;

	// Minimap
	if (UI.get_minimap()->Contains(screenPos)) {
		CursorOn = cursor_on::minimap;
	}
	// Handle edit unit area
	if (Editor.State == EditorEditUnit || Editor.State == EditorSetStartLocation) {
		if (EditorCallbackMouse_EditUnitArea(screenPos) == true) {
			return;
		}
	}

	// Handle tile area
	if (Editor.State == EditorEditTile) {
		if (EditorCallbackMouse_EditTileArea(screenPos) == true) {
			return;
		}
	}

	// Handle buttons
	if (UI.InfoPanel.X + 11 * scale_factor < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + 11 * scale_factor + Editor.Select.Icon->get_graphics()->Width
		&& UI.InfoPanel.Y + 7 * scale_factor < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + 7 * scale_factor + Editor.Select.Icon->get_graphics()->Height) {
		// FIXME: what is this button?
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = SelectButton;
		CursorOn = cursor_on::button;
		UI.StatusLine.Set(_("Select Mode"));
		return;
	}
	if (UI.InfoPanel.X + 11 * scale_factor + get_unit_icon_x() < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + 11 * scale_factor + get_unit_icon_x() + Editor.Units.Icon->get_graphics()->Width
		&& UI.InfoPanel.Y + 7 * scale_factor + get_unit_icon_y() < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + 7 * scale_factor + get_unit_icon_y() + Editor.Units.Icon->get_graphics()->Height) {
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = UnitButton;
		CursorOn = cursor_on::button;
		UI.StatusLine.Set(_("Unit Mode"));
		return;
	}
	if (Editor.TerrainEditable) {
		if (UI.InfoPanel.X + 11 * scale_factor + get_tile_icon_x() < CursorScreenPos.x
			&& CursorScreenPos.x < UI.InfoPanel.X + 11 * scale_factor + get_tile_icon_x() + wyrmgus::defines::get()->get_scaled_tile_width() + 7 * scale_factor
			&& UI.InfoPanel.Y + 4 * scale_factor + get_tile_icon_y() < CursorScreenPos.y
			&& CursorScreenPos.y < UI.InfoPanel.Y + 4 * scale_factor + get_tile_icon_y() + wyrmgus::defines::get()->get_scaled_tile_height() + 7 * scale_factor) {
			ButtonAreaUnderCursor = -1;
			ButtonUnderCursor = TileButton;
			CursorOn = cursor_on::button;
			UI.StatusLine.Set(_("Tile Mode"));
			return;
		}
	}

	int StartUnitWidth = Editor.StartUnit ? Editor.StartUnit->Icon.Icon->get_graphics()->Width : wyrmgus::defines::get()->get_scaled_tile_width() + 7 * scale_factor;
	int StartUnitHeight = Editor.StartUnit ? Editor.StartUnit->Icon.Icon->get_graphics()->Height : wyrmgus::defines::get()->get_scaled_tile_height() + 7 * scale_factor;
	if (UI.InfoPanel.X + 11 * scale_factor + get_start_icon_x() < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + 11 * scale_factor + get_start_icon_x() + StartUnitWidth
		&& UI.InfoPanel.Y + 5 * scale_factor + get_start_icon_y() < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + 5 * scale_factor + get_start_icon_y() + StartUnitHeight) {
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = StartButton;
		CursorOn = cursor_on::button;
		UI.StatusLine.Set(_("Set Start Location"));
		return;
	}
	if (UI.MenuButton.X != -1 && UI.MenuButton.Contains(screenPos)) {
		ButtonAreaUnderCursor = ButtonAreaMenu;
		ButtonUnderCursor = ButtonUnderMenu;
		CursorOn = cursor_on::button;
		return;
	}

	// Minimap
	if (UI.get_minimap()->Contains(screenPos)) {
		CursorOn = cursor_on::minimap;
		return;
	}

	// Map
	UnitUnderCursor = nullptr;
	if (UI.MapArea.Contains(screenPos)) {
		CViewport *vp = GetViewport(screenPos);
		Assert(vp);
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

	ShowLoadProgress(_("Loading Script \"%s\""), filename.c_str());
	LuaLoadFile(filename);
	LuaGarbageCollect();
	
	//Wyrmgus start
	if (this->UnitTypes.size() == 0) {
		//if editor's unit types vector is still empty after loading the editor's lua file, then fill it automatically
		for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			if (unit_type->is_template()) {
				continue;
			}

			if (unit_type->Icon.Name.empty() || unit_type->BoolFlag[VANISHES_INDEX].value || unit_type->BoolFlag[HIDDENINEDITOR_INDEX].value) {
				continue;
			}
			
			this->UnitTypes.push_back(unit_type->Ident);
		}
	}
	//Wyrmgus end

	CPlayer::SetThisPlayer(CPlayer::Players[0]);

	FlagRevealMap = 1; // editor without fog and all visible
	CMap::get()->NoFogOfWar = true;

	//Wyrmgus start
//	if (!*CurrentMapPath) { // new map!
	if (CurrentMapPath.empty() || IsMod) { // new map or is a mod
	//Wyrmgus end
		InitUnitTypes(1);
		//
		// Inititialize Map / Players.
		//
		InitPlayers();
		for (int i = 0; i < PlayerMax; ++i) {
			if (i == PlayerNumNeutral) {
				CreatePlayer(PlayerNeutral);
				CMap::get()->Info.PlayerType[i] = PlayerNeutral;
				//Wyrmgus start
//				CMap::get()->Info.PlayerSide[i] = CPlayer::Players[i]->Race = 0;
				CPlayer::Players[i]->set_civilization(wyrmgus::defines::get()->get_neutral_civilization());
				CMap::get()->Info.PlayerSide[i] = CPlayer::Players[i]->Race;
				//Wyrmgus end
			} else {
				CreatePlayer(PlayerNobody);
				CMap::get()->Info.PlayerType[i] = PlayerNobody;
			}
		}

		//Wyrmgus start
//		Map.Fields = new wyrmgus::tile[Map.Info.MapWidth * Map.Info.MapHeight];
		CMap::get()->ClearMapLayers();
		auto new_map_layer = std::make_unique<CMapLayer>(CMap::get()->Info.MapWidth, CMap::get()->Info.MapHeight);

		if (QApplication::instance()->thread() != QThread::currentThread()) {
			new_map_layer->moveToThread(QApplication::instance()->thread());
		}

		new_map_layer->ID = CMap::get()->MapLayers.size();
		CMap::get()->Info.MapWidths.clear();
		CMap::get()->Info.MapWidths.push_back(CMap::get()->Info.MapWidth);
		CMap::get()->Info.MapHeights.clear();
		CMap::get()->Info.MapHeights.push_back(CMap::get()->Info.MapHeight);
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
		CreateGame("", CMap::get(), IsMod);
	//Wyrmgus end
	} else {
		CreateGame(CurrentMapPath, CMap::get(), IsMod);
	}

	ReplayRevealMap = 1;
	FlagRevealMap = 0;
	Editor.SelectedPlayer = PlayerNumNeutral;

	// Place the start points, which the loader discarded.
	for (int i = 0; i < PlayerMax; ++i) {
		if (CMap::get()->Info.PlayerType[i] != PlayerNobody) {
			// Set SelectedPlayer to a valid player
			if (Editor.SelectedPlayer == PlayerNumNeutral) {
				Editor.SelectedPlayer = i;
				break;
			}
		}
	}
	//Wyrmgus start
//	ButtonPanelWidth = 170;//200;
//	ButtonPanelHeight = 160 + (Video.Height - 480);
	// adapt to new UI size, should make this more scriptable
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();
	ButtonPanelWidth = 243 * scale_factor;
	ButtonPanelHeight = 186 * scale_factor;
	//Wyrmgus end

	CalculateMaxIconSize();
	VisibleUnitIcons = CalculateVisibleIcons();

	if (!StartUnitName.empty()) {
		StartUnit = wyrmgus::unit_type::get(StartUnitName);
	}
	Select.Icon = nullptr;
	Select.Load();
	Units.Icon = nullptr;
	Units.Load();

	//Wyrmgus start
//	Map.Tileset->fillSolidTiles(&Editor.ShownTileTypes);
	Editor.ShownTileTypes.clear();
	for (wyrmgus::terrain_type *terrain_type : wyrmgus::terrain_type::get_all()) {
		if (!terrain_type->is_hidden()) {
			Editor.ShownTileTypes.push_back(terrain_type);
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
//Wyrmgus start
//int EditorSaveMap(const std::string &file)
int EditorSaveMap(const std::string &file, bool is_mod)
//Wyrmgus end
{
	std::string fullName;
	fullName = wyrmgus::database::get()->get_root_path().string() + "/" + file;
	//Wyrmgus start
	if (is_mod) { // if is a mod, save the file in a folder of the same name as the file's name
		std::string file_name;
		std::string path_name;
		int name_size = fullName.length();
		for (int i = (name_size - 1); i >= 0; --i) {
			if (fullName[i] == '/') {
				file_name = fullName.substr(i + 1, (name_size - i - 1));
				path_name = fullName.substr(0, i);
				break;
			}
		}
		std::string previous_path_name;
		name_size = path_name.length();
		for (int i = (name_size - 1); i >= 0; --i) {
			if (path_name[i] == '/') {
				previous_path_name = path_name.substr(i + 1, (name_size - i - 1));
				break;
			}
		}
		std::string file_name_without_ending = FindAndReplaceString(file_name, ".gz", "");
		file_name_without_ending = FindAndReplaceString(file_name_without_ending, ".smp", "");
		
		if (previous_path_name != file_name_without_ending) {
			struct stat tmp;
			
			path_name += "/" + file_name_without_ending + "/";
			if (stat(path_name.c_str(), &tmp) < 0) {
				makedir(path_name.c_str(), 0777);
			}
			fullName = path_name + file_name;
		}
	}
	
//	if (SaveStratagusMap(fullName, *CMap::get(), Editor.TerrainEditable) == -1) {
	if (SaveStratagusMap(fullName, *CMap::get(), Editor.TerrainEditable && !is_mod, is_mod) == -1) {
	//Wyrmgus end
		fprintf(stderr, "Cannot save map\n");
		return -1;
	}
	return 0;
}

/*----------------------------------------------------------------------------
--  Editor main loop
----------------------------------------------------------------------------*/

/**
**  Editor main event loop.
*/
void EditorMainLoop()
{
	const int scale_factor = wyrmgus::defines::get()->get_scale_factor();

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

	while (true) {
		Editor.MapLoaded = false;
		Editor.Running = EditorEditing;

		Editor.Init();

		if (first_init) {
			first_init = false;
			//Wyrmgus start
//			editorUnitSlider->setSize(ButtonPanelWidth/*176*/, 16);
//			editorSlider->setSize(ButtonPanelWidth/*176*/, 16);
			editorUnitSlider->setSize((218 - 24 - 6) * scale_factor, 16 * scale_factor); // adapt to new UI size, should make this more scriptable
			editorSlider->setSize((218 - 24 - 6) * scale_factor, 16 * scale_factor);
			//Wyrmgus end
			//Wyrmgus start
//			editorContainer->add(editorUnitSlider.get(), UI.ButtonPanel.X + 2, UI.ButtonPanel.Y - 16);
//			editorContainer->add(editorSlider.get(), UI.ButtonPanel.X + 2, UI.ButtonPanel.Y - 16);
			editorContainer->add(editorUnitSlider.get(), UI.InfoPanel.X + 12 * scale_factor, UI.InfoPanel.Y + (160 - 24) * scale_factor);
			editorContainer->add(editorSlider.get(), UI.InfoPanel.X + 12 * scale_factor, UI.InfoPanel.Y + (160 - 24) * scale_factor);
			//Wyrmgus end
		}
		//ProcessMenu("menu-editor-tips", 1);
		current_interface_state = interface_state::normal;

		SetVideoSync();

		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);
		current_interface_state = interface_state::normal;
		Editor.State = EditorSelecting;
		UI.SelectedViewport = UI.Viewports;
		TileCursorSize = 1;

		while (Editor.Running) {
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
				DoScrollArea(MouseScrollState, 0, MouseScrollState == 0 && KeyScrollState > 0);
			}
			if (UI.KeyScroll) {
				DoScrollArea(KeyScrollState, (KeyModifiers & ModifierControl) != 0, MouseScrollState == 0 && KeyScrollState > 0);
				if (CursorOn == cursor_on::map && (MouseButtons & LeftButton) &&
					(Editor.State == EditorEditTile ||
					 Editor.State == EditorEditUnit)) {
					EditorCallbackButtonDown(0);
				}
			}

			WaitEventsOneFrame();
		}
		CursorBuilding = nullptr;
		if (!Editor.MapLoaded) {
			break;
		}

		CleanModules();

		LoadCcl(parameters::get()->luaStartFilename); // Reload the main config file

		PreMenuSetup();

		current_interface_state = interface_state::menu;
		cursor::set_current_cursor(UI.get_cursor(cursor_type::point), true);

		Video.ClearScreen();
	}

	CommandLogDisabled = OldCommandLogDisabled;
	SetCallbacks(old_callbacks);
	Gui->setTop(oldTop);
	editorContainer.reset();
	editorUnitSliderListener.reset();
	editorSliderListener.reset();
	editorUnitSlider.reset();
	editorSlider.reset();
}

/**
**  Start the editor
**
**  @param filename  Map to load, null to create a new map
*/
//Wyrmgus start
//void StartEditor(const std::string &filename)
void StartEditor(const std::string &filename, bool is_mod)
//Wyrmgus end
{
	std::string nc, rc;

	if (!filename.empty()) {
		CurrentMapPath = filename;
	} else {
		// new map, choose some default values
		CurrentMapPath.clear();
		// Map.Info.Description.clear();
		// Map.Info.MapWidth = 64;
		// Map.Info.MapHeight = 64;
	}

	//Wyrmgus start
	if (!TileToolRandom) {
		TileToolRandom ^= 1;
	}
	
	IsMod = is_mod;
	//Wyrmgus end
	
	//Wyrmgus start
	CleanPlayers(); //clean players, as they could not have been cleansed after a scenario
	//Wyrmgus end
	
	// Run the editor.
	EditorMainLoop();

	// Clear screen
	Video.ClearScreen();

	Editor.TerrainEditable = true;

	Editor.ShownTileTypes.clear();
	CleanGame();
	CleanPlayers();
}
