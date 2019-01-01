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
/**@name button_action.h - The button action header file. */
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

#ifndef __BUTTON_ACTION_H__
#define __BUTTON_ACTION_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <string>
#include <vector>

#include "icons.h"
#include "unitsound.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CButtonLevel;
class CUnit;

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

/// Button Commands that need target selection
enum ButtonCmd {
	ButtonMove,           /// order move
	ButtonAttack,         /// order attack
	ButtonRepair,         /// order repair
	ButtonHarvest,        /// order harvest
	ButtonBuild,          /// order build
	ButtonPatrol,         /// order patrol
	ButtonAttackGround,   /// order attack ground
	ButtonSpellCast,      /// order cast spell
	ButtonUnload,         /// order unload unit
	ButtonStop,           /// order stop
	ButtonButton,         /// choose other button set
	ButtonTrain,          /// order train
	ButtonStandGround,    /// order stand ground
	ButtonReturn,         /// order return goods
	ButtonResearch,       /// order reseach
	ButtonLearnAbility,   /// order learn ability
	ButtonExperienceUpgradeTo,   /// order upgrade (experience)
	ButtonUpgradeTo,      /// order upgrade
	ButtonRallyPoint,		/// set rally point
	ButtonFaction,			/// change faction
	ButtonQuest,			/// receive quest
	ButtonBuy,				/// buy item
	ButtonProduceResource,	/// produce a resource
	ButtonSellResource,		/// sell a resource
	ButtonBuyResource,		/// buy a resource
	ButtonSalvage,			/// salvage a building
	ButtonEnterMapLayer,	/// enter a map layer
	ButtonUnit,				/// used to display popups for inventory items and for units in transporters
	ButtonEditorUnit,		/// used to display popups for editor unit type buttons
	ButtonCancel,         /// cancel
	ButtonCancelUpgrade,  /// cancel upgrade
	ButtonCancelTrain,    /// cancel training
	ButtonCancelBuild,    /// cancel building
	ButtonCallbackAction
};

class ButtonAction;
typedef bool (*ButtonCheckFunc)(const CUnit &, const ButtonAction &);

/// Action of button
class ButtonAction
{
public:
	ButtonAction() : Pos(0), Level(nullptr), AlwaysShow(false), Action(ButtonMove), Value(0), Payload(nullptr),
		Allowed(nullptr), Key(0) {}
	
	static void ProcessConfigData(const CConfigData *config_data);

	void SetTriggerData() const;
	void CleanTriggerData() const;
	int GetLevelID() const;
	int GetKey() const;
	std::string GetHint() const;

	int Pos;					/// button position in the grid
	CButtonLevel *Level;		/// requires button level
	bool AlwaysShow;			/// button is always shown but drawn grayscale if not available
	ButtonCmd Action;			/// command on button press
	int Value;					/// extra value for command
	void *Payload;
	std::string ValueStr;		/// keep original value string

	ButtonCheckFunc Allowed;    /// Check if this button is allowed
	std::string AllowStr;       /// argument for allowed
	std::string UnitMask;       /// for which units is it available
	IconConfig Icon;      		/// icon to display
	int Key;                    /// alternative on keyboard
	std::string Hint;           /// tip texts
	std::string Description;    /// description shown on status bar (optional)
	SoundConfig CommentSound;   /// Sound comment used when you press the button
	std::string ButtonCursor;   /// Custom cursor for button action (for example, to set spell target)
	std::string Popup;          /// Popup screen used for button
	std::string Mod;			/// Mod to which this button belongs to
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

extern std::vector<ButtonAction *> UnitButtonTable;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

//
// in botpanel.cpp
//
/// Generate all buttons
extern void InitButtons();
/// Free memory for buttons
extern void CleanButtons();
/// Make a new button
extern int AddButton(int pos, CButtonLevel *level, const std::string &IconIdent,
					 ButtonCmd action, const std::string &value, void* payload, const ButtonCheckFunc func,
					 const std::string &arg, const int key, const std::string &hint, const std::string &descr,
					 const std::string &sound, const std::string &cursor, const std::string &umask,
					 //Wyrmgus start
//					 const std::string &popup, bool alwaysShow);
					 const std::string &popup, bool alwaysShow, const std::string &mod_file);
					 //Wyrmgus end
// Check if the button is allowed for the unit.
extern bool IsButtonAllowed(const CUnit &unit, const ButtonAction &buttonaction);

// Check if the button is usable for the unit.
extern bool IsButtonUsable(const CUnit &unit, const ButtonAction &buttonaction);

// Get the cooldown for the button for the unit.
extern int GetButtonCooldown(const CUnit &unit, const ButtonAction &buttonaction);

// Get the cooldown percent for the button for the unit.
extern int GetButtonCooldownPercent(const CUnit &unit, const ButtonAction &buttonaction);

extern std::string GetButtonActionNameById(const int button_action);
extern int GetButtonActionIdByName(const std::string &button_action);
extern bool IsNeutralUsableButtonAction(const int button_action);

//@}

#endif // !__BUTTON_ACTION_H__
