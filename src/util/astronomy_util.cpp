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

#include "util/astronomy_util.h"

#include "util/fractional_int.h"

namespace wyrmgus::astronomy {

centesimal_int ly_to_pc(const centesimal_int &light_years)
{
	centesimal_int parsecs = light_years;
	parsecs *= 100;
	parsecs /= astronomy::ly_per_100_pc;
	return parsecs;
}

centesimal_int pc_to_ly(const centesimal_int &parsecs)
{
	centesimal_int light_years = parsecs;
	light_years *= astronomy::ly_per_100_pc;
	light_years /= 100;
	return light_years;
}

centesimal_int gm_to_au(const int gm)
{
	return centesimal_int(gm) / astronomy::gm_per_au;
}

int au_to_gm(const centesimal_int &au)
{
	return (au * astronomy::gm_per_au).to_int();
}

decimillesimal_int ra_to_lon(const decimillesimal_int &ra)
{
	decimillesimal_int res(ra);

	if (ra > 12) {
		res -= 24;
	}

	return res * 15;
}

decimillesimal_int lon_to_ra(const decimillesimal_int &lon)
{
	decimillesimal_int res(lon);

	res /= 15;

	if (lon < 0) {
		res += 24;
	}

	return res;
}

}
