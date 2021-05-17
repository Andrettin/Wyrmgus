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

#include "util/geopath_util.h"

#include "map/map_projection.h"
#include "util/geocoordinate.h"
#include "util/geocoordinate_util.h"
#include "util/georectangle_util.h"
#include "util/point_util.h"

namespace wyrmgus::geopath {

void write_to_image(const QGeoPath &geopath, QImage &image, const QColor &color, const QRect &georectangle, const map_projection *map_projection)
{
	const QGeoRectangle qgeorectangle = georectangle::to_qgeorectangle(georectangle);

	QPoint previous_pixel_pos(-1, -1);
	for (const QGeoCoordinate &geocoordinate : geopath.path()) {
		const QPoint pixel_pos = map_projection->geocoordinate_to_point(wyrmgus::geocoordinate(geocoordinate), georectangle, image.size());

		if (pixel_pos.x() >= 0 && pixel_pos.y() >= 0 && pixel_pos.x() < image.width() && pixel_pos.y() < image.height()) {
			//only write to the image if the position is actually in it, but take the position into account either way for the purpose of getting the previous pixel pos, so that map templates fit together well
			geopath::write_pixel_to_image(pixel_pos, color, image);
		}

		if (previous_pixel_pos != QPoint(-1, -1) && !point::is_cardinally_adjacent_to(pixel_pos, previous_pixel_pos)) {
			int horizontal_move_count = 0;
			int vertical_move_count = 0;
			const int horizontal_diff = std::abs(pixel_pos.x() - previous_pixel_pos.x());
			const int vertical_diff = std::abs(pixel_pos.y() - previous_pixel_pos.y());
			while (previous_pixel_pos != pixel_pos) {
				const int horizontal_progress = horizontal_diff != 0 ? (horizontal_move_count * 100 / horizontal_diff) : 100;
				const int vertical_progress = vertical_diff != 0 ? (vertical_move_count * 100 / vertical_diff) : 100;
				if (previous_pixel_pos.x() != pixel_pos.x() && horizontal_progress <= vertical_progress) {
					if (pixel_pos.x() < previous_pixel_pos.x()) {
						previous_pixel_pos.setX(previous_pixel_pos.x() - 1);
					} else {
						previous_pixel_pos.setX(previous_pixel_pos.x() + 1);
					}
					horizontal_move_count++;
				} else if (previous_pixel_pos.y() != pixel_pos.y()) {
					if (pixel_pos.y() < previous_pixel_pos.y()) {
						previous_pixel_pos.setY(previous_pixel_pos.y() - 1);
					} else {
						previous_pixel_pos.setY(previous_pixel_pos.y() + 1);
					}
					vertical_move_count++;
				}

				if (previous_pixel_pos.x() >= 0 && previous_pixel_pos.y() >= 0 && previous_pixel_pos.x() < image.width() && previous_pixel_pos.y() < image.height()) {
					geopath::write_pixel_to_image(previous_pixel_pos, color, image);
				}
			}
		}

		previous_pixel_pos = pixel_pos;
	}
}

}
