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

#include "util/geocoordinate_util.h"
#include "util/geopath_util.h"
#include "util/georectangle_util.h"
#include "util/point_util.h"

namespace wyrmgus::geoshape {

void write_to_image(const QGeoShape &geoshape, QImage &image, const QColor &color, const QGeoRectangle &georectangle, const std::string &image_checkpoint_save_filename)
{
	QGeoRectangle bounding_georectangle = geoshape.boundingGeoRectangle();

	if (!bounding_georectangle.intersects(georectangle)) {
		return;
	}

	if (geoshape.type() == QGeoShape::PathType) {
		const QGeoPath &geopath = static_cast<const QGeoPath &>(geoshape);
		geopath::write_to_image(geopath, image, color, georectangle);

		//if the geopath's width is 0, there is nothing further to do here, but otherwise, use the normal method of geoshape writing as well
		if (geopath.width() == 0) {
			return;
		}

		//increase the bounding rectangle of geopaths slightly, as otherwise a part of the path's width is cut off
		QGeoCoordinate bottom_left = bounding_georectangle.bottomLeft();
		QGeoCoordinate top_right = bounding_georectangle.topRight();
		bottom_left.setLatitude(bottom_left.latitude() - 0.1);
		bottom_left.setLongitude(bottom_left.longitude() - 0.1);
		top_right.setLatitude(top_right.latitude() + 0.1);
		top_right.setLongitude(top_right.longitude() + 0.1);
		bounding_georectangle.setBottomLeft(bottom_left);
		bounding_georectangle.setTopRight(top_right);
	}

	const double lon_per_pixel = qgeocoordinate::longitude_per_pixel(georectangle.width(), image.size());
	const double lat_per_pixel = qgeocoordinate::latitude_per_pixel(georectangle.height(), image.size());

	const QRectF unsigned_georectangle = georectangle::to_unsigned_georectangle(georectangle);

	const QRectF unsigned_bounding_georectangle = georectangle::to_unsigned_georectangle(bounding_georectangle);

	const double start_lon = std::max(unsigned_bounding_georectangle.x(), unsigned_georectangle.x());
	const double end_lon = std::min(unsigned_bounding_georectangle.right(), unsigned_georectangle.right());
	const int start_x = std::max(qgeocoordinate::unsigned_longitude_to_x(start_lon - unsigned_georectangle.x(), lon_per_pixel) - 1, 0);
	const int end_x = std::min(qgeocoordinate::unsigned_longitude_to_x(end_lon - unsigned_georectangle.x(), lon_per_pixel) + 1, image.width() - 1);

	const double start_lat = std::min(unsigned_bounding_georectangle.y(), unsigned_georectangle.y());
	const double end_lat = std::max(unsigned_bounding_georectangle.bottom(), unsigned_georectangle.bottom());
	const int start_y = std::max(qgeocoordinate::unsigned_latitude_to_y(start_lat - unsigned_georectangle.y(), lat_per_pixel) - 1, 0);
	const int end_y = std::min(qgeocoordinate::unsigned_latitude_to_y(end_lat - unsigned_georectangle.y(), lat_per_pixel) + 1, image.height() - 1);

	int pixel_checkpoint_count = 0;
	static constexpr int pixel_checkpoint_threshold = 32 * 32;

	for (int x = start_x; x <= end_x; ++x) {
		for (int y = start_y; y <= end_y; ++y) {
			const QPoint pixel_pos(x, y);
			const QGeoCoordinate coordinate = point::to_qgeocoordinate(pixel_pos, image.size(), unsigned_georectangle);

			if (image.pixelColor(pixel_pos).alpha() != 0) {
				continue; //ignore already-written pixels
			}

			if (!geoshape.contains(coordinate)) {
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
