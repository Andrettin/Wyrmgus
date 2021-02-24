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

#include "util/point_util.h"

#include "util/geocoordinate.h"
#include "util/geocoordinate_util.h"
#include "util/number_util.h"
#include "util/util.h"

namespace wyrmgus::point {

geocoordinate to_unsigned_geocoordinate(const QPoint &point, const QSize &area_size, const QRect &unsigned_georectangle)
{
	const longitude lon = longitude(point.x()) * unsigned_georectangle.width() / area_size.width() + longitude(unsigned_georectangle.x());
	const latitude lat = latitude(point.y()) * unsigned_georectangle.height() / area_size.height() + latitude(unsigned_georectangle.y());
	return geocoordinate(lon, lat);
}

geocoordinate to_geocoordinate(const QPoint &point, const QSize &area_size, const QRect &unsigned_georectangle)
{
	const geocoordinate unsigned_geocoordinate = point::to_unsigned_geocoordinate(point, area_size, unsigned_georectangle);
	return geocoordinate::from_unsigned_geocoordinate(unsigned_geocoordinate);
}

QGeoCoordinate to_qgeocoordinate(const QPoint &point, const QSize &area_size, const QRectF &unsigned_georectangle)
{
	const QPointF unsigned_geocoordinate = point::to_unsigned_geocoordinate(point, area_size, unsigned_georectangle);
	return qgeocoordinate::from_unsigned_geocoordinate(unsigned_geocoordinate);
}

int distance_to(const QPoint &point, const QPoint &other_point)
{
	return isqrt(point::square_distance_to(point, other_point));
}

}
