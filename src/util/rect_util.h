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

#include "util/point_container.h"

namespace wyrmgus::rect {

template <typename function_type>
inline void for_each_point(const QRect &rect, const function_type &function)
{
	const QPoint min_pos = rect.topLeft();
	const QPoint max_pos = rect.bottomRight();

	for (int x = min_pos.x(); x <= max_pos.x(); ++x) {
		for (int y = min_pos.y(); y <= max_pos.y(); ++y) {
			function(QPoint(x, y));
		}
	}
}

template <typename function_type>
inline std::vector<QPoint> find_points_if(const QRect &rect, const function_type &function)
{
	std::vector<QPoint> points;

	rect::for_each_point(rect, [&](QPoint &&point) {
		if (function(point)) {
			points.push_back(std::move(point));
		}
	});

	return points;
}

template <typename function_type>
inline point_set find_point_set_if(const QRect &rect, const function_type &function)
{
	point_set points;

	rect::for_each_point(rect, [&](QPoint &&point) {
		if (function(point)) {
			points.insert(std::move(point));
		}
	});

	return points;
}

}
