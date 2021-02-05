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
//      (c) Copyright 2012-2021 by Joris Dauphin and Andrettin
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

#include "color.h"
#include "unit/unit_type_type.h"
#include "vec2i.h"

class CPopup;
class CUpgrade;
enum class ButtonCmd;
struct StringDesc;

namespace wyrmgus {
	class button;
	class font;
	class font_color;
	class unit_class;
	class upgrade_class;
	enum class item_class;
}

constexpr int MARGIN_X = 4;
constexpr int MARGIN_Y = 2;

class PopupConditionPanel final
{
public:
	PopupConditionPanel();

	bool HasHint = false; //check if button has hint.
	bool HasDescription = false; //check if button has description.
	bool HasConditions = false; //check if button has conditions or restrictions.
	//Wyrmgus start
	bool Class = false;			/// check if the button's unit type has a class.
	bool Description = false;	/// check if the button's unit type has a description.
	bool Quote = false;			/// check if the button's unit type has a quote.
	bool Encyclopedia = false;	/// check if the button's unit type has an encyclopedia entry.
	bool settlement_name = false;	/// check if the button's unit has a settlement name.
	bool CanActiveHarvest = false;	/// check if the active unit can harvest the button's unit.
	//Wyrmgus end
	ButtonCmd ButtonAction;	/// action type of button
	//Wyrmgus start
	::UnitTypeType UnitTypeType = UnitTypeType::None;	/// unit type type (i.e. land, fly, etc.) of the button's unit type
	const wyrmgus::unit_class *unit_class = nullptr; /// unit type class of the button's unit type
	wyrmgus::item_class item_class;				/// item class of the button's item
	int CanStore = -1;			/// whether the button's unit type can store a particular resource
	int ImproveIncome = -1;		/// whether the button's unit type improves the processing of a particular resource
	CUpgrade *ResearchedUpgrade = nullptr;	/// whether the button's player has researched a particular upgrade
	const wyrmgus::upgrade_class *researched_upgrade_class = nullptr; //whether the button's player has researched a particular upgrade class
	//Wyrmgus end
	std::string ButtonValue;    /// value used in ValueStr field of button

	//Wyrmgus start
	char Opponent = 0;			/// check if button's item is an opponent
	char Neutral = 0;			/// check if button's item is neutral
	char AutoCast = 0;			/// check if button's spell can be autocasted
	char Equipped = 0;			/// check if button's item is equipped
	char Equippable = 0;		/// check if button's item is equippable by its owner
	char Consumable = 0;		/// check if button's item is consumable
	char Affixed = 0;			/// check if button's item has an affix
	char Spell = 0;				/// check if button's item has a spell
	char CanUse = 0;			/// check if button's item's can be used
	char Work = 0;				/// check if button's item is a work
	char ReadWork = 0;			/// check if button's item is a work that has been read
	char Elixir = 0;			/// check if button's item is an elixir
	char ConsumedElixir = 0;	/// check if button's item is an elixir that has been consumed
	char Unique = 0;			/// check if button's item is unique
	char UniqueSet = 0;			/// check if button's item is part of a unique item set
	char Bound = 0;				/// check if button's item is bound to its owner
	char Identified = 0;		/// check if button's item has been identified
	char Weapon = 0;			/// check if button's item is a weapon
	char Shield = 0;			/// check if button's item is a shield
	char Boots = 0;				/// check if button's item are boots
	char Arrows = 0;			/// check if button's item are arrows
	char Regeneration = 0;		/// check if button's item has regeneration
	char FactionUpgrade = 0;	/// check if the button's upgrade is a faction upgrade
	char FactionCoreSettlements = 0; /// check if the button's faction has core settlements
	char DynastyUpgrade = 0;	/// check if the button's upgrade is a dynasty upgrade
	char UpgradeResearched = 0;	/// check if the button's upgrade has already been researched
	char Ability = 0;			/// check if the button's upgrade is an ability
	char ChildResources = 0;	/// check if the button's resource has child resources
	char ImproveIncomes = 0;	/// check if the button's unit type has processing bonuses for any resource
	char LuxuryResource = 0;	/// check if the button's resource is a luxury resource
	char RequirementsString = 0;	/// check if the button's unit type or upgrade has a requirements string
	char ExperienceRequirementsString = 0;	/// check if the button's unit type or upgrade has an experience requirements string
	char BuildingRulesString = 0;	/// check if the button's unit type has a building rules string
	//Wyrmgus end
	char Overlord = 0; //check whether the button's player has an overlord
	char TopOverlord = 0; //check whether the button's player has a top overlord, and whether it is different from its overlord
	char terrain_feature = 0; //check whether the popup's tile has a terrain feature
	std::unique_ptr<char[]> BoolFlags;            /// array of condition about user flags.
	std::unique_ptr<char[]> Variables;            /// array of variable to verify (enable and max > 0)
};

class CPopupContentType
{
public:
	virtual ~CPopupContentType()
	{
	}

