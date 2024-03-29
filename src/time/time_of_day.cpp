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

#include "stratagus.h"

#include "time/time_of_day.h"

namespace wyrmgus {

void time_of_day::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "color_modification") {
		this->ColorModification.R = std::stoi(values[0]);
		this->ColorModification.G = std::stoi(values[1]);
		this->ColorModification.B = std::stoi(values[2]);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

/**
**	@brief	Gets whether the time of day modifies the color of graphics
**
**	@return	Whether the time of day modifies the color of graphics
*/
bool time_of_day::HasColorModification() const
{
	return this->ColorModification.R != 0 || this->ColorModification.G != 0 || this->ColorModification.B != 0;
}

}
