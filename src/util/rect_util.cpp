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

#include "util/rect_util.h"

#include "util/point_util.h"
#include "util/util.h"

namespace wyrmgus::rect {

int distance_to(const QRect &rect, const QRect &other_rect)
{
	const QPoint &top_left_pos = rect.topLeft();
	const QPoint &other_top_left_pos = other_rect.topLeft();

	int dx;
	int dy;

	if (top_left_pos.x() + rect.width() <= other_top_left_pos.x()) {
		dx = std::max<int>(0, other_top_left_pos.x() - top_left_pos.x() - rect.width() + 1);
	} else {
		dx = std::max<int>(0, top_left_pos.x() - other_top_left_pos.x() - other_rect.width() + 1);
	}

	if (top_left_pos.y() + rect.height() <= other_top_left_pos.y()) {
		dy = other_top_left_pos.y() - top_left_pos.y() - rect.height() + 1;
	} else {
		dy = std::max<int>(0, top_left_pos.y() - other_top_left_pos.y() - other_rect.height() + 1);
	}

	return isqrt(dy * dy + dx * dx);
}

}
