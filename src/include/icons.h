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
class CIcon
{
public:
	CIcon(const std::string &ident);
	~CIcon();

	static CIcon *New(const std::string &ident);
	static CIcon *Get(const std::string &ident);

	void ProcessConfigData(const CConfigData *config_data);
	
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

	const std::string &GetIdent() const
	{
		return this->Ident;
	}

	const std::string &get_file() const
	{
		return this->file;
	}

private:
	std::string Ident;        /// Icon identifier
public:
	CPlayerColorGraphic *G = nullptr; //graphic data
	CPlayerColorGraphic *GScale = nullptr; //icon when drawn grayscaled
private:
	std::string file;
	Vec2i size = Vec2i(0, 0);
public:
	int Frame = 0; //frame number in graphic
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
extern void CleanIcons();  /// Cleanup icons

using IconMap = std::map<std::string, CIcon *>;
extern IconMap Icons;
