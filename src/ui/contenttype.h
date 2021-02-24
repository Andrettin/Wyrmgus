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

#pragma once

#include "vec2i.h"

class CUnit;
class ConditionPanel;
enum EnumUnit : int;
enum class VariableAttribute;
struct lua_State;
struct StringDesc;

namespace wyrmgus {
	class font;
	class font_color;
}

/**
**  Infos to display the contents of panel.
*/
class CContentType
{
public:
	virtual ~CContentType();

	/// Tell how show the variable Index.
	virtual void Draw(const CUnit &unit, wyrmgus::font *defaultfont) const = 0;

	virtual void Parse(lua_State *l) = 0;

public:
	PixelPos Pos = PixelPos(0, 0); /// Coordinate where to display.
	std::unique_ptr<ConditionPanel> Condition; /// Condition to show the content; if null, no condition.
	const wyrmgus::font_color *TextColor = nullptr;      /// Color used for plain text in content.
	const wyrmgus::font_color *HighlightColor = nullptr; /// Color used for highlighted letters.
};

/**
**  Show simple text followed by variable value.
*/
class CContentTypeText : public CContentType
{
public:
	CContentTypeText();
	virtual ~CContentTypeText();

	virtual void Draw(const CUnit &unit, wyrmgus::font *defaultfont) const override;
	virtual void Parse(lua_State *l) override;

private:
	std::unique_ptr<StringDesc> Text;  /// Text to display.
	wyrmgus::font *Font = nullptr; /// Font to use.
	char Centered = 0;           /// if true, center the display.
	int Index = -1;              /// Index of the variable to show, -1 if not.
	VariableAttribute Component; /// Component of the variable.
	char ShowName = 0;           /// If true, Show name's unit.
	char Stat = 0;               /// true to special display.(value or value + diff)
};

/**
**  Show formatted text with variable value.
*/
class CContentTypeFormattedText : public CContentType
{
public:
	CContentTypeFormattedText();
	virtual ~CContentTypeFormattedText() {}

	virtual void Draw(const CUnit &unit, wyrmgus::font *defaultfont) const override;
	virtual void Parse(lua_State *l) override;

private:
	std::string Format;          /// Text to display
	wyrmgus::font *Font = nullptr; /// Font to use.
	bool Centered = false;       /// if true, center the display.
	int Index = -1;              /// Index of the variable to show.
	VariableAttribute Component; /// Component of the variable.
};

/**
**  Show formatted text with variable value.
*/
class CContentTypeFormattedText2 : public CContentType
{
public:
	CContentTypeFormattedText2();
	virtual ~CContentTypeFormattedText2() {}

	virtual void Draw(const CUnit &unit, wyrmgus::font *defaultfont) const override;
	virtual void Parse(lua_State *l) override;

private:
	std::string Format;          /// Text to display
	wyrmgus::font *Font = nullptr; /// Font to use.
	bool Centered = false;       /// if true, center the display.
	int Index1 = -1;             /// Index of the variable1 to show.
	VariableAttribute Component1; /// Component of the variable1.
	int Index2 = -1;             /// Index of the variable to show.
	VariableAttribute Component2; /// Component of the variable.
};

/**
**  Show icon of the unit
*/
class CContentTypeIcon : public CContentType
{
public:
	virtual void Draw(const CUnit &unit, wyrmgus::font *defaultfont) const override;
	virtual void Parse(lua_State *l) override;

private:
	EnumUnit UnitRef;           /// Which unit icon to display.(itself, container, ...)
};

/**
**  Show bar which change color depend of value.
*/
class CContentTypeLifeBar : public CContentType
{
public:
	CContentTypeLifeBar() : Index(-1), Width(0), Height(0) {}

	virtual void Draw(const CUnit &unit, wyrmgus::font *defaultfont) const override;
	virtual void Parse(lua_State *l) override;

private:
	int Index;           /// Index of the variable to show, -1 if not.
	int Width;           /// Width of the bar.
	int Height;          /// Height of the bar.
#if 0 // FIXME : something for color and value parametrisation (not implemented)
	Color *colors;       /// array of color to show (depend of value)
	int *values;         /// list of percentage to change color.
#endif
};

/**
**  Show bar.
*/
class CContentTypeCompleteBar : public CContentType
{
public:
	CContentTypeCompleteBar() : varIndex(-1), width(0), height(0), hasBorder(false), colorIndex(-1) {}

	virtual void Draw(const CUnit &unit, wyrmgus::font *defaultfont) const override;
	virtual void Parse(lua_State *l) override;

private:
	int varIndex;    /// Index of the variable to show, -1 if not.
	int width;       /// Width of the bar.
	int height;      /// Height of the bar.
	bool hasBorder;  /// True for additional border.
	int colorIndex;  /// Index of Color to show.
};
