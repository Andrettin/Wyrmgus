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
//      (c) Copyright 2021 by Andrettin
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

#include "util/geocircle_util.h"

#include "map/map_projection.h"
#include "util/geocoordinate.h"
#include "util/geoshape_util.h"

namespace wyrmgus::geocircle {

void write_to_image(const QGeoCircle &geocircle, QImage &image, const QColor &color, const georectangle &georectangle, const map_projection *map_projection)
{
	const QGeoCoordinate geocoordinate = geocircle.center();

	const QPoint base_pixel_pos = map_projection->geocoordinate_to_point(wyrmgus::geocoordinate(geocoordinate), georectangle, image.size());

	//write a 2x2 block so that the point can be expanded from in terrain generation, and so that it won't be removed if the terrain type does not allow single tiles
	for (int x_offset = 0; x_offset <= 1; ++x_offset) {
		for (int y_offset = 0; y_offset <= 1; ++y_offset) {
			const QPoint pixel_pos = base_pixel_pos + QPoint(x_offset, y_offset);

			if (pixel_pos.x() >= 0 && pixel_pos.y() >= 0 && pixel_pos.x() < image.width() && pixel_pos.y() < image.height()) {
				geoshape::write_pixel_to_image(pixel_pos, color, image);
			}
		}
	}
}

}
