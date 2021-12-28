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
/**@name ui.h - The user interface header file. */
//
//      (c) Copyright 1999-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/// @todo this only the start of the new user interface
/// @todo all user interface variables should go here and be configurable

#include "color.h"
#include "economy/resource.h"
#include "ui/interface.h"
#include "ui/statusline.h"
#include "util/singleton.h"
#include "viewport.h"

class CContentType;
class CFile;
class CMapLayer;
class CPopup;
class CUnit;
class LuaActionListener;

namespace wyrmgus {
	class cursor;
	class font;
	class font_color;
	class minimap;
	enum class cursor_type;
}

enum class TextAlignment {
	Undefined,
	Center,
	Left,
	Right
};

class ButtonStyleProperties
{
public:
	std::shared_ptr<CGraphic> Sprite;
	int Frame = 0;
	TextAlignment TextAlign = TextAlignment::Undefined; /// Text alignment
	PixelPos TextPos = PixelPos(0, 0); /// Text location
	const wyrmgus::font_color *TextNormalColor = nullptr; /// Normal text color
	const wyrmgus::font_color *TextReverseColor = nullptr; /// Reverse text color
};

class ButtonStyle
{
public:
	int Width = 0;                  /// Button width
	int Height = 0;                 /// Button height
	wyrmgus::font *Font = nullptr; /// Font
	const wyrmgus::font_color *TextNormalColor = nullptr; /// Normal text color
	const wyrmgus::font_color *TextReverseColor = nullptr; /// Reverse text color
	TextAlignment TextAlign = TextAlignment::Undefined; /// Text alignment
	int TextX = 0;                  /// Text X location
	int TextY = 0;                  /// Text Y location
	ButtonStyleProperties Default;  /// Default button properties
	ButtonStyleProperties Hover;    /// Hover button properties
	ButtonStyleProperties Clicked;  /// Clicked button properties
};

/// buttons on screen themselves
class CUIButton
{
public:
	bool Contains(const PixelPos &screenPos) const;

public:
	int X = 0;                          /// x coordinate on the screen
	int Y = 0;                          /// y coordinate on the screen
	//Wyrmgus start
	bool Clicked = false;					/// whether the button is currently clicked or not
	bool HotkeyPressed = false;				/// whether the button's hotkey is currently pressed or not
	//Wyrmgus end
	std::string Text;               /// button text
	ButtonStyle *Style = nullptr;             /// button style
	LuaActionListener *Callback = nullptr;    /// callback function
};

constexpr int MAX_NUM_VIEWPORTS = 8;         /// Number of supported viewports

class CMapArea final
{
public:
	CMapArea()
	{
	}

	explicit CMapArea(const QRect &rect) : rect(rect)
	{
	}

	bool contains(const QPoint &screen_pos) const;

	const QRect &get_rect() const
	{
		return this->rect;
	}

	int get_bottom() const
	{
		return this->get_rect().bottom();
	}

private:
	QRect rect;
public:
	int ScrollPaddingLeft = 0;          /// Scrollable area past the left of map
	int ScrollPaddingRight = 0;         /// Scrollable area past the right of map
	int ScrollPaddingTop = 0;           /// Scrollable area past the top of map
	int ScrollPaddingBottom = 0;        /// Scrollable area past the bottom of map
};

/**
**  Condition to show panel content.
*/
class ConditionPanel final
{
public:
	bool ShowOnlySelected = false; /// if true, show only for selected unit.

	bool HideNeutral = false;   /// if true, don't show for neutral unit.
	bool HideAllied = false;    /// if true, don't show for allied unit. (but show own units)
	bool ShowOpponent = false;  /// if true, show for opponent unit.
	bool ShowIfCanCastAnySpell = false; /// if true, show only if the unit can cast any spell

	//Wyrmgus start
	char Affixed = 0;			/// check if the button's unit has an affix
	char Unique = 0;			/// check if the button's unit is unique
	char Replenishment = 0;		/// check if the button's unit has resource replenishment
	//Wyrmgus end
	std::unique_ptr<char[]> BoolFlags;  /// array of condition about user flags.
	std::unique_ptr<char[]> Variables;  /// array of variable to verify (enable and max > 0)
};

