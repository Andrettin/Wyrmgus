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

namespace stratagus::geojson {

extern std::vector<QVariantList> parse_folder(const std::filesystem::path &path);

template <typename function_type>
inline void process_features(const std::vector<QVariantList> &geojson_data_list, const function_type &processing_function)
{
	for (const QVariantList &geojson_data : geojson_data_list) {
		const QVariantMap feature_collection = geojson_data.front().toMap();
		const QVariantList feature_collection_data = feature_collection.value("data").toList();

		for (const QVariant &feature_variant : feature_collection_data) {
			const QVariantMap feature = feature_variant.toMap();
			processing_function(feature);
		}
	}
}

}

#include "util/geocoordinate_util.h"
#include "util/point_util.h"

namespace stratagus::geopolygon {

inline void write_to_image(const QGeoShape &geoshape, QImage &image, const QColor &color)
{
	const double lon_per_pixel = 360.0 / static_cast<double>(image.size().width());
	const double lat_per_pixel = 180.0 / static_cast<double>(image.size().height());

	QGeoRectangle georectangle = geoshape.boundingGeoRectangle();
	QGeoCoordinate bottom_left = georectangle.bottomLeft();
	QGeoCoordinate top_right = georectangle.topRight();

	if (geoshape.type() == QGeoShape::ShapeType::PathType) {
		//increase the bounding rectangle of paths slightly, as otherwise a part of the path's width is cut off
		bottom_left.setLatitude(bottom_left.latitude() - 0.1);
		bottom_left.setLongitude(bottom_left.longitude() - 0.1);
		top_right.setLatitude(top_right.latitude() + 0.1);
		top_right.setLongitude(top_right.longitude() + 0.1);
	}

	const double start_lon = bottom_left.longitude();
	const double end_lon = top_right.longitude();

	double lon = start_lon;
	lon = geocoordinate::longitude_to_pixel_longitude(lon, lon_per_pixel);
	const int start_x = geocoordinate::longitude_to_x(lon, lon_per_pixel);

	const double start_lat = bottom_left.latitude();
	const double end_lat = top_right.latitude();
	const double normalized_start_lat = geocoordinate::latitude_to_pixel_latitude(start_lat, lat_per_pixel);

	const int pixel_width = static_cast<int>(std::round((std::abs(end_lon - start_lon)) / lon_per_pixel));
	const bool show_progress = pixel_width >= 512;

	for (; lon <= end_lon; lon += lon_per_pixel) {
		const int x = geocoordinate::longitude_to_x(lon, lon_per_pixel);

		for (double lat = normalized_start_lat; lat <= end_lat; lat += lat_per_pixel) {
			QGeoCoordinate coordinate(lat, lon);

			const int y = geocoordinate::latitude_to_y(lat, lat_per_pixel);
			const int pixel_index = point::to_index(x, y, image.size());

			if (!geoshape.contains(coordinate)) {
				continue;
			}

			image.setPixelColor(x, y, color);
		}
	}
}

}