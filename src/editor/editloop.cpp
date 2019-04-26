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
//      (c) Copyright 2002-2019 by Lutz Sammer, Jimmy Salmon and
//		Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <deque>

#include "stratagus.h"

#include "editor/editor.h"

#include "civilization.h"
#include "commands.h"
#include "game/game.h"
#include "game/replay.h"
#include "guichan.h"
#include "iocompat.h"
#include "iolib.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "menus.h"
#include "network/network.h"
#include "parameters.h"
#include "player.h"
#include "script.h"
#include "settings.h"
#include "sound/sound.h"
#include "sound/sound_server.h"
#include "translate.h"
#include "ui/button_action.h"
#include "ui/icon.h"
#include "ui/interface.h"
#include "ui/ui.h"
#include "ui/widgets.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "video/font.h"
#include "video/video.h"


extern void DoScrollArea(int state, bool fast, bool isKeyboard);
extern void DrawGuichanWidgets();
extern void CleanGame();

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define UNIT_ICON_X (IconWidth + 7)			/// Unit mode icon
constexpr int UNIT_ICON_Y = (0);			/// Unit mode icon
#define TILE_ICON_X (IconWidth * 2 + 16)	/// Tile mode icon
constexpr int TILE_ICON_Y = (2);			/// Tile mode icon
#define START_ICON_X (IconWidth * 3 + 16)	/// Start mode icon
constexpr int START_ICON_Y = (2);			/// Start mode icon

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

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
	const CUnitType *UnitType;
	CPlayer *Player;
};

static std::deque<EditorAction> EditorUndoActions;
static std::deque<EditorAction> EditorRedoActions;

static void EditorUndoAction();
static void EditorRedoAction();
static void EditorAddUndoAction(EditorAction action);

extern gcn::Gui *Gui;
static gcn::Container *editorContainer;
static gcn::Slider *editorUnitSlider;
static gcn::Slider *editorSlider;

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

static EditorUnitSliderListener *editorUnitSliderListener;

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

static EditorSliderListener *editorSliderListener;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
static void EditTile(const Vec2i &pos, CTerrainType *terrain)
//Wyrmgus end
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, UI.CurrentMapLayer));
	
	const CTileset &tileset = *CMap::Map.Tileset;
	CMapField &mf = *UI.CurrentMapLayer->Field(pos);
	
	//Wyrmgus start
	int value = 0;
	if ((terrain->Flags & MapFieldForest) || (terrain->Flags & MapFieldRocks)) {
		value = CResource::GetAll()[terrain->Resource]->DefaultAmount;
	}
//	mf.setTileIndex(tileset, tileIndex, 0);
	mf.SetTerrain(terrain);
	if (!terrain->Overlay && !(KeyModifiers & ModifierShift)) { // don't remove overlay terrains if holding shift
		mf.RemoveOverlayTerrain();
	}
	mf.Value = value;
//	mf.playerInfo.SeenTile = mf.getGraphicTile();
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

	UI.Minimap.UpdateSeenXY(pos);
	//Wyrmgus start
