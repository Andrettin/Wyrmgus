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
/**@name interface.h - The user interface header file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "sound/unit_sound.h"
#include "vec2i.h"

#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class ButtonAction;
class CButtonLevel;
class CUIButton;
class CUnit;
struct EventCallback;

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

/// Button area under cursor
enum _button_area_ {
	ButtonAreaSelected,      /// Selected button
	ButtonAreaTraining,      /// Training button
	ButtonAreaUpgrading,     /// Upgrading button
	ButtonAreaResearching,   /// Researching button
	ButtonAreaTransporting,  /// Transporting button
	ButtonAreaButton,        /// Button panel button
	ButtonAreaMenu,          /// Menu button
	ButtonAreaIdleWorker,	 /// Idle worker button
	ButtonAreaLevelUpUnit,	 /// Level up unit button
	ButtonAreaHeroUnit,		 /// Hero unit button
	ButtonAreaInventory,  /// Transporting button
	ButtonAreaMapLayerPlane,	/// Plane button
	ButtonAreaMapLayerWorld,	/// World button
	ButtonAreaMapLayerSurfaceLayer,	/// Surface layer button
	ButtonAreaUser           /// User buttons
};

/// Menu button under cursor
enum _menu_button_under_ {
	ButtonUnderMenu,              /// Menu button
	ButtonUnderNetworkMenu,       /// Network menu button
	ButtonUnderNetworkDiplomacy   /// Diplomacy button
};

/// current interface state
enum _iface_state_ {
	IfaceStateNormal,  /// Normal Game state
	IfaceStateMenu     /// Menu active
};

/// current key state
enum _key_state_ {
	KeyStateCommand = 0,  /// keys -> commands
	KeyStateInput         /// keys -> line editor
};                        /// current keyboard state

/// Key modifier
#define ModifierShift 1        /// any shift key pressed
#define ModifierControl 2      /// any control key pressed
#define ModifierAlt 4          /// any alt key pressed
#define ModifierSuper 8        /// super key (reserved for WM)
#define ModifierDoublePress 16 /// key double pressed

#define MouseDoubleShift 8   /// shift for double click button
#define MouseDragShift   16  /// shift for drag button
#define MouseHoldShift   24  /// shift for hold button

/// pressed mouse button flags
#define NoButton 0      /// No button
#define LeftButton 2    /// Left button on mouse
#define MiddleButton 4  /// Middle button on mouse
#define RightButton 8   /// Right button on mouse

#define UpButton 16    /// Scroll up button on mouse
#define DownButton 32  /// Scroll down button on mouse

#define LeftAndMiddleButton  (LeftButton | MiddleButton)  /// Left + Middle button on mouse
#define LeftAndRightButton   (LeftButton | RightButton)   /// Left + Right button on mouse
#define MiddleAndRightButton (MiddleButton | RightButton) /// Middle + Right button on mouse

/// Where is our cursor ?
enum _cursor_on_ {
	CursorOnUnknown = -1,     /// not known
	CursorOnMinimap,          /// minimap area
	CursorOnButton,           /// button area see: ButtonUnderCursor
	CursorOnMap,              /// over map area
	CursorOnScrollUp,         /// in scroll up area
	CursorOnScrollDown,       /// in scroll down area
	CursorOnScrollLeft,       /// in scroll left area
	CursorOnScrollRight,      /// in scroll right area
	CursorOnScrollLeftUp,     /// in scroll left+up area
	CursorOnScrollLeftDown,   /// in scroll left+down area
	CursorOnScrollRightUp,    /// in scroll right+up area
	CursorOnScrollRightDown   /// in scroll right+down area
};

/// Are We Scrolling With the Keyboard ?
#define ScrollNone 0        /// not scrolling
#define ScrollUp 1          /// scroll up only
#define ScrollDown 2        /// scroll down only
#define ScrollLeft 4        /// scroll left only
#define ScrollRight 8       /// scroll right only
#define ScrollLeftUp 5      /// scroll left + up
#define ScrollLeftDown 6    /// scroll left + down
#define ScrollRightUp 9     /// scroll right + up
#define ScrollRightDown 10  /// scroll right + down

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/// Flag telling if the game is running
extern bool GameRunning;
/// Flag telling if the game is paused
extern bool GamePaused;
/// Flag telling if the game is in observe mode
extern bool GameObserve;
/// Flag telling if the game is in establishing mode
extern bool GameEstablishing;
/// Flag telling not to advance to the next game cycle
extern char SkipGameCycle;
/// Invincibility cheat
extern bool GodMode;
/// Whether the map is the only thing displayed or not
extern char BigMapMode;
/// Flag telling if the SDL window is visible
extern bool IsSDLWindowVisible;

/// pressed mouse buttons (normal,double,dragged,long)
extern int MouseButtons;
/// current active modifiers
extern int KeyModifiers;
/// current interface state
extern enum _iface_state_ InterfaceState;
/// current scroll state of keyboard
extern int KeyScrollState;
/// current scroll state of mouse
extern int MouseScrollState;
/// current key state
extern enum _key_state_ KeyState;
/// shared pointer to unit under the cursor
extern CUnit *UnitUnderCursor;
/// button area under the cursor
extern int ButtonAreaUnderCursor;
/// button number under the cursor
extern int ButtonUnderCursor;
/// oldbutton number under the cursor
extern int OldButtonUnderCursor;
//Wyrmgus start
/*
/// menu button was clicked down
extern bool GameMenuButtonClicked;
/// diplomacy button was clicked down
extern bool GameDiplomacyButtonClicked;
*/
//Wyrmgus end
/// Mouse leaves windows stops scroll
extern bool LeaveStops;
/// current CursorOn field
extern enum _cursor_on_ CursorOn;