/**
**  Info for the panel.
*/
class CUnitInfoPanel final
{
public:
	~CUnitInfoPanel();

public:
	std::string Name;      /// Ident of the panel.
	int PosX = 0;              /// X coordinate of the panel.
	int PosY = 0;              /// Y coordinate of the panel.
	wyrmgus::font *DefaultFont = nullptr; /// Default font for content.

	std::vector<std::unique_ptr<CContentType>> Contents; /// Array of contents to display.

	std::unique_ptr<ConditionPanel> Condition; /// Condition to show the panel; if null, no condition.
};

class CFiller final
{
public:
	CFiller() {}

	CFiller(const CFiller &filler)
	{
		*this = filler;
	}

	CFiller(CFiller &&filler)
	{
		this->G = filler.G;
		this->X = filler.X;
		this->Y = filler.Y;
		this->loaded = filler.loaded;

		filler.G = nullptr;
	}

	CFiller &operator =(const CFiller &other_filler);

	bool is_loaded() const
	{
		return this->loaded;
	}

	void Load();
	bool OnGraphic(int x, int y) const;

	std::shared_ptr<CGraphic> G; /// Graphic
	int X = 0;               /// X coordinate
	int Y = 0;               /// Y coordinate
private:
	bool loaded = false;
};

class CButtonPanel
{
public:
	void Draw(std::vector<std::function<void(renderer *)>> &render_commands);
	void Update();
	void DoClicked(int button, const Qt::KeyboardModifiers key_modifiers);
	int DoKey(int key, const Qt::KeyboardModifiers key_modifiers);

private:
	void DoClicked_SelectTarget(int button);