//	UI.Minimap.UpdateXY(pos);
	UI.Minimap.UpdateXY(pos, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	//Wyrmgus start
//	EditorTileChanged(pos);
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
static void EditTilesInternal(const Vec2i &pos, CTerrainType *terrain, int size)
//Wyrmgus end
{
	Vec2i minPos = pos;
	Vec2i maxPos(pos.x + size - 1, pos.y + size - 1);

	CMap::Map.FixSelectionArea(minPos, maxPos, UI.CurrentMapLayer->ID);

	//Wyrmgus start
	std::vector<Vec2i> changed_tiles;
	//Wyrmgus end
	
	Vec2i itPos;
	for (itPos.y = minPos.y; itPos.y <= maxPos.y; ++itPos.y) {
		for (itPos.x = minPos.x; itPos.x <= maxPos.x; ++itPos.x) {
			//Wyrmgus start
			if (CMap::Map.GetTileTopTerrain(itPos, false, UI.CurrentMapLayer->ID) == terrain) {
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
		CTerrainType *tile_terrain = CMap::Map.GetTileTerrain(changed_tiles[i], terrain->Overlay, UI.CurrentMapLayer->ID);
		
		CMap::Map.CalculateTileTransitions(changed_tiles[i], false, UI.CurrentMapLayer->ID);
		CMap::Map.CalculateTileTransitions(changed_tiles[i], true, UI.CurrentMapLayer->ID);

		bool has_transitions = terrain->Overlay ? (UI.CurrentMapLayer->Field(changed_tiles[i])->OverlayTransitionTiles.size() > 0) : (UI.CurrentMapLayer->Field(changed_tiles[i])->TransitionTiles.size() > 0);
		bool solid_tile = true;
		
		if (tile_terrain && !tile_terrain->AllowSingle) {
			for (int x_offset = -1; x_offset <= 1; ++x_offset) {
				for (int y_offset = -1; y_offset <= 1; ++y_offset) {
					if (x_offset != 0 || y_offset != 0) {
						Vec2i adjacent_pos(changed_tiles[i].x + x_offset, changed_tiles[i].y + y_offset);
						if (CMap::Map.Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
							CMapField &adjacent_mf = *UI.CurrentMapLayer->Field(adjacent_pos);
									
							CTerrainType *adjacent_terrain = CMap::Map.GetTileTerrain(adjacent_pos, tile_terrain->Overlay, UI.CurrentMapLayer->ID);
							if (tile_terrain->Overlay && adjacent_terrain && UI.CurrentMapLayer->Field(adjacent_pos)->OverlayTerrainDestroyed) {
								adjacent_terrain = nullptr;
							}
							if (tile_terrain != adjacent_terrain && std::find(tile_terrain->OuterBorderTerrains.begin(), tile_terrain->OuterBorderTerrains.end(), adjacent_terrain) == tile_terrain->OuterBorderTerrains.end()) { // also happens if terrain is null, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
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
							if (CMap::Map.Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
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
					
					if (CMap::Map.Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
						for (int overlay = 1; overlay >= 0; --overlay) {
							CTerrainType *adjacent_terrain = CMap::Map.GetTileTerrain(adjacent_pos, overlay > 0, UI.CurrentMapLayer->ID);
							if (!adjacent_terrain || adjacent_terrain == CMap::Map.GetTileTerrain(changed_tiles[i], overlay > 0, UI.CurrentMapLayer->ID)) {
								continue;
							}
							CMap::Map.CalculateTileTransitions(adjacent_pos, overlay == 1, UI.CurrentMapLayer->ID);
							bool has_transitions = overlay ? (UI.CurrentMapLayer->Field(adjacent_pos)->OverlayTransitionTiles.size() > 0) : (UI.CurrentMapLayer->Field(adjacent_pos)->TransitionTiles.size() > 0);
							bool solid_tile = true;
							
							if (!overlay && std::find(adjacent_terrain->BorderTerrains.begin(), adjacent_terrain->BorderTerrains.end(), CMap::Map.GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID)) == adjacent_terrain->BorderTerrains.end()) {
								for (size_t j = 0; j != adjacent_terrain->BorderTerrains.size(); ++j) {
									CTerrainType *border_terrain = adjacent_terrain->BorderTerrains[j];
									if (std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), adjacent_terrain) != border_terrain->BorderTerrains.end() && std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), CMap::Map.GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID)) != border_terrain->BorderTerrains.end()) { // found a terrain type that can border both terrains
										CMap::Map.SetTileTerrain(adjacent_pos, border_terrain, UI.CurrentMapLayer->ID);
										changed_tiles.push_back(adjacent_pos);
										break;
									}
								}
							} else if (!adjacent_terrain->AllowSingle) {
								for (int sub_x_offset = -1; sub_x_offset <= 1; ++sub_x_offset) {
									for (int sub_y_offset = -1; sub_y_offset <= 1; ++sub_y_offset) {
										if (sub_x_offset != 0 || sub_y_offset != 0) {
											Vec2i sub_adjacent_pos(adjacent_pos.x + sub_x_offset, adjacent_pos.y + sub_y_offset);
											if (CMap::Map.Info.IsPointOnMap(sub_adjacent_pos, UI.CurrentMapLayer)) {
												CTerrainType *sub_adjacent_terrain = CMap::Map.GetTileTerrain(sub_adjacent_pos, overlay > 0, UI.CurrentMapLayer->ID);
												if (adjacent_terrain->Overlay && sub_adjacent_terrain && UI.CurrentMapLayer->Field(sub_adjacent_pos)->OverlayTerrainDestroyed) {
													sub_adjacent_terrain = nullptr;
												}
												if (adjacent_terrain != sub_adjacent_terrain && std::find(adjacent_terrain->OuterBorderTerrains.begin(), adjacent_terrain->OuterBorderTerrains.end(), sub_adjacent_terrain) == adjacent_terrain->OuterBorderTerrains.end()) { // also happens if terrain is null, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
													solid_tile = false;
													break;
												}
											}
										}
									}
								}
									
								if (!solid_tile && !has_transitions) {
									if (overlay) {
										CMap::Map.RemoveTileOverlayTerrain(adjacent_pos, UI.CurrentMapLayer->ID);
									} else {
										CMap::Map.SetTileTerrain(adjacent_pos, CMap::Map.GetTileTerrain(changed_tiles[i], false, UI.CurrentMapLayer->ID), UI.CurrentMapLayer->ID);
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
	
	for (size_t i = 0; i != changed_tiles.size(); ++i) {
		CMap::Map.CalculateTileTransitions(changed_tiles[i], false, UI.CurrentMapLayer->ID);
		CMap::Map.CalculateTileTransitions(changed_tiles[i], true, UI.CurrentMapLayer->ID);
		UI.Minimap.UpdateXY(changed_tiles[i], UI.CurrentMapLayer->ID);
		
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(changed_tiles[i].x + x_offset, changed_tiles[i].y + y_offset);
					
					if (std::find(changed_tiles.begin(), changed_tiles.end(), adjacent_pos) != changed_tiles.end()) {
						continue;
					}
					
					if (CMap::Map.Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
						CMap::Map.CalculateTileTransitions(adjacent_pos, false, UI.CurrentMapLayer->ID);
						CMap::Map.CalculateTileTransitions(adjacent_pos, true, UI.CurrentMapLayer->ID);
						UI.Minimap.UpdateXY(adjacent_pos, UI.CurrentMapLayer->ID);
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
static void EditTiles(const Vec2i &pos, CTerrainType *terrain, int size)
//Wyrmgus end
{
	//Wyrmgus start
//	EditTilesInternal(pos, tile, size);
	EditTilesInternal(pos, terrain, size);
	//Wyrmgus end

	if (!MirrorEdit) {
		return;
	}
	const Vec2i mpos(UI.CurrentMapLayer->GetWidth() - size, UI.CurrentMapLayer->GetHeight() - size);
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
static void EditorActionPlaceUnit(const Vec2i &pos, const CUnitType &type, CPlayer *player)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, UI.CurrentMapLayer));

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
//	CBuildRestrictionOnTop *b = OnTopDetails(*unit, nullptr);
	CBuildRestrictionOnTop *b = OnTopDetails(*unit->Type, nullptr);
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
		if (type.GivesResource) {
			//Wyrmgus start
//			if (type.StartingResources != 0) {
			if (type.StartingResources.size() > 0) {
			//Wyrmgus end
				//Wyrmgus start
//				unit->ResourcesHeld = type.StartingResources;
//				unit->Variable[GIVERESOURCE_INDEX].Value = type.StartingResources;
//				unit->Variable[GIVERESOURCE_INDEX].Max = type.StartingResources;
				unit->SetResourcesHeld(type.StartingResources[SyncRand(type.StartingResources.size())]);
				unit->Variable[GIVERESOURCE_INDEX].Value = unit->ResourcesHeld;
				unit->Variable[GIVERESOURCE_INDEX].Max = unit->ResourcesHeld;
				//Wyrmgus end
			} else {
				unit->SetResourcesHeld(CResource::GetAll()[type.GivesResource]->DefaultAmount);
				unit->Variable[GIVERESOURCE_INDEX].Value = CResource::GetAll()[type.GivesResource]->DefaultAmount;
				unit->Variable[GIVERESOURCE_INDEX].Max = CResource::GetAll()[type.GivesResource]->DefaultAmount;
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
static void EditorPlaceUnit(const Vec2i &pos, CUnitType &type, CPlayer *player)
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
	UnitClearOrders(unit);
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
		w = CMap::Map.GetCurrentPixelTileSize().x;//+2,
		h = CMap::Map.GetCurrentPixelTileSize().y;//+2
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
		const CUnitType *type = CUnitType::Get(Editor.UnitTypes[i].c_str());
		Assert(type && type->GetIcon());
		const CIcon &icon = *type->GetIcon();

		IconWidth = std::max(IconWidth, icon.G->Width);
		IconHeight = std::max(IconHeight, icon.G->Height);
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
		const CUnitType *type = CUnitType::Get(Editor.UnitTypes[i]);
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
static void DrawPlayers()
{
	char buf[256];
	CLabel label(GetSmallFont());

	//Wyrmgus start
//	int x = UI.InfoPanel.X + 8;
	int x = UI.InfoPanel.X + 26;
	//Wyrmgus end
	int y = UI.InfoPanel.Y + 4 + IconHeight + 10;

	for (int i = 0; i < PlayerMax; ++i) {
		//Wyrmgus start
		if (i >= 15 && i < PlayerNumNeutral) {
			continue;
		}
//		if (i == PlayerMax / 2) {
		if ((i % 8) == 0) {
		//Wyrmgus end
			y += 20;
		}
		if (i == Editor.CursorPlayer && CMap::Map.Info.PlayerType[i] != PlayerNobody) {
			Video.DrawRectangle(ColorWhite, x + i % 8 * 20, y, 20, 20);
		}
		Video.DrawRectangle(
			i == Editor.CursorPlayer && CMap::Map.Info.PlayerType[i] != PlayerNobody ? ColorWhite : ColorGray,
			x + i % 8 * 20, y, 19, 19);
		if (CMap::Map.Info.PlayerType[i] != PlayerNobody) {
			Video.FillRectangle(CPlayer::Players[i]->Color, x + 1 + i % 8 * 20, y + 1, 17, 17);
		}
		if (i == Editor.SelectedPlayer) {
			Video.DrawRectangle(ColorGreen, x + 1 + i % 8 * 20, y + 1, 17, 17);
		}
		//Wyrmgus start
//		sprintf(buf, "%d", i);
		sprintf(buf, "%d", (i == PlayerNumNeutral) ? 16 : i + 1);
		//Wyrmgus end
		label.DrawCentered(x + i % 8 * 20 + 10, y + 7, buf);
	}

	//Wyrmgus start
//	x = UI.InfoPanel.X + 4;
	x = UI.InfoPanel.X + 22;
	//Wyrmgus end
	y += 18 * 1 + 4;
	if (Editor.SelectedPlayer != -1) {
		//Wyrmgus start
//		snprintf(buf, sizeof(buf), "Plyr %d %s ", Editor.SelectedPlayer,
//				 CCivilization::Get(CPlayer::Players[Editor.SelectedPlayer]->Race)->GetIdent().utf8().get_data());
		std::string civ_str = CCivilization::Get(CPlayer::Players[Editor.SelectedPlayer]->Race)->GetIdent().utf8().get_data();
		civ_str[0] = toupper(civ_str[0]);
		snprintf(buf, sizeof(buf), "Player %d %s ", (Editor.SelectedPlayer == PlayerNumNeutral) ? 16 : Editor.SelectedPlayer + 1, civ_str.c_str());
		//Wyrmgus end
		// CPlayer::Players[SelectedPlayer]->RaceName);

		switch (CMap::Map.Info.PlayerType[Editor.SelectedPlayer]) {
			case PlayerNeutral:
				strcat_s(buf, sizeof(buf), "Neutral");
				break;
			case PlayerNobody:
			default:
				strcat_s(buf, sizeof(buf), "Nobody");
				break;
			case PlayerPerson:
				strcat_s(buf, sizeof(buf), "Person");
				break;
			case PlayerComputer:
			case PlayerRescuePassive:
			case PlayerRescueActive:
				strcat_s(buf, sizeof(buf), "Computer");
				break;
		}
		label.SetFont(GetGameFont());
		label.Draw(x, y, buf);
	}
}

#if 0
extern void DrawPopupUnitInfo(const CUnitType *type,
							  int player_index, CFont *font,
							  Uint32 backgroundColor, int buttonX, int buttonY);

static void DrawPopup()
{
	if (Editor.State == EditorEditUnit && Editor.CursorUnitIndex != -1) {
		std::string nc, rc;
		GetDefaultTextColors(nc, rc);
		DrawPopupUnitInfo(Editor.ShownUnitTypes[Editor.CursorUnitIndex],
						  Editor.SelectedPlayer, GetSmallFont(),
						  Video.MapRGB(TheScreen->format, 38, 38, 78),
						  Editor.PopUpX, Editor.PopUpY);
		SetDefaultTextColors(nc, rc);
	}
}
#endif

/**
**  Draw unit icons.
*/
static void DrawUnitIcons()
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
//		CIcon &icon = *Editor.ShownUnitTypes[i]->Icon.Icon;
		CIcon &icon = (i != (int) Editor.ShownUnitTypes.size()) ? *Editor.ShownUnitTypes[i]->Icon.Icon : *CIcon::Get("icon-level-up");
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

		icon.DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", CPlayer::Players[Editor.SelectedPlayer]->Index);

		//Wyrmgus start
//		Video.DrawRectangleClip(ColorGray, x, y, icon.G->Width, icon.G->Height);
		//Wyrmgus end
		if (i == Editor.SelectedUnitIndex) {
			//Wyrmgus start
//			Video.DrawRectangleClip(ColorGreen, x + 1, y + 1,
//									icon.G->Width - 2, icon.G->Height - 2);
			Video.DrawRectangleClip(ColorGreen, x, y,
									icon.G->Width, icon.G->Height);
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
				DrawGenericPopup("Create Unit Type", UI.ButtonPanel.Buttons[j].X, UI.ButtonPanel.Buttons[j].Y);
			} else {
				DrawPopup(CurrentButtons[j], UI.ButtonPanel.Buttons[j].X, UI.ButtonPanel.Buttons[j].Y);
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
static void DrawTileIcon(unsigned tilenum, unsigned x, unsigned y, unsigned flags)
{
	//Wyrmgus start
	/*
	Uint32 color = (flags & IconActive) ? ColorGray : ColorBlack;

	Video.DrawRectangleClip(color, x, y, CMap::Map.GetCurrentPixelTileSize().x + 7, CMap::Map.GetCurrentPixelTileSize().y + 7);
	Video.DrawRectangleClip(ColorBlack, x + 1, y + 1, CMap::Map.GetCurrentPixelTileSize().x + 5, CMap::Map.GetCurrentPixelTileSize().y + 5);

	Video.DrawVLine(ColorGray, x + CMap::Map.GetCurrentPixelTileSize().x + 4, y + 5, CMap::Map.GetCurrentPixelTileSize().y - 1); // _|
	Video.DrawVLine(ColorGray, x + CMap::Map.GetCurrentPixelTileSize().x + 5, y + 5, CMap::Map.GetCurrentPixelTileSize().y - 1);
	Video.DrawHLine(ColorGray, x + 5, y + CMap::Map.GetCurrentPixelTileSize().y + 4, CMap::Map.GetCurrentPixelTileSize().x + 1);
	Video.DrawHLine(ColorGray, x + 5, y + CMap::Map.GetCurrentPixelTileSize().y + 5, CMap::Map.GetCurrentPixelTileSize().x + 1);

	color = (flags & IconClicked) ? ColorGray : ColorWhite;
	Video.DrawHLine(color, x + 5, y + 3, CMap::Map.GetCurrentPixelTileSize().x + 1);
	Video.DrawHLine(color, x + 5, y + 4, CMap::Map.GetCurrentPixelTileSize().x + 1);
	Video.DrawVLine(color, x + 3, y + 3, CMap::Map.GetCurrentPixelTileSize().y + 3);
	Video.DrawVLine(color, x + 4, y + 3, CMap::Map.GetCurrentPixelTileSize().y + 3);
	*/
	Video.DrawVLine(ColorGray, x + CMap::Map.GetCurrentPixelTileSize().x + 4 - 1, y + 5 - 1, CMap::Map.GetCurrentPixelTileSize().y - 1 - 1); // _|
	Video.DrawVLine(ColorGray, x + CMap::Map.GetCurrentPixelTileSize().x + 5 - 1, y + 5 - 1, CMap::Map.GetCurrentPixelTileSize().y - 1 - 1);
	Video.DrawHLine(ColorGray, x + 5 - 1, y + CMap::Map.GetCurrentPixelTileSize().y + 4 - 1, CMap::Map.GetCurrentPixelTileSize().x + 1 - 1);
	Video.DrawHLine(ColorGray, x + 5 - 1, y + CMap::Map.GetCurrentPixelTileSize().y + 5 - 1, CMap::Map.GetCurrentPixelTileSize().x + 1 - 1);

	Uint32 color = (flags & IconClicked) ? ColorGray : ColorWhite;
	Video.DrawHLine(color, x + 5 - 1, y + 3 - 1, CMap::Map.GetCurrentPixelTileSize().x + 1 - 1);
	Video.DrawHLine(color, x + 5 - 1, y + 4 - 1, CMap::Map.GetCurrentPixelTileSize().x + 1 - 1);
	Video.DrawVLine(color, x + 3 - 1, y + 3 - 1, CMap::Map.GetCurrentPixelTileSize().y + 3 - 1);
	Video.DrawVLine(color, x + 4 - 1, y + 3 - 1, CMap::Map.GetCurrentPixelTileSize().y + 3 - 1);
	
	color = (flags & IconActive) ? ColorGray : ColorBlack;

	Video.DrawRectangleClip(color, x, y, CMap::Map.GetCurrentPixelTileSize().x + 7, CMap::Map.GetCurrentPixelTileSize().y + 7);
	Video.DrawRectangleClip(ColorBlack, x + 1, y + 1, CMap::Map.GetCurrentPixelTileSize().x + 5, CMap::Map.GetCurrentPixelTileSize().y + 5);
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
//	CMap::Map.TileGraphic->DrawFrameClip(CMap::Map.Tileset->tiles[tilenum].tile, x, y);
	const CTerrainType *terrain = Editor.ShownTileTypes[0];
	terrain->GetGraphics()->DrawFrameClip(terrain->SolidTiles[0], x, y);
	//Wyrmgus end

	if (flags & IconSelected) {
		Video.DrawRectangleClip(ColorGreen, x, y, CMap::Map.GetCurrentPixelTileSize().x, CMap::Map.GetCurrentPixelTileSize().y);
	}
}

/**
**  Draw tile icons.
**
**  @todo for the start the solid tiles are hardcoded
**        If we have more solid tiles, than they fit into the panel, we need
**        some new ideas.
*/
static void DrawTileIcons()
{
	CLabel label(GetGameFont());
	int x = UI.InfoPanel.X + 46;
	int y = UI.InfoPanel.Y + 4 + IconHeight + 11;

	if (CursorOn == CursorOnButton && 300 <= ButtonUnderCursor && ButtonUnderCursor < 306) {
		//Wyrmgus start
//		Video.DrawRectangle(ColorGray, x - 42, y - 3 + (ButtonUnderCursor - 300) * 20, 100, 20);
		if (ButtonUnderCursor <= 303) {
			Video.DrawRectangle(ColorGray, x - 42, y - 3 + (ButtonUnderCursor - 300) * 20, 100, 20);
		} else {
			Video.DrawRectangle(ColorGray, x - 42 + 100, y - 3 + (ButtonUnderCursor - 304) * 20, 100, 20);
		}
		//Wyrmgus end
	}

	if (TileCursorSize == 1) {
		label.DrawReverseCentered(x, y, "1x1");
	} else {
		label.DrawCentered(x, y, "1x1");
	}
	y += 20;
	if (TileCursorSize == 2) {
		label.DrawReverseCentered(x, y, "2x2");
	} else {
		label.DrawCentered(x, y, "2x2");
	}
	y += 20;
	if (TileCursorSize == 3) {
		label.DrawReverseCentered(x, y, "3x3");
	} else {
		label.DrawCentered(x, y, "3x3");
	}
	y += 20;
	if (TileCursorSize == 4) {
		label.DrawReverseCentered(x, y, "4x4");
	} else {
		label.DrawCentered(x, y, "4x4");
	}
	//Wyrmgus start
//	y += 20;
	x += 100;
	y -= 20 * 3;
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
		label.DrawReverseCentered(x, y, "5x5");
	} else {
		label.DrawCentered(x, y, "5x5");
	}
	//Wyrmgus end
	y += 20;
	//Wyrmgus start
	/*
	if (TileToolDecoration) {
		label.DrawReverseCentered(x, y, "Filler");
	} else {
		label.DrawCentered(x, y, "Filler");
	}
	*/
	if (TileCursorSize == 10) {
		label.DrawReverseCentered(x, y, "10x10");
	} else {
		label.DrawCentered(x, y, "10x10");
	}
	//Wyrmgus end
	y += 20;

	int i = Editor.TileIndex;
	Assert(Editor.TileIndex != -1);
	y = UI.ButtonPanel.Y + 24;
	while (y < UI.ButtonPanel.Y + ButtonPanelHeight - CMap::Map.GetCurrentPixelTileSize().y) {
		if (i >= (int)Editor.ShownTileTypes.size()) {
			break;
		}
		//Wyrmgus start
//		x = UI.ButtonPanel.X + 10;
		x = UI.ButtonPanel.X + 10 + 6;
		//Wyrmgus end
		while (x < UI.ButtonPanel.X + ButtonPanelWidth - CMap::Map.GetCurrentPixelTileSize().x) {
			if (i >= (int) Editor.ShownTileTypes.size()) {
				break;
			}
			//Wyrmgus start
//			const unsigned int tile = Editor.ShownTileTypes[i];

//			CMap::Map.TileGraphic->DrawFrameClip(tile, x, y);

			const CTerrainType *terrain = Editor.ShownTileTypes[i];

			if (terrain->GetGraphics() && terrain->SolidTiles.size() > 0) {
				terrain->GetGraphics()->DrawFrameClip(terrain->SolidTiles[0], x, y);
			}
			//Wyrmgus end
			Video.DrawRectangleClip(ColorGray, x, y, CMap::Map.GetCurrentPixelTileSize().x, CMap::Map.GetCurrentPixelTileSize().y);

			if (i == Editor.SelectedTileIndex) {
				Video.DrawRectangleClip(ColorGreen, x + 1, y + 1,
										CMap::Map.GetCurrentPixelTileSize().x - 2, CMap::Map.GetCurrentPixelTileSize().y - 2);
			}
			if (i == Editor.CursorTileIndex) {
				Video.DrawRectangleClip(ColorWhite, x - 1, y - 1,
										CMap::Map.GetCurrentPixelTileSize().x + 2, CMap::Map.GetCurrentPixelTileSize().y + 2);
				Editor.PopUpX = x;
				Editor.PopUpY = y;
			}

			//Wyrmgus start
//			x += CMap::Map.GetCurrentPixelTileSize().x + 8;
			x += CMap::Map.GetCurrentPixelTileSize().x + 30; // to allow 5 tile types per row with the new UI
			//Wyrmgus end
			++i;
		}
		//Wyrmgus start
//		y += CMap::Map.GetCurrentPixelTileSize().y + 2;
		y += CMap::Map.GetCurrentPixelTileSize().y + 18; // make this space a little larger (as large as the space between the top of the panel and the first icon, minus the parts of the panel which are "lower" so to speak)
		//Wyrmgus end
	}
}

static void DrawEditorPanel_SelectIcon()
{
	//Wyrmgus start
//	const PixelPos pos(UI.InfoPanel.X + 4, UI.InfoPanel.Y + 4);
	const PixelPos pos(UI.InfoPanel.X + 11, UI.InfoPanel.Y + 7);
	//Wyrmgus end
	CIcon *icon = Editor.Select.Icon;
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
	icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", Editor.SelectedPlayer);
}

static void DrawEditorPanel_UnitsIcon()
{
	//Wyrmgus start
//	const PixelPos pos(UI.InfoPanel.X + 4 + UNIT_ICON_X, UI.InfoPanel.Y + 4 + UNIT_ICON_Y);
	const PixelPos pos(UI.InfoPanel.X + 11 + UNIT_ICON_X, UI.InfoPanel.Y + 7 + UNIT_ICON_Y);
	//Wyrmgus end
	CIcon *icon = Editor.Units.Icon;
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
	//Wyrmgus start
//	icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "");
	icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", Editor.SelectedPlayer);
	//Wyrmgus end
}

static void DrawEditorPanel_StartIcon()
{
	//Wyrmgus start
//	int x = UI.InfoPanel.X + 4;
	int x = UI.InfoPanel.X + 11;
	//Wyrmgus end
	int y = UI.InfoPanel.Y + 5;

	if (Editor.StartUnit) {
		CIcon *icon = Editor.StartUnit->GetIcon();
		Assert(icon);
		const PixelPos pos(x + START_ICON_X, y + START_ICON_Y);
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
		
		icon->DrawUnitIcon(*UI.SingleSelectedButton->Style, flag, pos, "", Editor.SelectedPlayer);
	} else {
		//  No unit specified : draw a cross.
		//  Todo : FIXME Should we just warn user to define Start unit ?
		PushClipping();
		x += START_ICON_X + 1;
		y += START_ICON_Y + 1;
		if (ButtonUnderCursor == StartButton) {
			Video.DrawRectangleClip(ColorGray, x - 1, y - 1, IconHeight, IconHeight);
		}
		Video.FillRectangleClip(ColorBlack, x, y, IconHeight - 2, IconHeight - 2);

		const PixelPos lt(x, y);
		//Wyrmgus start
//		const PixelPos lb(x, y + IconHeight - 2);
//		const PixelPos rt(x + IconHeight - 2, y);
		const PixelPos lb(x, y + IconHeight - 3);
		const PixelPos rt(x + IconHeight - 3, y);
		//Wyrmgus end
		const PixelPos rb(x + IconHeight - 2, y + IconHeight - 2);
		//Wyrmgus start
//		const Uint32 color = PlayerColors[Editor.SelectedPlayer][0];
		const Uint32 color = CPlayer::Players[Editor.SelectedPlayer]->Color;

		//Wyrmgus end

		Video.DrawLineClip(color, lt, rb);
		Video.DrawLineClip(color, rt, lb);
		PopClipping();
	}
}

/**
**  Draw the editor panels.
*/
static void DrawEditorPanel()
{
	DrawEditorPanel_SelectIcon();
	DrawEditorPanel_UnitsIcon();

	if (Editor.TerrainEditable) {
		//Wyrmgus start
//		const int x = UI.InfoPanel.X + TILE_ICON_X + 4;
		const int x = UI.InfoPanel.X + TILE_ICON_X + 11;
		//Wyrmgus end
		const int y = UI.InfoPanel.Y + TILE_ICON_Y + 4;

		DrawTileIcon(0x10 + 4 * 16, x, y,
					 (ButtonUnderCursor == TileButton ? IconActive : 0) |
					 (Editor.State == EditorEditTile ? IconSelected : 0));
	}
	DrawEditorPanel_StartIcon();

	switch (Editor.State) {
		case EditorSelecting:
			break;
		case EditorEditTile:
			DrawTileIcons();
			break;
		case EditorSetStartLocation:
			DrawPlayers();
			break;
		case EditorEditUnit:
			DrawPlayers();
			DrawUnitIcons();
			break;
	}
}

/**
**  Draw special cursor on map.
**
**  @todo support for bigger cursors (2x2, 3x3 ...)
*/
static void DrawMapCursor()
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
					CursorBuilding = const_cast<CUnitType *>(Editor.ShownUnitTypes[Editor.SelectedUnitIndex]);
				}
				break;
			case EditorSetStartLocation:
				if (Editor.StartUnit) {
					CursorBuilding = const_cast<CUnitType *>(Editor.StartUnit);
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
			const CTerrainType *terrain = Editor.ShownTileTypes[Editor.SelectedTileIndex];
			//Wyrmgus end
			PushClipping();
			UI.MouseViewport->SetClipping();

			PixelPos screenPosIt;
			for (int j = 0; j < TileCursorSize; ++j) {
				screenPosIt.y = screenPos.y + j * CMap::Map.GetCurrentPixelTileSize().y;
				if (screenPosIt.y >= UI.MouseViewport->GetBottomRightPos().y) {
					break;
				}
				for (int i = 0; i < TileCursorSize; ++i) {
					screenPosIt.x = screenPos.x + i * CMap::Map.GetCurrentPixelTileSize().x;
					if (screenPosIt.x >= UI.MouseViewport->GetBottomRightPos().x) {
						break;
					}
					//Wyrmgus start
//					CMap::Map.TileGraphic->DrawFrameClip(tile, screenPosIt.x, screenPosIt.y);

					if (terrain->GetGraphics() && terrain->SolidTiles.size() > 0) {
						terrain->GetGraphics()->DrawFrameClip(terrain->SolidTiles[0], screenPosIt.x, screenPosIt.y);
					}
					//Wyrmgus end
				}
			}
			Video.DrawRectangleClip(ColorWhite, screenPos.x, screenPos.y, CMap::Map.GetCurrentPixelTileSize().x * TileCursorSize, CMap::Map.GetCurrentPixelTileSize().y * TileCursorSize);
			PopClipping();
		} else {
			// If there is an unit under the cursor, it's selection thing
			//  is drawn somewhere else (Check DrawUnitSelection.)
			if (UnitUnderCursor != nullptr) {
				PushClipping();
				UI.MouseViewport->SetClipping();
				Video.DrawRectangleClip(ColorWhite, screenPos.x, screenPos.y, CMap::Map.GetCurrentPixelTileSize().x, CMap::Map.GetCurrentPixelTileSize().y);
				PopClipping();
			}
		}
	}
}

static void DrawCross(const PixelPos &topleft_pos, const PixelSize &size, Uint32 color)
{
	const PixelPos lt = topleft_pos;
	const PixelPos lb(topleft_pos.x, topleft_pos.y + size.y);
	const PixelPos rt(topleft_pos.x + size.x, topleft_pos.y);
	const PixelPos rb = topleft_pos + size;

	Video.DrawLineClip(color, lt, rb);
	Video.DrawLineClip(color, lb, rt);
}

/**
**  Draw the start locations of all active players on the map
*/
static void DrawStartLocations()
{
	const CUnitType *type = Editor.StartUnit;
	for (const CViewport *vp = UI.Viewports; vp < UI.Viewports + UI.NumViewports; ++vp) {
		PushClipping();
		vp->SetClipping();

		for (int i = 0; i < PlayerMax; i++) {
			if (CMap::Map.Info.PlayerType[i] != PlayerNobody && CMap::Map.Info.PlayerType[i] != PlayerNeutral && CPlayer::Players[i]->StartMapLayer == UI.CurrentMapLayer->ID) {
				const PixelPos startScreenPos = vp->TilePosToScreen_TopLeft(CPlayer::Players[i]->StartPos);

				if (type) {
					DrawUnitType(*type, type->Sprite, i, 0, startScreenPos);
				} else { // Draw a cross
					DrawCross(startScreenPos, CMap::Map.GetCurrentPixelTileSize(), CPlayer::Players[i]->Color);
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
static void DrawEditorInfo()
{
#if 1
	Vec2i pos(0, 0);

	if (UI.MouseViewport) {
		pos = UI.MouseViewport->ScreenToTilePos(CursorScreenPos);
	}

	char buf[256];
	snprintf(buf, sizeof(buf), _("Editor (%d %d)"), pos.x, pos.y);
	CLabel(GetGameFont()).Draw(UI.StatusLine.TextX + 2, UI.StatusLine.TextY - 12, buf);
	const CMapField &mf = *UI.CurrentMapLayer->Field(pos);
	//
	// Flags info
	//
	const unsigned flag = mf.getFlag();
	sprintf(buf, "%02X|%04X|%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
			mf.Value, flag,
			flag & MapFieldUnpassable   ? 'u' : '-',
			flag & MapFieldAirUnpassable   ? 'A' : '-',
			flag & MapFieldNoBuilding   ? 'n' : '-',
			flag & MapFieldWall         ? 'w' : '-',
			flag & MapFieldRocks        ? 'r' : '-',
			flag & MapFieldForest       ? 'f' : '-',
			flag & MapFieldLandAllowed  ? 'L' : '-',
			flag & MapFieldCoastAllowed ? 'C' : '-',
			flag & MapFieldWaterAllowed ? 'W' : '-',
			flag & MapFieldLandUnit     ? 'l' : '-',
			flag & MapFieldAirUnit      ? 'a' : '-',
			flag & MapFieldSeaUnit      ? 's' : '-',
			flag & MapFieldBuilding     ? 'b' : '-',
			flag & MapFieldItem         ? 'i' : '-',
			flag & MapFieldStumps       ? 't' : '-',
			flag & MapFieldGravel       ? 'g' : '-',
			flag & MapFieldBridge       ? 'B' : '-');

	CLabel(GetGameFont()).Draw(UI.StatusLine.TextX + 118, UI.StatusLine.TextY - 12, buf);

	// Tile info
	const CTileset &tileset = *CMap::Map.Tileset;
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
	snprintf(buf, sizeof(buf), "%s %s", baseTerrainStr, mixTerrainStr);
	*/
	std::string terrain_name;
	if (mf.Terrain) {
		if (mf.OverlayTerrain) {
			terrain_name = mf.OverlayTerrain->Name + " (" + mf.Terrain->Name + ")";
		} else {
			terrain_name = mf.Terrain->Name;
		}
	}
	snprintf(buf, sizeof(buf), "%s", terrain_name.c_str());
	//Wyrmgus end
	//Wyrmgus start
//	CLabel(GetGameFont()).Draw(UI.StatusLine.TextX + 250, UI.StatusLine.TextY - 16, buf);
	CLabel(GetGameFont()).Draw(UI.StatusLine.TextX + 298, UI.StatusLine.TextY - 12, buf);
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
	char buf[256];

	//Wyrmgus start
//	int n = sprintf(buf, _("#%d '%s' Player:#%d %s"), UnitNumber(unit),
	int n = sprintf(buf, _("#%d '%s' Player: #%d %s"), UnitNumber(unit),
	//Wyrmgus end
					//Wyrmgus start
//					unit.Type->Name.c_str(), unit.Player->Index,
					unit.GetTypeName().c_str(), (unit.Player->Index == PlayerNumNeutral) ? 16 : unit.Player->Index + 1,
					//Wyrmgus end
					unit.Active ? "active" : "passive");
	if (unit.Type->GivesResource) {
		sprintf(buf + n, _(" Amount %d"), unit.ResourcesHeld);
	}
	UI.StatusLine.Set(buf);
}

/**
**  Update editor display.
*/
void EditorUpdateDisplay()
{
	ColorCycle();

	DrawMapArea(); // draw the map area

	DrawStartLocations();

	//Wyrmgus start
	/*
	// Fillers
	for (size_t i = 0; i != UI.Fillers.size(); ++i) {
		UI.Fillers[i].G->DrawClip(UI.Fillers[i].X, UI.Fillers[i].Y);
	}
	*/
	//Wyrmgus end

	if (CursorOn == CursorOnMap && Gui->getTop() == editorContainer && !GamePaused) {
		DrawMapCursor(); // cursor on map
	}

	//Wyrmgus start
	if (CursorBuilding && CursorOn == CursorOnMap) {
		DrawBuildingCursor();
	}
	//Wyrmgus end
	
	//Wyrmgus start
	// Fillers
	for (size_t i = 0; i != UI.Fillers.size(); ++i) {
		UI.Fillers[i].G->DrawClip(UI.Fillers[i].X, UI.Fillers[i].Y);
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
				 UI.MenuButton.Text);


	// Minimap
	if (UI.SelectedViewport) {
		UI.Minimap.Draw();
		UI.Minimap.DrawViewportArea(*UI.SelectedViewport);
	}
	// Info panel
	if (UI.InfoPanel.G) {
		UI.InfoPanel.G->DrawClip(UI.InfoPanel.X, UI.InfoPanel.Y);
	}
	// Button panel
	if (UI.ButtonPanel.G) {
		UI.ButtonPanel.G->DrawClip(UI.ButtonPanel.X, UI.ButtonPanel.Y);
	}
	DrawEditorPanel();

	if (CursorOn == CursorOnMap) {
		DrawEditorInfo();
	}

	// Status line
	UI.StatusLine.Draw();

	DrawGuichanWidgets();

	// DrawPopup();

	DrawCursor();

	// refresh entire screen, so no further invalidate needed
	Invalidate();
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
	if (GameCursor == UI.Scroll.Cursor) {
		// Move map.
		GameCursor = UI.Point.Cursor; // Reset
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
					char buf[256];
					snprintf(buf, sizeof(buf), "if (EditorCreateUnitType() ~= nil) then EditorCreateUnitType() end;");
					CclCommand(buf);
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
	if (CursorOn == CursorOnButton && ButtonAreaUnderCursor == ButtonAreaMenu &&
		//Wyrmgus start
//		(MouseButtons & LeftButton) && !GameMenuButtonClicked) {
		(MouseButtons & LeftButton) && !UI.MenuButton.Clicked) {
		//Wyrmgus end
		PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);
		//Wyrmgus start
//		GameMenuButtonClicked = true;
		UI.MenuButton.Clicked = true;
		//Wyrmgus end
		return;
	}
	// Click on minimap
	if (CursorOn == CursorOnMinimap) {
		if (MouseButtons & LeftButton) { // enter move mini-mode
			const Vec2i tilePos = UI.Minimap.ScreenToTilePos(CursorScreenPos);
			UI.SelectedViewport->Center(CMap::Map.TilePosToMapPixelPos_Center(tilePos, UI.CurrentMapLayer));
		}
		return;
	}
	// Click on mode area
	if (CursorOn == CursorOnButton) {
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
				if (EditorEditTile) {
					Editor.State = EditorEditTile;
				}
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
		if (CursorOn == CursorOnButton && ButtonUnderCursor >= 100) {
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
			if (CMap::Map.Info.PlayerType[Editor.CursorPlayer] != PlayerNobody) {
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
			//Wyrmgus start
			/*
			if (MouseButtons & LeftButton) {
				Editor.SelectedUnitIndex = Editor.CursorUnitIndex;
				CursorBuilding = const_cast<CUnitType *>(Editor.ShownUnitTypes[Editor.CursorUnitIndex]);
				return;
			} else if (MouseButtons & RightButton) {
				char buf[256];
				snprintf(buf, sizeof(buf), "if (EditUnitTypeProperties ~= nil) then EditUnitTypeProperties(\"%s\") end;", Editor.ShownUnitTypes[Editor.CursorUnitIndex]->Ident.c_str());
				Editor.CursorUnitIndex = -1;
				CclCommand(buf);
				return;
			}
			*/
			if (Editor.CursorUnitIndex != (int) Editor.ShownUnitTypes.size()) {
				if (MouseButtons & LeftButton) {
					Editor.SelectedUnitIndex = Editor.CursorUnitIndex;
					CursorBuilding = const_cast<CUnitType *>(Editor.ShownUnitTypes[Editor.CursorUnitIndex]);
					return;
				} else if (MouseButtons & RightButton) {
					char buf[256];
					snprintf(buf, sizeof(buf), "if (EditUnitTypeProperties ~= nil) then EditUnitTypeProperties(\"%s\") end;", Editor.ShownUnitTypes[Editor.CursorUnitIndex]->Ident.c_str());
					Editor.CursorUnitIndex = -1;
					CclCommand(buf);
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
	if (CursorOn == CursorOnMap) {
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
						PlayGameSound(GameSounds.PlacementSuccess[CPlayer::GetThisPlayer()->Race].Sound,
									  MaxSampleVolume);
						EditorPlaceUnit(tilePos, *CursorBuilding, CPlayer::Players[Editor.SelectedPlayer]);
						UnitPlacedThisPress = true;
						UI.StatusLine.Clear();
					} else {
						UI.StatusLine.Set(_("Unit cannot be placed here."));
						PlayGameSound(GameSounds.PlacementError[CPlayer::GetThisPlayer()->Race].Sound,
									  MaxSampleVolume);
					}
				}
			} else if (Editor.State == EditorSetStartLocation) {
				CPlayer::Players[Editor.SelectedPlayer]->StartPos = tilePos;
				CPlayer::Players[Editor.SelectedPlayer]->StartMapLayer = UI.CurrentMapLayer->ID;
			}
		} else if (MouseButtons & MiddleButton) {
			// enter move map mode
			CursorStartScreenPos = CursorScreenPos;
			GameCursor = UI.Scroll.Cursor;
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
		case 'r': // CTRL+R Randomize map
			if (KeyModifiers & ModifierControl) {
				Editor.CreateRandomMap();
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
			if (UnitUnderCursor != nullptr && CMap::Map.Info.PlayerType[(int) key - '1'] != PlayerNobody) {
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

	// Scrollbar
	if (UI.ButtonPanel.X + 4 < CursorScreenPos.x
		&& CursorScreenPos.x < UI.ButtonPanel.X + 176 - 4
		&& UI.ButtonPanel.Y + 4 < CursorScreenPos.y
		//Wyrmgus start
//		&& CursorScreenPos.y < UI.ButtonPanel.Y + 24) {
		&& CursorScreenPos.y < UI.ButtonPanel.Y) {
		//Wyrmgus end
		return true;
	}
	//Wyrmgus start
//	int bx = UI.InfoPanel.X + 8;
	int bx = UI.InfoPanel.X + 26;
	//Wyrmgus end
	int by = UI.InfoPanel.Y + 4 + IconHeight + 10;
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
			bx = UI.InfoPanel.X + 26;
			//Wyrmgus end
			by += 20;
		}
		if (bx < screenPos.x && screenPos.x < bx + 20 && by < screenPos.y && screenPos.y < by + 20) {
			if (CMap::Map.Info.PlayerType[i] != PlayerNobody) {
				char buf[256];
				//Wyrmgus start
//				snprintf(buf, sizeof(buf), _("Select player #%d"), i);
				snprintf(buf, sizeof(buf), _("Select player #%d"), (i == PlayerNumNeutral) ? 16 : i + 1);
				//Wyrmgus end
				UI.StatusLine.Set(buf);
			} else {
				UI.StatusLine.Clear();
			}
			Editor.CursorPlayer = i;
#if 0
			ButtonUnderCursor = i + 100;
			CursorOn = CursorOnButton;
#endif
			return true;
		}
		bx += 20;
	}

	int i = Editor.UnitIndex;
	by = UI.ButtonPanel.Y + 24;
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
				ButtonAction &ba = *(new ButtonAction);
				CurrentButtons.push_back(ba);
			}
			CurrentButtons[j].Hint = Editor.ShownUnitTypes[i]->GetName().utf8().get_data();
			CurrentButtons[j].Pos = j;
			CurrentButtons[j].Level = 0;
			CurrentButtons[j].Action = ButtonEditorUnit;
			CurrentButtons[j].ValueStr = Editor.ShownUnitTypes[i]->Ident;
			CurrentButtons[j].Value = Editor.ShownUnitTypes[i]->GetIndex();
			CurrentButtons[j].Popup = "popup-unit";
		}
		//Wyrmgus end
		if (x < screenPos.x && screenPos.x < x + IconWidth
			&& y < screenPos.y && screenPos.y < y + IconHeight) {
			//Wyrmgus start
			/*
			char buf[256];
			snprintf(buf, sizeof(buf), "%s \"%s\"",
					 Editor.ShownUnitTypes[i]->Ident.c_str(),
					 Editor.ShownUnitTypes[i]->Name.c_str());
			UI.StatusLine.Set(buf);
			*/
			if (!Preference.NoStatusLineTooltips) {
				char buf[256];
				if (i == (int) Editor.ShownUnitTypes.size()) {
					snprintf(buf, sizeof(buf), "Create Unit Type");
				} else {
					snprintf(buf, sizeof(buf), "%s \"%s\"",
							 Editor.ShownUnitTypes[i]->Ident.c_str(),
							 Editor.ShownUnitTypes[i]->GetName().utf8().get_data());
				}
				UI.StatusLine.Set(buf);
			}
			//Wyrmgus end
			Editor.CursorUnitIndex = i;
			return true;
		}
		++i;
	}
	return false;
}

static bool EditorCallbackMouse_EditTileArea(const PixelPos &screenPos)
{
	//Wyrmgus start
//	int bx = UI.InfoPanel.X + 4;
	int bx = UI.InfoPanel.X + 11;
	//Wyrmgus end
	int by = UI.InfoPanel.Y + 4 + IconHeight + 10;

	for (int i = 0; i < 6; ++i) {
		if (bx < screenPos.x && screenPos.x < bx + 100 && by < screenPos.y && screenPos.y < by + 18) {
			ButtonUnderCursor = i + 300;
			CursorOn = CursorOnButton;
			return true;
		}
		by += 20;
		//Wyrmgus start
		if (i == 3) {
			by = UI.InfoPanel.Y + 4 + IconHeight + 10;
			bx += 100;
		}
		//Wyrmgus end
	}

	int i = Editor.TileIndex;
	by = UI.ButtonPanel.Y + 24;
	while (by < UI.ButtonPanel.Y + ButtonPanelHeight - CMap::Map.GetCurrentPixelTileSize().y) {
		if (i >= (int)Editor.ShownTileTypes.size()) {
			break;
		}
		//Wyrmgus start
//		bx = UI.ButtonPanel.X + 10;
		bx = UI.ButtonPanel.X + 10 + 6;
		//Wyrmgus end
		while (bx < UI.ButtonPanel.X + ButtonPanelWidth - CMap::Map.GetCurrentPixelTileSize().x) {
			if (i >= (int)Editor.ShownTileTypes.size()) {
				break;
			}
			if (bx < screenPos.x && screenPos.x < bx + CMap::Map.GetCurrentPixelTileSize().x
				&& by < screenPos.y && screenPos.y < by + CMap::Map.GetCurrentPixelTileSize().y) {
				//Wyrmgus start
//				const int tile = Editor.ShownTileTypes[i];
//				const int tileindex = CMap::Map.Tileset->findTileIndexByTile(tile);
//				const int base = CMap::Map.Tileset->tiles[tileindex].tileinfo.BaseTerrain;
//				UI.StatusLine.Set(CMap::Map.Tileset->getTerrainName(base));
				UI.StatusLine.Set(Editor.ShownTileTypes[i]->Name);
				//Wyrmgus end
				Editor.CursorTileIndex = i;
				return true;
			}
			//Wyrmgus start
//			bx += CMap::Map.GetCurrentPixelTileSize().x + 8;
			bx += CMap::Map.GetCurrentPixelTileSize().x + 30;
			//Wyrmgus end
			i++;
		}
		//Wyrmgus start
//		by += CMap::Map.GetCurrentPixelTileSize().y + 2;
		by += CMap::Map.GetCurrentPixelTileSize().y + 18;
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
	static int LastMapX;
	static int LastMapY;

	PixelPos restrictPos = pos;
	HandleCursorMove(&restrictPos.x, &restrictPos.y); // Reduce to screen
	const PixelPos screenPos = pos;

	// Move map.
	if (GameCursor == UI.Scroll.Cursor) {
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
		UI.MouseViewport->Set(tilePos, CMap::Map.GetCurrentPixelTileSize() / 2);
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
	if (CursorOn == CursorOnMap && (MouseButtons & LeftButton)
		&& (Editor.State == EditorEditTile || Editor.State == EditorEditUnit)) {
		Vec2i vpTilePos = UI.SelectedViewport->MapPos;
		// Scroll the map
		if (CursorScreenPos.x <= UI.SelectedViewport->GetTopLeftPos().x) {
			vpTilePos.x--;
			UI.SelectedViewport->Set(vpTilePos, CMap::Map.GetCurrentPixelTileSize() / 2);
		} else if (CursorScreenPos.x >= UI.SelectedViewport->GetBottomRightPos().x) {
			vpTilePos.x++;
			UI.SelectedViewport->Set(vpTilePos, CMap::Map.GetCurrentPixelTileSize() / 2);
		}

		if (CursorScreenPos.y <= UI.SelectedViewport->GetTopLeftPos().y) {
			vpTilePos.y--;
			UI.SelectedViewport->Set(vpTilePos, CMap::Map.GetCurrentPixelTileSize() / 2);
		} else if (CursorScreenPos.y >= UI.SelectedViewport->GetBottomRightPos().y) {
			vpTilePos.y++;
			UI.SelectedViewport->Set(vpTilePos, CMap::Map.GetCurrentPixelTileSize() / 2);
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
	if (CursorOn == CursorOnMinimap && (MouseButtons & LeftButton)) {
		RestrictCursorToMinimap();
		const Vec2i tilePos = UI.Minimap.ScreenToTilePos(CursorScreenPos);

		UI.SelectedViewport->Center(CMap::Map.TilePosToMapPixelPos_Center(tilePos, UI.CurrentMapLayer));
		return;
	}

	MouseScrollState = ScrollNone;
	GameCursor = UI.Point.Cursor;
	CursorOn = CursorOnUnknown;
	Editor.CursorPlayer = -1;
	Editor.CursorUnitIndex = -1;
	Editor.CursorTileIndex = -1;
	ButtonUnderCursor = -1;
	OldButtonUnderCursor = -1;

	// Minimap
	if (UI.Minimap.Contains(screenPos)) {
		CursorOn = CursorOnMinimap;
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
	//Wyrmgus start
//	if (UI.InfoPanel.X + 4 < CursorScreenPos.x
	if (UI.InfoPanel.X + 11 < CursorScreenPos.x
	//Wyrmgus end
		//Wyrmgus start
//		&& CursorScreenPos.x < UI.InfoPanel.X + 4 + Editor.Select.Icon->G->Width
//		&& UI.InfoPanel.Y + 4 < CursorScreenPos.y
//		&& CursorScreenPos.y < UI.InfoPanel.Y + 4 + Editor.Select.Icon->G->Width) {
		&& CursorScreenPos.x < UI.InfoPanel.X + 11 + Editor.Select.Icon->G->Width
		&& UI.InfoPanel.Y + 7 < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + 7 + Editor.Select.Icon->G->Width) {
		//Wyrmgus end
		// FIXME: what is this button?
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = SelectButton;
		CursorOn = CursorOnButton;
		UI.StatusLine.Set(_("Select Mode"));
		return;
	}
	//Wyrmgus start
//	if (UI.InfoPanel.X + 4 + UNIT_ICON_X < CursorScreenPos.x
//		&& CursorScreenPos.x < UI.InfoPanel.X + 4 + UNIT_ICON_X + Editor.Units.Icon->G->Width
//		&& UI.InfoPanel.Y + 4 + UNIT_ICON_Y < CursorScreenPos.y
//		&& CursorScreenPos.y < UI.InfoPanel.Y + 4 + UNIT_ICON_Y + Editor.Units.Icon->G->Height) {
	if (UI.InfoPanel.X + 11 + UNIT_ICON_X < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + 11 + UNIT_ICON_X + Editor.Units.Icon->G->Width
		&& UI.InfoPanel.Y + 7 + UNIT_ICON_Y < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + 7 + UNIT_ICON_Y + Editor.Units.Icon->G->Height) {
	//Wyrmgus end
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = UnitButton;
		CursorOn = CursorOnButton;
		UI.StatusLine.Set(_("Unit Mode"));
		return;
	}
	if (Editor.TerrainEditable) {
		//Wyrmgus start
//		if (UI.InfoPanel.X + 4 + TILE_ICON_X < CursorScreenPos.x
//			&& CursorScreenPos.x < UI.InfoPanel.X + 4 + TILE_ICON_X + CMap::Map.GetCurrentPixelTileSize().x + 7
		if (UI.InfoPanel.X + 11 + TILE_ICON_X < CursorScreenPos.x
			&& CursorScreenPos.x < UI.InfoPanel.X + 11 + TILE_ICON_X + CMap::Map.GetCurrentPixelTileSize().x + 7
		//Wyrmgus end
			&& UI.InfoPanel.Y + 4 + TILE_ICON_Y < CursorScreenPos.y
			&& CursorScreenPos.y < UI.InfoPanel.Y + 4 + TILE_ICON_Y + CMap::Map.GetCurrentPixelTileSize().y + 7) {
			ButtonAreaUnderCursor = -1;
			ButtonUnderCursor = TileButton;
			CursorOn = CursorOnButton;
			UI.StatusLine.Set(_("Tile Mode"));
			return;
		}
	}

	int StartUnitWidth = Editor.StartUnit ?
						 Editor.StartUnit->GetIcon()->G->Width : CMap::Map.GetCurrentPixelTileSize().x + 7;
	int StartUnitHeight = Editor.StartUnit ?
						  Editor.StartUnit->GetIcon()->G->Height : CMap::Map.GetCurrentPixelTileSize().y + 7;
	//Wyrmgus start
//	if (UI.InfoPanel.X + 4 + START_ICON_X < CursorScreenPos.x
//		&& CursorScreenPos.x < UI.InfoPanel.X + 4 + START_ICON_X + StartUnitWidth
//		&& UI.InfoPanel.Y + 4 + START_ICON_Y < CursorScreenPos.y
//		&& CursorScreenPos.y < UI.InfoPanel.Y + 4 + START_ICON_Y + StartUnitHeight) {
	if (UI.InfoPanel.X + 11 + START_ICON_X < CursorScreenPos.x
		&& CursorScreenPos.x < UI.InfoPanel.X + 11 + START_ICON_X + StartUnitWidth
		&& UI.InfoPanel.Y + 5 + START_ICON_Y < CursorScreenPos.y
		&& CursorScreenPos.y < UI.InfoPanel.Y + 5 + START_ICON_Y + StartUnitHeight) {
	//Wyrmgus end
		ButtonAreaUnderCursor = -1;
		ButtonUnderCursor = StartButton;
		CursorOn = CursorOnButton;
		UI.StatusLine.Set(_("Set Start Location"));
		return;
	}
	if (UI.MenuButton.X != -1 && UI.MenuButton.Contains(screenPos)) {
		ButtonAreaUnderCursor = ButtonAreaMenu;
		ButtonUnderCursor = ButtonUnderMenu;
		CursorOn = CursorOnButton;
		return;
	}

	// Minimap
	if (UI.Minimap.Contains(screenPos)) {
		CursorOn = CursorOnMinimap;
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
		CursorOn = CursorOnMap;

		if (UI.MouseViewport) {
			// Look if there is an unit under the cursor.
			const PixelPos cursorMapPos = UI.MouseViewport->ScreenToMapPixelPos(CursorScreenPos);
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
	const std::string filename = LibraryFileName(Parameters::Instance.luaEditorStartFilename.c_str());
	if (!CanAccessFile(filename.c_str())) {
		fprintf(stderr, "Editor configuration file '%s' was not found\n"
				"Specify another with '-E file.lua'\n",
				Parameters::Instance.luaEditorStartFilename.c_str());
		ExitFatal(-1);
	}

	ShowLoadProgress(_("Loading Script \"%s\""), filename.c_str());
	LuaLoadFile(filename);
	LuaGarbageCollect();
	
	//Wyrmgus start
	if (this->UnitTypes.size() == 0) {
		//if editor's unit types vector is still empty after loading the editor's lua file, then fill it automatically
		for (CUnitType *unit_type : CUnitType::GetAll()) {
			if (unit_type->IsHiddenInEditor()) {
				continue;
			}
			
			this->UnitTypes.push_back(unit_type->Ident);
		}
	}
	//Wyrmgus end

	CPlayer::SetThisPlayer(CPlayer::Players[0]);

	FlagRevealMap = 1; // editor without fog and all visible
	CMap::Map.NoFogOfWar = true;

	//Wyrmgus start
//	if (!*CurrentMapPath) { // new map!
	if (!*CurrentMapPath || IsMod) { // new map or is a mod
	//Wyrmgus end
		InitUnitTypes(1);
		//
		// Inititialize Map / Players.
		//
		InitPlayers();
		for (int i = 0; i < PlayerMax; ++i) {
			if (i == PlayerNumNeutral) {
				CreatePlayer(PlayerNeutral);
				CMap::Map.Info.PlayerType[i] = PlayerNeutral;
				//Wyrmgus start
//				CMap::Map.Info.PlayerSide[i] = CPlayer::Players[i]->Race = 0;
				CPlayer::Players[i]->SetCivilization(0);
				CMap::Map.Info.PlayerSide[i] = CPlayer::Players[i]->Race;
				//Wyrmgus end
			} else {
				CreatePlayer(PlayerNobody);
				CMap::Map.Info.PlayerType[i] = PlayerNobody;
			}
		}

		//Wyrmgus start
//		CMap::Map.Fields = new CMapField[CMap::Map.Info.MapWidth * CMap::Map.Info.MapHeight];
		CMap::Map.ClearMapLayers();
		CMapLayer *map_layer = new CMapLayer(CMap::Map.Info.MapWidth, CMap::Map.Info.MapHeight);
		map_layer->ID = CMap::Map.MapLayers.size();
		CMap::Map.MapLayers.push_back(map_layer);
		CMap::Map.Info.MapWidths.clear();
		CMap::Map.Info.MapWidths.push_back(CMap::Map.Info.MapWidth);
		CMap::Map.Info.MapHeights.clear();
		CMap::Map.Info.MapHeights.push_back(CMap::Map.Info.MapHeight);
		//Wyrmgus end

		const int defaultTile = CMap::Map.Tileset->getDefaultTileIndex();
		//Wyrmgus start
		const CTileset &tileset = *CMap::Map.Tileset;
		//Wyrmgus end

		//Wyrmgus start
		for (CMapLayer *map_layer : CMap::Map.MapLayers) {
			int max_tile_index = map_layer->GetWidth() * map_layer->GetHeight();
			for (int i = 0; i < max_tile_index; ++i) {
				//Wyrmgus start
	//			CMap::Map.Fields[i].setTileIndex(*CMap::Map.Tileset, defaultTile, 0);
				map_layer->Field(i)->setTileIndex(*CMap::Map.Tileset, tileset.getTileNumber(defaultTile, true, false), 0);
				//Wyrmgus end
			}
		}
		//Wyrmgus start
		GameSettings.Resources = SettingsPresetMapDefault;
		//Wyrmgus start
//		CreateGame("", &CMap::Map);
		//Wyrmgus end
	//Wyrmgus start
	}
	if (!*CurrentMapPath) {
		CreateGame("", &CMap::Map, IsMod);
	//Wyrmgus end
	} else {
		CreateGame(CurrentMapPath, &CMap::Map, IsMod);
	}

	ReplayRevealMap = 1;
	FlagRevealMap = 0;
	Editor.SelectedPlayer = PlayerNumNeutral;

	// Place the start points, which the loader discarded.
	for (int i = 0; i < PlayerMax; ++i) {
		if (CMap::Map.Info.PlayerType[i] != PlayerNobody) {
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
	ButtonPanelWidth = 243;
	ButtonPanelHeight = 186;
	//Wyrmgus end

	CalculateMaxIconSize();
	VisibleUnitIcons = CalculateVisibleIcons();

	if (!StartUnitName.empty()) {
		StartUnit = CUnitType::Get(StartUnitName);
	}
	Select.Icon = nullptr;
	Select.Load();
	Units.Icon = nullptr;
	Units.Load();

	//Wyrmgus start
//	CMap::Map.Tileset->fillSolidTiles(&Editor.ShownTileTypes);
	Editor.ShownTileTypes.clear();
	for (CTerrainType *terrain_type : CTerrainType::GetAll()) {
		if (!terrain_type->Hidden && terrain_type->PixelTileSize == CMap::Map.GetCurrentPixelTileSize()) {
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
	fullName = StratagusLibPath + "/" + file;
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
	
//	if (SaveStratagusMap(fullName, CMap::Map, Editor.TerrainEditable) == -1) {
	if (SaveStratagusMap(fullName, CMap::Map, Editor.TerrainEditable && !is_mod, is_mod) == -1) {
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
	bool OldCommandLogDisabled = CommandLogDisabled;
	const EventCallback *old_callbacks = GetCallbacks();
	bool first_init = true;

	CommandLogDisabled = true;

	gcn::Widget *oldTop = Gui->getTop();

	editorContainer = new gcn::Container();
	editorContainer->setDimension(gcn::Rectangle(0, 0, Video.Width, Video.Height));
	editorContainer->setOpaque(false);
	Gui->setTop(editorContainer);

	editorUnitSliderListener = new EditorUnitSliderListener();
	editorSliderListener = new EditorSliderListener();

	editorUnitSlider = new gcn::Slider();
	editorUnitSlider->setBaseColor(gcn::Color(38, 38, 78));
	editorUnitSlider->setForegroundColor(gcn::Color(200, 200, 120));
	editorUnitSlider->setBackgroundColor(gcn::Color(200, 200, 120));
	editorUnitSlider->setVisible(false);
	editorUnitSlider->addActionListener(editorUnitSliderListener);

	editorSlider = new gcn::Slider();
	editorSlider->setBaseColor(gcn::Color(38, 38, 78));
	editorSlider->setForegroundColor(gcn::Color(200, 200, 120));
	editorSlider->setBackgroundColor(gcn::Color(200, 200, 120));
	editorSlider->setVisible(false);
	editorSlider->addActionListener(editorSliderListener);

	UpdateMinimap = true;

	while (1) {
		Editor.MapLoaded = false;
		Editor.Running = EditorEditing;

		Editor.Init();

		if (first_init) {
			first_init = false;
			//Wyrmgus start
//			editorUnitSlider->setSize(ButtonPanelWidth/*176*/, 16);
//			editorSlider->setSize(ButtonPanelWidth/*176*/, 16);
			editorUnitSlider->setSize(218 - 24 - 6, 16); // adapt to new UI size, should make this more scriptable
			editorSlider->setSize(218 - 24 - 6, 16);
			//Wyrmgus end
			//Wyrmgus start
//			editorContainer->add(editorUnitSlider, UI.ButtonPanel.X + 2, UI.ButtonPanel.Y - 16);
//			editorContainer->add(editorSlider, UI.ButtonPanel.X + 2, UI.ButtonPanel.Y - 16);
			editorContainer->add(editorUnitSlider, UI.InfoPanel.X + 12, UI.InfoPanel.Y + 160 - 24);
			editorContainer->add(editorSlider, UI.InfoPanel.X + 12, UI.InfoPanel.Y + 160 - 24);
			//Wyrmgus end
		}
		//ProcessMenu("menu-editor-tips", 1);
		InterfaceState = IfaceStateNormal;

		SetVideoSync();

		GameCursor = UI.Point.Cursor;
		InterfaceState = IfaceStateNormal;
		Editor.State = EditorSelecting;
		UI.SelectedViewport = UI.Viewports;
		TileCursorSize = 1;

		while (Editor.Running) {
			if (FrameCounter % FRAMES_PER_SECOND == 0) {
				if (UpdateMinimap) {
					UI.Minimap.Update();
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
				if (CursorOn == CursorOnMap && (MouseButtons & LeftButton) &&
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

		LoadCcl(Parameters::Instance.luaStartFilename); // Reload the main config file

		PreMenuSetup();

		InterfaceState = IfaceStateMenu;
		GameCursor = UI.Point.Cursor;

		Video.ClearScreen();
		Invalidate();
	}

	CommandLogDisabled = OldCommandLogDisabled;
	SetCallbacks(old_callbacks);
	Gui->setTop(oldTop);
	delete editorContainer;
	delete editorUnitSliderListener;
	delete editorSliderListener;
	delete editorUnitSlider;
	delete editorSlider;
}

/**
**  Start the editor
**
**  @param filename  Map to load, null to create a new map
*/
//Wyrmgus start
//void StartEditor(const char *filename)
void StartEditor(const char *filename, bool is_mod)
//Wyrmgus end
{
	std::string nc, rc;

	GetDefaultTextColors(nc, rc);
	if (filename) {
		if (strcpy_s(CurrentMapPath, sizeof(CurrentMapPath), filename) != 0) {
			filename = nullptr;
		}
	}
	if (!filename) {
		// new map, choose some default values
		strcpy_s(CurrentMapPath, sizeof(CurrentMapPath), "");
		// CMap::Map.Info.Description.clear();
		// CMap::Map.Info.MapWidth = 64;
		// CMap::Map.Info.MapHeight = 64;
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
	Invalidate();

	Editor.TerrainEditable = true;

	Editor.ShownTileTypes.clear();
	CleanGame();
	CleanPlayers();

	SetDefaultTextColors(nc, rc);
}
