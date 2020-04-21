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
//      (c) Copyright 1999-2020 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/// @todo this only the start of the new user interface
/// @todo all user interface variables should go here and be configurable

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "color.h"
#include "cursor.h"
#include "map/minimap.h"
#include "script.h"
#include "viewport.h"
#include "ui/interface.h"
#include "ui/statusline.h"
#include "ui/uitimer.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CContentType;
class CFile;
class CFont;
class CMapLayer;
class CPopup;
class CUnit;
class LuaActionListener;

/*----------------------------------------------------------------------------
--  Definitions
----------------------------------------------------------------------------*/

enum class TextAlignment {
	Undefined,
	Center,
	Left,
	Right
};

class ButtonStyleProperties
{
public:
	ButtonStyleProperties() : Sprite(nullptr), Frame(0), BorderColor(0),
		BorderSize(0), TextAlign(TextAlignment::Undefined),
		TextPos(0, 0)
	{}

	CGraphic *Sprite;
	int Frame;
	CColor BorderColorRGB;
	IntColor BorderColor;
	int BorderSize;
	TextAlignment TextAlign;        /// Text alignment
	PixelPos TextPos;               /// Text location
	std::string TextNormalColor;    /// Normal text color
	std::string TextReverseColor;   /// Reverse text color
};

class ButtonStyle
{
public:
	ButtonStyle() : Width(0), Height(0), Font(nullptr),
		TextAlign(TextAlignment::Undefined), TextX(0), TextY(0) {}

	int Width;                      /// Button width
	int Height;                     /// Button height
	CFont *Font;                    /// Font
	std::string TextNormalColor;    /// Normal text color
	std::string TextReverseColor;   /// Reverse text color
	TextAlignment TextAlign;        /// Text alignment
	int TextX;                      /// Text X location
	int TextY;                      /// Text Y location
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

static constexpr int MAX_NUM_VIEWPORTS = 8;         /// Number of supported viewports

/**
**  Enumeration of the different predefined viewport configurations.
**
**  @todo this should be later user configurable
*/
enum ViewportModeType {
	VIEWPORT_SINGLE = 0,                /// Old single viewport
	VIEWPORT_SPLIT_HORIZ,           /// Two viewports split horizontal
	VIEWPORT_SPLIT_HORIZ3,          /// Three viewports split horiontal
	VIEWPORT_SPLIT_VERT,            /// Two viewports split vertical
	VIEWPORT_QUAD,                  /// Four viewports split symmetric
	NUM_VIEWPORT_MODES             /// Number of different viewports.
};

class CMapArea
{
public:
	CMapArea() : X(0), Y(0), EndX(0), EndY(0),
		ScrollPaddingLeft(0), ScrollPaddingRight(0),
		ScrollPaddingTop(0), ScrollPaddingBottom(0) {}

	bool Contains(const PixelPos &screenPos) const;

public:
	int X;                          /// Screen pixel left corner x coordinate
	int Y;                          /// Screen pixel upper corner y coordinate
	int EndX;                       /// Screen pixel right x coordinate
	int EndY;                       /// Screen pixel bottom y coordinate
	int ScrollPaddingLeft;          /// Scrollable area past the left of map
	int ScrollPaddingRight;         /// Scrollable area past the right of map
	int ScrollPaddingTop;           /// Scrollable area past the top of map
	int ScrollPaddingBottom;        /// Scrollable area past the bottom of map
};

/**
**  Condition to show panel content.
*/
class ConditionPanel
{
public:
	ConditionPanel() : ShowOnlySelected(false), HideNeutral(false),
		HideAllied(false), ShowOpponent(false), ShowIfCanCastAnySpell(false), BoolFlags(nullptr),
		//Wyrmgus start
		Affixed(0), Unique(0), Replenishment(0),
		//Wyrmgus end
		Variables(nullptr) {}
	~ConditionPanel()
	{
		delete[] BoolFlags;
		delete[] Variables;
	}

	bool ShowOnlySelected;      /// if true, show only for selected unit.

	bool HideNeutral;           /// if true, don't show for neutral unit.
	bool HideAllied;            /// if true, don't show for allied unit. (but show own units)
	bool ShowOpponent;          /// if true, show for opponent unit.
	bool ShowIfCanCastAnySpell; /// if true, show only if the unit can cast any spell

	//Wyrmgus start
	char Affixed;				/// check if the button's unit has an affix
	char Unique;				/// check if the button's unit is unique
	char Replenishment;			/// check if the button's unit has resource replenishment
	//Wyrmgus end
	char *BoolFlags;            /// array of condition about user flags.
	char *Variables;            /// array of variable to verify (enable and max > 0)
};

/**
**  Info for the panel.
*/
class CUnitInfoPanel
{
public:
	CUnitInfoPanel() : PosX(0), PosY(0), DefaultFont(0),
		Contents(), Condition(nullptr) {}
	~CUnitInfoPanel();

public:
	std::string Name;      /// Ident of the panel.
	int PosX;              /// X coordinate of the panel.
	int PosY;              /// Y coordinate of the panel.
	CFont *DefaultFont;    /// Default font for content.

