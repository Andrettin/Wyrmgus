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
//      (c) Copyright 1998-2021 by Lutz Sammer and Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"
#include <vec2i.h>

class CGraphic;
enum class ButtonCmd;

namespace wyrmgus {

class civilization;
class renderer;
class unit_type;
enum class cursor_type;

/// Private type which specifies the cursor-type
class cursor final : public data_entry, public data_type<cursor>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::cursor_type type MEMBER type READ get_type)
	Q_PROPERTY(wyrmgus::civilization* civilization MEMBER civilization)
	Q_PROPERTY(QString file READ get_file_qstring)
	Q_PROPERTY(QPoint hot_pos MEMBER hot_pos READ get_hot_pos)
	Q_PROPERTY(QSize frame_size MEMBER frame_size READ get_frame_size)
	Q_PROPERTY(int frame_rate MEMBER frame_rate READ get_frame_rate)

public:
	static constexpr const char *class_identifier = "cursor";
	static constexpr const char *database_folder = "cursors";

	static void clear();

	static cursor *get_cursor_by_type(const cursor_type type)
	{
		auto find_iterator = cursor::cursors_by_type.find(type);
		if (find_iterator != cursor::cursors_by_type.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	static void map_cursor(const cursor_type type, cursor *cursor)
	{
		if (cursor::cursors_by_type.contains(type)) {
			throw std::runtime_error("Another cursor is already registered for type \"" + std::to_string(static_cast<int>(type)) + "\".");
		}

		cursor::cursors_by_type[type] = cursor;
	}

	static cursor *get_current_cursor()
	{
		return cursor::current_cursor;
	}

	static void set_current_cursor(cursor *cursor, const bool notify);
	static void on_current_cursor_changed();

private:
	static inline std::map<cursor_type, cursor *> cursors_by_type;
	static inline cursor *current_cursor = nullptr;

public:
	explicit cursor(const std::string &identifier);
	~cursor();

	virtual void initialize() override;

	cursor_type get_type() const
	{
		return this->type;
	}

	const wyrmgus::civilization *get_civilization() const
	{
		return this->civilization;
	}

	const std::filesystem::path &get_file() const
	{
		return this->file;
	}

	void set_file(const std::filesystem::path &filepath);

	QString get_file_qstring() const
	{
		return QString::fromStdString(this->get_file().string());
	}

	Q_INVOKABLE void set_file(const std::string &filepath)
	{
		this->set_file(std::filesystem::path(filepath));
	}

	const std::shared_ptr<CGraphic> &get_graphics() const
	{
		return this->graphics;
	}

	const QPoint &get_hot_pos() const
	{
		return this->hot_pos;
	}

	const QSize &get_frame_size() const
	{
		return this->frame_size;
	}

	int get_current_frame() const
	{
		return this->current_frame;
	}

	void increment_current_frame()
	{
		this->current_frame++;
	}

	void reset_current_frame()
	{
		this->current_frame = 0;
	}

	int get_frame_rate() const
	{
		return this->frame_rate;
	}

private:
	cursor_type type;
	wyrmgus::civilization *civilization = nullptr;
	std::filesystem::path file;
	std::shared_ptr<CGraphic> graphics; /// Cursor sprite image
	QPoint hot_pos = QPoint(0, 0);     /// Hot point
	QSize frame_size = QSize(0, 0);
	int current_frame = 0;  /// Current displayed cursor frame
	int frame_rate = 0;    /// Rate of changing the frames
};

}

/// Cursor state
enum class CursorState {
	Point,      /// Normal cursor
	Select,     /// Select position
	Rectangle,  /// Rectangle selecting
	PieMenu     /// Displaying Pie Menu
};

extern CursorState CurrentCursorState;  /// current cursor state (point,...)
extern ButtonCmd CursorAction;          /// action for selection
extern int CursorValue;           /// value for action (spell type f.e.)
extern const wyrmgus::unit_type *CursorBuilding; /// building cursor
extern std::string CustomCursor;  /// custom cursor for button

extern PixelPos CursorScreenPos; /// cursor position on screen
extern PixelPos CursorStartScreenPos; /// rectangle started on screen
extern PixelPos CursorStartMapPos; /// the same in screen map coordinate system

/// Get amount of cursors to load
extern int GetCursorsCount();

/// Draw any cursor
extern void DrawCursor(std::vector<std::function<void(renderer *)>> &render_commands);
//Wyrmgus start
/// Draw building cursor
extern void DrawBuildingCursor(std::vector<std::function<void(renderer *)>> &render_commands);
//Wyrmgus end
/// Animate the cursor
extern void CursorAnimate(unsigned ticks);
