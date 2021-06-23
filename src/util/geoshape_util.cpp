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

#include "util/geoshape_util.h"

#include "map/map_projection.h"
#include "util/geocoordinate.h"
#include "util/geopath_util.h"
#include "util/georectangle_util.h"
#include "util/point_util.h"

namespace wyrmgus::geoshape {

void write_to_image(const QGeoShape &geoshape, QImage &image, const QColor &color, const QRect &georectangle, const map_projection *map_projection, const std::string &image_checkpoint_save_filename)
{
	const QGeoRectangle qgeorectangle = georectangle::to_qgeorectangle(georectangle);
	QGeoRectangle bounding_qgeorectangle = geoshape.boundingGeoRectangle();

	if (!bounding_qgeorectangle.intersects(qgeorectangle)) {
		return;
	}

	if (geoshape.type() == QGeoShape::PathType) {
		const QGeoPath &geopath = static_cast<const QGeoPath &>(geoshape);
		geopath::write_to_image(geopath, image, color, georectangle, map_projection);

		//if the geopath's width is 0, there is nothing further to do here, but otherwise, use the normal method of geoshape writing as well
		if (geopath.width() == 0) {
			return;
		}

		//increase the bounding rectangle of geopaths slightly, as otherwise a part of the path's width is cut off
		QGeoCoordinate bottom_left = bounding_qgeorectangle.bottomLeft();
		QGeoCoordinate top_right = bounding_qgeorectangle.topRight();
		bottom_left.setLatitude(bottom_left.latitude() - 0.1);
		bottom_left.setLongitude(bottom_left.longitude() - 0.1);
		top_right.setLatitude(top_right.latitude() + 0.1);
		top_right.setLongitude(top_right.longitude() + 0.1);
		bounding_qgeorectangle.setBottomLeft(bottom_left);
		bounding_qgeorectangle.setTopRight(top_right);
	}

	const QRect bounding_georectangle = georectangle::from_qgeorectangle(bounding_qgeorectangle);

	const longitude start_lon = longitude(bounding_georectangle.x() - 1);
	const longitude end_lon = longitude(bounding_georectangle.right() + 1);

	const latitude start_lat = latitude(bounding_georectangle.bottom() + 1);
	const latitude end_lat = latitude(bounding_georectangle.y() - 1);

	const geocoordinate start_geocoordinate(start_lon, start_lat);
	const geocoordinate end_geocoordinate(end_lon, end_lat);

	QPoint start_pos = map_projection->geocoordinate_to_point(start_geocoordinate, georectangle, image.size());
	if (start_pos.x() < 0) {
		start_pos.setX(0);
	}
	if (start_pos.y() < 0) {
		start_pos.setY(0);
	}

	QPoint end_pos = map_projection->geocoordinate_to_point(end_geocoordinate, georectangle, image.size());
	if (end_pos.x() > (image.width() - 1)) {
		end_pos.setX(image.width() - 1);
	}
	if (end_pos.y() > (image.height() - 1)) {
		end_pos.setY(image.height() - 1);
	}

	const volatile QRect area_rect(start_pos, end_pos);

	int pixel_checkpoint_count = 0;
	static constexpr int pixel_checkpoint_threshold = 32 * 32;

	for (int x = start_pos.x(); x <= end_pos.x(); ++x) {
		for (int y = start_pos.y(); y <= end_pos.y(); ++y) {
			const QPoint pixel_pos(x, y);

			if (image.pixelColor(pixel_pos).alpha() != 0) {
				continue; //ignore already-written pixels
			}

			const geocoordinate geocoordinate = map_projection->point_to_geocoordinate(pixel_pos, georectangle, image.size());
			const QGeoCoordinate qgeocoordinate = geocoordinate.to_qgeocoordinate();

			if (!geoshape.contains(qgeocoordinate)) {
				continue;
			}

			image.setPixelColor(pixel_pos, color);
			pixel_checkpoint_count++;

			if (pixel_checkpoint_count >= pixel_checkpoint_threshold) {
				image.save(QString::fromStdString(image_checkpoint_save_filename));
				pixel_checkpoint_count = 0;
			}
		}
	}
}

}
