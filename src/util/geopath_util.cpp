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
//      (c) Copyright 2020-2022 by Andrettin
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
#include "util/georectangle.h"
#include "util/geoshape_util.h"
#include "util/point_util.h"

namespace wyrmgus::geopath {

void write_to_image(const QGeoPath &geopath, QImage &image, const QColor &color, const georectangle &georectangle, const map_projection *map_projection)
{
	QPoint previous_pixel_pos(-1, -1);
	for (const QGeoCoordinate &geocoordinate : geopath.path()) {
		const QPoint pixel_pos = map_projection->geocoordinate_to_point(wyrmgus::geocoordinate(geocoordinate), georectangle, image.size());

		if (pixel_pos.x() >= 0 && pixel_pos.y() >= 0 && pixel_pos.x() < image.width() && pixel_pos.y() < image.height()) {
			//only write to the image if the position is actually in it, but take the position into account either way for the purpose of getting the previous pixel pos, so that map templates fit together well
			geoshape::write_pixel_to_image(pixel_pos, color, image);
		}

		if (previous_pixel_pos != QPoint(-1, -1) && !point::is_cardinally_adjacent_to(pixel_pos, previous_pixel_pos)) {
			const std::vector<QPoint> straight_pixel_path = point::get_straight_path_to(previous_pixel_pos, pixel_pos);

			for (const QPoint &straight_path_pos : straight_pixel_path) {
				if (straight_path_pos.x() >= 0 && straight_path_pos.y() >= 0 && straight_path_pos.x() < image.width() && straight_path_pos.y() < image.height()) {
					geoshape::write_pixel_to_image(straight_path_pos, color, image);
				}
			}
		}

		previous_pixel_pos = pixel_pos;
	}
}

}