/// vladi: used for unit buttons sub-menus etc
extern CButtonLevel *CurrentButtonLevel;
/// Last drawn popup : used to speed up drawing
extern ButtonAction *LastDrawnButtonPopup;

/// Time to detect double clicks
extern int DoubleClickDelay;
/// Time to detect hold clicks
extern int HoldClickDelay;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern CUnit *GetUnitUnderCursor();

//
// in mouse.cpp
//
/// Called if any mouse button is pressed down
extern void HandleButtonDown(unsigned button);
/// Called if any mouse button is released up
extern void HandleButtonUp(unsigned button);
/// Keep coordinates in window and update cursor position
extern void HandleCursorMove(int *x, int *y);
/// Called if the mouse is moved
extern void HandleMouseMove(const PixelPos &pos);
/// Called if the mouse exits the game window (only for some videomodes)
extern void HandleMouseExit();

/// Update KeyModifiers if a key is pressed
extern int HandleKeyModifiersDown(unsigned keycode, unsigned keychar);
/// Update KeyModifiers if a key is released
extern int HandleKeyModifiersUp(unsigned keycode, unsigned keychar);

/// Called if a key is pressed
extern void HandleKeyDown(unsigned keycode, unsigned keychar);
/// Called when a key is released
extern void HandleKeyUp(unsigned keycode, unsigned keychar);
/// Called when a key is repeated
extern void HandleKeyRepeat(unsigned keycode, unsigned keychar);

//
// in interface.c (for link between video and mouse.c)
//
/// Called if any mouse button is pressed down
extern void InputMouseButtonPress(const EventCallback &callbacks, unsigned ticks, unsigned button);
/// Called if any mouse button is released up
extern void InputMouseButtonRelease(const EventCallback &callbacks, unsigned ticks, unsigned button);
/// Called if the mouse is moved
extern void InputMouseMove(const EventCallback &callbacks, unsigned ticks, int x, int y);
/// Called if the mouse exits the game window (when supported by videomode)
extern void InputMouseExit(const EventCallback &callbacks, unsigned ticks);
/// Called to look for mouse timeouts
extern void InputMouseTimeout(const EventCallback &callbacks, unsigned ticks);

/// Called if any key button is pressed down
extern void InputKeyButtonPress(const EventCallback &callbacks, unsigned ticks, unsigned ikey, unsigned ikeychar);
/// Called if any key button is released up
extern void InputKeyButtonRelease(const EventCallback &callbacks, unsigned ticks, unsigned ikey, unsigned ikeychar);
/// Called to look for key timeouts
extern void InputKeyTimeout(const EventCallback &callbacks, unsigned ticks);

/// Get double click delay
extern int GetDoubleClickDelay();
/// Set double click delay
extern void SetDoubleClickDelay(int delay);
/// Get hold click delay
extern int GetHoldClickDelay();
/// Set hold click delay
extern void SetHoldClickDelay(int delay);

/// Toggle pause mode
extern void UiTogglePause();
/// Toggle big map
extern void UiToggleBigMap();
/// Toggle terrain display on/off.
extern void UiToggleTerrain();
/// Find the next idle worker
extern void UiFindIdleWorker();
//Wyrmgus start
/// Find the next level up unit
extern void UiFindLevelUpUnit();
/// Find custom hero
extern void UiFindHeroUnit(int hero_index);
//Wyrmgus end
/// Track unit, the viewport follows the unit.
extern void UiTrackUnit();

//Wyrmgus start
extern bool IsMouseLeftButtonPressed();
extern std::string GetCurrentButtonValueStr();
//Wyrmgus end

/// Handle cheats
extern int HandleCheats(const std::string &input);

/// Call the lua function HandleCommandKey
bool HandleCommandKey(int key);

//
// Chaos pur.
//
/// Cancel the building input mode
extern void CancelBuildingMode();