	std::vector<CContentType *>Contents; /// Array of contents to display.

	ConditionPanel *Condition; /// Condition to show the panel; if null, no condition.
};


class CFiller
{
	struct bits_map {
		~bits_map();

		void Init(CGraphic *g);

		bool TransparentPixel(int x, int y)
		{
			if (bstore) {
				const unsigned int x_index = x / 32;
				y *= Width;
				y /= 32;
				x -= (x_index * 32);
				return ((bstore[y + x_index] & (1 << x)) == 0);
			}
			return false;
		};

		int Width = 0;
		int Height = 0;
		unsigned int *bstore = nullptr;
	};

	bits_map map;
public:
	void Load();

	bool OnGraphic(int x, int y)
	{
		x -= X;
		y -= Y;
		if (x >= 0 && y >= 0 && x < map.Width && y < map.Height) {
			return !map.TransparentPixel(x, y);
		}
		return false;
	}
	CGraphic *G = nullptr;         /// Graphic
	int X = 0;               /// X coordinate
	int Y = 0;               /// Y coordinate
};

class CButtonPanel
{
public:
	void Draw();
	void Update();
	void DoClicked(int button);
	int DoKey(int key);

private:
	void DoClicked_SelectTarget(int button);

	void DoClicked_Unload(int button);
	void DoClicked_SpellCast(int button);
	void DoClicked_Repair(int button);
	void DoClicked_Return();
	void DoClicked_Stop();
	void DoClicked_StandGround();
	void DoClicked_Button(int button);
	void DoClicked_CancelUpgrade();
	void DoClicked_CancelTrain();
	void DoClicked_CancelBuild();
	void DoClicked_Build(int button);
	void DoClicked_Train(int button);
	void DoClicked_UpgradeTo(int button);
	void DoClicked_Research(int button);
	void DoClicked_CallbackAction(int button);
	void DoClicked_LearnAbility(int button);
	void DoClicked_ExperienceUpgradeTo(int button);
	void DoClicked_RallyPoint(int button);
	void DoClicked_Faction(int button);
	void DoClicked_Quest(int button);
	void DoClicked_Buy(int button);
	void DoClicked_ProduceResource(int button);
	void DoClicked_SellResource(int button);
	void DoClicked_BuyResource(int button);
	void DoClicked_Salvage();
	void DoClicked_EnterMapLayer();


public:
	CGraphic *G = nullptr;
	int X = 0;
	int Y = 0;
	std::vector<CUIButton> Buttons;
	CColor AutoCastBorderColorRGB;
	bool ShowCommandKey = true;
};

class CPieMenu
{
public:
	CPieMenu() : G(nullptr), MouseButton(NoButton)
	{
		memset(this->X, 0, sizeof(this->X));
		memset(this->Y, 0, sizeof(this->Y));
	}

	CGraphic *G;         /// Optional background image
	int MouseButton;     /// Which mouse button pops up the piemenu, deactivate with NoButton
	int X[9];            /// X position of the pies
	int Y[9];            /// Y position of the pies

	void SetRadius(int radius)
	{
		const int coeffX[] = {    0,  193, 256, 193,   0, -193, -256, -193, 0};
		const int coeffY[] = { -256, -193,   0, 193, 256,  193,    0, -193, 0};
		for (int i = 0; i < 9; ++i) {
			this->X[i] = (coeffX[i] * radius) >> 8;
			this->Y[i] = (coeffY[i] * radius) >> 8;
		}
	}
};

class CResourceInfo
{
public:
	CResourceInfo() : G(nullptr), IconFrame(0), IconX(0), IconY(0), IconWidth(-1),
		Font(nullptr), TextX(-1), TextY(-1) {}

	CGraphic *G;	/// icon graphic
	int IconFrame;	/// icon frame
	int IconX;		/// icon X position
	int IconY;		/// icon Y position
	int IconWidth;	/// icon W size
	CFont *Font;	/// Font
	std::string Text;	/// text
	int TextX;	/// text X position
	int TextY;	/// text Y position
};
static constexpr int MaxResourceInfo = MaxCosts + 4; /// +4 for food and score and mana and free workers count

class CInfoPanel
{
public:
	CInfoPanel() : G(nullptr), X(0), Y(0) {}

	void Draw();

	CGraphic *G;
	int X;
	int Y;
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
class CUserInterface
{
public:
	CUserInterface();
	~CUserInterface();

	void Load();

