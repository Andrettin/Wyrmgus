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
//      (c) Copyright 1998-2022 by Lutz Sammer, Fabrice Rossi, Jimmy Salmon
//      and Andrettin
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

#include "util/singleton.h"

namespace wyrmgus {

class gsml_data;
class gsml_property;
class sound;

/**
**  Global game sounds, not associated with any unit-type
*/
class game_sound_set final : public QObject, public singleton<game_sound_set>
{
	Q_OBJECT

	Q_PROPERTY(wyrmgus::sound* placement_error MEMBER placement_error)
	Q_PROPERTY(wyrmgus::sound* placement_success MEMBER placement_success)
	Q_PROPERTY(wyrmgus::sound* click MEMBER click)
	Q_PROPERTY(wyrmgus::sound* docking MEMBER docking)
	Q_PROPERTY(wyrmgus::sound* building_construction MEMBER building_construction)
	Q_PROPERTY(wyrmgus::sound* rescue MEMBER rescue)
	Q_PROPERTY(wyrmgus::sound* chat_message MEMBER chat_message)

public:
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const sound *get_placement_error_sound() const
	{
		return this->placement_error;
	}

	const sound *get_placement_success_sound() const
	{
		return this->placement_success;
	}

	const sound *get_click_sound() const
	{
		return this->click;
	}

	const sound *get_docking_sound() const
	{
		return this->docking;
	}

	const sound *get_building_construction_sound() const
	{
		return this->building_construction;
	}

	const sound *get_rescue_sound() const
	{
		return this->rescue;
	}

	const sound *get_chat_message_sound() const
	{
		return this->chat_message;
	}

private:
	sound *placement_error = nullptr;		/// used by ui
	sound *placement_success = nullptr;		/// used by ui
	sound *click = nullptr;					/// used by ui
	sound *docking = nullptr;				/// ship reaches coast
	sound *building_construction = nullptr;	/// building under construction
	sound *rescue = nullptr;				/// rescue units
	sound *chat_message = nullptr;					/// chat message
};

}