/// Draw menu button area
extern void DrawMenuButtonArea();
/// Draw user defined buttons
extern void DrawUserDefinedButtons();
/// Update messages
extern void UpdateMessages();
/// Draw messages as overlay over of the map
extern void DrawMessages();
/// Draw the player resource in resource line
extern void DrawResources();
/// Draw the time of day, season and date
extern void DrawTime();
/// Draw the age
extern void DrawAge();
/// Draw the map layer buttons
extern void DrawMapLayerButtons();
//Wyrmgus start
/// Draw certain popups if something is being hovered over
extern void DrawPopups();
//Wyrmgus end
/// Set message to display
extern void SetMessage(const char *fmt, ...) PRINTF_VAARG_ATTRIBUTE(1, 2);
/// Set message to display with event point
//Wyrmgus start
//extern void SetMessageEvent(const Vec2i &pos, const char *fmt, ...) PRINTF_VAARG_ATTRIBUTE(2, 3);
extern void SetMessageEvent(const Vec2i &pos, int z, const char *fmt, ...) PRINTF_VAARG_ATTRIBUTE(3, 4);
//Wyrmgus end
//Wyrmgus start
/// Set objective to display
extern void SetObjective(const char *fmt, ...) PRINTF_VAARG_ATTRIBUTE(1, 2);
//Wyrmgus end
/// Center view-point on last event message
extern void CenterOnMessage();
/// Cleanup all messages
extern void CleanMessages();
//Wyrmgus start
/// Cleanup all objectives
extern void CleanObjectives();
//Wyrmgus end
/// show/hide messages
extern void ToggleShowMessages();

/// Draw the timer
extern void DrawTimer();
/// Update the timer
extern void UpdateTimer();
/// Update the status line with hints from the button
extern void UpdateStatusLineForButton(const ButtonAction &button);
/// Draw the Pie Menu
extern void DrawPieMenu();
/// Draw the button popup
extern void DrawPopup(const ButtonAction &button, int x = 0, int y = 0, bool above = true);
//Wyrmgus start
extern void DrawGenericPopup(const std::string &popup_text, int x = 0, int y = 0, std::string text_color = "", std::string highlight_color = "", bool above = true);
//Wyrmgus end

/// Handle the mouse in scroll area
extern bool HandleMouseScrollArea(const PixelPos &mousePos);

//
// in button_checks.cpp
//
/// Check is always true
extern bool ButtonCheckTrue(const CUnit &unit, const ButtonAction &button);
/// Check is always false
extern bool ButtonCheckFalse(const CUnit &unit, const ButtonAction &button);
/// Check if allowed upgrade is ready
extern bool ButtonCheckUpgrade(const CUnit &unit, const ButtonAction &button);
/// Check if the upgrade has not been acquired
extern bool ButtonCheckUpgradeNot(const CUnit &unit, const ButtonAction &button);
/// Check if any of the allowed upgrade list are ready
extern bool ButtonCheckUpgradeOr(const CUnit &unit, const ButtonAction &button);
/// Check if unit has an individual upgrade
extern bool ButtonCheckIndividualUpgrade(const CUnit &unit, const ButtonAction &button);
/// Check if unit has any of the individual upgrades in the list
extern bool ButtonCheckIndividualUpgradeOr(const CUnit &unit, const ButtonAction &button);
/// Check if unit's variables pass the condition check
extern bool ButtonCheckUnitVariable(const CUnit &unit, const ButtonAction &button);
/// Check if allowed units exists
extern bool ButtonCheckUnitsOr(const CUnit &unit, const ButtonAction &button);
/// Check if allowed units exists
extern bool ButtonCheckUnitsAnd(const CUnit &unit, const ButtonAction &button);
/// Check if units don't exist
extern bool ButtonCheckUnitsNot(const CUnit &unit, const ButtonAction &button);
/// Check if have network play
extern bool ButtonCheckNetwork(const CUnit &unit, const ButtonAction &button);
/// Check if don't have network play
extern bool ButtonCheckNoNetwork(const CUnit &unit, const ButtonAction &button);
/// Check if unit isn't working (train,upgrade,research)
extern bool ButtonCheckNoWork(const CUnit &unit, const ButtonAction &button);
/// Check if unit isn't researching or upgrading
extern bool ButtonCheckNoResearch(const CUnit &unit, const ButtonAction &button);
/// Check if all requirements for an attack to are meet
extern bool ButtonCheckAttack(const CUnit &unit, const ButtonAction &button);
/// Check if all requirements for an upgrade to are meet
extern bool ButtonCheckUpgradeTo(const CUnit &unit, const ButtonAction &button);
/// Check if all requirements for a research are meet
extern bool ButtonCheckResearch(const CUnit &unit, const ButtonAction &button);
/// Check if all requirements for a single research are meet
extern bool ButtonCheckSingleResearch(const CUnit &unit, const ButtonAction &button);
//Wyrmgus start
/// Check if the unit has an inventory
extern bool ButtonCheckHasInventory(const CUnit &unit, const ButtonAction &button);
/// Check if the unit has buttons for the desired level
extern bool ButtonCheckHasSubButtons(const CUnit &unit, const ButtonAction &button);
//Wyrmgus end

//
// in ccl_ui.c
//
/// Called whenever the units selection is altered
extern void SelectionChanged();
/// Called whenever the selected unit was updated
extern void SelectedUnitChanged();

//
// in game.cpp
//
/// Set the game paused or unpaused
extern void SetGamePaused(bool paused);
/// Get the game paused or unpaused
extern bool GetGamePaused();
/// Set the game speed
extern void SetGameSpeed(int speed);
/// Get the game speed
extern int GetGameSpeed();

#endif
