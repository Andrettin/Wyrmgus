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

#include "database/data_type.h"
#include "ui/icon_base.h"
#include "util/color_container.h"
#include "vec2i.h"

/**
**  @class IconConfig icon.h
**
**  \#include "icon.h"
**
**  This structure contains all configuration information about an icon.
**
**  IconConfig::Name
**
**    Unique identifier of the icon, used to reference icons in config
**    files and during startup.  The name is resolved during game
**    start and the pointer placed in the next field.
**
**  IconConfig::Icon
**
**    Pointer to an icon. This pointer is resolved during game start.
*/

constexpr int IconActive = 1; //cursor on icon
constexpr int IconClicked = 2; //mouse button down on icon
constexpr int IconSelected = 4; //this the selected icon
constexpr int IconDisabled = 8; //icon disabled
constexpr int IconAutoCast = 16; //auto cast icon
//Wyrmgus start
constexpr int IconCommandButton = 32; //if the icon is a command button
//Wyrmgus end

class CPlayerColorGraphic;
class ButtonStyle;
struct lua_State;

static int CclDefineIcon(lua_State *l);

namespace wyrmgus {

class player_color;
class renderer;

/// Icon: rectangle image used in menus
class icon final : public icon_base, public data_type<icon>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)
	Q_PROPERTY(double hue_rotation MEMBER hue_rotation READ get_hue_rotation)

public:
	static constexpr const char *class_identifier = "icon";
	static constexpr const char *database_folder = "icons";

	static int get_to_load_count();
	static void load_all();

	explicit icon(const std::string &identifier);

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	virtual const QSize &get_size() const override;

	player_color *get_conversible_player_color() const
	{
		return this->conversible_player_color;
	}

	double get_hue_rotation() const
	{
		return this->hue_rotation;
	}

	const color_set &get_hue_ignored_colors() const
	{
		return this->hue_ignored_colors;
	}

	std::shared_ptr<CPlayerColorGraphic> get_graphics() const;

	/// Draw icon
	void DrawIcon(const PixelPos &pos, const player_color *player_color, std::vector<std::function<void(renderer *)>> &render_commands) const;
	/// Draw grayscale icon
	void DrawGrayscaleIcon(const PixelPos &pos, std::vector<std::function<void(renderer *)>> &render_commands) const;
	/// Draw cooldown spell
	void DrawCooldownSpellIcon(const PixelPos &pos, const int percent, std::vector<std::function<void(renderer *)>> &render_commands) const;

	/// Draw icon of a unit
	void DrawUnitIcon(const ButtonStyle &style, const unsigned flags, const PixelPos &pos, const std::string &text, const player_color *player, QColor border_color, const bool transparent, const bool grayscale, const int show_percent, std::vector<std::function<void(renderer *)>> &render_commands) const;

	void DrawUnitIcon(const ButtonStyle &style, const unsigned flags, const PixelPos &pos, const std::string &text, const player_color *player, std::vector<std::function<void(renderer *)>> &render_commands) const
	{
		this->DrawUnitIcon(style, flags, pos, text, player, QColor(), false, false, 100, render_commands);
	}

private:
	player_color *conversible_player_color = nullptr;
	double hue_rotation = 0;
	color_set hue_ignored_colors;

	friend int ::CclDefineIcon(lua_State *l);
};

}

/// Icon reference (used in config tables)
class IconConfig final
{
public:
	bool LoadNoLog();
	bool Load();
public:
	std::string Name; //config icon name
	wyrmgus::icon *Icon = nullptr; //icon pointer to use to run time
};
