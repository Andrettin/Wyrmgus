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

#pragma once

#include "ui/icon.h"
#include "vec2i.h"

//Wyrmgus start
class CUnit;
//Wyrmgus end

namespace wyrmgus {
	class terrain_type;
	class unit_type;
}

enum EditorStateType {
	EditorSelecting,         /// Select
	EditorEditTile,          /// Edit tiles
	EditorEditUnit,          /// Edit units
	EditorSetStartLocation   /// Set the start location
};

class CEditor final : public QObject, public singleton<CEditor>
{
	Q_OBJECT

	Q_PROPERTY(bool running READ is_running WRITE set_running_async NOTIFY running_changed)

public:
	CEditor();

	void Init();

	bool is_running() const
	{
		return this->running;
	}

	void set_running(const bool running)
	{
		if (running == this->is_running()) {
			return;
		}

		this->running = running;

		emit running_changed();
	}

	void set_running_async(const bool running);

signals:
	void running_changed();

public:
	std::vector<std::string> UnitTypes;             /// Sorted editor unit-type table.
	std::vector<const wyrmgus::unit_type *> ShownUnitTypes;  /// Shown editor unit-type table.
	//Wyrmgus start
//	std::vector<unsigned int> ShownTileTypes;        /// Shown editor tile-type table.
	std::vector<wyrmgus::terrain_type *> ShownTileTypes;        /// Shown editor tile-type table.
	//Wyrmgus end

	bool TerrainEditable = true; /// Is the terrain editable ?
	IconConfig Select;           /// Editor's select icon.
	IconConfig Units;            /// Editor's units icon.
	std::string StartUnitName;   /// name of the Unit used to display the start location.
	const wyrmgus::unit_type *StartUnit = nullptr;  /// Unit used to display the start location.

	int UnitIndex = 0;               /// Unit icon draw index.
	int CursorUnitIndex = -1;         /// Unit icon under cursor.
	int SelectedUnitIndex = -1;       /// Unit type to draw.

	int TileIndex = 0;              /// tile icon draw index.
	int CursorTileIndex = -1;		/// tile icon under cursor.
	int SelectedTileIndex = -1;       /// tile type to draw.

	int CursorPlayer = -1;            /// Player under the cursor.
	int SelectedPlayer;          /// Player selected for draw.

	bool WriteCompressedMaps = true;    /// Use compression when saving

private:
	bool running = false;

public:
	EditorStateType State;       /// Current editor state

	int PopUpX = -1;
	int PopUpY = -1;
};

extern char TileToolRandom;

/// Start the editor
extern void StartEditor(const std::string &filename);

/// Editor main event loop
extern void EditorMainLoop();
/// Update editor display
extern void EditorUpdateDisplay();

//Wyrmgus start
extern void EditorActionRemoveUnit(CUnit &unit, bool display = true);
//Wyrmgus end

/// Save a map from editor
extern int EditorSaveMap(const std::string &file);

extern std::string get_user_maps_path();

/// Register ccl features
extern void EditorCclRegister();

//Wyrmgus start
extern void RecalculateShownUnits();
//Wyrmgus end
