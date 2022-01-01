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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

/**
**  User defined variable type.
**
**  It is used to define variables and use it after
**  to manage magic, energy, shield or other stuff.
*/
class unit_variable
{
public:
	bool operator ==(const unit_variable &rhs) const
	{
		return this->Max == rhs.Max
			   && this->Value == rhs.Value
			   && this->Increase == rhs.Increase
			   && this->Enable == rhs.Enable;
	}
	bool operator !=(const unit_variable &rhs) const { return !(*this == rhs); }

public:
	int Max = 0;        /// Maximum for the variable. (Assume min is 0.)
	int Value = 0;      /// Current (or initial) value of the variable (or initial value).
	char Increase = 0;  /// Number to increase(decrease) Value by second.
	char Enable = 0;    /// True if the unit doesn't have this variable. (f.e shield)
};

}