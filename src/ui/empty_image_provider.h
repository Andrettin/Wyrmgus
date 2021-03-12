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
//      (c) Copyright 2019-2021 by Andrettin
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

#include <QQuickImageProvider>

namespace wyrmgus {

class empty_image_provider final : public QQuickImageProvider
{
public:
	empty_image_provider() : QQuickImageProvider(QQuickImageProvider::Image)
	{
	}

	virtual QImage requestImage(const QString &id, QSize *size, const QSize &requested_size) override
	{
		Q_UNUSED(id)
		Q_UNUSED(requested_size)

		QImage image(QSize(1, 1), QImage::Format_ARGB32);
		image.fill(qRgba(0, 0, 0, 0));

		if (size != nullptr) {
			*size = image.size();
		}

		return image;
	}
};

}
