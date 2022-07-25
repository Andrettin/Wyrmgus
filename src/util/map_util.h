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

#pragma once

#include "util/qvariant_util.h"

namespace wyrmgus::map {

template <typename T>
inline std::vector<typename T::key_type> get_keys(const T &map)
{
	std::vector<typename T::key_type> keys;

	for (const auto &kv_pair : map) {
		keys.push_back(kv_pair.first);
	}

	return keys;
}

template <typename T>
inline std::vector<typename T::value_type> get_values(const T &map)
{
	std::vector<typename T::value_type> values;

	for (const auto &kv_pair : map) {
		values.push_back(kv_pair.second);
	}

	return values;
}

template <typename T>
inline void remove_value(T &map, const typename T::value_type &value)
{
	for (auto it = map.begin(); it != map.end();) {
		if ((*it).second == value) {
			it = map.erase(it);
		} else {
			it++;
		}
	}
}

template <typename T, typename function_type>
inline void remove_value_if(T &map, const function_type &function)
{
	for (auto it = map.begin(); it != map.end();) {
		if (function((*it).second)) {
			it = map.erase(it);
		} else {
			it++;
		}
	}
}

template <typename T>
inline QVariantList to_qvariant_list(const T &map)
{
	QVariantList qvariant_list;

	for (const auto &[key, value] : map) {
		QVariantMap qvariant_map;

		qvariant_map["key"] = qvariant::from_value<typename T::key_type>(key);
		qvariant_map["value"] = qvariant::from_value<typename T::mapped_type>(value);

		qvariant_list.push_back(std::move(qvariant_map));
	}

	return qvariant_list;
}

template <typename T>
inline QVariantMap to_qvariant_map(const T &map)
{
	QVariantMap qvariant_map;

	for (const auto &[key, value] : map) {
		QString key_qstr;

		if constexpr (std::is_same_v<typename T::key_type, std::string>) {
			key_qstr = QString::fromStdString(key);
		} else if constexpr (std::is_pointer_v<typename T::key_type>) {
			key_qstr = key->get_identifier();
		} else {
			key_qstr = key;
		}

		qvariant_map[key_qstr] = qvariant::from_value<typename T::mapped_type>(value);
	}

	return qvariant_map;
}

}
