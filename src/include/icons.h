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

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CIcon icons.h
**
**  \#include "icons.h"
**
**  This structure contains all information about an icon.
**
**  The icon structure members:
**
**  CIcon::Ident
**
**    Unique identifier of the icon, used to reference it in config
**    files and during startup.  Don't use this in game, use instead
**    the pointer to this structure.
**
**  CIcon::G
**
**    Graphic image containing the loaded graphics. Loaded by
**    LoadIcons(). All icons belonging to the same icon file shares
**    this structure.
**
**  CIcon::Frame
**
**    Frame number in the graphic to display.
*/

/**
**  @class IconConfig icons.h
**
**  \#include "icons.h"
**
**  This structure contains all configuration information about an icon.
**
**  IconConfig::Name
**
**    Unique identifier of the icon, used to reference icons in config
**    files and during startup.  The name is resolved during game
**    start and the pointer placed in the next field.
**    @see CIcon::Ident
**
**  IconConfig::Icon
**
**    Pointer to an icon. This pointer is resolved during game start.
*/

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

static constexpr int IconActive = 1; //cursor on icon
static constexpr int IconClicked = 2; //mouse button down on icon
static constexpr int IconSelected = 4; //this the selected icon
static constexpr int IconDisabled = 8; //icon disabled
static constexpr int IconAutoCast = 16; //auto cast icon
//Wyrmgus start
static constexpr int IconCommandButton = 32; //if the icon is a command button
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CGraphic;
class CPlayerColorGraphic;
class CPlayer;
class ButtonStyle;
struct lua_State;

/// Icon: rectangle image used in menus
class CIcon final : public stratagus::data_entry, public stratagus::data_type<CIcon>
{
	Q_OBJECT

	Q_PROPERTY(int frame MEMBER frame READ get_frame)
	Q_PROPERTY(QString file READ get_file_qstring WRITE set_file_qstring)

public:
	static constexpr const char *class_identifier = "icon";
	static constexpr const char *database_folder = "icons";

	//needed by scripts via tolua++ for now
	static CIcon *Get(const std::string &ident)
	{
		return CIcon::get(ident);
	}

	CIcon(const std::string &identifier);
	~CIcon();

	virtual void initialize() override;

	virtual void check() const override
	{
		if (this->get_file().empty()) {
			throw std::runtime_error("Icon \"" + this->get_identifier() + "\" has no image file associated with it.");
		}
	}

	void ProcessConfigData(const CConfigData *config_data);
	
	const std::filesystem::path &get_file() const
	{
		return this->file;
	}

	void set_file(const std::filesystem::path &filepath);

	QString get_file_qstring() const
	{
		return QString::fromStdString(this->get_file().string());
	}

	void set_file_qstring(const QString &file)
	{
		this->set_file(file.toStdString());
	}

	int get_frame() const
	{
		return this->frame;
	}

	void Load();

	/// Draw icon
	void DrawIcon(const PixelPos &pos, const int player = -1) const;
	/// Draw grayscale icon
	void DrawGrayscaleIcon(const PixelPos &pos, const int player = -1) const;
	/// Draw cooldown spell
	void DrawCooldownSpellIcon(const PixelPos &pos, const int percent) const;
	/// Draw icon of a unit
	void DrawUnitIcon(const ButtonStyle &style,
					  //Wyrmgus start
//					  unsigned flags, const PixelPos &pos, const std::string &text, const int player = -1) const;
					  unsigned flags, const PixelPos &pos, const std::string &text, const int player = -1, bool transparent = false, bool grayscale = false, int show_percent = 100) const;
					  //Wyrmgus end

public:
	CPlayerColorGraphic *G = nullptr; //graphic data
	CPlayerColorGraphic *GScale = nullptr; //icon when drawn grayscaled
private:
	std::filesystem::path file;
	int frame = 0; //frame number in the icon's image
public:
	//Wyrmgus start
	bool Loaded = false;
	//Wyrmgus end

	friend int CclDefineIcon(lua_State *l);
};

/// Icon reference (used in config tables)
class IconConfig
{
public:
	IconConfig() : Icon(nullptr) {}

	bool LoadNoLog();
	bool Load();
public:
	std::string Name;    /// config icon name
	CIcon *Icon;         /// icon pointer to use to run time
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void LoadIcons();   /// Load icons
extern int  GetIconsCount();
