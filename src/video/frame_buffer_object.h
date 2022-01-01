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

namespace wyrmgus {

//a class providing an OpenGL frame buffer to be used by QtQuick
class frame_buffer_object : public QQuickFramebufferObject
{
private:
	static inline frame_buffer_object *instance = nullptr;
	static inline std::mutex mutex;

public:
	static void request_update()
	{
		std::lock_guard<std::mutex> lock(frame_buffer_object::mutex);

		if (frame_buffer_object::instance != nullptr) {
			QMetaObject::invokeMethod(frame_buffer_object::instance, &frame_buffer_object::update, Qt::QueuedConnection);
		}
	}

	frame_buffer_object()
	{
		std::lock_guard<std::mutex> lock(frame_buffer_object::mutex);

		frame_buffer_object::instance = this;
	}

	~frame_buffer_object()
	{
		if (frame_buffer_object::instance == this) {
			std::lock_guard<std::mutex> lock(frame_buffer_object::mutex);
			frame_buffer_object::instance = nullptr;
		}
	}

public:
	virtual QQuickFramebufferObject::Renderer *createRenderer() const override;
};

}
