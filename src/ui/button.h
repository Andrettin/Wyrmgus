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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "database/data_entry.h"
#include "database/data_type.h"
#include "sound/unitsound.h"
#include "ui/icon.h"

class CUnit;
enum class ButtonCmd;
struct lua_State;

static int CclDefineButton(lua_State *l);

namespace wyrmgus {

class button;
class button_level;
class unit_class;

typedef bool (*button_check_func)(const CUnit &, const button &);

class button : public data_entry, public data_type<button>
{
	Q_OBJECT

	Q_PROPERTY(int pos MEMBER pos READ get_pos)
	Q_PROPERTY(wyrmgus::button_level* level MEMBER level READ get_level)
	Q_PROPERTY(bool always_show MEMBER always_show READ is_always_shown)

public:
	static constexpr const char *class_identifier = "button";
	static constexpr const char *database_folder = "buttons";

	static void ProcessConfigData(const CConfigData *config_data);

	static void add_button_key_to_name(std::string &value_name, const std::string &button_key);

	button(const std::string &identifier = "");

	button &operator =(const button &other_button)
	{
		this->pos = other_button.pos;
		this->level = other_button.level;
		this->always_show = other_button.always_show;
		this->Action = other_button.Action;
		this->Value = other_button.Value;
		this->Payload = other_button.Payload;
		this->ValueStr = other_button.ValueStr;
		this->Allowed = other_button.Allowed;
		this->allow_strings = other_button.allow_strings;
		this->UnitMask = other_button.UnitMask;
		this->unit_classes = other_button.unit_classes;
		this->Icon = other_button.Icon;
		this->Key = other_button.Key;
		this->Hint = other_button.Hint;
		this->Description = other_button.Description;
		this->CommentSound = other_button.CommentSound;
		this->Popup = other_button.Popup;
		this->Mod = other_button.Mod;

		return *this;
	}

	virtual void process_sml_property(const sml_property &property) override;
	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;

	int get_pos() const
	{
		return this->pos;
	}

	button_level *get_level() const
	{
		return this->level;
	}

	bool is_always_shown() const
	{
		return this->always_show;
	}

	const CUnit *get_unit() const;
	const unit_type *get_unit_type(const CUnit *unit) const;

	const unit_type *get_unit_type() const
	{
		return this->get_unit_type(this->get_unit());
	}

	const unit_type *get_value_unit_type(const CUnit *unit) const;
	const CUpgrade *get_value_upgrade(const CUnit *unit) const;

	const CUpgrade *get_value_upgrade() const
	{
		return this->get_value_upgrade(this->get_unit());
	}

	void SetTriggerData() const;
	void CleanTriggerData() const;
	int GetLevelID() const;
	int get_key() const;
	std::string get_hint() const;

	const std::vector<unit_class *> &get_unit_classes() const
	{
		return this->unit_classes;
	}

	int pos = 0; //button position in the grid
	button_level *level = nullptr;		/// requires button level
private:
	bool always_show = false;			/// button is always shown but drawn grayscale if not available
public:
	ButtonCmd Action;	/// command on button press
	int Value = 0;					/// extra value for command
	void *Payload = nullptr;
	std::string ValueStr;		/// keep original value string

	button_check_func Allowed = nullptr;    /// Check if this button is allowed
	std::vector<std::string> allow_strings; //arguments for allowed
	std::string UnitMask;       //for which units is it available
private:
	std::vector<unit_class *> unit_classes; //unit classes for which the button is available
public:
	IconConfig Icon;      		/// icon to display
	int Key = 0;                    /// alternative on keyboard
	std::string Hint;           /// tip texts
	std::string Description;    /// description shown on status bar (optional)
	SoundConfig CommentSound;   /// Sound comment used when you press the button
	std::string Popup;          /// Popup screen used for button
	std::string Mod;			/// Mod to which this button belongs to

	friend int ::CclDefineButton(lua_State *l);
};

}

//
// in botpanel.cpp
//
/// Generate all buttons
extern void InitButtons();
/// Free memory for buttons
extern void CleanButtons();
// Check if the button is allowed for the unit.
extern bool IsButtonAllowed(const CUnit &unit, const wyrmgus::button &buttonaction);

// Check if the button is usable for the unit.
extern bool IsButtonUsable(const CUnit &unit, const wyrmgus::button &buttonaction);

// Get the cooldown for the button for the unit.
extern int GetButtonCooldown(const CUnit &unit, const wyrmgus::button &buttonaction);

// Get the cooldown percent for the button for the unit.
extern int GetButtonCooldownPercent(const CUnit &unit, const wyrmgus::button &buttonaction);

extern std::string GetButtonActionNameById(const ButtonCmd button_action);
extern ButtonCmd GetButtonActionIdByName(const std::string &button_action);
extern bool IsNeutralUsableButtonAction(const ButtonCmd button_action);
