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
//      (c) Copyright 1998-2021 by Lutz Sammer, Fabrice Rossi, Jimmy Salmon
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

namespace wyrmgus {

enum class unit_sound_type {
	none = -1,
	selected,		/// if selected
	acknowledging,	/// acknowledge command
	ready,			/// command completed
	help,			/// if attacked
	dying,			/// if killed
	work_completed,	/// only worker, work completed
	construction,	/// only for building under construction
	docking,		/// only for transport reaching coast
	repairing,		/// repairing
	harvesting,		/// harvesting
	attack,			/// attack command
	idle,			/// idle sound
	hit,			/// hit another unit
	miss,			/// attacked another unit, but missed
	fire_missile,	/// fired a missile at another unit
	step,			/// stepped
	used,			/// used (for items)
	build			/// worker goes to build a building
};

inline unit_sound_type string_to_unit_sound_type(const std::string &str)
{
	if (str == "selected") {
		return unit_sound_type::selected;
	} else if (str == "acknowledge") {
		return unit_sound_type::acknowledging;
	} else if (str == "ready") {
		return unit_sound_type::ready;
	} else if (str == "help") {
		return unit_sound_type::help;
	} else if (str == "dead") {
		return unit_sound_type::dying;
	} else if (str == "work_completed") {
		return unit_sound_type::work_completed;
	} else if (str == "construction") {
		return unit_sound_type::construction;
	} else if (str == "docking") {
		return unit_sound_type::docking;
	} else if (str == "repairing") {
		return unit_sound_type::repairing;
	} else if (str == "harvesting") {
		return unit_sound_type::harvesting;
	} else if (str == "attack") {
		return unit_sound_type::attack;
	} else if (str == "idle") {
		return unit_sound_type::idle;
	} else if (str == "hit") {
		return unit_sound_type::hit;
	} else if (str == "miss") {
		return unit_sound_type::miss;
	} else if (str == "fire_missile") {
		return unit_sound_type::fire_missile;
	} else if (str == "step") {
		return unit_sound_type::step;
	} else if (str == "used") {
		return unit_sound_type::used;
	} else if (str == "build") {
		return unit_sound_type::build;
	}

	throw std::runtime_error("Invalid unit sound type: \"" + str + "\".");
}

inline bool is_voice_unit_sound_type(const unit_sound_type unit_sound_type)
{
	switch (unit_sound_type) {
		case unit_sound_type::none:
		case unit_sound_type::hit:
		case unit_sound_type::miss:
		case unit_sound_type::fire_missile:
		case unit_sound_type::step:
		case unit_sound_type::dying:
		case unit_sound_type::used:
			return false;
		default:
			return true;
	}
}

extern int get_unit_sound_type_range(const unit_sound_type unit_sound_type);

}

Q_DECLARE_METATYPE(wyrmgus::unit_sound_type)
