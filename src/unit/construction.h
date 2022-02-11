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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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
**  @class construction construction.h
**
**  Each building perhaps also units can have its own construction
**    frames. This construction frames are currently not animated,
**    this is planned for the future. What construction frames a
**    building has, is handled by UnitType::Construction.
*/

#include "database/data_entry.h"
#include "database/data_type.h"

class CGraphic;
class CPlayerColorGraphic;
struct lua_State;

#undef main

namespace wyrmgus {

enum class construction_image_type {
	construction,
	main
};

class construction_frame final
{
public:
	void process_gsml_property(const gsml_property &property);

	void process_gsml_scope(const gsml_data &scope)
	{
		throw std::runtime_error("Invalid construction frame scope: \"" + scope.get_tag() + "\".");
	}

	int get_percent() const
	{
		return this->percent;
	}

	construction_image_type get_image_type() const
	{
		return this->image_type;
	}

	int get_frame() const
	{
		return this->frame;
	}

	const construction_frame *get_next() const
	{
		return this->next;
	}

private:
	int percent = 0;                    /// Percent complete
	construction_image_type image_type = construction_image_type::construction; /// Graphic to use
	int frame = 0;                      /// Frame number
	const construction_frame *next = nullptr; /// Next pointer

	friend class construction;
};

/// Construction shown during construction of a building
class construction final : public data_entry, public data_type<construction>
{
	Q_OBJECT

	Q_PROPERTY(QSize frame_size MEMBER frame_size READ get_frame_size)

public:
	static constexpr const char *class_identifier = "construction";
	static constexpr const char *database_folder = "constructions";

	explicit construction(const std::string &identifier) : data_entry(identifier)
	{
	}

	~construction();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	virtual void check() const
	{
		if (this->frames.empty()) {
			throw std::runtime_error("Construction \"" + this->get_identifier() + "\" has no frames.");
		}
	}

	void load();

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

	const std::shared_ptr<CPlayerColorGraphic> &get_graphics() const
	{
		return this->graphics;
	}

	const construction_frame *get_initial_frame() const
	{
		return this->frames.front().get();
	}

private:
	std::filesystem::path image_file;
	QSize frame_size = QSize(0, 0); //sprite frame size
	std::vector<std::unique_ptr<construction_frame>> frames;  /// construction frames
	std::shared_ptr<CPlayerColorGraphic> graphics; /// construction sprite image
};

}

/// Load the graphics for constructions
extern void LoadConstructions();
/// Count the amount of constructions to load
extern int GetConstructionsCount();