	bool MouseScroll;                   /// Enable mouse scrolling
	bool KeyScroll;                     /// Enable keyboard scrolling
	/// Key Scroll Speed
	int KeyScrollSpeed;
	/// Mouse Scroll Speed (screenpixels per mousepixel)
	int MouseScrollSpeed;
	/// Middle-Mouse Scroll Speed (screenpixels per mousepixel)
	int MouseScrollSpeedDefault;
	/// Middle-Mouse Scroll Speed with Control pressed
	int MouseScrollSpeedControl;

	PixelPos MouseWarpPos;				/// Cursor warp screen position

	std::string NormalFontColor;		/// Color for normal text displayed
	std::string ReverseFontColor;		/// Color for reverse text displayed

	std::vector<CFiller> Fillers;		/// Filler graphics

	CResourceInfo Resources[MaxResourceInfo];	/// Icon+Text of all resources

	CInfoPanel InfoPanel;				/// Info panel
	CResourceInfo TimeOfDayPanel;		/// Time of day panel
	CResourceInfo SeasonPanel;			/// Season panel
	CResourceInfo DatePanel;			/// Date panel
	CResourceInfo AgePanel;				/// Age panel
	std::vector<CUnitInfoPanel *> InfoPanelContents;	/// Info panel contents

	std::vector<CPopup *> ButtonPopups;	/// Popup windows for buttons

	CUIButton *SingleSelectedButton;	/// Button for single selected unit

	std::vector<CUIButton> SelectedButtons;	/// Selected buttons
	CFont *MaxSelectedFont;				/// Font type to use
	int MaxSelectedTextX;				/// position to place '+#' text
	int MaxSelectedTextY;				/// if > maximum units selected

	CUIButton *SingleTrainingButton;	/// Button for single training
	std::string SingleTrainingText;		/// Text for single training
	CFont *SingleTrainingFont;			/// Font for single traning
	int SingleTrainingTextX;			/// X text position single training
	int SingleTrainingTextY;			/// Y text position single training

	std::vector<CUIButton> TrainingButtons;/// Training buttons
	std::string TrainingText;           /// Multiple Training Text
	CFont *TrainingFont;                /// Multiple Training Font
	int TrainingTextX;                  /// Multiple Training X Text position
	int TrainingTextY;                  /// Multiple Training Y Text position

	CUIButton *UpgradingButton;         /// Button info for upgrade

	CUIButton *ResearchingButton;       /// Button info for researching

	std::vector<CUIButton> TransportingButtons;/// Button info for transporting

	CUIButton *IdleWorkerButton;		/// Button for the idle worker notification
	CUIButton *LevelUpUnitButton;		/// Button for the level up unit notification
	std::vector<CUIButton> HeroUnitButtons;	/// Button for the hero units
	std::vector<CUIButton> InventoryButtons;	/// Button info for inventory items
	
	// Completed bar
	CColor CompletedBarColorRGB;     /// color for completed bar
	IntColor CompletedBarColor;      /// color for completed bar
	bool CompletedBarShadow;         /// should complete bar have shadow

	// Button panel
	CButtonPanel ButtonPanel;

	// Pie Menu
	CPieMenu PieMenu;

	// Map area
	ViewportModeType ViewportMode;      /// Current viewport mode
	CViewport *MouseViewport;           /// Viewport containing mouse
	CViewport *SelectedViewport;        /// Current selected active viewport
	int NumViewports;                   /// # Viewports currently used
	CViewport Viewports[MAX_NUM_VIEWPORTS]; /// Parameters of all viewports
	CMapArea MapArea;                   /// geometry of the whole map area
	CFont *MessageFont;                 /// Font used for messages
	int MessageScrollSpeed;             /// Scroll speed in seconds for messages
	
	CMapLayer *CurrentMapLayer;
	CMapLayer *PreviousMapLayer;

	// Menu buttons
	CUIButton MenuButton;               /// menu button
	//Wyrmgus start
//	CUIButton NetworkMenuButton;        /// network menu button
	//Wyrmgus end
	CUIButton NetworkDiplomacyButton;   /// network diplomacy button
	
	//Wyrmgus start
	std::vector<CUIButton> PlaneButtons;	/// Button info for plane map layer buttons
	std::vector<CUIButton> WorldButtons;	/// Button info for world map layer buttons
	std::vector<CUIButton> SurfaceLayerButtons;	/// Button info for surface layer map layer buttons
	//Wyrmgus end

	// Used defined buttons
	std::vector<CUIUserButton> UserButtons; /// User buttons

	// The minimap
	CMinimap Minimap;                   /// minimap
	IntColor ViewportCursorColor;       /// minimap cursor color

	// The status line
	CStatusLine StatusLine;             /// status line

	// Game timer
	CUITimer Timer;                     /// game timer