	void DoClicked_Unload(int button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_SpellCast(int button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_Repair(int button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_Return(const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_Stop();
	void DoClicked_StandGround(const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_Button(const int button);
	void DoClicked_CancelUpgrade();
	void DoClicked_CancelTrain();
	void DoClicked_CancelBuild();
	void DoClicked_Build(const std::unique_ptr<wyrmgus::button> &button);
	void DoClicked_Train(const std::unique_ptr<wyrmgus::button> &button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_UpgradeTo(const std::unique_ptr<wyrmgus::button> &button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_ExperienceUpgradeTo(int button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_Research(const std::unique_ptr<wyrmgus::button> &button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_CallbackAction(int button);
	void DoClicked_LearnAbility(int button);
	void DoClicked_Faction(int button);
	void DoClicked_Quest(int button);
	void DoClicked_Buy(int button);
	void DoClicked_ProduceResource(int button);
	void DoClicked_SellResource(int button, const Qt::KeyboardModifiers key_modifiers);
	void DoClicked_BuyResource(int button);
	void DoClicked_Salvage();
	void DoClicked_EnterMapLayer();


public:
	std::shared_ptr<CGraphic> G;
	int X = 0;
	int Y = 0;
	std::vector<CUIButton> Buttons;
};

class CResourceInfo
{
public:
	int IconX = -1;		/// icon X position
	int IconY = -1;		/// icon Y position
	int IconWidth = -1;	/// icon W size
	wyrmgus::font *Font = nullptr; /// Font
	std::string Text;	/// text
	int TextX = -1;	/// text X position
	int TextY = -1;	/// text Y position
};

class CInfoPanel final
{
public:
	void Draw(std::vector<std::function<void(renderer *)>> &render_commands);

	std::shared_ptr<CGraphic> G;
	int X = 0;
	int Y = 0;
};

class CUIUserButton
{
public:
	CUIUserButton() : Clicked(false) {}

	bool Clicked;            // true if button is clicked, false otherwise
	CUIButton Button;        // User button
};

/**
**  Defines the user interface.
*/
class CUserInterface final : public wyrmgus::singleton<CUserInterface>
{
public:
	static CUserInterface *get()
	{
		return wyrmgus::singleton<CUserInterface>::get();
	}

	CUserInterface();
	~CUserInterface();

	void Load();

	wyrmgus::minimap *get_minimap() const
	{
		return this->minimap.get();
	}

	cursor *get_cursor(const cursor_type cursor_type) const
	{
		const auto find_iterator = this->cursors.find(cursor_type);

		if (find_iterator != this->cursors.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error("No cursor found for type \"" + std::to_string(static_cast<int>(cursor_type)) + "\".");
	}

	int get_tooltip_cycle_count() const
	{
		return this->tooltip_cycle_count;
	}

	void increment_tooltip_cycle_count()
	{
		this->tooltip_cycle_count++;
	}

	void reset_tooltip_cycle_count()
	{
		this->tooltip_cycle_count = 0;
	}

	int get_tooltip_cycle_threshold() const
	{
		return CYCLES_PER_SECOND;
	}

	bool MouseScroll = false;                   /// Enable mouse scrolling
	bool KeyScroll = false;                     /// Enable keyboard scrolling
	/// Middle-Mouse Scroll Speed (screenpixels per mousepixel)
	int MouseScrollSpeedDefault = 0;
	/// Middle-Mouse Scroll Speed with Control pressed
	int MouseScrollSpeedControl = 0;

	PixelPos MouseWarpPos;				/// Cursor warp screen position

	std::vector<CFiller> Fillers;		/// Filler graphics

	CInfoPanel InfoPanel;				/// Info panel
	CResourceInfo TimeOfDayPanel;		/// Time of day panel
	CResourceInfo SeasonPanel;			/// Season panel
	CResourceInfo DatePanel;			/// Date panel
	std::vector<std::unique_ptr<CUnitInfoPanel>> InfoPanelContents;	/// Info panel contents

	std::vector<std::unique_ptr<CPopup>> ButtonPopups;	/// Popup windows for buttons

	CUIButton *SingleSelectedButton = nullptr;	/// Button for single selected unit

	std::vector<CUIButton> SelectedButtons;	/// Selected buttons
	wyrmgus::font *MaxSelectedFont = nullptr; /// Font type to use
	int MaxSelectedTextX = 0;				/// position to place '+#' text
	int MaxSelectedTextY = 0;				/// if > maximum units selected

	CUIButton *SingleTrainingButton = nullptr;	/// Button for single training
	std::string SingleTrainingText;		/// Text for single training
	wyrmgus::font *SingleTrainingFont = nullptr; /// Font for single traning
	int SingleTrainingTextX = 0;			/// X text position single training
	int SingleTrainingTextY = 0;			/// Y text position single training

	std::vector<CUIButton> TrainingButtons;/// Training buttons
	std::string TrainingText;           /// Multiple Training Text
	wyrmgus::font *TrainingFont = nullptr; /// Multiple Training Font
	int TrainingTextX = 0;                  /// Multiple Training X Text position
	int TrainingTextY = 0;                  /// Multiple Training Y Text position

	CUIButton *UpgradingButton = nullptr;         /// Button info for upgrade

	CUIButton *ResearchingButton = nullptr;       /// Button info for researching

	std::vector<CUIButton> TransportingButtons;/// Button info for transporting

	CUIButton *IdleWorkerButton = nullptr;		/// Button for the idle worker notification
	CUIButton *LevelUpUnitButton = nullptr;		/// Button for the level up unit notification
	std::vector<CUIButton> HeroUnitButtons;	/// Button for the hero units
	std::vector<CUIButton> InventoryButtons;	/// Button info for inventory items
	
	// Completed bar
	CColor CompletedBarColorRGB;     /// color for completed bar
	IntColor CompletedBarColor = 0;      /// color for completed bar
	bool CompletedBarShadow = 0;         /// should complete bar have shadow

	// Button panel
	CButtonPanel ButtonPanel;

	// Map area
	CViewport *MouseViewport = nullptr;           /// Viewport containing mouse
	CViewport *SelectedViewport = nullptr;        /// Current selected active viewport
	int NumViewports = 0;                   /// # Viewports currently used
	CViewport Viewports[MAX_NUM_VIEWPORTS]; /// Parameters of all viewports
	CMapArea MapArea;                   /// geometry of the whole map area
	wyrmgus::font *MessageFont = nullptr; /// Font used for messages
	int MessageScrollSpeed = 5;             /// Scroll speed in seconds for messages
	
	CMapLayer *CurrentMapLayer = nullptr;
	CMapLayer *PreviousMapLayer = nullptr;

	//Wyrmgus start
	std::vector<CUIButton> WorldButtons;	/// Button info for world map layer buttons
	//Wyrmgus end

	// Used defined buttons
	std::vector<CUIUserButton> UserButtons; /// User buttons

private:
	std::unique_ptr<wyrmgus::minimap> minimap;
public:
	IntColor ViewportCursorColor = 0;       /// minimap cursor color

	// The status line
	CStatusLine StatusLine;             /// status line

	//
	//  Cursors used.
	//
private:
	std::map<cursor_type, cursor *> cursors;

	/// @todo could use different sounds/speech for the errors
	/// Is in gamesounds?
	/// SoundConfig PlacementError;         /// played on placements errors
	/// SoundConfig PlacementSuccess;       /// played on placements success
	/// SoundConfig Click;                  /// click noice used often

	int tooltip_cycle_count = 0;

public:
	std::shared_ptr<CGraphic> VictoryBackgroundG;       /// Victory background graphic
	std::shared_ptr<CGraphic> DefeatBackgroundG;        /// Defeat background graphic
};

extern std::vector<std::unique_ptr<wyrmgus::button>> CurrentButtons;  /// Current Selected Buttons

extern CUserInterface &UI;                           /// The user interface

extern std::string ClickMissile;            /// Missile to show when you click
extern std::string DamageMissile;           /// Missile to show damage caused


/// Hash table of all the button styles
extern std::map<std::string, ButtonStyle *> ButtonStyleHash;

extern bool RightButtonAttacks;         /// right button attacks

extern std::string UiGroupKeys;               /// Up to 11 keys used for group selection

/// Initialize the ui
extern void InitUserInterface();
/// Save the ui state
extern void SaveUserInterface(CFile &file);
/// Clean up the ui module
extern void CleanUserInterface();

extern void FreeButtonStyles();

/// Register ccl features
extern void UserInterfaceCclRegister();

/// return popup by ident string
extern CPopup *PopupByIdent(const std::string &ident);

/// Find a button style
extern ButtonStyle *FindButtonStyle(const std::string &style);

/// Called if the mouse is moved in Normal interface state
extern void UIHandleMouseMove(const PixelPos &pos, const Qt::KeyboardModifiers key_modifiers);
/// Called if any mouse button is pressed down
extern void UIHandleButtonDown(unsigned button, const Qt::KeyboardModifiers key_modifiers);
/// Called if any mouse button is released up
extern void UIHandleButtonUp(unsigned button, const Qt::KeyboardModifiers key_modifiers);

/// Restrict mouse cursor to viewport
extern void RestrictCursorToViewport();
/// Restrict mouse cursor to minimap
extern void RestrictCursorToMinimap();

extern void MouseScrollMap(const PixelPos &pos, const Qt::KeyboardModifiers key_modifiers);

/// Get viewport for screen pixel position
extern CViewport *GetViewport(const PixelPos &screenPos);

/// Check if mouse scrolling is enabled
extern bool GetMouseScroll();
/// Enable/disable scrolling with the mouse
extern void SetMouseScroll(bool enabled);
/// Check if keyboard scrolling is enabled
extern bool GetKeyScroll();
/// Enable/disable scrolling with the keyboard
extern void SetKeyScroll(bool enabled);
/// Check if scrolling stops when leaving the window
extern bool GetLeaveStops();
/// Enable/disable leaving the window stops scrolling
extern void SetLeaveStops(bool enabled);

extern int AddHandler(lua_State *l);
extern void CallHandler(unsigned int handle, int value);

/// Show load progress
extern void ShowLoadProgress(const char *fmt, ...) PRINTF_VAARG_ATTRIBUTE(1, 2);

extern void CalculateItemsToLoad();
extern void IncItemsLoaded();
extern void ResetItemsToLoad();
