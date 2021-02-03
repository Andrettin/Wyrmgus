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
//      (c) Copyright 2021 by Andrettin
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

namespace wyrmgus {
	template <int N>
	class fractional_int;

	using centesimal_int = fractional_int<2>;
	using decimillesimal_int = fractional_int<4>;
}

namespace wyrmgus::astronomy {

constexpr int ly_per_100_pc = 326; //light-years per parsecs; 1 parsec = 3.26 light years
constexpr int gm_per_au = 150; //gigameters per astronomical units

//light-years to parsecs
extern centesimal_int ly_to_pc(const centesimal_int &light_years);

//parsecs to light-years
extern centesimal_int pc_to_ly(const centesimal_int &parsecs);

//gigameters to astronomical units
extern centesimal_int gm_to_au(const int gm);

//astronomical units to gigameters
extern int au_to_gm(const centesimal_int &au);

//right ascension to longitude
extern decimillesimal_int ra_to_lon(const decimillesimal_int &ra);

//longitude to right ascension
extern decimillesimal_int lon_to_ra(const decimillesimal_int &lon);

}
