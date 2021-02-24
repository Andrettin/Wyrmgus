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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "util/georectangle_util.h"

#include "util/geocoordinate.h"
#include "util/geocoordinate_util.h"

namespace wyrmgus::georectangle {

QRect to_unsigned_georectangle(const QRect &georectangle)
{
	return QRect(geocoordinate(georectangle.topLeft()).to_unsigned_geocoordinate().to_point(), geocoordinate(georectangle.bottomRight()).to_unsigned_geocoordinate().to_point());
}

QRectF to_unsigned_georectangle(const QGeoRectangle &georectangle)
{
	return QRectF(qgeocoordinate::to_unsigned_geocoordinate(georectangle.topLeft()), qgeocoordinate::to_unsigned_geocoordinate(georectangle.bottomRight()));
}

QRectF to_scaled_georectangle(const QGeoRectangle &georectangle, const std::vector<std::unique_ptr<degree_scaling>> &longitude_scalings, const std::vector<std::unique_ptr<degree_scaling>> &latitude_scalings)
{
	return QRectF(qgeocoordinate::to_scaled_geocoordinate(georectangle.topLeft(), longitude_scalings, latitude_scalings), qgeocoordinate::to_scaled_geocoordinate(georectangle.bottomRight(), longitude_scalings, latitude_scalings));
}

}
