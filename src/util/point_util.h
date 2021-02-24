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

namespace wyrmgus {
	class geocoordinate;
}

namespace wyrmgus::point {

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

extern geocoordinate to_unsigned_geocoordinate(const QPoint &point, const QSize &area_size, const QRect &unsigned_georectangle);

inline QPointF to_unsigned_geocoordinate(const QPoint &point, const QSize &area_size, const QRectF &unsigned_georectangle)
{
	const double lon = point.x() * unsigned_georectangle.width() / area_size.width() + unsigned_georectangle.x();
	const double lat = point.y() * unsigned_georectangle.height() / area_size.height() + unsigned_georectangle.y();
	return QPointF(lon, lat);
}

extern geocoordinate to_geocoordinate(const QPoint &point, const QSize &area_size, const QRect &unsigned_georectangle);

extern QGeoCoordinate to_qgeocoordinate(const QPoint &point, const QSize &area_size, const QRectF &unsigned_georectangle);

inline int square_distance_to(const QPoint &point, const QPoint &other_point)
{
	const int dx = point.x() - other_point.x();
	const int dy = point.y() - other_point.y();
	return dx * dx + dy * dy;
}

extern int distance_to(const QPoint &point, const QPoint &other_point);

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

inline QPoint get_circle_point(const QPoint &point, const int64_t source_radius, const int64_t target_radius)
{
	const int64_t x = point.x() * target_radius / source_radius;
	const int64_t y = point.y() * target_radius / source_radius;
	return QPoint(static_cast<int>(x), static_cast<int>(y));
}

inline QPoint get_nearest_circle_edge_point(const QPoint &point, const int64_t radius)
{
	return point::get_circle_point(point, point::distance_to(point, QPoint(0, 0)), radius);
}

template <typename function_type>
inline void for_each_adjacent(const QPoint &point, const function_type &function)
{
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset == 0 && y_offset == 0) {
				continue;
			}

			function(QPoint(point.x() + x_offset, point.y() + y_offset));
		}
	}
}

template <typename function_type>
inline void for_each_adjacent_until(const QPoint &point, const function_type &function)
{
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset == 0 && y_offset == 0) {
				continue;
			}

			const QPoint adjacent_point(point.x() + x_offset, point.y() + y_offset);
			if (function(adjacent_point)) {
				return;
			}
		}
	}
}

template <typename function_type>
inline std::optional<QPoint> find_adjacent_if(const QPoint &point, const function_type &function)
{
	std::optional<QPoint> result;

	wyrmgus::point::for_each_adjacent_until(point, [&](const QPoint &adjacent_point) {
		if (function(adjacent_point)) {
			result = adjacent_point;
			return true;
		}

		return false;
	});

	return result;
}

template <typename function_type>
inline std::vector<QPoint> get_adjacent_if(const QPoint &point, const function_type &function)
{
	std::vector<QPoint> adjacent_points;

	wyrmgus::point::for_each_adjacent(point, [&](QPoint &&adjacent_point) {
		if (function(adjacent_point)) {
			adjacent_points.push_back(std::move(adjacent_point));
		}
	});

	return adjacent_points;
}

template <typename function_type>
inline std::vector<QPoint> get_diagonally_adjacent_if(const QPoint &point, const function_type &function)
{
	std::vector<QPoint> adjacent_points;

	for (int x_offset = -1; x_offset <= 1; x_offset += 2) { // +2 so that only diagonals are used
		for (int y_offset = -1; y_offset <= 1; y_offset += 2) {
			QPoint adjacent_point = point + QPoint(x_offset, y_offset);
			const QPoint &adjacent_point_ref = adjacent_point;
			if (function(adjacent_point_ref)) {
				adjacent_points.push_back(std::move(adjacent_point));
			}
		}
	}

	return adjacent_points;
}

inline std::string to_string(const QPoint &point)
{
	return "(" + std::to_string(point.x()) + ", " + std::to_string(point.y()) + ")";
}

}
