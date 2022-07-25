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

#pragma once

#include "util/path_util.h"
#include "util/type_traits.h"

namespace wyrmgus::qvariant {

template <typename T>
static QVariant from_value(const T &value)
{
	if constexpr (std::is_same_v<T, std::filesystem::path>) {
		return QVariant::fromValue(path::to_qstring(value));
	} else if constexpr (std::is_same_v<T, std::string>) {
		return QVariant::fromValue(QString::fromStdString(value));
	} else if constexpr (std::is_pointer_v<T>) {
		using mutable_type = std::add_pointer_t<std::remove_const_t<std::remove_pointer_t<T>>>;

		if constexpr (std::is_same_v<T, mutable_type>) {
			return QVariant::fromValue(value);
		} else {
			return QVariant::fromValue(const_cast<mutable_type>(value));
		}
	} else if constexpr (is_specialization_of_v<T, std::unique_ptr>) {
		return QVariant::fromValue(value.get());
	} else {
		return QVariant::fromValue(value);
	}
}

template <typename T>
static T *to_object(const QVariant &variant)
{
	QObject *object = qvariant_cast<QObject *>(variant);
	return qobject_cast<T *>(object);
}

}
