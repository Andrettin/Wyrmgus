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
/**@name color.h - The platform independent color header file. */
//
//      (c) Copyright 2012-2021 by Joris Dauphin and Andrettin
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

class CConfigData;
struct lua_State;

/// platform independent color
class CColor
{
public:
	CColor() : R(0), G(0), B(0), A(0) {}
	CColor(unsigned char r, unsigned char g, unsigned char b,
		   unsigned char a = 0) : R(r), G(g), B(b), A(a) {}
	CColor(const CColor &color) : R(color.R), G(color.G), B(color.B), A(color.A) {}
	
	static CColor FromString(const std::string &str);

	void ProcessConfigData(const CConfigData *config_data);
	void Parse(lua_State *l, int index = -1);

	bool operator <(const CColor &rhs) const {
		if (this->R < rhs.R) {
			return true;
        } else if (this->R == rhs.R) {
			if (this->G < rhs.G) {
				return true;
			} else if (this->G == rhs.G) {
				if (this->B < rhs.B) {
					return true;
				} else if (this->B == rhs.B) {
					return this->A < rhs.A;
				}
			}
		}

		return false;
	}
	
	bool operator <=(const CColor &rhs) const {
		if (this->R < rhs.R) {
			return true;
        } else if (this->R == rhs.R) {
			if (this->G < rhs.G) {
				return true;
			} else if (this->G == rhs.G) {
				if (this->B < rhs.B) {
					return true;
				} else if (this->B == rhs.B) {
					return this->A <= rhs.A;
				}
			}
		}

		return false;
	}
	
	bool operator >(const CColor &rhs) const {
		if (this->R > rhs.R) {
			return true;
        } else if (this->R == rhs.R) {
			if (this->G > rhs.G) {
				return true;
			} else if (this->G == rhs.G) {
				if (this->B > rhs.B) {
					return true;
				} else if (this->B == rhs.B) {
					return this->A > rhs.A;
				}
			}
		}

		return false;
	}
	
	bool operator >=(const CColor &rhs) const {
		if (this->R > rhs.R) {
			return true;
        } else if (this->R == rhs.R) {
			if (this->G > rhs.G) {
				return true;
			} else if (this->G == rhs.G) {
				if (this->B > rhs.B) {
					return true;
				} else if (this->B == rhs.B) {
					return this->A >= rhs.A;
				}
			}
		}

		return false;
	}
	
	bool operator ==(const CColor &rhs) const {
		return this->R == rhs.R && this->G == rhs.G && this->B == rhs.B && this->A == rhs.A;
	}
	
public:
	//these variables are short integers instead of unsigned chars so that they can be negative, for the case they need to represent a color modification (which can have negative values)
	short R;			/// Red
	short G;			/// Green
	short B;			/// Blue
	short A;			/// Alpha
};

typedef uint32_t IntColor; // Uint32 in SDL
