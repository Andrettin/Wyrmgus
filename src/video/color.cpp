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
/**@name color.h - The A platform independent color. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#include "stratagus.h"

#include "video/color.h"

#include "config.h"
#include "config_operator.h"
#include "script.h"

#pragma warning(push, 0)
#include <core/print_string.h>
#pragma warning(pop)

#include <SDL.h>

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

CColor::operator SDL_Color() const
{
	SDL_Color c = { static_cast<unsigned char>(R), static_cast<unsigned char>(G), static_cast<unsigned char>(B), static_cast<unsigned char>(A) };
	return c;
}

CColor CColor::FromString(const std::string &str)
{
	CColor color;
	std::vector<std::string> color_vector = SplitString(str, ".");
	
	if (color_vector.size() >= 1) {
		color.R = std::stoi(color_vector[0]);
	}
	if (color_vector.size() >= 2) {
		color.G = std::stoi(color_vector[1]);
	}
	if (color_vector.size() >= 3) {
		color.B = std::stoi(color_vector[2]);
	}
	if (color_vector.size() >= 4) {
		color.A = std::stoi(color_vector[3]);
	}
	
	return color;
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CColor::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
			continue;
		}
		
		if (property.Key == "red") {
			this->R = property.Value.to_int();
		} else if (property.Key == "green") {
			this->G = property.Value.to_int();
		} else if (property.Key == "blue") {
			this->B = property.Value.to_int();
		} else if (property.Key == "alpha") {
			this->A = property.Value.to_int();
		} else {
			fprintf(stderr, "Invalid color property: \"%s\".\n", property.Key.utf8().get_data());
		}
	}
}

void CColor::Parse(lua_State *l, const int offset)
{
	if (!lua_istable(l, offset) || lua_rawlen(l, offset) != 3) {
		LuaError(l, "incorrect argument");
	}
	const int r = LuaToNumber(l, offset, 1);
	const int g = LuaToNumber(l, offset, 2);
	const int b = LuaToNumber(l, offset, 3);

	if (!(0 <= r && r <= 255
		  && 0 <= g && g <= 255
		  && 0 <= b && b <= 255)) {
		LuaError(l, "Arguments must be in the range 0-255");
	}
	this->R = r;
	this->G = g;
	this->B = b;
	this->A = 0;
}
