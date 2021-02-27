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

#pragma once

namespace wyrmgus::qgeocoordinate {

constexpr int longitude_size = 360;
constexpr int latitude_size = 180;

inline double longitude_to_unsigned_longitude(const double longitude)
{
	return longitude + (qgeocoordinate::longitude_size / 2);
}

inline double latitude_to_unsigned_latitude(const double latitude)
{
	return latitude * -1 + (qgeocoordinate::latitude_size / 2);
}

inline QPointF to_unsigned_geocoordinate(const QGeoCoordinate &geocoordinate)
{
	//converts a geocoordinate into a (floating) coordinate point, having x values from 0 to 360, and y values from 0 to 180 (top to bottom, contrary to geocoordinates, which work south to north)
	const double x = qgeocoordinate::longitude_to_unsigned_longitude(geocoordinate.longitude());
	const double y = qgeocoordinate::latitude_to_unsigned_latitude(geocoordinate.latitude());
	return QPointF(x, y);
}

inline QGeoCoordinate from_unsigned_geocoordinate(const QPointF &unsigned_geocoordinate)
{
	const double lon = unsigned_geocoordinate.x() - (qgeocoordinate::longitude_size / 2);
	const double lat = (unsigned_geocoordinate.y() - (qgeocoordinate::latitude_size / 2)) * -1;
	return QGeoCoordinate(lat, lon);
}

}
