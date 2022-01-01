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
//      (c) Copyright 2019-2022 by Andrettin
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

namespace wyrmgus {

template <typename T>
class singleton
{
protected:
	singleton() = default;
	singleton(const singleton &other) = delete;
	singleton &operator =(const singleton &other) = delete;

public:
	static T *get()
	{
		static std::unique_ptr<T> instance = T::create();

		return instance.get();
	}

private:
	static std::unique_ptr<T> create()
	{
		std::unique_ptr<T> instance = std::make_unique<T>();

		if constexpr (std::is_base_of_v<QObject, T>) {
			if (QApplication::instance()->thread() != QThread::currentThread()) {
				instance->moveToThread(QApplication::instance()->thread());
			}
		}

		return instance;
	}
};

}
