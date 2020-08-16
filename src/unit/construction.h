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
//      (c) Copyright 1998-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/**
**  @class construction construction.h
**
**  Each building perhaps also units can have its own construction
**    frames. This construction frames are currently not animated,
**    this is planned for the future. What construction frames a
**    building has, is handled by UnitType::Construction.
**
**  The construction structure members:
**
**  CConstruction::File
**
**    Path file name of the sprite file.
**
**  CConstruction::Frames
**
**    Frames of the construction sequence.
**
**  CConstruction::Sprite
**
**    Sprite image.
**
**  CConstruction::Width CConstruction::Height
**
**    Size of a sprite frame in pixels. All frames of a sprite have
**    the same size. Also all sprites (tilesets) must have the same
**    size.
**
**    @todo
**      Need ::TilesetByName, ...
**      Only fixed number of constructions supported, more than
**      a single construction frame is not supported, animated
**      constructions aren't supported.
*/

#include "database/data_entry.h"
#include "database/data_type.h"

class CGraphic;
class CPlayerColorGraphic;
struct lua_State;

static int CclDefineConstruction(lua_State *l);

enum class ConstructionFileType {
	Construction,
	Main
};

/// Construction frame
class CConstructionFrame
{
public:
	int Percent = 0;                    /// Percent complete
	ConstructionFileType File = ConstructionFileType::Construction; /// Graphic to use
	int Frame = 0;                      /// Frame number
	CConstructionFrame *Next = nullptr; /// Next pointer
};

namespace wyrmgus {

/// Construction shown during construction of a building
class construction final : public data_entry, public data_type<construction>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "construction";
	static constexpr const char *database_folder = "constructions";

	explicit construction(const std::string &identifier) : data_entry(identifier)
	{
	}

	~construction();

	virtual void process_sml_property(const sml_property &property) override;

	void Clean();
	void Load();

	const std::filesystem::path &get_image_file() const
	{
		return this->image_file;
	}

	const QSize &get_frame_size() const
	{
		return this->frame_size;
	}
	
	int get_frame_width() const
	{
		return this->get_frame_size().width();
	}
	
	int get_frame_height() const
	{
		return this->get_frame_size().height();
	}

private:
	std::filesystem::path image_file;
	QSize frame_size = QSize(0, 0); //sprite frame size

public:
	CConstructionFrame *Frames = nullptr;  /// construction frames
	CPlayerColorGraphic *Sprite = nullptr; /// construction sprite image

	friend int ::CclDefineConstruction(lua_State *l);
};

}

/// Load the graphics for constructions
extern void LoadConstructions();
/// Count the amount of constructions to load
extern int GetConstructionsCount();

/// Register ccl features
extern void ConstructionCclRegister();
