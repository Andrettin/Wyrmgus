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

namespace wyrmgus::geojson {

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
