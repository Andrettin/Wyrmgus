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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "sound/unit_sound.h"
#include "ui/icon_config.h"

#include <core/object.h>
#include <core/method_bind_free_func.gen.inc>

#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CButtonLevel;
class CUnit;
class UnitClass;
struct lua_State;
struct Vector2i;

/*----------------------------------------------------------------------------
--  Definitons
----------------------------------------------------------------------------*/

/// Button Commands that need target selection
enum ButtonCmd {
	ButtonMove,				/// order move
	ButtonAttack,			/// order attack
	ButtonRepair,			/// order repair
	ButtonHarvest,			/// order harvest
	ButtonBuild,			/// order build
	ButtonPatrol,			/// order patrol
	ButtonAttackGround,		/// order attack ground
	ButtonSpellCast,		/// order cast spell
	ButtonUnload,			/// order unload unit
	ButtonStop,				/// order stop
	ButtonButton,			/// choose other button set
	ButtonTrain,			/// order train
	ButtonStandGround,		/// order stand ground
	ButtonReturn,			/// order return goods
	ButtonResearch,			/// order reseach
	ButtonLearnAbility,		/// order learn ability
	ButtonExperienceUpgradeTo,	/// order upgrade (experience)
	ButtonUpgradeTo,		/// order upgrade
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
	ButtonCancel,			/// cancel
	ButtonCancelUpgrade,	/// cancel upgrade
	ButtonCancelTrain,		/// cancel training
	ButtonCancelBuild		/// cancel building
};

class ButtonAction;
typedef bool (*ButtonCheckFunc)(const CUnit &, const ButtonAction &);

/// Action of button
class ButtonAction : public Object
{
	GDCLASS(ButtonAction, Object)
	
public:
	static void ProcessConfigData(const CConfigData *config_data);
	
private:
	void Initialize();

public:	   
	void Copy(const ButtonAction &button_action)
	{
		this->Pos = button_action.Pos;
		this->Level = button_action.Level;
		this->AlwaysShow = button_action.AlwaysShow;
		this->Action = button_action.Action;
		this->Value = button_action.Value;
		this->ValueStr = button_action.ValueStr;
		this->Allowed = button_action.Allowed;
		this->AllowStr = button_action.AllowStr;
		this->UnitMask = button_action.UnitMask;
		this->ForUnitClasses = button_action.ForUnitClasses;
		this->Icon = button_action.Icon;
		this->Key = button_action.Key;
		this->Hint = button_action.Hint;
		this->Description = button_action.Description;
		this->CommentSound = button_action.CommentSound;
		this->ButtonCursor = button_action.ButtonCursor;
		this->Popup = button_action.Popup;
		this->Mod = button_action.Mod;
	}
	
	int GetPos() const
	{
		return this->Pos;
	}
	
	ButtonCmd GetAction() const
	{
		return this->Action;
	}

	void SetTriggerData() const;
	void CleanTriggerData() const;
	
	const CButtonLevel *GetLevel() const
	{
		return this->Level;
	}
	
	int GetLevelIndex() const;
	
	int GetKey() const;
	std::string GetHint() const;
	
	bool IsAvailableForUnitClass(const UnitClass *unit_class) const
	{
		return this->ForUnitClasses.find(unit_class) != this->ForUnitClasses.end();
	}
	
	const std::set<const UnitClass *> &GetForUnitClasses() const
	{
		return this->ForUnitClasses;
	}

private:
	int Pos = 0;					/// button position in the grid
	const CButtonLevel *Level = nullptr;	/// requires button level
public:
	bool AlwaysShow = false;		/// button is always shown but drawn grayscale if not available
private:
	ButtonCmd Action = ButtonMove;	/// command on button press
public:
	int Value = 0;					/// extra value for command
	std::string ValueStr;			/// keep original value string

	ButtonCheckFunc Allowed = nullptr;	/// Check if this button is allowed
	std::string AllowStr;		/// argument for allowed
	std::string UnitMask;		/// for which units this button is available
private:
	std::set<const UnitClass *> ForUnitClasses;	/// for which unit classes this button is available
public:
	IconConfig Icon;			/// icon to display
	int Key = 0;				/// alternative on keyboard
	std::string Hint;			/// tip texts
	std::string Description;	/// description shown on status bar (optional)
	SoundConfig CommentSound;	/// Sound comment used when you press the button
	std::string ButtonCursor;	/// Custom cursor for button action (for example, to set spell target)
	std::string Popup;			/// Popup screen used for button
	std::string Mod;			/// Mod to which this button belongs to
	
	friend class CButtonPanel;
	friend bool EditorCallbackMouse_EditUnitArea(const Vector2i &screenPos);
	friend int CclDefineButton(lua_State *l);
	friend void DrawPopups();
	friend void UpdateButtonPanelMultipleUnits(std::vector<ButtonAction> *buttonActions);
	friend void UpdateButtonPanelSingleUnit(const CUnit &unit, std::vector<ButtonAction> *buttonActions);
	
protected:
	static void _bind_methods();
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

#endif
