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
//

#include "util/geojson_util.h"

#include "util/exception_util.h"

#ifdef USE_GEOJSON
#include <QtLocation/private/qgeojson_p.h>
#endif

namespace wyrmgus::geojson {

std::vector<QVariantList> parse_folder(const std::filesystem::path &path)
{
#ifdef USE_GEOJSON
	std::vector<QVariantList> geojson_data_list;

	std::filesystem::recursive_directory_iterator dir_iterator(path);

	for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
		if (!dir_entry.is_regular_file() || dir_entry.path().extension() != ".geojson") {
			continue;
		}

		std::ifstream ifstream(dir_entry.path());

		if (!ifstream) {
			exception::throw_with_trace(std::runtime_error("Failed to open file: " + dir_entry.path().string()));
		}

		const std::string raw_geojson_data(std::istreambuf_iterator<char>{ifstream}, std::istreambuf_iterator<char>{});
		const QByteArray raw_geojson_byte_array = QByteArray::fromStdString(raw_geojson_data);

		QJsonParseError json_error;
		const QJsonDocument json = QJsonDocument::fromJson(raw_geojson_byte_array, &json_error);

		if (json.isNull()) {
			exception::throw_with_trace(std::runtime_error("JSON parsing failed: " + json_error.errorString().toStdString() + "."));
		}

		QVariantList geojson_data = QGeoJson::importGeoJson(json);
		geojson_data_list.push_back(std::move(geojson_data));
	}

	return geojson_data_list;
#else
	exception::throw_with_trace(std::runtime_error("GeoJSON support not enabled."));
#endif
}

}
