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
**  \#include "ui/icon.h"
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

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

constexpr int IconActive = 1;	/// cursor on icon
constexpr int IconClicked = 2;	/// mouse button down on icon
constexpr int IconSelected = 4;	/// this the selected icon
constexpr int IconDisabled = 8;	/// icon disabled
constexpr int IconAutoCast = 16;	/// auto cast icon
//Wyrmgus start
constexpr int IconCommandButton = 32;	/// is the icon a command button
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"
#include "property.h"
#include "vec2i.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class ButtonStyle;
class CPlayerColorGraphic;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

/// Icon: rectangle image used in menus
class CIcon : public DataElement, public DataType<CIcon>
{
	DATA_TYPE(CIcon, DataElement)
	
public:
	/**
	**  CIcon constructor
	*/
	CIcon(const std::string &ident = "") : DataElement(ident)
	{
	}
	
	~CIcon();

public:
	static constexpr const char *ClassIdentifier = "icon";
	
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	
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

	int GetFrame() const
	{
		return this->Frame;
	}
	
	const String &GetFile() const
	{
		return this->File;
	}
	
public:
	CPlayerColorGraphic *G = nullptr;		/// graphic data
	CPlayerColorGraphic *GScale = nullptr;	/// icon when drawn grayscaled
private:
	String File;							/// the file containing the icon graphics
public:
	Vec2i Size = Vec2i(0, 0);				/// the size of the icon, in pixels
private:
	int Frame = 0;							/// frame number in graphic
public:
	//Wyrmgus start
	bool Loaded = false;
	//Wyrmgus end
	
	friend int CclDefineIcon(lua_State *l);

protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern void LoadIcons();   /// Load icons
extern int GetIconsCount();

#endif
