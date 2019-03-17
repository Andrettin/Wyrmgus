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
/**@name editor.h - The editor file. */
//
//      (c) Copyright 2002-2006 by Lutz Sammer
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

#ifndef __EDITOR_H__
#define __EDITOR_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "ui/icon_config.h"
#include "vec2i.h"

#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnitType;
//Wyrmgus start
class CUnit;
class CTerrainType;
//Wyrmgus end


enum EditorRunningType {
	EditorNotRunning = 0,    /// Not Running
	EditorStarted = 1,       /// Editor Enabled at all
	EditorCommandLine = 2,   /// Called from Command Line
	EditorEditing = 4        /// Editor is fully running
};

enum EditorStateType {
	EditorSelecting,         /// Select
	EditorEditTile,          /// Edit tiles
	EditorEditUnit,          /// Edit units
	EditorSetStartLocation   /// Set the start location
};

class CEditor
{
public:
	CEditor();
	~CEditor() {}

	void Init();
	/// Make random map
	void CreateRandomMap() const;


	std::vector<std::string> UnitTypes;             /// Sorted editor unit-type table.
	std::vector<const CUnitType *> ShownUnitTypes;  /// Shown editor unit-type table.
	//Wyrmgus start
//	std::vector<unsigned int> ShownTileTypes;        /// Shown editor tile-type table.
	std::vector<CTerrainType *> ShownTileTypes;        /// Shown editor tile-type table.
	//Wyrmgus end

	bool TerrainEditable;        /// Is the terrain editable ?
	IconConfig Select;           /// Editor's select icon.
	IconConfig Units;            /// Editor's units icon.
	std::string StartUnitName;   /// name of the Unit used to display the start location.
	const CUnitType *StartUnit;  /// Unit used to display the start location.

	int UnitIndex;               /// Unit icon draw index.
	int CursorUnitIndex;         /// Unit icon under cursor.
	int SelectedUnitIndex;       /// Unit type to draw.

	int TileIndex;              /// tile icon draw index.
	int CursorTileIndex;		/// tile icon under cursor.
	int SelectedTileIndex;       /// tile type to draw.

	int CursorPlayer;            /// Player under the cursor.
	int SelectedPlayer;          /// Player selected for draw.

	bool MapLoaded;              /// Map loaded in editor
	bool WriteCompressedMaps;    /// Use compression when saving

	EditorRunningType Running;   /// Editor is running

	EditorStateType State;       /// Current editor state

	int PopUpX;
	int PopUpY;

};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern CEditor Editor;

extern char TileToolRandom;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Start the editor
//Wyrmgus start
//extern void StartEditor(const char *filename);
extern void StartEditor(const char *filename, bool is_mod = false);
//Wyrmgus end

/// Editor main event loop
extern void EditorMainLoop();
/// Update editor display
extern void EditorUpdateDisplay();

//Wyrmgus start
extern void EditorActionRemoveUnit(CUnit &unit, bool display = true);
//Wyrmgus end

/// Save a map from editor
//Wyrmgus start
//extern int EditorSaveMap(const std::string &file);
extern int EditorSaveMap(const std::string &file, bool is_mod = false);
//Wyrmgus end

/// Register ccl features
extern void EditorCclRegister();

/// Update surroundings for tile changes
//Wyrmgus start
//extern void EditorTileChanged(const Vec2i &pos);
extern void EditorTileChanged(const Vec2i &pos, int tile);

//extern void EditorChangeTile(const Vec2i &pos, int tileIndex, int d);
extern void EditorChangeTile(const Vec2i &pos, int tileIndex);
//Wyrmgus end


//Wyrmgus start
extern void RecalculateShownUnits();
//Wyrmgus end

#endif
