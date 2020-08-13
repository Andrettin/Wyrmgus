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
//      (c) Copyright 1998-2020 by Lutz Sammer and Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"
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

static constexpr int IconActive = 1; //cursor on icon
static constexpr int IconClicked = 2; //mouse button down on icon
static constexpr int IconSelected = 4; //this the selected icon
static constexpr int IconDisabled = 8; //icon disabled
static constexpr int IconAutoCast = 16; //auto cast icon
//Wyrmgus start
static constexpr int IconCommandButton = 32; //if the icon is a command button
//Wyrmgus end

class CConfigData;
class CGraphic;
class CPlayerColorGraphic;
class CPlayer;
class ButtonStyle;
struct lua_State;

int CclDefineIcon(lua_State *l);;

namespace stratagus {

class player_color;

/// Icon: rectangle image used in menus
class icon final : public data_entry, public data_type<icon>
{
	Q_OBJECT

	Q_PROPERTY(QString file READ get_file_qstring)
	Q_PROPERTY(int frame MEMBER frame READ get_frame)
	Q_PROPERTY(stratagus::player_color* conversible_player_color MEMBER conversible_player_color READ get_conversible_player_color)

public:
	static constexpr const char *class_identifier = "icon";
	static constexpr const char *database_folder = "icons";

	//needed by scripts via tolua++ for now
	static icon *Get(const std::string &ident)
	{
		return icon::get(ident);
	}

	explicit icon(const std::string &identifier);
	~icon();

	virtual void initialize() override;

	virtual void check() const override
	{
		if (this->get_file().empty()) {
			throw std::runtime_error("Icon \"" + this->get_identifier() + "\" has no image file associated with it.");
		}
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

	int get_frame() const
	{
		return this->frame;
	}

	player_color *get_conversible_player_color() const
	{
		return this->conversible_player_color;
	}

	/// Draw icon
	void DrawIcon(const PixelPos &pos, const player_color *player_color = nullptr) const;
	/// Draw grayscale icon
	void DrawGrayscaleIcon(const PixelPos &pos) const;
	/// Draw cooldown spell
	void DrawCooldownSpellIcon(const PixelPos &pos, const int percent) const;
	/// Draw icon of a unit
	void DrawUnitIcon(const ButtonStyle &style,
					  unsigned flags, const PixelPos &pos, const std::string &text, const player_color *player = nullptr, bool transparent = false, bool grayscale = false, int show_percent = 100) const;

private:
	void load();

public:
	CPlayerColorGraphic *G = nullptr; //graphic data
private:
	std::filesystem::path file;
	int frame = 0; //frame number in the icon's image
	player_color *conversible_player_color = nullptr;

	friend int ::CclDefineIcon(lua_State *l);
};

}

/// Icon reference (used in config tables)
class IconConfig
{
public:
	IconConfig() : Icon(nullptr) {}

	bool LoadNoLog();
	bool Load();
public:
	std::string Name; //config icon name
	stratagus::icon *Icon = nullptr; //icon pointer to use to run time
};
