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
/**@name icon.h - The icon header file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer and Andrettin
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

#ifndef __ICON_H__
#define __ICON_H__

/*----------------------------------------------------------------------------
--  Documentation
----------------------------------------------------------------------------*/

/**
**  @class CIcon icon.h
**
**  \#include "icon.h"
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
**    @see CIcon::Ident
**
**  IconConfig::Icon
**
**    Pointer to an icon. This pointer is resolved during game start.
*/

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

#define IconActive   1  /// cursor on icon
#define IconClicked  2  /// mouse button down on icon
#define IconSelected 4  /// this the selected icon
#define IconDisabled 8  /// icon disabled
#define IconAutoCast 16 /// auto cast icon
//Wyrmgus start
#define IconCommandButton 32 /// is the icon a command button
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "vec2i.h"

#include <core/object.h>

#include <map>
#include <string>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CGraphic;
class CPlayerColorGraphic;
class CPlayer;
class ButtonStyle;

/// Icon: rectangle image used in menus
class CIcon : public Object
{
	GDCLASS(CIcon, Object)
	
public:
	CIcon()
	{
	}
	
	/**
	**  CIcon constructor
	*/
	CIcon(const std::string &ident) : Ident(ident)
	{
	}
	
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

	const std::string &GetIdent() const { return this->Ident; }

	String GetFile() const
	{
		return this->File.c_str();
	}
	
private:
	std::string Ident;						/// Icon identifier
public:
	CPlayerColorGraphic *G = nullptr;		/// Graphic data
	CPlayerColorGraphic *GScale = nullptr;	/// Icon when drawn grayscaled
	std::string File;						/// The file containing the icon graphics
	Vec2i Size = Vec2i(0, 0);				/// The size of the icon, in pixels
	int Frame = 0;							/// Frame number in graphic
	//Wyrmgus start
	bool Loaded = false;
	//Wyrmgus end

protected:
	static void _bind_methods();
};

/// Icon reference (used in config tables)
class IconConfig
{
public:
	bool LoadNoLog();
	bool Load();

public:
	std::string Name;		/// config icon name
	CIcon *Icon = nullptr;	/// icon pointer to use to run time
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void LoadIcons();   /// Load icons
extern int  GetIconsCount();
extern void CleanIcons();  /// Cleanup icons

typedef std::map<std::string, CIcon *> IconMap;
extern IconMap Icons;

#endif
