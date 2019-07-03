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
/**@name contenttype.h - content type header file. */
//
//      (c) Copyright 1999-2012 by Lutz Sammer and Jimmy Salmon
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

#ifndef __CONTENT_TYPE_H__
#define __CONTENT_TYPE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "script.h"
#include "vec2i.h"

#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnit;
class CFont;
class ConditionPanel;

/**
**  Infos to display the contents of panel.
*/
class CContentType
{
public:
	virtual ~CContentType();

	/// Tell how show the variable Index.
	virtual void Draw(const CUnit &unit, CFont *defaultfont) const = 0;

	virtual void Parse(lua_State *l) = 0;

public:
	PixelPos Pos = PixelPos(0, 0);	/// Coordinate where to display.
	ConditionPanel *Condition = nullptr;	/// Condition to show the content; if null, no condition.
//Wyrmgus start
	std::string TextColor;      /// Color used for plain text in content.
	std::string HighlightColor; /// Color used for highlighted letters.
//Wyrmgus end
};

/**
**  Show simple text followed by variable value.
*/
class CContentTypeText : public CContentType
{
public:
	virtual ~CContentTypeText()
	{
		FreeStringDesc(Text);
		delete Text;
	}

	virtual void Draw(const CUnit &unit, CFont *defaultfont) const;
	virtual void Parse(lua_State *l);

private:
	StringDesc *Text = nullptr;		/// Text to display.
	CFont *Font = nullptr;			/// Font to use.
	char Centered = 0;				/// if true, center the display.
	int Index = -1;					/// Index of the variable to show, -1 if not.
	EnumVariable Component = VariableValue;	/// Component of the variable.
	char ShowName = 0;				/// If true, Show name's unit.
	char Stat = 0;					/// true to special display.(value or value + diff)
};

/**
**  Show formatted text with variable value.
*/
class CContentTypeFormattedText : public CContentType
{
public:
	virtual ~CContentTypeFormattedText() {}

	virtual void Draw(const CUnit &unit, CFont *defaultfont) const;
	virtual void Parse(lua_State *l);

private:
	std::string Format;			/// Text to display
	CFont *Font = nullptr;		/// Font to use.
	bool Centered = false;		/// if true, center the display.
	int Index = -1;				/// Index of the variable to show.
	EnumVariable Component = VariableValue;	/// Component of the variable.
};

/**
**  Show formatted text with variable value.
*/
class CContentTypeFormattedText2 : public CContentType
{
public:
	virtual ~CContentTypeFormattedText2() {}

	virtual void Draw(const CUnit &unit, CFont *defaultfont) const;
	virtual void Parse(lua_State *l);

private:
	std::string Format;			/// Text to display
	CFont *Font = nullptr;		/// Font to use.
	bool Centered = false;		/// if true, center the display.
	int Index1 = -1;			/// Index of the variable1 to show.
	EnumVariable Component1 = VariableValue;	/// Component of the variable1.
	int Index2 = -1;			/// Index of the variable to show.
	EnumVariable Component2 = VariableValue;	/// Component of the variable.
};

/**
**  Show icon of the unit
*/
class CContentTypeIcon : public CContentType
{
public:
	virtual void Draw(const CUnit &unit, CFont *defaultfont) const;
	virtual void Parse(lua_State *l);

private:
	EnumUnit UnitRef;           /// Which unit icon to display.(itself, container, ...)
};

/**
**  Show bar which change color depend of value.
*/
class CContentTypeLifeBar : public CContentType
{
public:
	virtual void Draw(const CUnit &unit, CFont *defaultfont) const;
	virtual void Parse(lua_State *l);

private:
	int Index = -1;		/// Index of the variable to show, -1 if not.
	int Width = 0;		/// Width of the bar.
	int Height = 0;		/// Height of the bar.
#if 0 // FIXME : something for color and value parametrisation (not implemented)
	Color *colors;		/// array of color to show (depend of value)
	int *values;		/// list of percentage to change color.
#endif
};

/**
**  Show bar.
*/
class CContentTypeCompleteBar : public CContentType
{
public:
	virtual void Draw(const CUnit &unit, CFont *defaultfont) const;
	virtual void Parse(lua_State *l);

private:
	int varIndex = -1;		/// Index of the variable to show, -1 if not.
	int width = 0;			/// Width of the bar.
	int height = 0;			/// Height of the bar.
	bool hasBorder = false;	/// True for additional border.
	int colorIndex = -1;	/// Index of Color to show.
};

#endif
