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
/**@name popup.h - The Popup header file. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#ifndef __POPUP_H__
#define __POPUP_H__

//@{

#include "script.h"
#include "color.h"
#include "vec2i.h"
#include <vector>
#include <string>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class ButtonAction;
class CFont;
class CPopup;

#define MARGIN_X 4
#define MARGIN_Y 2

class PopupConditionPanel
{
public:
	PopupConditionPanel() :  HasHint(false), HasDescription(false), HasDependencies(false),
		//Wyrmgus start
		Description(false), Quote(false),
		AutoCast(0), Equipped(0), Equippable(0), Consumable(0), Affixed(0), Spell(0), CanUse(0), Work(0), ReadWork(0), Unique(0), Bound(0), Weapon(0), Shield(0), Boots(0), Arrows(0), Regeneration(0),
//		ButtonAction(-1), BoolFlags(NULL), Variables(NULL) {}
		ButtonAction(-1), ItemClass(-1), BoolFlags(NULL), Variables(NULL) {}
		//Wyrmgus end
	~PopupConditionPanel()
	{
		delete[] BoolFlags;
		delete[] Variables;
	}

	bool HasHint;               /// check if button has hint.
	bool HasDescription;        /// check if button has description.
	bool HasDependencies;       /// check if button has dependencies or restrictions.
	//Wyrmgus start
	bool Description;			/// check if the button's unit type has a description.
	bool Quote;					/// check if the button's unit type has a quote.
	//Wyrmgus end
	int ButtonAction;           /// action type of button
	//Wyrmgus start
	int ItemClass;				/// item class of the button's item
	//Wyrmgus end
	std::string ButtonValue;    /// value used in ValueStr field of button

	//Wyrmgus start
	char AutoCast;				/// check if button's spell can be autocasted
	char Equipped;				/// check if button's item is equipped.
	char Equippable;			/// check if button's item is equippable by its owner.
	char Consumable;			/// check if button's item is consumable.
	char Affixed;				/// check if button's item has an affix
	char Spell;					/// check if button's item has a spell
	char CanUse;				/// check if button's item's can be used
	char Work;					/// check if button's item is a work.
	char ReadWork;				/// check if button's item is a work that has been read.
	char Unique;				/// check if button's item is unique
	char Bound;					/// check if button's item is bound to its owner
	char Weapon;				/// check if button's item is a weapon
	char Shield;				/// check if button's item is a shield
	char Boots;					/// check if button's item are boots
	char Arrows;				/// check if button's item are arrows
	char Regeneration;			/// check if button's item has regeneration
	//Wyrmgus end
	char *BoolFlags;            /// array of condition about user flags.
	char *Variables;            /// array of variable to verify (enable and max > 0)
};

class CPopupContentType
{
public:
	CPopupContentType() : pos(0, 0),
		MarginX(MARGIN_X), MarginY(MARGIN_Y), minSize(0, 0),
		Wrap(true), Condition(NULL) {}
	virtual ~CPopupContentType() { delete Condition; }

	/// Tell how show the variable Index.
	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const ButtonAction &button, int *Costs) const = 0;
	/// Get the content's width
	virtual int GetWidth(const ButtonAction &button, int *Costs) const = 0;
	/// Get the content's height
	virtual int GetHeight(const ButtonAction &button, int *Costs) const = 0;

	virtual void Parse(lua_State *l) = 0;

	static CPopupContentType *ParsePopupContent(lua_State *l);

public:
	PixelPos pos;               /// position to draw.

	int MarginX;                /// Left and right margin width.
	int MarginY;                /// Upper and lower margin height.
	PixelSize minSize;          /// Minimal size covered by content type.
	bool Wrap;                  /// If true, the next content will be placed on the next "line".
protected:
	std::string TextColor;      /// Color used for plain text in content.
	std::string HighlightColor; /// Color used for highlighted letters.
public:
	PopupConditionPanel *Condition; /// Condition to show the content; if NULL, no condition.
};

enum PopupButtonInfo_Types {
	PopupButtonInfo_Hint,
	PopupButtonInfo_Description,
	PopupButtonInfo_Dependencies
};

class CPopupContentTypeButtonInfo : public CPopupContentType
{
public:
	CPopupContentTypeButtonInfo() : InfoType(0), MaxWidth(0), Font(NULL) {}
	virtual ~CPopupContentTypeButtonInfo() {}

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const ButtonAction &button, int *Costs) const;

	virtual int GetWidth(const ButtonAction &button, int *Costs) const;
	virtual int GetHeight(const ButtonAction &button, int *Costs) const;

	virtual void Parse(lua_State *l);

private:
	int InfoType;                /// Type of information to show.
	unsigned int MaxWidth;       /// Maximum width of multilined information.
	CFont *Font;                 /// Font to use.
};

class CPopupContentTypeText : public CPopupContentType
{
public:
	//Wyrmgus start
//	CPopupContentTypeText() : MaxWidth(0), Font(NULL) {}
	CPopupContentTypeText() : Text(NULL), MaxWidth(0), Font(NULL) {}
	//Wyrmgus end
	//Wyrmgus start
//	virtual ~CPopupContentTypeText() {}
	virtual ~CPopupContentTypeText() {
		FreeStringDesc(Text);
		delete Text;
	}
	//Wyrmgus end

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const ButtonAction &button, int *Costs) const;

	virtual int GetWidth(const ButtonAction &button, int *Costs) const;
	virtual int GetHeight(const ButtonAction &button, int *Costs) const;

	virtual void Parse(lua_State *l);

private:
	//Wyrmgus start
//	std::string Text;            /// Text to display
	StringDesc *Text;            /// Text to display.
	//Wyrmgus end
	unsigned int MaxWidth;       /// Maximum width of multilined text.
	CFont *Font;                 /// Font to use.
};

class CPopupContentTypeCosts : public CPopupContentType
{
public:
	CPopupContentTypeCosts() : Font(NULL), Centered(0) {}
	virtual ~CPopupContentTypeCosts() {}

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const ButtonAction &button, int *Costs) const;

	virtual int GetWidth(const ButtonAction &button, int *Costs) const;
	virtual int GetHeight(const ButtonAction &button, int *Costs) const;

	virtual void Parse(lua_State *l);

private:
	CFont *Font;                 /// Font to use.
	char Centered;               /// if true, center the display.
};

class CPopupContentTypeLine : public CPopupContentType
{
public:
	CPopupContentTypeLine();
	virtual ~CPopupContentTypeLine() {}

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const ButtonAction &button, int *Costs) const;

	virtual int GetWidth(const ButtonAction &button, int *Costs) const;
	virtual int GetHeight(const ButtonAction &button, int *Costs) const;

	virtual void Parse(lua_State *l);

private:
	IntColor Color;  /// Color used for line.
	unsigned int Width;     /// line height
	unsigned int Height;    /// line height
};

class CPopupContentTypeVariable : public CPopupContentType
{
public:
	CPopupContentTypeVariable() : Text(NULL), Font(NULL), Centered(0), Index(-1) {}
	virtual ~CPopupContentTypeVariable()
	{
		FreeStringDesc(Text);
		delete Text;
	}

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const ButtonAction &button, int *Costs) const;

	virtual int GetWidth(const ButtonAction &button, int *Costs) const;
	virtual int GetHeight(const ButtonAction &button, int *Costs) const;

	virtual void Parse(lua_State *l);

private:
	StringDesc *Text;            /// Text to display.
	CFont *Font;                 /// Font to use.
	char Centered;               /// if true, center the display.
	int Index;                   /// Index of the variable to show, -1 if not.
};

class CPopup
{
public:
	CPopup();
	~CPopup();

	std::vector<CPopupContentType *> Contents; /// Array of contents to display.
	std::string Ident;                         /// Ident of the popup.
	int MarginX;                               /// Left and right margin width.
	int MarginY;                               /// Upper and lower margin height.
	int MinWidth;                              /// Minimal width covered by popup.
	int MinHeight;                             /// Minimal height covered by popup.
	CFont *DefaultFont;                        /// Default font for content.
	IntColor BackgroundColor;                  /// Color used for popup's background.
	IntColor BorderColor;                      /// Color used for popup's borders.
};


//@}

#endif // !__UI_H__