	/// Tell how show the variable Index.
	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *Costs) const = 0;
	/// Get the content's width
	virtual int GetWidth(const wyrmgus::button &button, int *Costs) const = 0;
	/// Get the content's height
	virtual int GetHeight(const wyrmgus::button &button, int *Costs) const = 0;

	virtual void Parse(lua_State *l) = 0;

	static std::unique_ptr<CPopupContentType> ParsePopupContent(lua_State *l);

public:
	PixelPos pos = PixelPos(0, 0); /// position to draw.

	int MarginX = MARGIN_X; /// Left and right margin width.
	int MarginY = MARGIN_Y; /// Upper and lower margin height.
	PixelSize minSize = PixelSize(0, 0); /// Minimal size covered by content type.
	bool Wrap = true; /// If true, the next content will be placed on the next "line".
protected:
	const wyrmgus::font_color *TextColor = nullptr;      /// Color used for plain text in content.
	const wyrmgus::font_color *HighlightColor = nullptr; /// Color used for highlighted letters.
public:
	std::unique_ptr<PopupConditionPanel> Condition; /// Condition to show the content; if null, no condition.
};

enum PopupButtonInfo_Types {
	PopupButtonInfo_Hint,
	PopupButtonInfo_Description,
	PopupButtonInfo_Conditions
};

class CPopupContentTypeButtonInfo final : public CPopupContentType
{
public:
	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *Costs) const override;

	virtual int GetWidth(const wyrmgus::button &button, int *Costs) const override;
	virtual int GetHeight(const wyrmgus::button &button, int *Costs) const override;

	virtual void Parse(lua_State *l) override;

private:
	int InfoType = 0;                /// Type of information to show.
	unsigned int MaxWidth = 0;       /// Maximum width of multilined information.
	wyrmgus::font *Font = nullptr;                 /// Font to use.
};

class CPopupContentTypeText final : public CPopupContentType
{
public:
	//Wyrmgus start
//	virtual ~CPopupContentTypeText() {}
	virtual ~CPopupContentTypeText();
	//Wyrmgus end

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *Costs) const override;

	virtual int GetWidth(const wyrmgus::button &button, int *Costs) const override;
	virtual int GetHeight(const wyrmgus::button &button, int *Costs) const override;

	virtual void Parse(lua_State *l) override;

private:
	//Wyrmgus start
//	std::string Text;            /// Text to display
	std::unique_ptr<StringDesc> Text;  /// Text to display.
	//Wyrmgus end
	unsigned int MaxWidth = 0;   /// Maximum width of multilined text.
	wyrmgus::font *Font = nullptr; /// Font to use.
};

class CPopupContentTypeCosts final : public CPopupContentType
{
public:
	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *Costs) const override;

	virtual int GetWidth(const wyrmgus::button &button, int *Costs) const override;
	virtual int GetHeight(const wyrmgus::button &button, int *Costs) const override;

	virtual void Parse(lua_State *l) override;

private:
	wyrmgus::font *Font = nullptr; /// Font to use.
	char Centered = 0;               /// if true, center the display.
};

class CPopupContentTypeLine final : public CPopupContentType
{
public:
	CPopupContentTypeLine();

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *Costs) const override;

	virtual int GetWidth(const wyrmgus::button &button, int *Costs) const override;
	virtual int GetHeight(const wyrmgus::button &button, int *Costs) const override;

	virtual void Parse(lua_State *l) override;

private:
	IntColor Color;  /// Color used for line.
	unsigned int Width = 0;     /// line height
	unsigned int Height = 1;    /// line height
};

class CPopupContentTypeVariable final : public CPopupContentType
{
public:
	virtual ~CPopupContentTypeVariable();

	virtual void Draw(int x, int y, const CPopup &popup, const unsigned int popupWidth, const wyrmgus::button &button, int *Costs) const override;

	virtual int GetWidth(const wyrmgus::button &button, int *Costs) const override;
	virtual int GetHeight(const wyrmgus::button &button, int *Costs) const override;

	virtual void Parse(lua_State *l) override;

private:
	std::unique_ptr<StringDesc> Text;            /// Text to display.
	wyrmgus::font *Font = nullptr;   /// Font to use.
	char Centered = 0;                /// if true, center the display.
	int Index = -1;                   /// Index of the variable to show, -1 if not.
};

class CPopup final
{
public:
	CPopup();
	~CPopup();

	std::vector<std::unique_ptr<CPopupContentType>> Contents; /// Array of contents to display.
	std::string Ident;                         /// Ident of the popup.
	int MarginX;                               /// Left and right margin width.
	int MarginY;                               /// Upper and lower margin height.
	int MinWidth = 0;                          /// Minimal width covered by popup.
	int MinHeight = 0;                         /// Minimal height covered by popup.
	const wyrmgus::font *DefaultFont = nullptr; /// Default font for content.
	IntColor BackgroundColor;                  /// Color used for popup's background.
	IntColor BorderColor;                      /// Color used for popup's borders.
};

