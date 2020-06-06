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
//      (c) Copyright 2020 by Andrettin
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

namespace stratagus::point {

inline constexpr int to_index(const int x, const int y, const int width)
{
	return x + y * width;
}

inline constexpr int to_index(const int x, const int y, const QSize &size)
{
	return point::to_index(x, y, size.width());
}

inline constexpr int to_index(const QPoint &point, const int width)
{
	return point::to_index(point.x(), point.y(), width);
}

inline constexpr int to_index(const QPoint &point, const QSize &size)
{
	return point::to_index(point.x(), point.y(), size);
}

inline constexpr QPoint from_index(const int index, const int width)
{
	return QPoint(index % width, index / width);
}

inline constexpr QPoint from_index(const int index, const QSize &size)
{
	return point::from_index(index, size.width());
}

inline constexpr QSize to_size(const QPoint &point)
{
	return QSize(point.x(), point.y());
}

inline QPointF to_unsigned_geocoordinate(const QPoint &point, const QSize &area_size, const QRectF &unsigned_georectangle)
{
	const double lon = point.x() * unsigned_georectangle.width() / area_size.width() + unsigned_georectangle.x();
	const double lat = point.y() * unsigned_georectangle.height() / area_size.height() + unsigned_georectangle.y();
	return QPointF(lon, lat);
}

extern QGeoCoordinate to_geocoordinate(const QPoint &point, const QSize &area_size, const QRectF &unsigned_georectangle);

extern constexpr int distance_to(const QPoint &point, const QPoint &other_point);

inline bool is_horizontally_adjacent_to(const QPoint &point, const QPoint &other_point)
{
	if (point.y() != other_point.y()) {
		return false;
	}

	const int diff = std::abs(point.x() - other_point.x());
	return diff == 1;
}

inline bool is_vertically_adjacent_to(const QPoint &point, const QPoint &other_point)
{
	if (point.x() != other_point.x()) {
		return false;
	}

	const int diff = std::abs(point.y() - other_point.y());
	return diff == 1;
}

inline bool is_cardinally_adjacent_to(const QPoint &point, const QPoint &other_point)
{
	//returns whether the point is adjacent to the other one in any of the four cardinal directions
	return point::is_horizontally_adjacent_to(point, other_point) || point::is_vertically_adjacent_to(point, other_point);
}

}