	// Offsets for 640x480 center used by menus
	int Offset640X;                     /// Offset for 640x480 X position
	int Offset480Y;                     /// Offset for 640x480 Y position

	//
	//  Cursors used.
	//
	CursorConfig Point;                 /// General pointing cursor
	CursorConfig Glass;                 /// HourGlass, system is waiting
	CursorConfig Cross;                 /// Multi-select cursor.
	CursorConfig YellowHair;            /// Yellow action,attack cursor.
	CursorConfig GreenHair;             /// Green action,attack cursor.
	CursorConfig RedHair;               /// Red action,attack cursor.
	CursorConfig Scroll;                /// Cursor for scrolling map around.

	CursorConfig ArrowE;                /// Cursor pointing east
	CursorConfig ArrowNE;               /// Cursor pointing north east
	CursorConfig ArrowN;                /// Cursor pointing north
	CursorConfig ArrowNW;               /// Cursor pointing north west
	CursorConfig ArrowW;                /// Cursor pointing west
	CursorConfig ArrowSW;               /// Cursor pointing south west
	CursorConfig ArrowS;                /// Cursor pointing south
	CursorConfig ArrowSE;               /// Cursor pointing south east

	/// @todo could use different sounds/speech for the errors
	/// Is in gamesounds?
	/// SoundConfig PlacementError;         /// played on placements errors
	/// SoundConfig PlacementSuccess;       /// played on placements success
	/// SoundConfig Click;                  /// click noice used often

	CGraphic *VictoryBackgroundG;       /// Victory background graphic
	CGraphic *DefeatBackgroundG;        /// Defeat background graphic
};

extern std::vector<ButtonAction> CurrentButtons;  /// Current Selected Buttons

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern CUserInterface UI;                           /// The user interface

extern std::string ClickMissile;            /// Missile to show when you click
extern std::string DamageMissile;           /// Missile to show damage caused


/// Hash table of all the button styles
extern std::map<std::string, ButtonStyle *> ButtonStyleHash;

extern bool RightButtonAttacks;         /// right button attacks

extern const char DefaultGroupKeys[];         /// Default group keys
extern std::string UiGroupKeys;               /// Up to 11 keys used for group selection

extern bool FancyBuildings;             /// Mirror buildings 1 yes, 0 now.

// only exported to save them

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/// Initialize the ui
extern void InitUserInterface();
/// Save the ui state
extern void SaveUserInterface(CFile &file);
/// Clean up the ui module
extern void CleanUserInterface();
//Wyrmgus start
void CleanUserInterfaceFillers();
extern void UpdateSurfaceLayerButtons();
//Wyrmgus end

extern void FreeButtonStyles();

/// Register ccl features
extern void UserInterfaceCclRegister();

/// return popup by ident string
extern CPopup *PopupByIdent(const std::string &ident);

/// Find a button style
extern ButtonStyle *FindButtonStyle(const std::string &style);

/// Called if the mouse is moved in Normal interface state
extern void UIHandleMouseMove(const PixelPos &pos);
/// Called if any mouse button is pressed down
extern void UIHandleButtonDown(unsigned button);
/// Called if any mouse button is released up
extern void UIHandleButtonUp(unsigned button);

/// Restrict mouse cursor to viewport
extern void RestrictCursorToViewport();
/// Restrict mouse cursor to minimap
extern void RestrictCursorToMinimap();

/// Get viewport for screen pixel position
extern CViewport *GetViewport(const PixelPos &screenPos);
/// Cycle through all available viewport modes
extern void CycleViewportMode(int);
/// Select viewport mode
extern void SetViewportMode(ViewportModeType mode);
extern void SetNewViewportMode(ViewportModeType mode);
extern void CheckViewportMode();

/// Check if mouse scrolling is enabled
extern bool GetMouseScroll();
/// Enable/disable scrolling with the mouse
extern void SetMouseScroll(bool enabled);
/// Check if keyboard scrolling is enabled
extern bool GetKeyScroll();
/// Enable/disable scrolling with the keyboard
extern void SetKeyScroll(bool enabled);
/// Check if mouse grabbing is enabled
extern bool GetGrabMouse();
/// Enable/disable grabbing the mouse
extern void SetGrabMouse(bool enabled);
/// Check if scrolling stops when leaving the window
extern bool GetLeaveStops();
/// Enable/disable leaving the window stops scrolling
extern void SetLeaveStops(bool enabled);

extern int AddHandler(lua_State *l);
extern void CallHandler(unsigned int handle, int value);

/// Show load progress
extern void ShowLoadProgress(const char *fmt, ...) PRINTF_VAARG_ATTRIBUTE(1, 2);
/// Update load progress
extern void UpdateLoadProgress();

extern void CalculateItemsToLoad();
extern void UpdateLoadingBar();
extern void IncItemsLoaded();
extern void ResetItemsToLoad();
