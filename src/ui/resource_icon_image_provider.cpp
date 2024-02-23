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

#include "stratagus.h"

#include "ui/resource_icon_image_provider.h"

#include "database/preferences.h"
#include "ui/resource_icon.h"
#include "util/exception_util.h"
#include "util/string_util.h"
#include "video/color_modification.h"
#include "video/video.h"

namespace wyrmgus {

QImage resource_icon_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(requested_size)

	try {
		const std::string id_str = id.toStdString();
		const resource_icon *resource_icon = resource_icon::get(id_str);

		std::shared_ptr<CGraphic> graphics = resource_icon->get_graphics();
		graphics->Load(preferences::get()->get_scale_factor());

		const QImage &image = graphics->get_or_create_frame_image(resource_icon->get_frame(), color_modification(), false);

		if (image.isNull()) {
			throw std::runtime_error("Resource icon image for ID \"" + id_str + "\" is null.");
		}

		if (size != nullptr) {
			*size = image.size();
		}

		return image;
	} catch (...) {
		exception::report(std::current_exception());
		return QImage();
	}
}

}
