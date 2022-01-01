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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "util/angle_util.h"

#include "util/fractional_int.h"
#include "util/number_util.h"

#include <boost/math/constants/constants.hpp>

namespace wyrmgus::angle {

const number_type pi("3.1416");
const number_type half_pi = pi / 2;

number_type degrees_to_radians(const number_type &degrees)
{
	return degrees * angle::pi / 180;
}

double degrees_to_radians(const double degrees)
{
	return degrees * boost::math::constants::pi<double>() / 180.;
}

number_type radians_to_degrees(const number_type &radians)
{
	return radians * 180 / angle::pi;
}

double radians_to_degrees(const double radians)
{
	return radians * 180. / boost::math::constants::pi<double>();
}

//n is in the -pi to +pi range
number_type gudermannian(const number_type &n)
{
	const double d = n.to_double();
	return angle::radians_to_degrees(number_type(atan(sinh(d))));
}

number_type gudermannian_inverse(const number_type &degrees)
{
	const number_type sign = number::sign(degrees);
	const double sin = std::sin((angle::degrees_to_radians(degrees) * sign).to_double());
	return sign * (number_type(log((1.0 + sin) / (1.0 - sin))) / 2);
}

}
