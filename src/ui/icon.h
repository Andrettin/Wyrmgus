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
//      (c) Copyright 1998-2025 by Lutz Sammer and Andrettin
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

extern int CclDefineIcon(lua_State *l);

namespace archimedes {
	enum class colorization_type;
}

namespace wyrmgus {

class player_color;
class renderer;

/// Icon: rectangle image used in menus
class icon final : public icon_base, public data_type<icon>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)
	Q_PROPERTY(int hue_rotation MEMBER hue_rotation READ get_hue_rotation)
	Q_PROPERTY(wyrmgus::colorization_type colorization MEMBER colorization READ get_colorization)

public:
	static constexpr const char *class_identifier = "icon";
	static constexpr const char property_class_identifier[] = "wyrmgus::icon*";
	static constexpr const char *database_folder = "icons";

	static int get_to_load_count();
	static void load_all();

	explicit icon(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	virtual const QSize &get_size() const override;

	player_color *get_conversible_player_color() const
	{
		return this->conversible_player_color;
	}

	int get_hue_rotation() const
	{
		return this->hue_rotation;
	}

	colorization_type get_colorization() const
	{
		return this->colorization;
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
	int hue_rotation = 0;
	colorization_type colorization;
	color_set hue_ignored_colors;

	friend int ::CclDefineIcon(lua_State *l);
};

}
